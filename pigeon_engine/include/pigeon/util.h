#pragma once

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define ERROR_RETURN_TYPE _Check_return_ int
#else
#define ERROR_RETURN_TYPE int __attribute__((warn_unused_result))
#endif

#define ASSERT_1(x) if(!(x)) {assert(false); return 1;}
#define ASSERT_0(x) if(!(x)) {assert(false); return 0;}
#define ASSERT_x(x, r) if(!(x)) {assert(false); return r;}

#define ERRLOG(s) fputs(s "\n", stderr)

#define ASSERT__1(x, s) if(!(x)) {ERRLOG(s); assert(false); return 1;}
#define ASSERT__0(x, s) if(!(x)) {ERRLOG(s); assert(false); return 0;}
#define ASSERT__x(x, s, r) if(!(x)) {ERRLOG(s); assert(false); return r;}

// Returns malloc'd pointer. Call free on it when done
size_t* load_file(const char* file, unsigned int extra, unsigned long * file_size);


static inline void free2(void**p)
{
    assert(p);
    if(*p) {
        free(*p);
        *p = NULL;
    }
}

unsigned int round_up(unsigned int x, unsigned int round);
