#define OPENSSL_API_COMPAT 0x10100000L
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#else
#define DEBUG_UNUSED
#endif
#define OPENSSL_NO_DEPRECATED
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/tls1.h>
#include <openssl/cryptoerr.h>
#include <pigeon/util.h>
