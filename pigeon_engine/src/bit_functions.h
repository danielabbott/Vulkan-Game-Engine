#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#include <stdint.h>
#include <intrin.h>

inline static unsigned int find_first_zero_bit(uint64_t bits)
{
	unsigned long i;
	unsigned char success = _BitScanForward64(&i, ~bits);
	return success ? i : -1;
}

#else

#define _GNU_SOURCE
#define __USE_MISC

#include <strings.h>
#include <stdint.h>

#undef _GNU_SOURCE
#undef __USE_MISC

inline static int find_first_zero_bit(uint64_t bits)
{
	int i = ffsll((int64_t)~bits);
	return (i > 0) ? (i - 1) : -1;
}

// inline static int find_first_zero_bit(uint64_t bits)
// {
// 	for(unsigned int i = 0; i < 64; i++) {
// 		if(!(bits & (1ull << i))) {
// 			return (int)i;
// 		}
// 	}
// 	return -1;
// }
#endif
