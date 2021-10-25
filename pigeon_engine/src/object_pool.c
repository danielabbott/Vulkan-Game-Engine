#include "bit_functions.h"
#include <pigeon/object_pool.h>
#include <pigeon/assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct GroupBitmap
{
    uint64_t x[PIGEON_GROUP_SIZE_DIV64];
} GroupBitmap;

typedef void* GroupDataPointer;

void pigeon_create_object_pool(PigeonObjectPool* pool, unsigned int object_size, bool zero_on_allocate)
{
    assert(pool && !pool->object_size && object_size);

    pigeon_create_array_list(&pool->group_bitmaps, sizeof(GroupBitmap));
    pigeon_create_array_list(&pool->group_data_pointers, sizeof(GroupDataPointer));
    pool->object_size = object_size;
    pool->zero_on_allocate = zero_on_allocate;
}

PIGEON_ERR_RET pigeon_create_object_pool2(PigeonObjectPool* pool, unsigned int object_size,
    unsigned int capacity, bool zero_on_allocate)
{
    ASSERT_R1(pool && !pool->object_size && object_size);

    #define CLEANUP() pigeon_destroy_object_pool(pool);

    pigeon_create_array_list(&pool->group_bitmaps, sizeof(GroupBitmap));
    pigeon_create_array_list(&pool->group_data_pointers, sizeof(GroupDataPointer));
    pool->object_size = object_size;
    pool->zero_on_allocate = zero_on_allocate;

    unsigned int objects_per_group = PIGEON_GROUP_SIZE_DIV64*64;
    unsigned int groups_to_create = (capacity+objects_per_group-1) / objects_per_group;

    ASSERT_R1(pigeon_array_list_add(&pool->group_bitmaps, groups_to_create));
    memset(pool->group_bitmaps.elements, 0, groups_to_create * sizeof(GroupBitmap));

    ASSERT_R1(pigeon_array_list_add(&pool->group_data_pointers, groups_to_create));
    memset(pool->group_data_pointers.elements, 0, groups_to_create * sizeof(GroupDataPointer));



    for(unsigned int i = 0; i < groups_to_create; i++) {
        GroupDataPointer* g = &((GroupDataPointer *)pool->group_data_pointers.elements)[i];
        void * d = malloc(object_size * objects_per_group);
        ASSERT_R1(d);
        *g = d;
    }

    #undef CLEANUP

    return 0;
}


PIGEON_CHECK_RET void * pigeon_object_pool_allocate(PigeonObjectPool* pool)
{
    ASSERT_R0(pool && pool->object_size && pool->group_bitmaps.size == pool->group_data_pointers.size);

    if(pool->alloc_search_start_i >= pool->group_bitmaps.size)
        pool->alloc_search_start_i = pool->group_bitmaps.size;

    for(unsigned int i = pool->alloc_search_start_i; i < pool->group_bitmaps.size; i++) {
        GroupBitmap * bitmap = &((GroupBitmap*)pool->group_bitmaps.elements)[i];
        uintptr_t group_addr = (uintptr_t)((GroupDataPointer *)pool->group_data_pointers.elements)[i];

        for(unsigned int j = 0; j < PIGEON_GROUP_SIZE_DIV64; j++) {
            if(bitmap->x[j] == UINT64_MAX) continue;

            unsigned int k = (unsigned int) find_first_zero_bit(bitmap->x[j]);
            assert(k < 64);
            bitmap->x[j] |= 1ull << k;

            void * p = (void *) (group_addr + (j*64+k) * pool->object_size);

            if(pool->zero_on_allocate)
                memset(p, 0, pool->object_size);

            pool->allocated_obj_count++;
            return p;
        }

        pool->alloc_search_start_i++;
    }

    void * data = malloc(pool->object_size * PIGEON_GROUP_SIZE_DIV64 * 64);
    ASSERT_R0(data);

    if(!pool->zero_on_allocate) {
        // objects are not zero'd again after this.
        memset(data, 0, pool->object_size * PIGEON_GROUP_SIZE_DIV64 * 64);
    }

    GroupBitmap * new_group_bitmap = (GroupBitmap *)pigeon_array_list_add(&pool->group_bitmaps, 1);
    if(!new_group_bitmap) {
        free(data);
        ASSERT_R0(false);
    }

    GroupDataPointer * new_group_data_pointer = (GroupDataPointer *)pigeon_array_list_add(&pool->group_data_pointers, 1);
    if(!new_group_data_pointer) {
        free(data);
        pigeon_array_list_remove(&pool->group_bitmaps, pool->group_bitmaps.size-1, 1);
        ASSERT_R0(false);
    }

    new_group_bitmap->x[0] = 1;
    if(PIGEON_GROUP_SIZE_DIV64 > 1)
        memset(&new_group_bitmap->x[1], 0, (PIGEON_GROUP_SIZE_DIV64-1)*8);
    *new_group_data_pointer = data;

    if(pool->zero_on_allocate)
        memset(data, 0, pool->object_size);

    pool->allocated_obj_count++;
    return data;
}

void pigeon_object_pool_free(PigeonObjectPool* pool, void * object)
{
    assert(pool && pool->object_size && object && pool->group_bitmaps.size == pool->group_data_pointers.size);

    uintptr_t object_addr = (uintptr_t)object;
    unsigned int objects_per_group = PIGEON_GROUP_SIZE_DIV64*64;

    for(unsigned int i = 0; i < pool->group_bitmaps.size; i++) {
        uintptr_t group_addr = (uintptr_t)((GroupDataPointer *)pool->group_data_pointers.elements)[i];

        if(object_addr < group_addr || object_addr >= group_addr + pool->object_size*objects_per_group)
            continue;

        // Object is within this group

        GroupBitmap * bitmap = &((GroupBitmap*)pool->group_bitmaps.elements)[i];

        unsigned int j = (unsigned int) (object_addr - group_addr) / pool->object_size;

        if(bitmap->x[j/64] & (1ull << (j % 64))) {
            bitmap->x[j/64] &= ~(1ull << (j % 64));

            if(pool->alloc_search_start_i > i)
                pool->alloc_search_start_i = i;

            pool->allocated_obj_count--;
        }
        else assert(false); // double-free

        break;
    }
}

void pigeon_object_pool_for_each(PigeonObjectPool * pool, void (*f)(void* e))
{
    for(unsigned int i = 0; i < pool->group_bitmaps.size; i++) {
        uintptr_t group_addr = (uintptr_t)((GroupDataPointer *)pool->group_data_pointers.elements)[i];

        for(unsigned int j = 0; j < PIGEON_GROUP_SIZE_DIV64; j++) {
            uint64_t b = ((GroupBitmap*)pool->group_bitmaps.elements)[i].x[j];
            if(!b) continue;

            for(unsigned int k = 0; k < 64; k++) {
                if(b & (1ull << k)) {
                    void * element = (void *)(group_addr + pool->object_size*(j*64+k));
                    f(element);
                }
            }
        }
    }

}

void pigeon_object_pool_for_each2(PigeonObjectPool * pool, void (*f)(void* e, void* x), void * x)
{
    for(unsigned int i = 0; i < pool->group_bitmaps.size; i++) {
        uintptr_t group_addr = (uintptr_t)((GroupDataPointer *)pool->group_data_pointers.elements)[i];

        for(unsigned int j = 0; j < PIGEON_GROUP_SIZE_DIV64; j++) {
            uint64_t b = ((GroupBitmap*)pool->group_bitmaps.elements)[i].x[j];
            if(!b) continue;

            for(unsigned int k = 0; k < 64; k++) {
                if(b & (1ull << k)) {
                    void * element = (void *)(group_addr + pool->object_size*(j*64+k));
                    f(element, x);
                }
            }
        }
    }

}

void pigeon_destroy_object_pool(PigeonObjectPool* pool)
{
    for(unsigned int i = 0; i < pool->group_data_pointers.size; i++) {
        GroupDataPointer d = ((GroupDataPointer *)pool->group_data_pointers.elements)[i];
        if(d) free(d);
    }
    pigeon_destroy_array_list(&pool->group_data_pointers);
    pigeon_destroy_array_list(&pool->group_bitmaps);
    pool->object_size = 0;
}

