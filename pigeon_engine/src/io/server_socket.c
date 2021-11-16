#include <pigeon/io/server_socket.h>
#include <pigeon/util.h>
#include <pigeon/assert.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define WIN32_LEAN_AND_MEAN // must be defined for winsock2
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define CLOSE_SOCKET(x) closesocket(x)
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
#endif

PIGEON_ERR_RET pigeon_network_create_server_socket_blocking(PigeonNetworkSocket* sock,
    PigeonNetworkPort port, PigeonNetworkProtocol protocol)
{
    ASSERT_R1(sock && port);

    sock->handle = socket(AF_INET6,
        protocol == PIGEON_NETWORK_PROTOCOL_UDP ? SOCK_DGRAM : SOCK_STREAM,
        protocol == PIGEON_NETWORK_PROTOCOL_UDP ? IPPROTO_UDP : IPPROTO_TCP);

    ASSERT_R1(sock->handle != INVALID_SOCKET);

    if(protocol == PIGEON_NETWORK_PROTOCOL_TCP) {
        // Allow multiple sockets to be bound to the same port
        // So multiple threads can use different listening sockets on the same port

        int one = 1;
        if(setsockopt(sock->handle, SOL_SOCKET, SO_REUSEPORT, &one, sizeof one)) {
            CLOSE_SOCKET(sock->handle);
            ASSERT_R1(false);
        }
    }

    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    if (bind(sock->handle, (struct sockaddr *)&addr, sizeof addr) < 0) {
        CLOSE_SOCKET(sock->handle);
        ASSERT_R1(false);
    }
    

    if(protocol == PIGEON_NETWORK_PROTOCOL_TCP) {
        if (listen(sock->handle, 256) < 0) {
            CLOSE_SOCKET(sock->handle);
            ASSERT_R1(false);
        }
    }

    sock->port = port;
    sock->protocol = protocol;
    sock->is_server_socket = true;

    return 0;
}