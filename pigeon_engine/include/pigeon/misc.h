#pragma once

#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

// Returns malloc'd pointer. Call free on it when done
void* pigeon_load_file(const char* file, unsigned int extra, unsigned long * file_size);

#define free_if(x) \
    if((x)) { free(x); x = NULL; }

#define free2(x) \
    { assert((x)); free_if((x)); }


static inline unsigned int round_up(unsigned int x, unsigned int round)
{
    return ((x + round-1) / round) * round;
}


static inline float mixf(float a, float b, float x)
{
	return x*b + (1.0f-x)*a;
}

static inline double mix(double a, double b, double x)
{
	return x*b + (1.0-x)*a;
}
