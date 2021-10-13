#include "pointer_list.h"
#include <pigeon/object_pool.h>
#include <pigeon/assert.h>

PigeonObjectPool pigeon_pointer_list_pool;

void pigeon_init_pointer_pool(void);
void pigeon_init_pointer_pool(void)
{
    pigeon_create_object_pool(&pigeon_pointer_list_pool, sizeof(PigeonArrayList), false);
}


void pigeon_deinit_pointer_pool(void);
void pigeon_deinit_pointer_pool(void)
{
    pigeon_destroy_object_pool(&pigeon_pointer_list_pool);
}



PIGEON_ERR_RET add_to_ptr_list(PigeonArrayList ** alp, void* x)
{
    ASSERT_R1(alp && x);

    PigeonArrayList * al = *alp;

    if(al) {
        void** c = (void**) pigeon_array_list_add(al, 1);
        if(!c) return 1;
        *c = x;
    }
    else {
        al = pigeon_object_pool_allocate(&pigeon_pointer_list_pool);
        if(!al) return 1;

        al->size = 0;

        if(!al->element_size) {
            pigeon_create_array_list(al, sizeof(void*));
        }

        void** c = (void**) pigeon_array_list_add(al, 1);
        if(!c) {
            pigeon_object_pool_free(&pigeon_pointer_list_pool, al);
            return 1;
        }
        *c = x;
        *alp = al;
    }
    return 0;
}



void remove_from_ptr_list(PigeonArrayList ** alp, void* x)
{
    assert(alp && x);

    PigeonArrayList * al = *alp;
    if(!al) {
        assert(false);
        return;
    }

    assert(al->size > 0);
    if(al->size > 1) {
        void** children = (void**) al->elements;
        for(unsigned int i = 0; i < al->size; i++) {
            if(children[i] == x) {
                pigeon_array_list_remove(al, i, 1);
                break;
            }
        }
    }
    else {
        pigeon_object_pool_free(&pigeon_pointer_list_pool, al);
        *alp = NULL;
    }

}

