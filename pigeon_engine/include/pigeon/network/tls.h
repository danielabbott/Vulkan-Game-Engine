#pragma once

#include <pigeon/util.h>
#include <pigeon/network/socket.h>

PIGEON_ERR_RET pigeon_init_openssl(void);
void pigeon_deinit_openssl(void);

struct bio_st;
struct ssl_st;

typedef struct PigeonNetworkClientTLSConnection
{
    struct bio_st * bio;
    struct ssl_st * ssl;
} PigeonNetworkClientTLSConnection;


// N.B. This does not use PigeonNetworkSocket, OpenSSL handles the TCP connection
PIGEON_ERR_RET pigeon_network_create_client_tls_connection_blocking(PigeonNetworkClientTLSConnection*,
    const char * remote_host, PigeonNetworkPort remote_port, bool no_ipv6);


PIGEON_ERR_RET pigeon_network_client_tls_recv_blocking(PigeonNetworkClientTLSConnection*, unsigned int max_bytes, 
    unsigned int * bytes_received, void * data);
PIGEON_ERR_RET pigeon_network_client_tls_send_blocking(PigeonNetworkClientTLSConnection*, unsigned int data_size, 
    unsigned int * bytes_sent, const void * data);

void pigeon_network_close_client_tls_connection_blocking(PigeonNetworkClientTLSConnection*);
