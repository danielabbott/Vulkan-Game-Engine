#include <pigeon/network/socket.h>
#include <pigeon/util.h>
#include <pigeon/assert.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define WIN32_LEAN_AND_MEAN // must be defined for winsock2
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

static WSADATA wsaData;

#define CLOSE_SOCKET(x) closesocket(x)
#define SOCKET_READ(x,y,z) recv(x,y,z,0)
#define SOCKET_WRITE(x,y,z) send(x,y,z,0)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>

#define INVALID_SOCKET -1

#define CLOSE_SOCKET(x) close(x)
#define SOCKET_READ read
#define SOCKET_WRITE write
#endif

PIGEON_ERR_RET pigeon_init_sockets_api(void)
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    ASSERT_R1(!WSAStartup(MAKEWORD(2, 2), &wsaData));
#endif

    return 0;
}

void pigeon_deinit_sockets_api(void)
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    WSACleanup();
#endif
}


static struct addrinfo* resolve_hostname(const char* remote_host, const char* port_string,
    PigeonNetworkProtocol protocol)
{
    struct addrinfo hints = { 0 };
    hints.ai_family = AF_UNSPEC; // ipv4 or ipv6
    hints.ai_socktype = protocol == PIGEON_NETWORK_PROTOCOL_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = protocol == PIGEON_NETWORK_PROTOCOL_UDP ? IPPROTO_UDP : IPPROTO_TCP;

    struct addrinfo* result;
    int s = s = getaddrinfo(remote_host, port_string, &hints, &result);
    ASSERT_R0(!s);

    return result;
}

PIGEON_ERR_RET pigeon_network_create_client_socket_blocking(PigeonNetworkSocket* sock,
    const char* remote_host, PigeonNetworkPort remote_port,
    PigeonNetworkProtocol protocol)
{
    ASSERT_R1(sock && remote_host && remote_port);

    char port_string[6];
    snprintf(port_string, sizeof port_string, "%u", remote_port);

    struct addrinfo* addresses = resolve_hostname(remote_host, port_string, protocol);
    ASSERT_R1(addresses);

    bool got_connection = false;

    for (int try_ipv4 = 0; try_ipv4 < 2; ++try_ipv4) {
        for (struct addrinfo* address = addresses; address != NULL; address = address->ai_next) {
            if (!try_ipv4 && address->ai_family != AF_INET6) continue;
            if (try_ipv4 && address->ai_family != AF_INET) continue;

            sock->handle = socket(address->ai_family,
                protocol == PIGEON_NETWORK_PROTOCOL_UDP ? SOCK_DGRAM : SOCK_STREAM,
                protocol == PIGEON_NETWORK_PROTOCOL_UDP ? IPPROTO_UDP : IPPROTO_TCP);

            if (sock->handle == INVALID_SOCKET) continue;

            if (connect(sock->handle, address->ai_addr, (int)address->ai_addrlen) < 0) {
                CLOSE_SOCKET(sock->handle);
                continue;
            }

            got_connection = true;
            break;
        }
        if (got_connection) break;
    }
    freeaddrinfo(addresses);

    sock->port = remote_port;
    sock->protocol = protocol;

    return 0;
}


PIGEON_ERR_RET pigeon_network_recv_blocking(PigeonNetworkSocket* sock, unsigned int max_bytes,
    unsigned int* bytes_received, void* buffer)
{
    ASSERT_R1(sock && sock->handle && max_bytes && bytes_received && buffer);

    int in = (int)SOCKET_READ(sock->handle, buffer, max_bytes);

    if (in < 0) {
        CLOSE_SOCKET(sock->handle);
        sock->handle = 0;
        ASSERT_R1(false);
    }

    *bytes_received = (unsigned)in;


    return 0;
}

PIGEON_ERR_RET pigeon_network_send_blocking(PigeonNetworkSocket* sock, unsigned int data_size,
    unsigned int* bytes_sent, const void* buffer)
{
    ASSERT_R1(sock && sock->handle && data_size && bytes_sent && buffer);

    int out = (int)SOCKET_WRITE(sock->handle, buffer, data_size);

    if (out < 0) {
        CLOSE_SOCKET(sock->handle);
        sock->handle = 0;
        ASSERT_R1(false);
    }

    *bytes_sent = (unsigned)out;

    return 0;
}

void pigeon_network_close_socket_blocking(PigeonNetworkSocket* sock)
{
    assert(sock);

    if (sock->handle) {
        CLOSE_SOCKET(sock->handle);
        sock->handle = 0;
    }

}