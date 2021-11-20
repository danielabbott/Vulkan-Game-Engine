/*
This creates a very basic HTTP server on port 25565.
It responds with 200 OK 'bye', regardless of what data the client sends.
If data is received on UDP port 25565, a message is sent on UDP port 25566.
*/

#include <pigeon/assert.h>
#include <pigeon/io/http.h>
#include <pigeon/io/io_hub.h>
#include <pigeon/io/server_socket.h>
#include <pigeon/io/socket.h>
#include <pigeon/io/tls.h>
#include <pigeon/object_pool.h>
#include <pigeon/util.h>
#include <stdlib.h>
#include <string.h>

typedef struct Connection {
	PigeonIPv6Address ip;
	PigeonNetworkPort port;

	// char write_buffer[1024];
} Connection;

PigeonObjectPool connection_object_pool; // Connection
PigeonNetworkSocket tcp_socket;
PigeonNetworkSocket udp_socket;
char udp_read_buffer[512];

static void new_connection(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h, PigeonNetworkSocket* socket,
	PigeonIPv6Address ip, PigeonNetworkPort port)
{
	Connection* conn = pigeon_object_pool_allocate(&connection_object_pool);
	if (!conn) {
		assert(false);
		if (pigeon_io_hub_close_tcp_socket(io_hub, h)) {
			assert(false);
			exit(1);
		}

		return;
	}

	char ip_string[40];
	pigeon_ipv6_to_string(ip, ip_string);

	assert(socket == &tcp_socket);
	printf("New TCP connection: IP %s, port %u\n", ip_string, port);

	conn->ip = ip;
	conn->port = port;

	pigeon_io_hub_set_connection_object(h, conn);

	if (pigeon_io_hub_tcp_read(io_hub, h)) {
		assert(false);
		exit(1);
	}
}

static void connection_closed(PigeonIOHub* io_hub, PigeonIOHubConnectionObject conn_)
{
	(void)io_hub;
	Connection* conn = conn_;

	char ip_string[40];
	pigeon_ipv6_to_string(conn->ip, ip_string);

	printf("connection from %s:%u closed\n", ip_string, conn->port);

	pigeon_object_pool_free(&connection_object_pool, conn);
}

static void data_in(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h, PigeonIOHubConnectionObject conn_,
	unsigned int data_length, void* data)
{
	Connection* conn = conn_;
	printf("Received %u bytes from port %u: %.*s\n", data_length, conn->port, data_length, (char*)data);

	const char* s = "HTTP/1.1 200 OK\r\n"
					"Content-Length: 3\r\n"
					"Content-Type: text/plain\r\n"
					"Connection: close\r\n\r\n"
					"bye";

	if (pigeon_io_hub_tcp_send(io_hub, h, s, (unsigned)strlen(s), 0) || pigeon_io_hub_tcp_shutdown_read(io_hub, h)) {
		assert(false);
		exit(1);
	}
}

static void data_in_udp(PigeonIOHub* io_hub, PigeonIPv6Address remote_ip, PigeonNetworkPort remote_port,
	unsigned int data_length, void* data)
{
	char ip_string[40];
	pigeon_ipv6_to_string(remote_ip, ip_string);

	printf("Received %u bytes (UDP) from %s:%u: %.*s\n", data_length, ip_string, remote_port, data_length, (char*)data);

	const char* s = "hello";
	if (pigeon_io_hub_udp_send(io_hub, &udp_socket, remote_ip, 25566, s, (unsigned)strlen(s))) {
		assert(false);
		exit(1);
	}
}

static void write_done(PigeonIOHub* io_hub, PigeonIOHubConnectionHandle h, PigeonIOHubConnectionObject conn_,
	unsigned int data_length, const void* data, uint64_t arg0)
{
	(void)conn_;
	(void)data_length;
	(void)data;
	(void)arg0;
	Connection* conn = conn_;
	printf("Sent %u bytes to port %u\n", data_length, conn->port);
	int err = pigeon_io_hub_close_tcp_socket(io_hub, h);
	assert(!err);
}

PIGEON_ERR_RET pigeon_test_server(void);
PIGEON_ERR_RET pigeon_test_server(void)
{
	pigeon_io_hubs_init();
	pigeon_create_object_pool(&connection_object_pool, sizeof(connection_object_pool), true);

	ASSERT_R1(!pigeon_network_create_server_socket_blocking(&tcp_socket, 25565, PIGEON_NETWORK_PROTOCOL_TCP));

	ASSERT_R1(!pigeon_network_create_server_socket_blocking(&udp_socket, 25565, PIGEON_NETWORK_PROTOCOL_UDP));

	PigeonIOHub io = { 0 };

	ASSERT_R1(!pigeon_create_io_hub(&io, new_connection, connection_closed, data_in, data_in_udp, write_done));

	ASSERT_R1(!pigeon_io_hub_add_server_socket(&io, &tcp_socket));
	ASSERT_R1(!pigeon_io_hub_add_server_socket(&io, &udp_socket));
	// ASSERT_R1(!pigeon_io_hub_start_fopen(&io, "README.md"));

	while (true) {
		ASSERT_R1(!pigeon_io_hub_wait(&io));
	}

	pigeon_destroy_io_hub(&io);
	pigeon_network_close_socket_blocking(&tcp_socket);
	pigeon_network_close_socket_blocking(&udp_socket);
	pigeon_destroy_object_pool(&connection_object_pool);
}
