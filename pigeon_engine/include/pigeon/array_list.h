#pragma once

#include <pigeon/util.h>

typedef struct PigeonArrayList
{
    unsigned int element_size;
    unsigned int capacity;
    unsigned int size;
    void * elements;
} PigeonArrayList;

void pigeon_create_array_list(PigeonArrayList*, unsigned int element_size);
PIGEON_ERR_RET pigeon_create_array_list2(PigeonArrayList*, unsigned int element_size, unsigned int capacity);

PIGEON_CHECK_RET void * pigeon_array_list_add(PigeonArrayList*, unsigned int elements);
void pigeon_array_list_remove(PigeonArrayList*, unsigned int start_index, unsigned int elements);
void pigeon_array_list_remove_preserve_order(PigeonArrayList*, unsigned int start_index, unsigned int elements);

void pigeon_array_list_remove2(PigeonArrayList*, void * element);
void pigeon_array_list_remove_preserve_order2(PigeonArrayList*, void * element);

PIGEON_ERR_RET pigeon_array_list_resize(PigeonArrayList*, unsigned int new_size);

// memset elements array to 0 (up to size, not capacity)
void pigeon_array_list_zero(PigeonArrayList*);

void pigeon_destroy_array_list(PigeonArrayList*);
