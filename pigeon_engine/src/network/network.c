#include <pigeon/network/socket.h>
#include <pigeon/util.h>
#include <pigeon/assert.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define WIN32_LEAN_AND_MEAN // must be defined for winsock2
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#endif

PIGEON_ERR_RET pigeon_init_network_module(void);
PIGEON_ERR_RET pigeon_init_network_module(void)
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    ASSERT_R1(!WSAStartup(MAKEWORD(2, 2), &wsaData));
#endif

    return 0;
}

void pigeon_deinit_network_module(void);
void pigeon_deinit_network_module(void)
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    WSACleanup();
#endif
}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#else

static struct addrinfo * resolve_hostname(const char * remote_host, const char * port_string,
    PigeonNetworkProtocol protocol)
{
    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC; // ipv4 or ipv6
    hints.ai_socktype = protocol == PIGEON_NETWORK_PROTOCOL_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = protocol == PIGEON_NETWORK_PROTOCOL_UDP ? IPPROTO_UDP : IPPROTO_TCP;

    struct addrinfo * result;
    int s = s = getaddrinfo(remote_host, port_string, &hints, &result);
    ASSERT_R0(!s);

    return result;
}

PIGEON_ERR_RET pigeon_network_create_client_socket_blocking(PigeonNetworkSocket* sock,
    const char * remote_host, PigeonNetworkPort remote_port,
    PigeonNetworkProtocol protocol)
{
    ASSERT_R1(sock && remote_host && remote_port);

    char port_string[6];
    snprintf(port_string, sizeof port_string, "%u", remote_port);
    
    struct addrinfo * addresses = resolve_hostname(remote_host, port_string, protocol);
    ASSERT_R1(addresses);

    bool got_connection = false;

    for(int try_ipv4 = 0; try_ipv4 < 2; ++try_ipv4) {
        for (struct addrinfo * address = addresses; address != NULL; address = address->ai_next) {
            if(!try_ipv4 && address->ai_family != AF_INET6) continue;
            if(try_ipv4 && address->ai_family != AF_INET) continue;

            sock->id = socket(address->ai_family,
                protocol == PIGEON_NETWORK_PROTOCOL_UDP ? SOCK_DGRAM : SOCK_STREAM, 
                protocol == PIGEON_NETWORK_PROTOCOL_UDP ? IPPROTO_UDP : IPPROTO_TCP);

            if(sock->id < 0) continue;

            if (connect(sock->id, address->ai_addr, address->ai_addrlen) < 0) {
                close(sock->id);
                continue;
            }

            got_connection = true;
            break;
        }
        if(got_connection) break;
    }
    freeaddrinfo(addresses);

    sock->port = remote_port;
    sock->protocol = protocol;

    return 0;
}


PIGEON_ERR_RET pigeon_network_recv_blocking(PigeonNetworkSocket* sock, unsigned int max_bytes, 
    unsigned int * bytes_received, void * buffer)
{
    ASSERT_R1(sock && sock->id && max_bytes && bytes_received && buffer);

    ssize_t in = read(sock->id, buffer, max_bytes);

    if(in < 0) {
        close(sock->id);
        sock->id = 0;
        ASSERT_R1(false);
    }

    *bytes_received = (unsigned) in;


    return 0;
}

PIGEON_ERR_RET pigeon_network_send_blocking(PigeonNetworkSocket* sock, unsigned int data_size, 
    unsigned int * bytes_sent, const void * buffer)
{
    ASSERT_R1(sock && sock->id && data_size && bytes_sent && buffer);

    ssize_t out = write(sock->id, buffer, data_size);

    if(out < 0) {
        close(sock->id);
        sock->id = 0;
        ASSERT_R1(false);
    }

    *bytes_sent = (unsigned) out;

    return 0;
}

void pigeon_network_close_socket_blocking(PigeonNetworkSocket* sock)
{
    assert(sock);

    if(sock->id) {
        close(sock->id);
        sock->id = 0;
    }

}

#endif