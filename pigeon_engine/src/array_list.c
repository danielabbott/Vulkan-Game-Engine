#include <pigeon/array_list.h>
#include <stdint.h>
#include <pigeon/assert.h>
#include <pigeon/misc.h>
#include <string.h>
#include <stdlib.h>


void pigeon_create_array_list(PigeonArrayList* al, unsigned int element_size)
{
    assert(al && !al->element_size && element_size);

    al->element_size = element_size;
}

PIGEON_ERR_RET pigeon_create_array_list2(PigeonArrayList* al, unsigned int element_size, unsigned int capacity)
{
    ASSERT_R1(al && !al->element_size && element_size);

    al->element_size = element_size;
    al->capacity = capacity;

    if(capacity) {
        al->elements = malloc((size_t)capacity * (size_t)element_size);
        ASSERT_R1(al->elements);
    }

    return 0;
}


PIGEON_CHECK_RET void * pigeon_array_list_add(PigeonArrayList* al, unsigned int elements)
{
    ASSERT_R0(al && al->element_size && elements);

    unsigned int old_size = al->size;

    ASSERT_R0(!pigeon_array_list_resize(al, al->size+elements));

    void * p = (void *)((uintptr_t)al->elements + (size_t)old_size * (size_t)al->element_size);
    return p;
}

void pigeon_array_list_remove(PigeonArrayList* al, unsigned int start_index, unsigned int elements)
{
    if(!elements) return;

    assert(al && al->element_size && start_index+elements <= al->size);

    if(start_index + elements == al->size) {
        al->size -= elements;
        return;
    }


    uintptr_t d = (uintptr_t)al->elements;
    unsigned int n = elements;
    unsigned int maxn = al->size - (start_index + elements);
    if(n > maxn) n = maxn;    

    memcpy(
        (void *) (d + (size_t)al->element_size * (size_t) start_index), // dst
        (void *) (d + (size_t)al->element_size * (size_t) (al->size - n)), // src
        (size_t) al->element_size * (size_t) n // size
    );

    
    al->size -= elements;
}

void pigeon_array_list_remove_preserve_order(PigeonArrayList* al, unsigned int start_index, unsigned int elements)
{
    if(!elements) return;

    assert(al && al->element_size && start_index+elements <= al->size);

    if(start_index+elements == al->size) {
        al->size -= elements;
        return;
    }

    uintptr_t d = (uintptr_t)al->elements;
    memmove(
        (void *) (d + (size_t)al->element_size * (size_t)start_index), // dst
        (void *) (d + (size_t)al->element_size * (size_t)(start_index+elements)), // src
        (size_t)al->element_size * (size_t)(al->size - (start_index+elements))
    );

    al->size -= elements;
}


void pigeon_array_list_remove2(PigeonArrayList* al, void * element)
{
    pigeon_array_list_remove(
        al,
        (unsigned) (((uintptr_t)element - (uintptr_t)al->elements) / (uintptr_t)al->element_size),
        1
    );
}

void pigeon_array_list_remove_preserve_order2(PigeonArrayList* al, void * element)
{
    pigeon_array_list_remove_preserve_order(
        al,
        (unsigned) (((uintptr_t)element - (uintptr_t)al->elements) / (uintptr_t)al->element_size),
        1
    );
}

PIGEON_ERR_RET pigeon_array_list_resize(PigeonArrayList* al, unsigned int new_size)
{
    assert(al && al->element_size);

    if(new_size <= al->size) {
        al->size = new_size;
        return 0;
    }
    
    unsigned int new_capacity;
    if(new_size < 30) new_capacity = new_size * 2;
    else new_capacity = (new_size * 3) / 2;

    if(new_capacity < 2) new_capacity = 2;
    if(new_capacity < new_size) new_capacity = new_size;


    size_t new_capacity_bytes = (size_t)al->element_size * (size_t)new_capacity;
    void * x = NULL;
    if(al->elements)
        x = realloc(al->elements, new_capacity_bytes);
    else
        x = malloc(new_capacity_bytes);
    ASSERT_R0(x);

    al->elements = x;
    al->capacity = new_capacity;
    al->size = new_size;
    

    return 0;
}


void pigeon_array_list_zero(PigeonArrayList* al)
{
    if(!al->size || !al->element_size || !al->elements) return;

    memset(al->elements, 0, (size_t)al->element_size * (size_t)al->size);
}

void pigeon_destroy_array_list(PigeonArrayList* al)
{
    assert(al);
    free2(&al->elements);
    al->element_size = 0;
    al->size = 0;
    al->capacity = 0;
}


