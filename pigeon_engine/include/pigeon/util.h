#pragma once

#include <stdbool.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <sal.h>
#define PIGEON_ERR_RET _Check_return_ int
#define PIGEON_CHECK_RET _Check_return_
#else
#define PIGEON_ERR_RET int __attribute__((warn_unused_result))
#define PIGEON_CHECK_RET __attribute__((warn_unused_result))
#endif
