#define OPENSSL_API_COMPAT 0x10100000L
#define DEBUG_UNUSED
#define OPENSSL_NO_DEPRECATED
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/tls1.h>
#include <openssl/cryptoerr.h>
#include <pigeon/util.h>
