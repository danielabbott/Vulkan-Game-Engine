#include <pigeon/scene/light.h>
#include <pigeon/scene/scene.h>
#include <pigeon/array_list.h>
#include <pigeon/assert.h>
#include <stdlib.h>
#include "scene.h"

PigeonArrayList pigeon_lights; // array of PigeonLight*

void pigeon_init_light_array_list(void)
{
    pigeon_create_array_list(&pigeon_lights, sizeof(PigeonLight*));
}


void pigeon_deinit_light_array_list(void)
{
    pigeon_destroy_array_list(&pigeon_lights);
}

PigeonLight* pigeon_create_light(void)
{
    PigeonLight* l = calloc(1, sizeof *l);
    ASSERT_R0(l);

    PigeonLight** lptr = pigeon_array_list_add(&pigeon_lights, 1);
    if(!lptr) {
        free(l);
        return NULL;
    }

    *lptr = l;
    l->c.type = PIGEON_COMPONENT_TYPE_LIGHT;

    return l;
}

void pigeon_destroy_light(PigeonLight * l)
{
    assert(l);

    for(unsigned int i = 0; i < pigeon_lights.size; i++) {
        PigeonLight * l2 = ((PigeonLight**)pigeon_lights.elements)[i];
        if(l == l2) {
            pigeon_array_list_remove(&pigeon_lights, i, 1);
            break;
        }
    }

    free(l);
}
