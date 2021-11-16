#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct PigeonIPv6Address {
	union {
		uint8_t bytes[16];
		uint16_t words[8];
		uint32_t dwords[4];
	};
} PigeonIPv6Address;

static inline PigeonIPv6Address pigeon_ipv6_localhost(void)
{
    PigeonIPv6Address ip = {0};
    ip.bytes[15] = 1;
    return ip;
}

void pigeon_ipv6_to_string(PigeonIPv6Address ip, char dst[40]);

