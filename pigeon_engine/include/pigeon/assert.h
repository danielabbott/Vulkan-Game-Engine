#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#define ASSERT_R1(x) if(!(x)) {assert(false); return 1;}
#define ASSERT_R0(x) if(!(x)) {assert(false); return 0;}
#define ASSERT_R(x, r) if(!(x)) {assert(false); return r;}

#define ASSERT_R1_CLEANUP(x) if(!(x)) {assert(false); CLEANUP(); return 1;}
#define ASSERT_R0_CLEANUP(x) if(!(x)) {assert(false); CLEANUP(); return 0;}
#define ASSERT_R_CLEANUP(x, r) if(!(x)) {assert(false); CLEANUP(); return r;}

#define ERRLOG(s) fputs(s "\n", stderr)

#define ASSERT_LOG_R1(x, s) if(!(x)) {ERRLOG(s); assert(false); return 1;}
#define ASSERT_LOG_R0(x, s) if(!(x)) {ERRLOG(s); assert(false); return 0;}
#define ASSERT_LOG_R(x, s, r) if(!(x)) {ERRLOG(s); assert(false); return r;}

#define ASSERT_LOG_R1_CLEANUP(x, s) if(!(x)) {ERRLOG(s); assert(false); CLEANUP(); return 1;}
#define ASSERT_LOG_R0_CLEANUP(x, s) if(!(x)) {ERRLOG(s); assert(false); CLEANUP(); return 0;}
#define ASSERT_LOG_R_CLEANUP(x, s, r) if(!(x)) {ERRLOG(s); assert(false); CLEANUP(); return r;}

