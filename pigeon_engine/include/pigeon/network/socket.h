#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pigeon/util.h>

typedef uint16_t PigeonNetworkPort;

typedef enum {
    PIGEON_NETWORK_PROTOCOL_TCP,
    PIGEON_NETWORK_PROTOCOL_UDP
} PigeonNetworkProtocol;

typedef struct PigeonNetworkSocket
{
    union {
        void * handle;
        int id;
    };

    PigeonNetworkProtocol protocol;
    PigeonNetworkPort port;
    bool is_server_socket;
} PigeonNetworkSocket;

PIGEON_ERR_RET pigeon_network_create_client_socket_blocking(PigeonNetworkSocket*,
    const char * remote_host, PigeonNetworkPort remote_port,
    PigeonNetworkProtocol);

PIGEON_ERR_RET pigeon_network_recv_blocking(PigeonNetworkSocket*, unsigned int max_bytes, 
    unsigned int * bytes_received, void * buffer);
PIGEON_ERR_RET pigeon_network_send_blocking(PigeonNetworkSocket*, unsigned int data_size, 
    unsigned int * bytes_sent, const void * buffer);

void pigeon_network_close_socket_blocking(PigeonNetworkSocket*);
