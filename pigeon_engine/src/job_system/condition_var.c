#include <pigeon/job_system/threading.h>
#include <pigeon/assert.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#include <Windows.h>

typedef struct CV
{
	CONDITION_VARIABLE cond;
	CRITICAL_SECTION mutex;
} CV;

PigeonConditionVariable pigeon_create_condition_variable()
{
	CV * cv = malloc(sizeof(CV));
	ASSERT_R0(cv);

	InitializeCriticalSection(&cv->mutex);
	InitializeConditionVariable(&cv->cond);

	return (PigeonConditionVariable)cv;
}

void pigeon_wait_on_condition_variable(PigeonConditionVariable cv_)
{
    assert(cv_);
	CV * cv = (CV *)cv_;

	EnterCriticalSection(&cv->mutex);
	SleepConditionVariableCS(&cv->cond, &cv->mutex, INFINITE);
	LeaveCriticalSection(&cv->mutex);
}

void pigeon_notify_condition_variable(PigeonConditionVariable cv_)
{
    assert(cv_);
	CV * cv = (CV *)cv_;

	EnterCriticalSection(&cv->mutex);
	WakeConditionVariable(&cv->cond);
	LeaveCriticalSection(&cv->mutex);
}

void pigeon_destroy_condition_variable(PigeonConditionVariable cv_)
{
    assert(cv_);
	CV * cv = (CV *)cv_;
    // TODO deinit
	free(cv);
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

typedef struct CV
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
} CV;

static pthread_mutex_t matrix_template = PTHREAD_MUTEX_INITIALIZER;

PigeonConditionVariable pigeon_create_condition_variable(void)
{
    CV * cv = malloc(sizeof(CV));
	ASSERT_R0(cv);

	if(pthread_cond_init(&cv->cond, NULL)) {
        free(cv);
        ASSERT_R0(0);
    }

	memcpy(&cv->mutex, &matrix_template, sizeof(pthread_mutex_t));

	return cv;
}

void pigeon_wait_on_condition_variable(PigeonConditionVariable cv_)
{
    assert(cv_);
	CV * cv = cv_;

	pthread_mutex_lock(&cv->mutex);
	pthread_cond_wait(&cv->cond, &cv->mutex);
   	pthread_mutex_unlock(&cv->mutex);
}

void pigeon_notify_condition_variable(PigeonConditionVariable cv_)
{
    assert(cv_);
	CV * cv = cv_;


	pthread_mutex_lock(&cv->mutex);
	pthread_cond_signal(&cv->cond);
   	pthread_mutex_unlock(&cv->mutex);
}

void pigeon_destroy_condition_variable(PigeonConditionVariable cv_)
{
    assert(cv_);
	CV * cv = cv_;

    pthread_cond_destroy(&cv->cond);

	free(cv);
}


#endif