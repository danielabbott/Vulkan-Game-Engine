#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

static inline void CLEANUP(void) {}

#define ASSERT_R1(x) if(!(x)) {assert(false); CLEANUP(); return 1;}
#define ASSERT_R0(x) if(!(x)) {assert(false); CLEANUP(); return 0;}
#define ASSERT_R(x, r) if(!(x)) {assert(false); CLEANUP(); return r;}

#define ERRLOG(s) fputs(s "\n", stderr)

#define ASSERT_LOG_R1(x, s) if(!(x)) {ERRLOG(s); assert(false); CLEANUP(); return 1;}
#define ASSERT_LOG_R0(x, s) if(!(x)) {ERRLOG(s); assert(false); CLEANUP(); return 0;}
#define ASSERT_LOG_R(x, s, r) if(!(x)) {ERRLOG(s); assert(false); CLEANUP(); return r;}

