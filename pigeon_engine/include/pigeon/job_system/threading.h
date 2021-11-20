#pragma once

#include <stdint.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>

// untested

typedef struct PigeonAtomicInt {
	volatile int x;
} PigeonAtomicInt;

typedef struct PigeonAtomicPtr {
	volatile void* x;
} PigeonAtomicPtr;

static inline void pigeon_atomic_set_int(PigeonAtomicInt* atomic, int value) { atomic->x = value; }

static inline int pigeon_atomic_get_int(PigeonAtomicInt* atomic) { return atomic->x; }

static inline void pigeon_atomic_set_ptr(PigeonAtomicPtr* atomic, void* value) { atomic->x = value; }

static inline void* pigeon_atomic_get_ptr(PigeonAtomicPtr* atomic) { return atomic->x; }

static inline int pigeon_atomic_inc_int(PigeonAtomicInt* atomic) { return InterlockedIncrement(&atomic->x); }

static inline int pigeon_atomic_dev_int(PigeonAtomicInt* atomic) { return InterlockedDecrement(&atomic->x); }

static inline void* pigeon_atomic_swap_ptr(PigeonAtomicPtr* atomic, void* value)
{
	return InterlockedExchangePointer(&atomic->x, value);
}

void* pigeon_atomic_swap_ptr(PigeonAtomicPtr* atomic, void* value);

#define thread_local __declspec(thread)

#else
#include <stdatomic.h>

typedef struct PigeonAtomicInt {
	atomic_int x;
} PigeonAtomicInt;

typedef struct PigeonAtomicPtr {
	atomic_uintptr_t x;
} PigeonAtomicPtr;

static inline void pigeon_atomic_set_int(PigeonAtomicInt* atomic, int value) { atomic_store(&atomic->x, value); }

static inline int pigeon_atomic_get_int(PigeonAtomicInt* atomic) { return atomic_load(&atomic->x); }

static inline int pigeon_atomic_inc_int(PigeonAtomicInt* atomic) { return atomic_fetch_add(&atomic->x, 1); }

static inline int pigeon_atomic_dec_int(PigeonAtomicInt* atomic) { return atomic_fetch_add(&atomic->x, -1); }

static inline void pigeon_atomic_set_ptr(PigeonAtomicPtr* atomic, void* value)
{
	atomic_store(&atomic->x, (uintptr_t)value);
}

static inline void* pigeon_atomic_get_ptr(PigeonAtomicPtr* atomic) { return (void*)atomic_load(&atomic->x); }

static inline void* pigeon_atomic_swap_ptr(PigeonAtomicPtr* atomic, void* value)
{
	return (void*)atomic_exchange(&atomic->x, (uintptr_t)value);
}

#define thread_local __thread

#endif

typedef void* PigeonThread;

typedef void (*PigeonThreadFunction)(void* arg0);

// Start thread and run the given function with the given arguments
PigeonThread pigeon_start_thread(PigeonThreadFunction, void* arg0);

// Wait for thread then destroy
void pigeon_join_thread(PigeonThread);

// Sleep current thread
// void pigeon_thread_sleep(unsigned int milliseconds);

typedef void* PigeonMutex;

PigeonMutex pigeon_create_mutex(void);
void pigeon_aquire_mutex(PigeonMutex);
void pigeon_release_mutex(PigeonMutex);

// Returns 1 if the mutex is currently locked, 0 if it was not and has now been locked
// int pigeon_try_mutex(PigeonMutex);

// Mutex lock must be released before mutex destruction
void pigeon_destroy_mutex(PigeonMutex);

typedef void* PigeonConditionVariable;

PigeonConditionVariable pigeon_create_condition_variable(void);
void pigeon_wait_on_condition_variable(PigeonConditionVariable);
void pigeon_notify_condition_variable(PigeonConditionVariable);
void pigeon_destroy_condition_variable(PigeonConditionVariable);
