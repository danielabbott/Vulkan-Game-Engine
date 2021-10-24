#include <pigeon/job_system/threading.h>
#include <pigeon/assert.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#include <Windows.h>


PigeonThread pigeon_start_thread(PigeonThreadFunction f, void* arg0)
{
    PigeonThread thread = (PigeonThread) CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f, arg0, 0, NULL);
    assert(thread);
	return thread;
}

void pigeon_join_thread(PigeonThread thread)
{
    WaitForSingleObject((HANDLE)thread, INFINITE);
}

#else

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <bits/time.h>
#include <stdbool.h>
#include <string.h>

PigeonThread pigeon_start_thread(PigeonThreadFunction f, void* arg0)
{
    PigeonThread thread;
	ASSERT_R0(!pthread_create ((unsigned long *) &thread, NULL, (void * (*)(void *))f, (void *) arg0));
	return thread;
}

void pigeon_join_thread(PigeonThread thread)
{
    pthread_join((unsigned long) thread, NULL);
}

#endif