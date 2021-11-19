#include <netinet/in.h>
#include <arpa/inet.h>
#include <pigeon/io/io_hub.h>
#include <pigeon/assert.h>
#include <sys/utsname.h>
#include <pigeon/io/ip.h>
#include <sys/utsname.h>
#include <signal.h>

static bool io_uring_supported;


void pigeon_io_hubs_init(void)
{
#ifdef ENABLE_IO_URING
    struct utsname name;
    if (!uname(&name)) {
        puts(name.release);
        char * s = name.release;
        unsigned int maj = (unsigned)strtol(s, &s, 10);
        s++;
        unsigned int min = (unsigned)strtol(s, NULL, 10);
        if(maj > '5' || (maj == 5 && min >= 11)) {
            io_uring_supported = true;
        }
    }
#endif

    // Allow writing to closed sockets (returns error code instead of crashing process)

    sigaction(SIGPIPE, &(struct sigaction){{SIG_IGN}}, NULL);
}

PIGEON_ERR_RET pigeon_create_io_hub(PigeonIOHub* io_hub, 
    pigeon_io_hub_new_connection_callback new_connection_, 
    pigeon_io_hub_connection_closed_callback connection_closed_, 
    pigeon_io_hub_data_in_callback data_in_, 
    pigeon_io_hub_data_in_udp_callback data_in_udp_,
    pigeon_io_hub_write_done_callback write_done_
)
{
    ASSERT_R1(io_hub && new_connection_ && connection_closed_ && data_in_ && write_done_ && data_in_udp_);

#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_create_io_hub__uring(io_hub, new_connection_, connection_closed_, data_in_, data_in_udp_, write_done_);
    }
#endif

// TODO epoll io hub code

    return 1;
}


void pigeon_destroy_io_hub(PigeonIOHub* io_hub)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        pigeon_destroy_io_hub__uring(io_hub);
        return;
    }
#endif

    assert(io_hub);

// TODO
}


PIGEON_ERR_RET pigeon_io_hub_add_server_socket(PigeonIOHub* io_hub, PigeonNetworkSocket* socket)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_io_hub_add_server_socket__uring(io_hub, socket);
    }
#endif

    // TODO
    return 1;

}


PIGEON_ERR_RET pigeon_io_hub_wait(PigeonIOHub* io_hub)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_io_hub_wait__uring(io_hub);
    }
#endif

    // TODO
    return 1;
}

PIGEON_ERR_RET pigeon_io_hub_poll(PigeonIOHub* io_hub)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_io_hub_poll__uring(io_hub);
    }
#endif

    // TODO
    return 1;
}

PIGEON_ERR_RET pigeon_io_hub_close_tcp_socket(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_io_hub_close_tcp_socket__uring(io_hub, h);
    }
#endif
//TODO
    return 1;
}


void pigeon_io_hub_set_connection_object(PigeonIOHubConnectionHandle h, PigeonIOHubConnectionObject obj)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        pigeon_io_hub_set_connection_object__uring(h, obj);
        return;
    }
#endif
//TODO
}


PIGEON_ERR_RET pigeon_io_hub_tcp_read(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_io_hub_tcp_read__uring(io_hub, h);
    }
#endif
//TODO
    return 1;
}


PIGEON_ERR_RET pigeon_io_hub_tcp_shutdown_read(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_io_hub_tcp_shutdown_read__uring(io_hub, h);
    }
#endif
//TODO
    return 1;
}

PIGEON_ERR_RET pigeon_io_hub_tcp_shutdown_write(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_io_hub_tcp_shutdown_write__uring(io_hub, h);
    }
#endif
//TODO
    return 1;
}

PIGEON_ERR_RET pigeon_io_hub_tcp_send(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h,
    const void * data, unsigned int data_length, uint64_t arg)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_io_hub_tcp_send__uring(io_hub, h, data, data_length, arg);
    }
#endif
//TODO
    return 1;
}


PIGEON_ERR_RET pigeon_io_hub_udp_send(PigeonIOHub* io_hub, PigeonNetworkSocket* socket,
    PigeonIPv6Address remote_ip, PigeonNetworkPort remote_port,
    const void * data, unsigned int data_length)
{
#ifdef ENABLE_IO_URING
    if(io_uring_supported) {
        return pigeon_io_hub_udp_send__uring(io_hub, socket, remote_ip, remote_port, data, data_length);
    }
#endif
//TODO
    return 1;
}