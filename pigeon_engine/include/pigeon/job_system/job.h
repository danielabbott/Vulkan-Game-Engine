#pragma once

#include "threading.h"
#include <pigeon/util.h>


typedef PIGEON_ERR_RET (*PigeonJobFunction)(uint64_t arg0, void* arg1);

typedef struct PigeonJob {
    PigeonJobFunction function;
    uint64_t arg0;
    void* arg1;
} PigeonJob;

// First job is guaranteed to run on the main thread
PIGEON_ERR_RET pigeon_dispatch_jobs(PigeonJob*, unsigned int n);
