#include <pigeon/scene/scene.h>
#include <pigeon/scene/mesh_renderer.h>
#include <pigeon/scene/light.h>
#include <pigeon/scene/transform.h>
#include <pigeon/array_list.h>
#include <pigeon/object_pool.h>
#include <pigeon/wgi/wgi.h>
#include <pigeon/asset.h>
#include <pigeon/job_system/job.h>
#include <pigeon/misc.h>
#include <pigeon/assert.h>
#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
    #define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <cglm/mat4.h>
#include <cglm/quat.h>
#include <string.h>
#include "scene.h"

extern PigeonTransform * pigeon_scene_root;

static PigeonWGISceneUniformData scene_uniform_data = {0};
extern PigeonObjectPool pigeon_pool_rs;
extern PigeonObjectPool pigeon_pool_anim;
extern PigeonArrayList pigeon_lights;

static unsigned int total_draws;
static unsigned int total_multidraw_draws;
static unsigned int total_bones;
static unsigned int total_lights;
static PigeonTransform* camera;
static PigeonWGIShadowParameters shadows[4];

static void* draw_objects;
static PigeonWGIBoneMatrix* bone_matrices;

static PigeonArrayList job_array_list;
static PigeonJob* jobs;


void pigeon_init_scene_module(void);
void pigeon_init_scene_module(void)
{
    pigeon_init_pointer_pool();
    pigeon_init_transform_pool();
    pigeon_init_mesh_renderer_pool();
    pigeon_init_light_array_list();
    pigeon_init_audio_player_pool();
    pigeon_create_array_list(&job_array_list, sizeof(PigeonJob));
}

void pigeon_deinit_scene_module(void);
void pigeon_deinit_scene_module(void)
{
    pigeon_destroy_array_list(&job_array_list);
    pigeon_deinit_pointer_pool();
    pigeon_deinit_transform_pool();
    pigeon_deinit_mesh_renderer_pool();
    pigeon_deinit_light_array_list();
    pigeon_deinit_audio_player_pool();
}

static unsigned int render_state_index;

// not parallelisable
static void scene_graph_prepass_rs(void * rs_)
{
    PigeonRenderState * rs = rs_;

    rs->_start_draw_index = total_draws;
    rs->_start_multidraw_index = total_multidraw_draws;
    rs->_index = render_state_index++;

    unsigned int draws = 0, multidraws = 0;
    rs->count = 0;

    if(!rs->models) return;

    for(unsigned int i = 0; i < rs->models->size; i++) {
        PigeonModelMaterial* model = ((PigeonModelMaterial**)rs->models->elements)[i];

        if(!model->mr) continue;

        unsigned int instances = 0;
        for(unsigned int j = 0; j < model->mr->size; j++) {
            PigeonMaterialRenderer* mr = ((PigeonMaterialRenderer**)model->mr->elements)[j];

            if(mr->c.transforms) {
                for(unsigned int k = 0; k < mr->c.transforms->size; k++, instances++) {
                    PigeonTransform* t = ((PigeonTransform**)mr->c.transforms->elements)[k];
                    pigeon_scene_calculate_world_matrix(t);
                }
            }
        }

        if(pigeon_wgi_multidraw_supported() && instances) {
            // These are only used if there is only 1 multidraw for this render state
            rs->start_vertex = model->model_asset->mesh_meta.multimesh_start_vertex;
            rs->instances = instances;
            rs->first = model->model_asset->mesh_meta.multimesh_start_index
                    + model->model_asset->materials[model->material_index].first;
            rs->count = model->model_asset->materials[model->material_index].count;
            
            multidraws++;
        }

        draws += instances;
    }

    if(multidraws == 1) multidraws = 0;

    rs->_draws = draws;
    rs->_multidraws = multidraws;

    total_draws += draws;
    total_multidraw_draws += multidraws;
}

// not parallelisable
static void scene_graph_prepass_anim(void * anim_)
{
    PigeonAnimationState * anim = anim_;

    anim->_first_bone_index = total_bones;
    total_bones += round_up(anim->model_asset->bones_count, pigeon_wgi_get_bone_data_alignment());
}

static void scene_graph_prepass(void)
{
    total_draws = total_multidraw_draws = total_bones = render_state_index = 0;
    pigeon_object_pool_for_each(&pigeon_pool_rs, scene_graph_prepass_rs);
    pigeon_object_pool_for_each(&pigeon_pool_anim, scene_graph_prepass_anim);

    // lights & shadow

    unsigned int light_index = 0;
    for(unsigned int i = 0; i < pigeon_lights.size && light_index < 4; i++) {
        PigeonLight * l = ((PigeonLight**)pigeon_lights.elements)[i];
        if(!l || !l->c.transforms) continue;

        for(unsigned int j = 0; j < l->c.transforms->size && light_index < 4; j++, light_index++) {
            PigeonTransform * t = ((PigeonTransform**)l->c.transforms->elements)[j];
            pigeon_scene_calculate_world_matrix(t);

            if(l->shadow_resolution) {
                if(l->type == PIGEON_LIGHT_TYPE_DIRECTIONAL) {                
                    memcpy(shadows[light_index].inv_view_matrix, t->world_transform_cache, 64);
                    shadows[light_index].resolution = l->shadow_resolution;
                    shadows[light_index].near_plane = l->shadow_near;
                    shadows[light_index].far_plane = l->shadow_far;
                    shadows[light_index].sizeX = l->shadow_size_x;
                    shadows[light_index].sizeY = l->shadow_size_y;
                }
                else {
                    l->shadow_resolution = 0;
                }
            }
            else {
                shadows[light_index].resolution = 0;            
            }
        }        
    }
    total_lights = light_index;

    
}

static void set_object_uniform(PigeonModelMaterial const* model, PigeonMaterialRenderer const* mr,
    PigeonTransform const* t, uint32_t draw_index)
{
    assert(draw_index < total_draws);

    PigeonWGIDrawObject * data = (PigeonWGIDrawObject*) ((uintptr_t)draw_objects + draw_index * 
        round_up(sizeof(PigeonWGIDrawObject), pigeon_wgi_get_draw_data_alignment()));

    assert(t->world_transform_cached);

    // ** variables must be written in order

    /* Vertex meta & matrices */

    #define model_matrix t->world_transform_cache

	glm_mat4_mul(scene_uniform_data.proj_view, (vec4*)model_matrix, data->proj_view_model[0]);
    pigeon_wgi_set_object_shadow_mvp_uniform(data, (vec4*)model_matrix);
	memcpy(data->model, model_matrix, 64);
	pigeon_wgi_get_normal_model_matrix(model_matrix, data->normal_model_matrix);

    #undef model_matrix
    

	memcpy(data->position_min, model->model_asset->mesh_meta.bounds_min, 3 * 4);
	memcpy(data->position_range, model->model_asset->mesh_meta.bounds_range, 3 * 4);

	data->ssao_intensity = 1.35f;




	/* Material data */

    if (mr->diffuse_bind_point != UINT32_MAX)
    {
        data->texture_sampler_index_plus1 = mr->diffuse_bind_point+1;
        data->texture_index = (float)mr->diffuse_layer;
    }
    else
    {
        data->texture_sampler_index_plus1 = 0;
        data->texture_index = 0;
    }

	data->first_bone_index = mr->animation_state ? mr->animation_state->_first_bone_index : UINT32_MAX;
    data->specular_intensity = mr->specular_intensity * 10.0f;

    memcpy(data->colour, mr->colour, 3 * 4);
    data->luminosity = mr->luminosity;

    memcpy(data->under_colour, mr->under_colour, 3 * 4);

    data->alpha_channel_usage = mr->use_transparency ?
        (mr->use_under_colour ? PIGEON_WGI_ALPHA_CHANNEL_TRANSPARENCY : 
            PIGEON_WGI_ALPHA_CHANNEL_UNDER_COLOUR)
        : PIGEON_WGI_ALPHA_CHANNEL_UNUSED;
    

    if (mr->nmap_bind_point != UINT32_MAX)
    {
        data->normal_map_sampler_index_plus1 = mr->nmap_bind_point+1;
        data->normal_map_index = (float)mr->nmap_layer;
    }
    else
    {
        data->normal_map_sampler_index_plus1 = 0;
        data->normal_map_index = 0;
    }

    data->rsvd1 = 0;
    data->rsvd2 = 0;

}


// static void mat3x4_to_mat4(const float * mat3x4, float * mat4)
// {
// 	mat4[0] = mat3x4[0];
// 	mat4[1] = mat3x4[1];
// 	mat4[2] = mat3x4[2];
// 	mat4[3] = 0;
// 	mat4[4] = mat3x4[3];
// 	mat4[5] = mat3x4[4];
// 	mat4[6] = mat3x4[5];
// 	mat4[7] = 0;
// 	mat4[8] = mat3x4[6];
// 	mat4[9] = mat3x4[7];
// 	mat4[10] = mat3x4[8];
// 	mat4[11] = 0;
// 	mat4[12] = mat3x4[9];
// 	mat4[13] = mat3x4[10];
// 	mat4[14] = mat3x4[11];
// 	mat4[15] = 1;
// }

static void mat4_to_mat3x4(const float * mat4, float * mat3x4)
{
	mat3x4[0] = mat4[0];
	mat3x4[1] = mat4[1];
	mat3x4[2] = mat4[2];
	mat3x4[3] = mat4[4];
	mat3x4[4] = mat4[5];
	mat3x4[5] = mat4[6];
	mat3x4[6] = mat4[8];
	mat3x4[7] = mat4[9];
	mat3x4[8] = mat4[10];
	mat3x4[9] = mat4[12];
	mat3x4[10] = mat4[13];
	mat3x4[11] = mat4[14];
}

static PIGEON_ERR_RET set_object_bones(uint64_t arg0, void* arg1)
{
    (void)arg0;
    PigeonAnimationState const* a = arg1;

    unsigned int current_bone_index = a->_first_bone_index;

    
    if(a->animation_index < 0) {
        for(unsigned int i = 0; i < a->model_asset->bones_count; i++) {
            assert(current_bone_index < total_bones);
            PigeonWGIBoneMatrix * bone_matrix = &bone_matrices[current_bone_index++];
            bone_matrix->mat3x4[0] = 1;
            bone_matrix->mat3x4[1] = 0;
            bone_matrix->mat3x4[2] = 0;
            bone_matrix->mat3x4[3] = 0;
            bone_matrix->mat3x4[4] = 1;
            bone_matrix->mat3x4[5] = 0;
            bone_matrix->mat3x4[6] = 0;
            bone_matrix->mat3x4[7] = 0;
            bone_matrix->mat3x4[8] = 1;
            bone_matrix->mat3x4[9] = 0;
            bone_matrix->mat3x4[10] = 0;
            bone_matrix->mat3x4[11] = 0;
        }
        return 0;
    }

    if((unsigned)a->animation_index >= a->model_asset->animations_count) {
        assert(false);
        return 1;
    }

    PigeonWGIAnimationMeta* animation = &a->model_asset->animations[a->animation_index];

    double frame = (pigeon_wgi_get_time_seconds_double() - a->animation_start_time) 
        * (double)animation->fps;

    unsigned int bones_per_frame = a->model_asset->bones_count;

    unsigned int frame0 = ((unsigned)frame) % animation->frame_count;
    unsigned int frame1 = ((unsigned)ceil(frame)) % animation->frame_count;
    float frame_interp = (float)fmod(frame - floor(frame), 1.0f);

    unsigned int subr_i = 0;
    for(;; subr_i++) {
        if(!a->model_asset->mesh_meta.attribute_types[subr_i]) break;
    }
    if(a->model_asset->mesh_meta.index_count > 0) subr_i++;
    subr_i += (unsigned) a->animation_index;

    PigeonWGIBoneData const* bone_data = a->model_asset->subresources[subr_i].decompressed_data;

    for(unsigned int i = 0; i < a->model_asset->bones_count; i++) {
        assert(current_bone_index < total_bones);
        PigeonWGIBoneMatrix * bone_matrix = &bone_matrices[current_bone_index++];

        unsigned int animation_bone_index0 = frame0*bones_per_frame+ i;
        unsigned int animation_bone_index1 = frame1*bones_per_frame+ i;

        mat4 m = {0};

        float s = mixf(bone_data[animation_bone_index0].scale, bone_data[animation_bone_index1].scale, frame_interp);
        m[0][0] = s;
        m[1][1] = s;
        m[2][2] = s;
        m[3][3] = 1;



        float q0[4] = {bone_data[animation_bone_index0].rotate[1], bone_data[animation_bone_index0].rotate[2],
            bone_data[animation_bone_index0].rotate[3], bone_data[animation_bone_index0].rotate[0]};
        float q1[4] = {bone_data[animation_bone_index1].rotate[1], bone_data[animation_bone_index1].rotate[2],
            bone_data[animation_bone_index1].rotate[3], bone_data[animation_bone_index1].rotate[0]};

        float q[4];
        glm_quat_slerp(q0, q1, frame_interp, q);

        mat4 this_transform_rotation;
        glm_quat_mat4(q, this_transform_rotation);
        glm_mat4_mul(this_transform_rotation, m, m);


        mat4 this_transform_translate;
        glm_mat4_identity(this_transform_translate);
        this_transform_translate[3][0] = mixf(bone_data[animation_bone_index0].translate[0], 
            bone_data[animation_bone_index1].translate[0], frame_interp);
        this_transform_translate[3][1] = mixf(bone_data[animation_bone_index0].translate[1], 
            bone_data[animation_bone_index1].translate[1], frame_interp);
        this_transform_translate[3][2] = mixf(bone_data[animation_bone_index0].translate[2], 
            bone_data[animation_bone_index1].translate[2], frame_interp);
        glm_mat4_mul(this_transform_translate, m, m);
        
        mat4_to_mat3x4(&m[0][0], bone_matrix->mat3x4);
    }
    return 0;
}

static PIGEON_ERR_RET set_uniform_data_per_rs_(uint64_t arg0, void * rs_)
{
    (void)arg0;
    PigeonRenderState const* rs = rs_;

    bool multidraw_supported = pigeon_wgi_multidraw_supported();

    if(!rs->_draws) return 0;
    assert(rs->models && rs->models->size);



    unsigned int draw_index = rs->_start_draw_index;
    unsigned int multidraw_index = rs->_start_multidraw_index;

    for(unsigned int i = 0; i < rs->models->size; i++) {
        PigeonModelMaterial* model = ((PigeonModelMaterial**)rs->models->elements)[i];

        if(!model->mr) continue;

        unsigned int instances = 0;
        for(unsigned int j = 0; j < model->mr->size; j++) {
            PigeonMaterialRenderer* mr = ((PigeonMaterialRenderer**)model->mr->elements)[j];
            
            if(mr->c.transforms) {
                for(unsigned int k = 0; k < mr->c.transforms->size; k++, draw_index++,instances++) {
                    PigeonTransform* t = ((PigeonTransform**)mr->c.transforms->elements)[k];
                    set_object_uniform(model, mr, t, draw_index);
                }
            }   
        }


        if(multidraw_supported && instances) {
            if(rs->_multidraws) {
                pigeon_wgi_multidraw_draw(
                    multidraw_index++,
                    model->model_asset->mesh_meta.multimesh_start_vertex,
                    instances,
                    model->model_asset->mesh_meta.multimesh_start_index
                        + model->model_asset->materials[model->material_index].first, 
                    model->model_asset->materials[model->material_index].count,
                    draw_index - instances
                );
            }
        }
    }

    return 0;
}

static void set_per_scene_uniform_data(bool debug_disable_ssao)
{

	unsigned int window_width, window_height;
	pigeon_wgi_get_window_dimensions(&window_width, &window_height);

    glm_mat4_inv(camera->world_transform_cache, scene_uniform_data.view);

    vec4 eye_position = {0, 0, 0, 1};
    glm_mat4_mulv(camera->world_transform_cache, eye_position, scene_uniform_data.eye_position);


	pigeon_wgi_perspective(scene_uniform_data.proj, 45, (float)window_width / (float)window_height);

	glm_mat4_mul(scene_uniform_data.proj, scene_uniform_data.view, scene_uniform_data.proj_view);
	scene_uniform_data.viewport_size[0] = (float)window_width;
	scene_uniform_data.viewport_size[1] = (float)window_height;
	scene_uniform_data.one_pixel_x = 1.0f / (float)window_width;
	scene_uniform_data.one_pixel_y = 1.0f / (float)window_height;
	scene_uniform_data.time = pigeon_wgi_get_time_seconds();
	scene_uniform_data.ssao_cutoff = debug_disable_ssao ? -1 : 0.02f;

    // lights

    scene_uniform_data.number_of_lights = total_lights;

    unsigned int light_index = 0;
    for(unsigned int i = 0; i < pigeon_lights.size && light_index < 4; i++) {
        PigeonLight * l = ((PigeonLight**)pigeon_lights.elements)[i];
        if(!l || !l->c.transforms) continue;

        for(unsigned int j = 0; j < l->c.transforms->size && light_index < 4; j++, light_index++) {
            PigeonTransform * t = ((PigeonTransform**)l->c.transforms->elements)[j];
            PigeonWGILight* ldata = &scene_uniform_data.lights[light_index];
                        
            if(!l->shadow_resolution) {
                ldata->light_type = l->type == PIGEON_LIGHT_TYPE_POINT ? 1 : 0;

                vec4 position = {0, 0, 0, 1};
                glm_mat4_mulv(t->world_transform_cache, position, position);
                memcpy(ldata->world_position, position, 3*4); 

                vec4 dir = {0, 0, 1, 0};
                glm_mat4_mulv(t->world_transform_cache, dir, dir);
                memcpy(ldata->neg_direction, dir, 3*4);   

                ldata->is_shadow_caster = false;         
            }
            memcpy(ldata->intensity, l->intensity, 3*4);    

        }
    }
    assert(light_index == total_lights);

}

typedef enum {
    NON_MULTI_DRAW_ALL,
    NON_MULTI_DRAW_OPAQUE,
    NON_MULTI_DRAW_TRANSPARENT
} NonMultiDrawType;

typedef struct NonMultiDrawParameters {
    NonMultiDrawType type;
    PigeonWGIRenderStage render_stage;
} NonMultiDrawParameters;

static void non_multi_draw_per_rs(void * rs_, void * arg0)
{
    PigeonRenderState const* rs = rs_;
    NonMultiDrawParameters parameters = *(NonMultiDrawParameters*)arg0;

    if(!rs->_draws) return;

    if(parameters.type == NON_MULTI_DRAW_OPAQUE && rs->pipeline->transparent) return;
    if(parameters.type == NON_MULTI_DRAW_TRANSPARENT && !rs->pipeline->transparent) return;

    unsigned int draw_index = rs->_start_draw_index;

    for(unsigned int i = 0; i < rs->models->size; i++) {
        PigeonModelMaterial* model = ((PigeonModelMaterial**)rs->models->elements)[i];

        if(!model->mr) continue;

        for(unsigned int j = 0; j < model->mr->size; j++) {
            PigeonMaterialRenderer* mr = ((PigeonMaterialRenderer**)model->mr->elements)[j];

            unsigned int bone_index = 0, bone_count = 0;

            if(mr->animation_state) {
                bone_index = mr->animation_state->_first_bone_index;
                bone_count = model->model_asset->bones_count;
            }
            
            if(mr->c.transforms) {
                for(unsigned int k = 0; k < mr->c.transforms->size; k++, draw_index++) {                    
                    pigeon_wgi_draw(parameters.render_stage, rs->pipeline, rs->mesh,
                        model->model_asset->mesh_meta.multimesh_start_vertex,
                        draw_index, 1,
                        model->model_asset->mesh_meta.multimesh_start_index
                            + model->model_asset->materials[model->material_index].first,
                        model->model_asset->materials[model->material_index].count,
                        (int) mr->diffuse_bind_point, (int) mr->nmap_bind_point,
                        bone_index, bone_count);
                }
            }   
        }
    }
}

static void multi_draw_per_rs(void * rs_, void * arg0)
{
    PigeonRenderState const* rs = rs_;
    NonMultiDrawParameters parameters = *(NonMultiDrawParameters*)arg0;

    if(!rs->_draws) return;

    if(parameters.type == NON_MULTI_DRAW_OPAQUE && rs->pipeline->transparent) return;
    if(parameters.type == NON_MULTI_DRAW_TRANSPARENT && !rs->pipeline->transparent) return;

    if(rs->_multidraws == 0) {
        if(rs->count) {
            pigeon_wgi_draw(parameters.render_stage, rs->pipeline, rs->mesh, 
                rs->start_vertex, rs->_start_draw_index, rs->instances, rs->first, rs->count, -1, -1, 0, 0);
        }
    }
    else {
        pigeon_wgi_multidraw_submit(
            parameters.render_stage,
            rs->pipeline,
            rs->mesh,
            rs->_start_multidraw_index,
            rs->_multidraws,
            rs->_start_draw_index,
            rs->_draws
        );
    }
}

static PIGEON_ERR_RET render_frame(uint64_t arg0, void* arg1)
{
    PigeonWGIRenderStage render_stage = (PigeonWGIRenderStage) arg0;
    PigeonWGIPipeline * skybox_pipeline = arg1;

	ASSERT_R1(!pigeon_wgi_start_record(render_stage));

    if(pigeon_wgi_multidraw_supported()) {
        NonMultiDrawParameters parameters = {0};
        parameters.render_stage = render_stage;

        if(!skybox_pipeline) {
            parameters.type = NON_MULTI_DRAW_ALL;
            pigeon_object_pool_for_each2(&pigeon_pool_rs, multi_draw_per_rs, &parameters);
        }
        else {
            parameters.type = NON_MULTI_DRAW_OPAQUE;
            pigeon_object_pool_for_each2(&pigeon_pool_rs, multi_draw_per_rs, &parameters);

            pigeon_wgi_draw_without_mesh(render_stage, skybox_pipeline, 3);

            parameters.type = NON_MULTI_DRAW_TRANSPARENT;
            pigeon_object_pool_for_each2(&pigeon_pool_rs, multi_draw_per_rs, &parameters);
        }
    }
    else {
        NonMultiDrawParameters parameters = {0};
        parameters.render_stage = render_stage;

        if(!skybox_pipeline) {
            parameters.type = NON_MULTI_DRAW_ALL;
            pigeon_object_pool_for_each2(&pigeon_pool_rs, non_multi_draw_per_rs, &parameters);
        }
        else {
            parameters.type = NON_MULTI_DRAW_OPAQUE;
            pigeon_object_pool_for_each2(&pigeon_pool_rs, non_multi_draw_per_rs, &parameters);

            pigeon_wgi_draw_without_mesh(render_stage, skybox_pipeline, 3);

            parameters.type = NON_MULTI_DRAW_TRANSPARENT;
            pigeon_object_pool_for_each2(&pigeon_pool_rs, non_multi_draw_per_rs, &parameters);
        }
    }

	ASSERT_R1(!pigeon_wgi_end_record(render_stage));
	return 0;
}


static unsigned int create_draw_data_job__index;
static void create_draw_data_job_(void* e)
{
    PigeonRenderState* rs = e;

    unsigned int i = create_draw_data_job__index++;
    assert(i < job_array_list.size);
    jobs[i].function = set_uniform_data_per_rs_;
    jobs[i].arg1 = rs;
}

static unsigned int set_bone_matrices__index;
static void set_bone_matrices_(void* e)
{
    PigeonAnimationState* a = e;

    unsigned int i = set_bone_matrices__index++;
    assert(i < job_array_list.size);
    jobs[i].function = set_object_bones;
    jobs[i].arg1 = a;
}

static PIGEON_ERR_RET pigeon_uniform_data_jobs()
{
    // object data

    create_draw_data_job__index = 0;
    pigeon_object_pool_for_each(&pigeon_pool_rs, create_draw_data_job_);

    set_bone_matrices__index = 0 + pigeon_pool_rs.allocated_obj_count;
    pigeon_object_pool_for_each(&pigeon_pool_anim, set_bone_matrices_);
    return 0;
}

static PIGEON_ERR_RET job_call_function(uint64_t arg0, void * arg1)
{
    (void)arg0;
    return ((int (*)(void)) arg1)();
}

static PIGEON_ERR_RET post_and_gui(uint64_t arg0, void * arg1)
{
    (void)arg0;
    (void)arg1;
    ASSERT_R1(!pigeon_wgi_start_record(PIGEON_WGI_RENDER_STAGE_POST_AND_UI));
    ASSERT_R1(!pigeon_wgi_end_record(PIGEON_WGI_RENDER_STAGE_POST_AND_UI));
    return 0;
}

PIGEON_ERR_RET pigeon_prepare_draw_frame(PigeonTransform * camera_)
{
    camera = camera_;
    memset(shadows, 0, sizeof shadows);

    // Get minimum size of uniform data
    scene_graph_prepass();

    pigeon_scene_calculate_world_matrix(camera);


    // Create uniform buffers, get pointers

    ASSERT_R1(!pigeon_wgi_start_frame(total_draws, total_multidraw_draws, shadows, total_bones,
        &draw_objects, &bone_matrices));
    return 0;
}

PIGEON_ERR_RET pigeon_draw_frame(bool debug_disable_ssao, PigeonWGIPipeline* skybox_pipeline)
{
    set_per_scene_uniform_data(debug_disable_ssao);

    // Render

    if(pigeon_wgi_multithreading_supported()) {
        unsigned int shadow_lights_count = 0;
        for(unsigned int i = 0; i < 4; i++) {
            if(shadows[i].resolution) shadow_lights_count++;
        }

        bool ssao_record = pigeon_wgi_ssao_record_needed();
        bool post_bloom = pigeon_wgi_bloom_record_needed();

        job_array_list.size = 0;
        ASSERT_R1(!pigeon_array_list_resize(&job_array_list,
            pigeon_pool_anim.allocated_obj_count + 
            pigeon_pool_rs.allocated_obj_count +
            1 + // depth
            (ssao_record ? 1 : 0) +
            shadow_lights_count +
            1 + // hdr render
            (post_bloom ? 1 : 0) +
            1 // post-processing & gui
        ));
        pigeon_array_list_zero(&job_array_list);

        jobs = (PigeonJob *) job_array_list.elements;

        // Fill uniform buffers
        ASSERT_R1(!pigeon_uniform_data_jobs());

        unsigned int i = pigeon_pool_anim.allocated_obj_count + pigeon_pool_rs.allocated_obj_count;
        
        
        for(unsigned int j = 0; j < 4; j++) {
            if(!shadows[j].resolution) continue;

            jobs[i].function = render_frame;
            jobs[i++].arg0 = PIGEON_WGI_RENDER_STAGE_SHADOW0 + j;
        }
        
        jobs[i].function = render_frame;
        jobs[i++].arg0 = PIGEON_WGI_RENDER_STAGE_DEPTH;

        if(ssao_record) {
            jobs[i].function = job_call_function;
            jobs[i++].arg1 = pigeon_wgi_record_ssao;
        }

        jobs[i].function = render_frame;
        jobs[i].arg0 = PIGEON_WGI_RENDER_STAGE_RENDER;
        jobs[i++].arg1 = skybox_pipeline;

        if(post_bloom) {
            jobs[i].function = job_call_function;
            jobs[i++].arg1 = pigeon_wgi_record_bloom;
        }

        jobs[i].function = post_and_gui;

        ASSERT_R1(!pigeon_dispatch_jobs(jobs, job_array_list.size));

        ASSERT_R1(!pigeon_wgi_set_uniform_data(&scene_uniform_data));
    }
    else {
        job_array_list.size = 0;
        ASSERT_R1(!pigeon_array_list_resize(&job_array_list,
            pigeon_pool_anim.allocated_obj_count + 
            pigeon_pool_rs.allocated_obj_count
        ));
        pigeon_array_list_zero(&job_array_list);

        jobs = (PigeonJob *) job_array_list.elements;

        // Fill uniform buffers
        ASSERT_R1(!pigeon_uniform_data_jobs());

        ASSERT_R1(!pigeon_dispatch_jobs(jobs, job_array_list.size));
        

        // Copy data
        ASSERT_R1(!pigeon_wgi_set_uniform_data(&scene_uniform_data));


        // Render

        for(unsigned int i = 0; i < 4; i++) {
            if(shadows[i].resolution)
                ASSERT_R1(!render_frame(PIGEON_WGI_RENDER_STAGE_SHADOW0+i, NULL));
        }

        ASSERT_R1(!render_frame(PIGEON_WGI_RENDER_STAGE_DEPTH, NULL));
        ASSERT_R1(!pigeon_wgi_record_ssao());
        ASSERT_R1(!render_frame(PIGEON_WGI_RENDER_STAGE_RENDER, skybox_pipeline));
        ASSERT_R1(!pigeon_wgi_record_bloom());
	    ASSERT_R1(!pigeon_wgi_start_record(PIGEON_WGI_RENDER_STAGE_POST_AND_UI));
	    ASSERT_R1(!pigeon_wgi_end_record(PIGEON_WGI_RENDER_STAGE_POST_AND_UI));
    }

    return 0;
}
