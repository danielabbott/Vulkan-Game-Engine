#pragma once

#include <pigeon/array_list.h>
#include <pigeon/util.h>
#include <stdint.h>

#define PIGEON_GROUP_SIZE_DIV64 2

typedef struct PigeonObjectPool {
	unsigned int object_size;
	PigeonArrayList group_bitmaps; // array of GroupBitmap structs
	PigeonArrayList group_data_pointers; // array of void*
	unsigned int alloc_search_start_i;
	bool zero_on_allocate; // allocated objects are automatically memset to 0
	unsigned int allocated_obj_count;
} PigeonObjectPool;

void pigeon_create_object_pool(PigeonObjectPool*, unsigned int object_size, bool zero_on_allocate);
PIGEON_ERR_RET pigeon_create_object_pool2(
	PigeonObjectPool*, unsigned int object_size, unsigned int capacity, bool zero_on_allocate);

// zero = false is useful for situations where objects are never destroyed but are reused instead
PIGEON_CHECK_RET void* pigeon_object_pool_allocate(PigeonObjectPool*);
void pigeon_object_pool_free(PigeonObjectPool*, void* element);

// PIGEON_ERR_RET pigeon_object_pool_allocate_multiple(PigeonObjectPool*, unsigned int n, void ** output_pointers);
// void pigeon_object_pool_free_multiple(PigeonObjectPool*, unsigned int n, void ** pointers);

void pigeon_destroy_object_pool(PigeonObjectPool*);

// callback parameters:
//  e: element in object pool
//  x: custom parameter passed each time
void pigeon_object_pool_for_each(PigeonObjectPool*, void (*)(void* e));
void pigeon_object_pool_for_each2(PigeonObjectPool*, void (*)(void* e, void* x), void* x);

// Stops after limit objects
// If continue_if_error is true then the function will not stop if an error occurs, returned code will be 1 if multiple
// different error codes are encountered
PIGEON_ERR_RET pigeon_object_pool_for_each3(
	PigeonObjectPool*, PIGEON_ERR_RET (*)(void* e), unsigned int limit, bool continue_if_error);