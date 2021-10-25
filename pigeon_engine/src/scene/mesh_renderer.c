#include <pigeon/scene/mesh_renderer.h>
#include <pigeon/object_pool.h>
#include "pointer_list.h"
#include <pigeon/assert.h>
#include <pigeon/asset.h>
#include <pigeon/misc.h>
#include <string.h>
#include "scene.h"

PigeonObjectPool pigeon_pool_rs;
static PigeonObjectPool pigeon_pool_model;
PigeonObjectPool pigeon_pool_mr;
PigeonObjectPool pigeon_pool_anim;
extern PigeonObjectPool pigeon_pointer_list_pool;

void pigeon_init_mesh_renderer_pool(void)
{
    pigeon_create_object_pool(&pigeon_pool_rs, sizeof(PigeonRenderState), true);
    pigeon_create_object_pool(&pigeon_pool_model, sizeof(PigeonModelMaterial), true);
    pigeon_create_object_pool(&pigeon_pool_mr, sizeof(PigeonMaterialRenderer), true);
    pigeon_create_object_pool(&pigeon_pool_anim, sizeof(PigeonAnimationState), true);
}


void pigeon_deinit_mesh_renderer_pool(void)
{
    pigeon_destroy_object_pool(&pigeon_pool_rs);
    pigeon_destroy_object_pool(&pigeon_pool_model);
    pigeon_destroy_object_pool(&pigeon_pool_mr);
    pigeon_destroy_object_pool(&pigeon_pool_anim);
}

PigeonRenderState* pigeon_create_render_state(struct PigeonWGIMultiMesh * mesh, struct PigeonWGIPipeline * pipeline)
{
    ASSERT_R0(mesh && pipeline);
    PigeonRenderState * rs = (PigeonRenderState *) pigeon_object_pool_allocate(&pigeon_pool_rs);
    ASSERT_R0(rs);
    rs->mesh = mesh;
    rs->pipeline = pipeline;
    return rs;
}

#define CLEAR_PTR_LIST(this, var, foreign_type, foreign_var) \
    if(this->var) { \
        for(unsigned int i = 0; i < this->var->size; i++) { \
            foreign_type * e = ((foreign_type**) this->var->elements)[i]; \
            remove_from_ptr_list(&e->foreign_var, this); \
        } \
        pigeon_object_pool_free(&pigeon_pointer_list_pool, this->var); \
    }

#define CLEAR_PTR_LIST2(this, var, foreign_type, foreign_var) \
    if(this->var) { \
        for(unsigned int i = 0; i < this->var->size; i++) { \
            foreign_type * e = ((foreign_type**) this->var->elements)[i]; \
            e->foreign_var = NULL; \
        } \
        pigeon_object_pool_free(&pigeon_pointer_list_pool, this->var); \
    }

void pigeon_destroy_render_state(PigeonRenderState * rs)
{
    assert(rs);

    CLEAR_PTR_LIST(rs, models, PigeonModelMaterial, rs);
    CLEAR_PTR_LIST2(rs, mr, PigeonMaterialRenderer, render_state);    

    pigeon_object_pool_free(&pigeon_pool_rs, rs);
}


PigeonModelMaterial* pigeon_create_model_renderer(struct PigeonAsset * model_asset, unsigned int material_index)
{
    ASSERT_R0(model_asset && model_asset->type == PIGEON_ASSET_TYPE_MODEL);
    PigeonModelMaterial * model = (PigeonModelMaterial *) pigeon_object_pool_allocate(&pigeon_pool_model);
    ASSERT_R0(model);
    model->model_asset = model_asset;
    model->material_index = material_index;
    return model;
}


void pigeon_destroy_model_renderer(PigeonModelMaterial * model)
{
    CLEAR_PTR_LIST2(model, mr, PigeonMaterialRenderer, model);
    CLEAR_PTR_LIST(model, rs, PigeonRenderState, models);

    pigeon_object_pool_free(&pigeon_pool_model, model);
}



PigeonMaterialRenderer* pigeon_create_material_renderer(PigeonModelMaterial* model)
{
    ASSERT_R0(model);

    PigeonMaterialRenderer * mr = (PigeonMaterialRenderer *) pigeon_object_pool_allocate(&pigeon_pool_mr);
    ASSERT_R0(mr);

    mr->c.type = PIGEON_COMPONENT_TYPE_MATERIAL_RENDERER;

    mr->model = model;
    if(add_to_ptr_list(&model->mr, mr)) {
        pigeon_object_pool_free(&pigeon_pool_mr, mr);
        return NULL;
    }

    mr->diffuse_bind_point = mr->nmap_bind_point = UINT32_MAX;
    mr->colour[0] = mr->colour[1] = mr->colour[2] = 0.8f;

    return mr;
}


void pigeon_destroy_component(PigeonComponent*);
void pigeon_destroy_material_renderer(PigeonMaterialRenderer * mr)
{
    assert(mr);

    if(mr->model) {
        remove_from_ptr_list(&mr->model->mr, mr);
        mr->model = NULL;
    }

    if(mr->render_state) {
        remove_from_ptr_list(&mr->render_state->mr, mr);
        mr->render_state = NULL;
    }

    if(mr->animation_state)
        remove_from_ptr_list(&mr->animation_state->mr, mr);
    
    pigeon_destroy_component(&mr->c);
    pigeon_object_pool_free(&pigeon_pool_mr, mr);
}


PigeonAnimationState* pigeon_create_animation_state(PigeonAsset* model_asset)
{
    PigeonAnimationState * a = (PigeonAnimationState *) pigeon_object_pool_allocate(&pigeon_pool_anim);
    ASSERT_R0(a);

    a->animation_index = -1;
    a->model_asset = model_asset;

    return a;
}

void pigeon_destroy_animation_state(PigeonAnimationState * a)
{
    CLEAR_PTR_LIST2(a, mr, PigeonMaterialRenderer, animation_state);
    
    pigeon_object_pool_free(&pigeon_pool_anim, a);
}

PIGEON_ERR_RET pigeon_join_rs_model(PigeonRenderState* rs, PigeonModelMaterial* model)
{
    ASSERT_R1(rs && rs->mesh && model && model->model_asset);
    ASSERT_R1(memcmp(rs->mesh->attribute_types, model->model_asset->mesh_meta.attribute_types, 
        sizeof rs->mesh->attribute_types) == 0);

    ASSERT_R1(!add_to_ptr_list(&rs->models, model));
    if(add_to_ptr_list(&model->rs, rs)) {
        remove_from_ptr_list(&rs->models, model);
        return 1;
    }
    return 0;
}

void pigeon_unjoin_rs_model(PigeonRenderState* rs, PigeonModelMaterial* model)
{
    assert(rs && model);

    remove_from_ptr_list(&rs->models, model);
    remove_from_ptr_list(&model->rs, rs);
}


PIGEON_ERR_RET pigeon_join_mr_anim(PigeonMaterialRenderer* mr, PigeonAnimationState* a)
{
    assert(a && mr && !mr->animation_state);


    ASSERT_R1(!add_to_ptr_list(&a->mr, mr));
    mr->animation_state = a;
    return 0;

}

void pigeon_unjoin_mr_anim(PigeonMaterialRenderer* mr, PigeonAnimationState* a)
{
    assert(a && mr && mr->animation_state == a);

    remove_from_ptr_list(&a->mr, mr);
    mr->animation_state = NULL;
}


