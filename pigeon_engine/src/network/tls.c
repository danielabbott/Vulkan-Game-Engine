#include <pigeon/network/tls.h>
#include <pigeon/assert.h>
#include "tls.h"
#include <string.h>

static SSL_CTX* openssl_context;
static const SSL_METHOD* method;

PIGEON_ERR_RET pigeon_init_openssl(void);
PIGEON_ERR_RET pigeon_init_openssl(void)
{
#define OPENSSL_ASSERT_R1(x) \
    { \
        if(!(x)) { \
            ERR_print_errors_fp(stderr); \
            ASSERT_R1(false); \
        } \
    }

    method = TLS_client_method();
    OPENSSL_ASSERT_R1(method);

    openssl_context = SSL_CTX_new(method);
    OPENSSL_ASSERT_R1(openssl_context);    
    
    SSL_CTX_set_verify(openssl_context, SSL_VERIFY_PEER, NULL);

    SSL_CTX_ctrl(openssl_context, SSL_CTRL_SET_MIN_PROTO_VERSION, TLS1_2_VERSION, NULL);
    SSL_CTX_ctrl(openssl_context, SSL_CTRL_MODE, SSL_MODE_RELEASE_BUFFERS, NULL);

    OPENSSL_ASSERT_R1(SSL_CTX_set_cipher_list(openssl_context, "HIGH:!ADH:!MD5:!RC4:!SRP:!PSK:!DSS:!aNULL:!kRSA") == 1); // TLS1.2

    FILE * ca_cert_file = fopen("/etc/ssl/certs/ca-certificates.crt", "rb");
    if(ca_cert_file) {
        fclose(ca_cert_file);
        OPENSSL_ASSERT_R1(SSL_CTX_load_verify_locations(openssl_context, 
            "/etc/ssl/certs/ca-certificates.crt", NULL) == 1);
    }
    else {
        OPENSSL_ASSERT_R1(SSL_CTX_load_verify_locations(openssl_context,
            "build/standard_assets/ca-certificates.crt", NULL) == 1);
    }

    ERR_print_errors_fp(stderr);
#undef OPENSSL_ASSERT_R1
    return 0;
}


PIGEON_ERR_RET pigeon_network_create_client_tls_connection_blocking(PigeonNetworkClientTLSConnection* conn, 
    const char * remote_host, PigeonNetworkPort remote_port)
{

#define CLEANUP() pigeon_network_close_client_tls_connection_blocking(conn);

#define OPENSSL_ASSERT_R1(x) \
    { \
        if(!(x)) { \
            ERR_print_errors_fp(stderr); \
            ASSERT_R1(false); \
        } \
    }


    conn->bio = BIO_new_ssl_connect(openssl_context);
    OPENSSL_ASSERT_R1(conn->bio);

    unsigned int host_string_max_len = (unsigned)strlen(remote_host) + 6;
    char * host = malloc(host_string_max_len);
    ASSERT_R1(host);

    snprintf(host, host_string_max_len, "%s:%u", remote_host, remote_port);

    BIO_set_conn_hostname(conn->bio, host);
    free(host);

    BIO_get_ssl(conn->bio, &conn->ssl);
    OPENSSL_ASSERT_R1(conn->ssl);

    SSL_set_connect_state(conn->ssl);

    SSL_set_tlsext_host_name(conn->ssl, remote_host);

    OPENSSL_ASSERT_R1(BIO_do_connect(conn->bio) == 1);
    OPENSSL_ASSERT_R1(BIO_do_handshake(conn->bio) == 1);


#undef OPENSSL_ASSERT_R1
#undef CLEANUP
    return 0;
}

void pigeon_network_close_client_tls_connection_blocking(PigeonNetworkClientTLSConnection* conn)
{
    assert(conn);

    if(conn) {
        if(conn->bio) BIO_free_all(conn->bio);
        memset(conn, 0, sizeof *conn);
    }
}

PIGEON_ERR_RET pigeon_network_client_tls_recv_blocking(PigeonNetworkClientTLSConnection* conn, unsigned int max_bytes, 
    unsigned int * bytes_received, void * data)
{
    ASSERT_R1(max_bytes <= INT32_MAX);

    int recvd = BIO_read(conn->bio, data, (int)max_bytes);
    ASSERT_R1(recvd >= 0);

    if((unsigned) recvd > max_bytes)
        recvd = (int)max_bytes;

    *bytes_received = (unsigned)recvd;
    return 0;  
}

PIGEON_ERR_RET pigeon_network_client_tls_send_blocking(PigeonNetworkClientTLSConnection* conn, unsigned int data_size, 
    unsigned int * bytes_sent, const void * data)
{
    ASSERT_R1(data_size <= INT32_MAX);

    int sent = BIO_write(conn->bio, data, (int)data_size);
    ASSERT_R1(sent >= 0);

    if((unsigned) sent > data_size)
        sent = (int)data_size;

    *bytes_sent = (unsigned)sent;
    return 0;    
}


void pigeon_deinit_openssl(void);
void pigeon_deinit_openssl(void)
{
    if(openssl_context) {
        SSL_CTX_free(openssl_context);
        openssl_context = NULL;
    }
}

