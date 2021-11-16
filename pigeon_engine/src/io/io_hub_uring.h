#include <netinet/in.h>
#include <arpa/inet.h>
#include <liburing.h>
#include <pigeon/io/io_hub.h>
#include <pigeon/io/ip.h>
#include <pigeon/assert.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <pigeon/misc.h>
#include <pigeon/array_list.h>
#include <pigeon/object_pool.h>
#include <string.h>

typedef enum
{
    URING_CONNECTION_STATE_ACTIVE,
    URING_CONNECTION_STATE_CLOSING,
    URING_CONNECTION_STATE_CLOSED // Connection object can be free'd when all events done
} URingConnectionState;

typedef struct UringConnectionData
{
    union {
        struct {
            PigeonNetworkSocket * socket;
            struct sockaddr_in6 addr;
            unsigned int addr_len;
        } init;

        struct {
            int fd;
            int number_of_events_in_progress; // events currently in io uring
            URingConnectionState state;
            PigeonIOHubConnectionObject obj;
        };
    };
} UringConnectionData;

typedef struct UringAutoBuffers
{
    unsigned int group_id;
    unsigned int buffer_size;
    unsigned int buffer_count;
    void * data; // buffer_size * buffer_count bytes 
} UringAutoBuffers;

typedef enum {
    URING_EVENT_TYPE_BUFFER_REGISTER,
    URING_EVENT_TYPE_ACCEPT,
    URING_EVENT_TYPE_READ,
    URING_EVENT_TYPE_WRITE,
    URING_EVENT_TYPE_CLOSE,

    // !! A fixed number of these are malloc'd and reused for each socket
    URING_EVENT_TYPE_UDP_READ,

    // !! These are stored in a separate pool
    URING_EVENT_TYPE_UDP_WRITE
} UringEventType;

typedef struct URingEvent
{
    UringEventType type;

    // For TCP writes
    void * data;
    unsigned int data_length;
    uint64_t arg;

    // For TCP accept/read/write/close
    UringConnectionData * connection;
} URingEvent;

typedef struct URingUDPWriteEvent
{
    UringEventType type;
    struct iovec ptr_and_size;
    struct sockaddr_in6 addr;
    struct msghdr msg;
} URingUDPWriteEvent;

typedef struct URingUDPReadEvent
{
    UringEventType type;
    PigeonNetworkSocket* socket;
    struct iovec ptr_and_size;
    char data[2048];
    struct sockaddr_in6 addr;
    struct msghdr msg;
} URingUDPReadEvent;

typedef struct UringHubData
{
    struct io_uring uring;

    pigeon_io_hub_new_connection_callback new_connection;
    pigeon_io_hub_connection_closed_callback connection_closed;
    pigeon_io_hub_data_in_callback data_in;
    pigeon_io_hub_data_in_udp_callback data_in_udp;
    pigeon_io_hub_write_done_callback write_done;

     // Events added to queue but not submitted yet
    unsigned int events_awaiting_submission;

    // events_awaiting_submission + events already submitted but not competed and marked seen
    unsigned int events_in_circulation;

    // Highest possible number of events in circulation
    // (io_uring allows the application to go over this limit- cq ring is 2x size of sq ring)
    unsigned int maximum_events;

    unsigned int maximum_tcp_connections;

    PigeonObjectPool connections_object_pool; // UringConnectionData
    PigeonObjectPool events_pool; // URingEvent
    PigeonObjectPool udp_write_events_pool; // URingUDPEvent

    UringAutoBuffers tcp_read_buffers;
} UringHubData;

static PIGEON_ERR_RET create_event(UringHubData* hd, URingEvent** event, struct io_uring_sqe** sqe);
static PIGEON_ERR_RET uring_udp_read(UringHubData* hd, PigeonNetworkSocket* socket, URingUDPReadEvent * event);
static PIGEON_ERR_RET do_submit(UringHubData* hd);

static PIGEON_ERR_RET pigeon_create_io_hub__uring(PigeonIOHub* io_hub, 
    pigeon_io_hub_new_connection_callback new_connection_, 
    pigeon_io_hub_connection_closed_callback connection_closed_, 
    pigeon_io_hub_data_in_callback data_in_, 
    pigeon_io_hub_data_in_udp_callback data_in_udp_, 
    pigeon_io_hub_write_done_callback write_done_
)
{
    UringHubData * hd = calloc(1, sizeof *hd);
    ASSERT_R1(hd);
    io_hub->data = hd;

#define CLEANUP() pigeon_destroy_io_hub(io_hub);

    hd->new_connection = new_connection_;
    hd->connection_closed = connection_closed_;
    hd->data_in = data_in_;
    hd->data_in_udp = data_in_udp_;
    hd->write_done = write_done_;

    hd->maximum_events = 4096;
    hd->maximum_tcp_connections = 1024;
    ASSERT_R1(!io_uring_queue_init(hd->maximum_events, &hd->uring, 0));

    // pigeon_create_array_list(&hd->server_socket_pointers, sizeof(uintptr_t));
    pigeon_create_object_pool(&hd->connections_object_pool, sizeof(UringConnectionData), true);
    pigeon_create_object_pool(&hd->events_pool, sizeof(URingEvent), true);
    pigeon_create_object_pool(&hd->udp_write_events_pool, sizeof(URingUDPWriteEvent), true);
    


    hd->tcp_read_buffers.buffer_size = 16*1024; // TODO make this configurable
    hd->tcp_read_buffers.buffer_count = 150; // TODO make this configurable
    void * x = malloc(hd->tcp_read_buffers.buffer_size * 150);
    ASSERT_R1(x);
    hd->tcp_read_buffers.data = x;

    URingEvent* event = NULL;
    struct io_uring_sqe* sqe = NULL;
    ASSERT_R1(!create_event(hd, &event, &sqe));

    event->type = URING_EVENT_TYPE_BUFFER_REGISTER;
    io_uring_prep_provide_buffers(sqe, x, 
        (int)hd->tcp_read_buffers.buffer_size, (int)hd->tcp_read_buffers.buffer_count, 1, 1);

    io_uring_sqe_set_data(sqe, event);
    ASSERT_R1(!do_submit(hd));

    
#undef CLEANUP
    return 0;
}

static void pigeon_destroy_io_hub__uring(PigeonIOHub* io_hub)
{
    assert(io_hub);

    if(io_hub) {
        if(io_hub->data) {
            UringHubData* hd = io_hub->data;
            if(hd->uring.ring_fd) {
                io_uring_queue_exit(&hd->uring);
            }

            pigeon_destroy_object_pool(&hd->events_pool);
            pigeon_destroy_object_pool(&hd->udp_write_events_pool);
            pigeon_destroy_object_pool(&hd->connections_object_pool);


            free2(io_hub->data);
        }
    }
}

static struct io_uring_sqe * uring_get_sqe(UringHubData * hd)
{
    if(hd->events_in_circulation >= hd->maximum_events) {
        printf("%u events in circulation, max is %u !\n", hd->events_in_circulation, hd->maximum_events);
        // return NULL;
    }

    struct io_uring_sqe* sqe = io_uring_get_sqe(&hd->uring);
    ASSERT_R0(sqe);

    hd->events_awaiting_submission++;
    hd->events_in_circulation++;

    return sqe;
}

// Remember to set user data in sqe.
// It is not done here as prep_* functions reset it 0
static PIGEON_ERR_RET create_event(UringHubData* hd, URingEvent** event, struct io_uring_sqe** sqe)
{
    *event = pigeon_object_pool_allocate(&hd->events_pool);
    ASSERT_R1(*event);

    *sqe = uring_get_sqe(hd);
    if(!*sqe) {
        pigeon_object_pool_free(&hd->events_pool, *event);
        return 1;
    }
    return 0;
}

static PIGEON_ERR_RET uring_queue_accept(UringHubData* hd, PigeonNetworkSocket* socket)
{
    UringConnectionData * connection = pigeon_object_pool_allocate(&hd->connections_object_pool);
    ASSERT_R1(connection);
    connection->init.socket = socket;

    URingEvent * event;   
    struct io_uring_sqe* sqe;

    if(create_event(hd, &event, &sqe)) {
        pigeon_object_pool_free(&hd->connections_object_pool, connection);
        return 1;
    }

    event->type = URING_EVENT_TYPE_ACCEPT;
    event->connection = connection;

    connection->init.addr_len = sizeof connection->init.addr;
    io_uring_prep_accept(sqe, socket->handle, (struct sockaddr *)&connection->init.addr, &connection->init.addr_len, 0);
    io_uring_sqe_set_data(sqe, event);


    return 0;
}

// event is malloc'd if NULL
// if event is non-NULL then socket is ignored
static PIGEON_ERR_RET uring_udp_read(UringHubData* hd, PigeonNetworkSocket* socket, URingUDPReadEvent * event)
{
    if(event) {
        socket = event->socket;
    }
    else {
        event = malloc(sizeof *event);
        ASSERT_R1(event);
    }

    struct io_uring_sqe* sqe = uring_get_sqe(hd);
    if(!sqe) {
        free(event);
        ASSERT_R1(false);
    }

    event->type = URING_EVENT_TYPE_UDP_READ;
    event->socket = socket;    
    event->ptr_and_size.iov_base = event->data;
    event->ptr_and_size.iov_len = sizeof event->data;
    event->msg.msg_name = &event->addr;
    event->msg.msg_namelen = sizeof event->addr;
    event->msg.msg_iov = &event->ptr_and_size;
    event->msg.msg_iovlen = 1;
    event->msg.msg_control = NULL;
    event->msg.msg_controllen = 0;

    io_uring_prep_recvmsg(sqe, socket->handle, &event->msg, 0);
    io_uring_sqe_set_data(sqe, event);


    return 0;
}


static PIGEON_ERR_RET pigeon_io_hub_add_server_socket__uring(PigeonIOHub* io_hub, PigeonNetworkSocket* socket)
{
    ASSERT_R1(io_hub && io_hub->data && socket && socket->handle && socket->is_server_socket);

    UringHubData* hd = io_hub->data;

    if(socket->protocol == PIGEON_NETWORK_PROTOCOL_TCP) {
        if(uring_queue_accept(hd, socket)) {
            return 1;
        }    
    }
    else {
        for(unsigned int i = 0; i < 64; i++){
            if(uring_udp_read(hd, socket, NULL)) {
                return 1;
            } 
        }
    }

    return 0;
}

static PIGEON_ERR_RET do_submit(UringHubData* hd)
{
    while (hd->events_awaiting_submission > 0) {
        int submitted = io_uring_submit(&hd->uring);
        ASSERT_R1(submitted > 0 && (unsigned)submitted <= hd->events_awaiting_submission);

        hd->events_awaiting_submission -= (unsigned)submitted;
    }
    return 0;
}

// TODO socket does not close properly, only closes when client tries to send more data (which apache bench doesn't do.)
static PIGEON_ERR_RET pigeon_io_hub_close_tcp_socket__uring(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h)
{
    ASSERT_R1(io_hub && io_hub->data && h);

    UringHubData* hd = io_hub->data;
    UringConnectionData * connection = (UringConnectionData *)h;

    if(connection->state) return 0;
    connection->state = URING_CONNECTION_STATE_CLOSING;
    
    URingEvent * event;   
    struct io_uring_sqe* sqe;

    ASSERT_R1(!create_event(hd, &event, &sqe));

    event->type = URING_EVENT_TYPE_CLOSE;
    event->connection = connection;

    connection->number_of_events_in_progress++;

    io_uring_prep_close(sqe, connection->fd);
    io_uring_sqe_set_data(sqe, event);
    return 0;
}

static PIGEON_ERR_RET uring_tcp_shutdown(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h, int how)
{
    ASSERT_R1(io_hub && io_hub->data && h);

    UringHubData* hd = io_hub->data;
    UringConnectionData * connection = (UringConnectionData *)h;

    if(connection->state) return 0;
     
    struct io_uring_sqe* sqe = uring_get_sqe(hd);
    ASSERT_R1(sqe);

    io_uring_prep_shutdown(sqe, connection->fd, how);
    return 0;
}

static PIGEON_ERR_RET pigeon_io_hub_tcp_shutdown_read__uring(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h)
{
    return uring_tcp_shutdown(io_hub, h, SHUT_RD);
}

static PIGEON_ERR_RET pigeon_io_hub_tcp_shutdown_write__uring(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h)
{
    return uring_tcp_shutdown(io_hub, h, SHUT_WR);
}

void pigeon_io_hub_set_connection_object(PigeonIOHubConnectionHandle h, PigeonIOHubConnectionObject obj)
{
    assert(h);
    UringConnectionData * connection = (UringConnectionData *)h;
    connection->obj = obj;
}

static PIGEON_ERR_RET handle_cqe(PigeonIOHub* io_hub, struct io_uring_cqe* cqe)
{
    URingEvent * event = io_uring_cqe_get_data(cqe);
    UringHubData* hd = io_hub->data;

    if(!event) {
        io_uring_cqe_seen(&hd->uring, cqe);
        hd->events_in_circulation--;
        return 0;
    }
    
    if(event->type == URING_EVENT_TYPE_ACCEPT) {
        PigeonNetworkSocket * socket = event->connection->init.socket;
        if(cqe->res >= 0 && event->connection->init.addr_len >= sizeof(struct sockaddr_in6)) 
        {
            PigeonIPv6Address ip = *((PigeonIPv6Address *) &event->connection->init.addr.sin6_addr);
            uint16_t port = htons(event->connection->init.addr.sin6_port);

            // memset(event->connection, 0, sizeof event->connection));

            event->connection->fd = cqe->res;
            event->connection->number_of_events_in_progress = 0;
            event->connection->state = URING_CONNECTION_STATE_ACTIVE;
            event->connection->obj = NULL;

            hd->new_connection(io_hub, event->connection, socket, ip, port);
        }
        if(hd->events_in_circulation <= hd->maximum_events) {
            ASSERT_R1(!uring_queue_accept(hd, socket));
        } else {
            // TODO better handling of this. 
            //  ^ Track active connections & resume accepting connections when active drops below max
            fputs("IO Hub overloaded. No longer accepting connections on TCP socket", stderr);
        }
    }
    else if(event->type == URING_EVENT_TYPE_CLOSE) {
        #ifdef DEBUG
        if(cqe->res != 0) {
            fprintf(stderr, "Close failed. Error: %i %s\n", cqe->res, strerror(-cqe->res));
        }
        #endif

        event->connection->number_of_events_in_progress--;
        hd->connection_closed(io_hub, event->connection->obj);

        event->connection->state = URING_CONNECTION_STATE_CLOSED;
        if(event->connection->number_of_events_in_progress <= 0)
            pigeon_object_pool_free(&hd->connections_object_pool, event->connection);
    }
    else if(event->type == URING_EVENT_TYPE_WRITE) {
        event->connection->number_of_events_in_progress--;

        if(event->connection->state == URING_CONNECTION_STATE_ACTIVE) {
            if(cqe->res > 0) {
                hd->write_done(io_hub, event->connection, event->connection->obj, 
                    event->data_length, event->data, event->arg);
            } 
            else {
                #ifdef DEBUG
                if(cqe->res != 0)
                    fprintf(stderr, "Write failed. Error: %i %s\n", cqe->res, strerror(-cqe->res));
                #endif
                ASSERT_R1(!pigeon_io_hub_close_tcp_socket__uring(io_hub, event->connection));
            }
        }
        else if (event->connection->state == URING_CONNECTION_STATE_CLOSED &&
            event->connection->number_of_events_in_progress <= 0) 
        {
            pigeon_object_pool_free(&hd->connections_object_pool, event->connection);
        }
    }
    else if(event->type == URING_EVENT_TYPE_READ) {
        event->connection->number_of_events_in_progress--;

        unsigned int buffer_id = cqe->flags >> 16;
        unsigned int buffer_size = hd->tcp_read_buffers.buffer_size;
        void * buffer_ptr = (void *) ((uintptr_t)hd->tcp_read_buffers.data + buffer_size*(buffer_id-1));

        if(event->connection->state == URING_CONNECTION_STATE_ACTIVE) {
            if(cqe->res > 0) {
                hd->data_in(io_hub, event->connection, event->connection->obj, 
                    (unsigned)cqe->res, buffer_ptr);
            } 
            else {
                #ifdef DEBUG
                if(cqe->res != 0)
                    fprintf(stderr, "Read failed. Error: %i %s\n", cqe->res, strerror(-cqe->res));
                #endif
                
                ASSERT_R1(!pigeon_io_hub_close_tcp_socket__uring(io_hub, event->connection));
            }
        }
        else if (event->connection->state == URING_CONNECTION_STATE_CLOSED &&
            event->connection->number_of_events_in_progress <= 0) 
        {
            pigeon_object_pool_free(&hd->connections_object_pool, event->connection);
        }

        // Reregister buffer
                
        URingEvent* event2 = NULL;
        struct io_uring_sqe* sqe = NULL;
        ASSERT_R1(!create_event(hd, &event2, &sqe));

        event2->type = URING_EVENT_TYPE_BUFFER_REGISTER;
        io_uring_prep_provide_buffers(sqe, buffer_ptr, (int)buffer_size, 1, 1, (int)buffer_id);
        io_uring_sqe_set_data(sqe, event2);
        ASSERT_R1(!do_submit(hd));
    }
    else if(event->type == URING_EVENT_TYPE_UDP_READ) {
        URingUDPReadEvent * udp_event = (URingUDPReadEvent *) event;

        if(cqe->res > 0) {
            hd->data_in_udp(io_hub, 
                *(PigeonIPv6Address *)&udp_event->addr.sin6_addr, htons(udp_event->addr.sin6_port),
                (unsigned)cqe->res, udp_event->ptr_and_size.iov_base);

            ASSERT_R1(!uring_udp_read(hd, NULL, udp_event));
        }
        else {
            fprintf(stderr, "UDP read failed. Error: %i %s\n", cqe->res, strerror(-cqe->res));
            ASSERT_R1(false);
        }
    }

    if(event->type == URING_EVENT_TYPE_UDP_WRITE)
        pigeon_object_pool_free(&hd->udp_write_events_pool, event);
    if(event->type != URING_EVENT_TYPE_UDP_READ)
        pigeon_object_pool_free(&hd->events_pool, event);

    io_uring_cqe_seen(&hd->uring, cqe);
    hd->events_in_circulation--;
    return 0;
}

static PIGEON_ERR_RET pigeon_io_hub_wait__uring(PigeonIOHub* io_hub)
{
    ASSERT_R1(io_hub && io_hub->data);

    UringHubData* hd = io_hub->data;

    ASSERT_R1(!do_submit(hd));

    struct io_uring_cqe* cqe = NULL;

    int err = io_uring_wait_cqe(&hd->uring, &cqe);
    if(err) {
        err = io_uring_wait_cqe(&hd->uring, &cqe);
        if(err) {
            fprintf(stderr, "io_uring_wait_cqe failed. Error: %i %s\n", err, strerror(-err));
            ASSERT_R1(false);
        }
    }
    ASSERT_R1(cqe);

    ASSERT_R1(!handle_cqe(io_hub, cqe));
    ASSERT_R1(!do_submit(hd));
    return 0;
}

static PIGEON_ERR_RET pigeon_io_hub_poll__uring(PigeonIOHub* io_hub)
{
    ASSERT_R1(io_hub && io_hub->data);

    UringHubData* hd = io_hub->data;

    ASSERT_R1(!do_submit(hd));

    struct io_uring_cqe* cqe;
    int err = io_uring_peek_cqe(&hd->uring, &cqe);
    if(err) {
        fprintf(stderr, "io_uring_peek_cqe failed. Error: %i %s\n", err, strerror(-err));
        ASSERT_R1(false);
    }

    if(cqe) ASSERT_R1(!handle_cqe(io_hub, cqe));
    return 0;
}

// TODO scattered writes?
static PIGEON_ERR_RET pigeon_io_hub_tcp_send__uring(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h,
    const void * data, unsigned int data_length, uint64_t arg)
{
    ASSERT_R1(io_hub && io_hub->data && h && data);
    if(!data_length) return 0;
    
    UringHubData* hd = io_hub->data;
    UringConnectionData * connection = (UringConnectionData *) h;
    
    URingEvent * event;   
    struct io_uring_sqe* sqe;

    ASSERT_R1(!create_event(hd, &event, &sqe));

    event->type = URING_EVENT_TYPE_WRITE;
    event->connection = connection;
    event->data = (void *)data;
    event->data_length = data_length;
    event->arg = arg;

    connection->number_of_events_in_progress++;

    io_uring_prep_write(sqe, connection->fd, data, data_length, 0);
    io_uring_sqe_set_data(sqe, event);
    return 0;
}

static PIGEON_ERR_RET pigeon_io_hub_tcp_read__uring(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h)
{
    ASSERT_R1(io_hub && io_hub->data && h);
    
    UringHubData* hd = io_hub->data;
    UringConnectionData * connection = (UringConnectionData *) h;
    
    URingEvent * event;   
    struct io_uring_sqe* sqe;

    ASSERT_R1(!create_event(hd, &event, &sqe));

    event->type = URING_EVENT_TYPE_READ;
    event->connection = connection;

    connection->number_of_events_in_progress++;

    io_uring_prep_read(sqe, connection->fd, NULL, hd->tcp_read_buffers.buffer_size, 0);
    sqe->buf_group = 1;
    sqe->flags |= IOSQE_BUFFER_SELECT;
    io_uring_sqe_set_data(sqe, event);
    return 0;
}

static PIGEON_ERR_RET pigeon_io_hub_udp_send__uring(PigeonIOHub* io_hub, 
    PigeonNetworkSocket* socket,
    PigeonIPv6Address remote_ip, PigeonNetworkPort remote_port,
    const void * data, unsigned int data_length)
{
    ASSERT_R1(io_hub && io_hub->data && data && socket && socket->is_server_socket && socket->handle);
    if(!data_length) return 0;
    
    UringHubData* hd = io_hub->data;

    if(hd->events_in_circulation > hd->maximum_events) {
        // TODO better handling of this
        fputs("IO Hub overloaded. Dropping UDP packets.", stderr);
        return 1;
    }

    URingUDPWriteEvent * event = pigeon_object_pool_allocate(&hd->udp_write_events_pool);
    ASSERT_R1(event);

    struct io_uring_sqe *sqe = uring_get_sqe(hd);
    if(!sqe) {
        pigeon_object_pool_free(&hd->events_pool, event);
        ASSERT_R1(event);
    }

    event->type = URING_EVENT_TYPE_UDP_WRITE;
    event->ptr_and_size.iov_base = (void *)data;
    event->ptr_and_size.iov_len = data_length;

    event->addr.sin6_family = AF_INET6;
    memcpy(&event->addr.sin6_addr, &remote_ip, 16);
    event->addr.sin6_port = htons(remote_port);

    event->msg.msg_name = &event->addr;
    event->msg.msg_namelen = sizeof event->addr;
    event->msg.msg_iov = &event->ptr_and_size;
    event->msg.msg_iovlen = 1;
    event->msg.msg_control = NULL;
    event->msg.msg_controllen = 0;

    io_uring_prep_sendmsg(sqe, socket->handle, &event->msg, 0);
    io_uring_sqe_set_data(sqe, event);
    return 0;
}
