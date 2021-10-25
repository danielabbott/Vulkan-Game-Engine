#include <pigeon/scene/transform.h>
#include <pigeon/object_pool.h>
#include <pigeon/scene/component.h>
#include "pointer_list.h"
#include <pigeon/assert.h>
#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
    #define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <cglm/mat4.h>
#include <cglm/affine.h>
#include <cglm/quat.h>
#include <string.h>
#include "scene.h"

PigeonTransform * pigeon_scene_root;

static PigeonObjectPool pool;
extern PigeonObjectPool pigeon_pointer_list_pool;

void pigeon_init_transform_pool(void)
{
    pigeon_create_object_pool(&pool, sizeof(PigeonTransform), true);
}

void pigeon_deinit_transform_pool(void)
{
    pigeon_destroy_object_pool(&pool);
}

PigeonTransform* pigeon_create_transform(PigeonTransform * parent)
{
    if(!parent && !pigeon_scene_root) {
        pigeon_scene_root = pigeon_object_pool_allocate(&pool);
        ASSERT_R0(pigeon_scene_root);
    }
    if(!parent) {
        parent = pigeon_scene_root;
    }

    PigeonTransform * t = (PigeonTransform *) pigeon_object_pool_allocate(&pool);
    ASSERT_R0(t);


    t->parent = parent;

    if(add_to_ptr_list(&parent->children, t)) {
        pigeon_object_pool_free(&pool, t);
        return NULL;
    }

    return t;
}


static void recursive_infanticide(PigeonTransform * t)
{
    if(t->children) {
        for(unsigned int i = 0; i < t->children->size; i++) {
            recursive_infanticide(((PigeonTransform**)t->children->elements)[i]);
        }
    
        pigeon_object_pool_free(&pigeon_pointer_list_pool, t->children);
    }

    if(t->components) {
        for(unsigned int i = 0; i < t->components->size; i++) {
            PigeonComponent * c = ((PigeonComponent**) t->components->elements)[i];
            remove_from_ptr_list(&c->transforms, t);
        }
        pigeon_object_pool_free(&pigeon_pointer_list_pool, t->components);
    }

    pigeon_object_pool_free(&pool, t);
}

void pigeon_destroy_transform(PigeonTransform * t)
{
    assert(t);

    if(t->parent) {
        remove_from_ptr_list(&t->parent->children, t);
    }

    recursive_infanticide(t);
}



PIGEON_ERR_RET pigeon_join_transform_and_component(PigeonTransform* t, PigeonComponent* comp)
{
    ASSERT_R1(t && comp);

    ASSERT_R1(!add_to_ptr_list(&t->components, comp));
    if(add_to_ptr_list(&comp->transforms, t)) {
        remove_from_ptr_list(&t->components, comp);
        return 1;
    }
    return 0;
}

void pigeon_unjoin_transform_and_component(PigeonTransform* t, PigeonComponent* comp)
{
    assert(t && comp);

    remove_from_ptr_list(&comp->transforms, t);
    remove_from_ptr_list(&t->components, comp);
}

void pigeon_invalidate_world_transform(PigeonTransform* t)
{
    assert(t);
    t->world_transform_cached = false;
    if(t->children) {
        for(unsigned int i = 0; i < t->children->size; i++) {
            pigeon_invalidate_world_transform(((PigeonTransform**)t->children->elements)[i]);
        }
    }
}

static void get_local_mat4(PigeonTransform* t, mat4 m)
{
    if(t->transform_type == PIGEON_TRANSFORM_TYPE_MATRIX) {
        memcpy(m, t->matrix_transform, 4*4*4);
    }
    else if(t->transform_type == PIGEON_TRANSFORM_TYPE_SRT) {
        glm_scale_make(m, t->scale);

        mat4 m2;
        glm_quat_mat4(t->rotation, m2);
        glm_mat4_mul(m2, m, m);

        glm_translate_make(m2, t->translation);
        glm_mat4_mul(m2, m, m);
    }
    else {
        glm_mat4_identity(m);
    }
}

void pigeon_scene_calculate_world_matrix(PigeonTransform* t)
{
    if(t->world_transform_cached) return;

    if(t->parent) {
        pigeon_scene_calculate_world_matrix(t->parent);
    }

    get_local_mat4(t, t->world_transform_cache);
    if(t->parent && !(!t->parent->parent && !t->parent->transform_type)) {
        glm_mat4_mul(t->parent->world_transform_cache, t->world_transform_cache, t->world_transform_cache);
    }
    t->world_transform_cached = true;
}

void pigeon_destroy_component(PigeonComponent* comp);
void pigeon_destroy_component(PigeonComponent* comp)
{
    assert(comp);
    if(comp->transforms) {
        for(unsigned int i = 0; i < comp->transforms->size; i++) {
            PigeonTransform * t = ((PigeonTransform**) comp->transforms->elements)[i];
            remove_from_ptr_list(&t->components, comp);
        }
        pigeon_object_pool_free(&pigeon_pointer_list_pool, comp->transforms);
        comp->transforms = NULL;
    }
}
