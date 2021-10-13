#include <pigeon/scene/component.h>
#include <stdbool.h>

// PIGEON_COMPONENT_TYPE_LIGHT
typedef struct PigeonLight
{  
    PigeonComponent c;

    float intensity[3];

    unsigned int shadow_resolution; // 0 for no shadows
    float shadow_near;
    float shadow_far;
    float shadow_size_x;
    float shadow_size_y;
} PigeonLight;

PigeonLight* pigeon_create_light(void);
void pigeon_destroy_light(PigeonLight **);
