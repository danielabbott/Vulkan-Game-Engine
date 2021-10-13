#pragma once

#include <stdbool.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define PIGEON_ERR_RET _PIGEON_CHECK_RET_ int
#define PIGEON_CHECK_RET _PIGEON_CHECK_RET_
#else
#define PIGEON_ERR_RET int __attribute__((warn_unused_result))
#define PIGEON_CHECK_RET __attribute__((warn_unused_result))
#endif
