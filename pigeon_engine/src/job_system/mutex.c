#include <pigeon/job_system/threading.h>
#include <pigeon/assert.h>


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#include <Windows.h>

PigeonMutex pigeon_create_mutex(void)
{
	CRITICAL_SECTION * cs = malloc(sizeof(CRITICAL_SECTION));
    ASSERT_R0(cs);
	InitializeCriticalSection(cs);
	return cs;
}

void pigeon_aquire_mutex(PigeonMutex mutex)
{
    EnterCriticalSection((CRITICAL_SECTION *)mutex);
}

void pigeon_release_mutex(PigeonMutex mutex)
{
    LeaveCriticalSection((CRITICAL_SECTION *)mutex);
}

void pigeon_destroy_mutex(PigeonMutex mutex)
{
    assert(mutex);

    if(mutex) {
        DeleteCriticalSection((CRITICAL_SECTION *)mutex);
	    free(mutex);
    }
}

#else

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t matrix_template = PTHREAD_MUTEX_INITIALIZER;

PigeonMutex pigeon_create_mutex(void)
{
    pthread_mutex_t * mutex = malloc(sizeof(pthread_mutex_t));
    ASSERT_R0(mutex);
	memcpy(mutex, &matrix_template, sizeof(pthread_mutex_t));
	return mutex;
}

void pigeon_aquire_mutex(PigeonMutex mutex)
{
    pthread_mutex_lock((pthread_mutex_t *)mutex);
}

void pigeon_release_mutex(PigeonMutex mutex)
{
    pthread_mutex_unlock((pthread_mutex_t *)mutex);
}

void pigeon_destroy_mutex(PigeonMutex mutex)
{
    assert(mutex);

    if(mutex) {
        pthread_mutex_destroy((pthread_mutex_t *)mutex);
	    free(mutex);
    }
}


#endif
