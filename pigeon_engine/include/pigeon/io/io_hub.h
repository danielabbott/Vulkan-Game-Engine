#pragma once
#include "ip.h"
#include "socket.h"

struct PigeonNetworkSocket;

void pigeon_io_hubs_init(void);

typedef struct PigeonIOHub
{
    void * data;
} PigeonIOHub;

typedef const void* PigeonIOHubConnectionHandle;
typedef void* PigeonIOHubConnectionObject;

typedef void (*pigeon_io_hub_new_connection_callback)(
    PigeonIOHub*, PigeonIOHubConnectionHandle, PigeonNetworkSocket* socket, 
    PigeonIPv6Address ip, PigeonNetworkPort port);

typedef void (*pigeon_io_hub_connection_closed_callback)(
    PigeonIOHub*, PigeonIOHubConnectionObject);

// Returns true if read-side of TCP socket should be shut down
typedef void (*pigeon_io_hub_data_in_callback)(
    PigeonIOHub*, PigeonIOHubConnectionHandle, PigeonIOHubConnectionObject,
	unsigned int data_length, void * data);

typedef void (*pigeon_io_hub_data_in_udp_callback)(
    PigeonIOHub*, PigeonIPv6Address remote_ip, PigeonNetworkPort remote_port,
	unsigned int data_length, void * data);

typedef void (*pigeon_io_hub_write_done_callback)(
    PigeonIOHub*, PigeonIOHubConnectionHandle, PigeonIOHubConnectionObject,
	unsigned int data_length, const void * data, uint64_t arg);

PIGEON_ERR_RET pigeon_create_io_hub(PigeonIOHub*, 
    pigeon_io_hub_new_connection_callback, 
    pigeon_io_hub_connection_closed_callback, 
    pigeon_io_hub_data_in_callback, 
    pigeon_io_hub_data_in_udp_callback,
    pigeon_io_hub_write_done_callback
);

// Once added, a socket cannot be removed or closed until the io hub is destroyed
PIGEON_ERR_RET pigeon_io_hub_add_server_socket(PigeonIOHub*, PigeonNetworkSocket*);
// TODO pigeon_io_hub_remove_server_socket ?

PIGEON_ERR_RET pigeon_io_hub_wait(PigeonIOHub*);
PIGEON_ERR_RET pigeon_io_hub_poll(PigeonIOHub*);

void pigeon_io_hub_set_connection_object(PigeonIOHubConnectionHandle, PigeonIOHubConnectionObject);
PIGEON_ERR_RET pigeon_io_hub_tcp_shutdown_read(PigeonIOHub*, PigeonIOHubConnectionHandle);
PIGEON_ERR_RET pigeon_io_hub_tcp_shutdown_write(PigeonIOHub*, PigeonIOHubConnectionHandle);
PIGEON_ERR_RET pigeon_io_hub_close_tcp_socket(PigeonIOHub*, PigeonIOHubConnectionHandle);

PIGEON_ERR_RET pigeon_io_hub_tcp_read(PigeonIOHub*, PigeonIOHubConnectionHandle);
PIGEON_ERR_RET pigeon_io_hub_tcp_send(PigeonIOHub*, PigeonIOHubConnectionHandle,
    const void * data, unsigned int data_length, uint64_t arg0);

// There is no udp read command. When a socket is added, read requests are queued automatically

PIGEON_ERR_RET pigeon_io_hub_udp_send(PigeonIOHub*, PigeonNetworkSocket*, PigeonIPv6Address remote_ip, PigeonNetworkPort remote_port,
    const void *, unsigned int data_length); 

void pigeon_destroy_io_hub(PigeonIOHub*);




