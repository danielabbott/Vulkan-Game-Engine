/*
    When running this test, the following are required: 
        Internet access (www.wikipedia.org)
        A local UDP server (python3 code provided below)

from socket import socket,AF_INET,SOCK_DGRAM

reponse = str.encode("Hello UDP Client")
UDPServerSocket = socket(family=AF_INET, type=SOCK_DGRAM)
UDPServerSocket.bind(("127.0.0.1", 25565))
print("Listening...")

while(True):
    data_in = UDPServerSocket.recvfrom(16)
    message,address = data_in[0],data_in[1]
    print("Messsage: ", message, ", from: ", address)
    UDPServerSocket.sendto(reponse, address)
*/

#include <pigeon/assert.h>
#include <pigeon/util.h>
#include <pigeon/io/socket.h>
#include <pigeon/io/tls.h>
#include <pigeon/io/http.h>
#include <pigeon/io/io_hub.h>
#include <pigeon/io/server_socket.h>
#include <pigeon/object_pool.h>
#include <string.h>
#include <stdlib.h>

static int test_tcp(void)
{
    puts("Testing TCP...");

    PigeonNetworkSocket socket = {0};

	ASSERT_R1(!pigeon_network_create_client_socket_blocking(&socket,
		"www.wikipedia.org", 80, PIGEON_NETWORK_PROTOCOL_TCP, false));

	const char * send = HTTP_GET_HOMEPAGE;

	unsigned int bytes_sent = 0;
	ASSERT_R1(!pigeon_network_send_blocking(&socket, (unsigned int)strlen(send), 
		&bytes_sent, send));
    ASSERT_R1(bytes_sent == (unsigned) strlen(send));

    puts("The HTTP response for the main page of www.wikipedia.org should be shown below:");

    char * recv = malloc(1024*1024);
	ASSERT_R1(recv);

	for(unsigned int i = 0; i < 40; i++) {
	    unsigned int bytes_received = 0;
		ASSERT_R1(!pigeon_network_recv_blocking(&socket, 1024*1024-1, &bytes_received, recv));	
		recv[bytes_received] = 0;
		if(bytes_received) puts(recv);
	}
	free(recv);

	pigeon_network_close_socket_blocking(&socket);

    puts("TCP test complete.\n");
    return 0;
}

static int test_tls(void)
{
    puts("Testing TLS...");
    PigeonNetworkClientTLSConnection tls = {0};

	ASSERT_R1(!pigeon_network_create_client_tls_connection_blocking(&tls, "www.wikipedia.org", 443, false));

	const char * send = HTTP_GET_HOMEPAGE;
	
	unsigned int bytes_sent = 0;

	ASSERT_R1(!pigeon_network_client_tls_send_blocking(&tls, (unsigned)strlen(send), &bytes_sent, send));
	ASSERT_R1(bytes_sent == strlen(send));

    puts("The HTTP response for the main page of www.wikipedia.org should be shown below:");

	char * recv = malloc(1024*1024);
	ASSERT_R1(recv);

	for(unsigned int i = 0; i < 40; i++) {
	    unsigned int bytes_received = 0;
		ASSERT_R1(!pigeon_network_client_tls_recv_blocking(&tls, 1024*1024-1, &bytes_received, recv));	
		recv[bytes_received] = 0;
		if(bytes_received)puts(recv);
	}
	free(recv);

	pigeon_network_close_client_tls_connection_blocking(&tls);

    puts("TLS test complete\n");
    return 0;
}

static int test_udp(void)
{
    puts("Testing UDP...");

    PigeonNetworkSocket socket = {0};

	ASSERT_R1(!pigeon_network_create_client_socket_blocking(&socket,
		"127.0.0.1", 25565, PIGEON_NETWORK_PROTOCOL_UDP, false));

	const char * msg = "TEST MESSAGE";

	unsigned int sent = 0;
	ASSERT_R1(!pigeon_network_send_blocking(&socket, (unsigned int)strlen(msg), 
		&sent, msg));

    puts("UDP message sent. Check server for test message. Response should be shown below:");

	char in[256];

	unsigned int recvd = 0;
	ASSERT_R1(!pigeon_network_recv_blocking(&socket, 255, 
		&recvd, in));

	in[recvd] = 0;
	puts(in);

	pigeon_network_close_socket_blocking(&socket);

    puts("UDP test complete.\n");
    return 0;
}


#if !defined(WIN32) && !defined(_WIN32) && !defined(__WIN32__) && !defined(__NT__)
PIGEON_ERR_RET pigeon_test_server(void);
#endif

int main(int argc, char ** argv)
{   
	ASSERT_R1(!pigeon_init_sockets_api());
	ASSERT_R1(!pigeon_init_openssl());


#if !defined(WIN32) && !defined(_WIN32) && !defined(__WIN32__) && !defined(__NT__)
	bool test_server = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "server") == 0) {
			test_server = true;
			break;
		}
	}

	if (test_server) {
		ASSERT_R1(!pigeon_test_server());
	}
	else {
		ASSERT_R1(!test_tcp());
		ASSERT_R1(!test_tls());
		ASSERT_R1(!test_udp());
	}
#endif


	ASSERT_R1(!test_tcp());
	ASSERT_R1(!test_tls());
	ASSERT_R1(!test_udp());

	pigeon_deinit_openssl();
	pigeon_deinit_sockets_api();
    return 0;
}