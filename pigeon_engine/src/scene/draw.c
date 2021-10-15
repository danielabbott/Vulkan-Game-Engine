#include <pigeon/scene/scene.h>
#include <pigeon/scene/mesh_renderer.h>
#include <pigeon/scene/light.h>
#include <pigeon/scene/transform.h>
#include <pigeon/array_list.h>
#include <pigeon/object_pool.h>
#include <pigeon/wgi/wgi.h>
#include <pigeon/asset.h>
#include <pigeon/misc.h>
#include <pigeon/assert.h>
#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
    #define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <cglm/mat4.h>
#include <cglm/quat.h>
#include <string.h>

typedef struct DrawCall {
    PigeonWGIPipeline * pipeline;
    PigeonWGIMultiMesh * mesh;
    unsigned int draw_object_index;
    unsigned int draw_object_count;
    unsigned int multidraw_count; // or 0 if using regular indexed rendering
    unsigned int multidraw_index; 

    // These fields are only used for non-multidraw

    uint32_t start_vertex;
    uint32_t instances;
    uint32_t first;
    uint32_t count;
} DrawCall;

extern PigeonTransform * pigeon_scene_root;

// TODO don't have these, write straight to staging buffer
static PigeonArrayList draw_objects;
static PigeonArrayList bone_matrix_array;

static PigeonArrayList draw_calls; // DrawCall[]
static PigeonWGISceneUniformData scene_uniform_data = {0};
extern PigeonObjectPool pigeon_pool_rs;
extern PigeonObjectPool pigeon_pool_anim;
extern PigeonArrayList pigeon_lights;

static unsigned int total_draws;
static unsigned int total_multidraw_draws;
static unsigned int total_bones;
static unsigned int total_lights;
static PigeonTransform* camera;

void pigeon_init_scene_module(void);
void pigeon_init_scene_module(void)
{
    pigeon_create_array_list(&draw_objects, sizeof(PigeonWGIDrawObject));
    pigeon_create_array_list(&bone_matrix_array, sizeof(PigeonWGIBoneMatrix));
    pigeon_create_array_list(&draw_calls, sizeof(DrawCall));
}

void pigeon_deinit_scene_module(void);
void pigeon_deinit_scene_module(void)
{
    pigeon_destroy_array_list(&draw_objects);
    pigeon_destroy_array_list(&bone_matrix_array);
    pigeon_destroy_array_list(&draw_calls);
}

static void scene_graph_prepass_rs(void * rs_, void * arg0)
{
    (void)arg0;
    PigeonRenderState * rs = rs_;

    unsigned int draws = 0, multidraws = 0;

    if(!rs->models) return;

    for(unsigned int i = 0; i < rs->models->size; i++) {
        PigeonModelMaterial* model = ((PigeonModelMaterial**)rs->models->elements)[i];

        if(!model->mr) continue;

        unsigned int instances = 0;
        for(unsigned int j = 0; j < model->mr->size; j++) {
            PigeonMaterialRenderer* mr = ((PigeonMaterialRenderer**)model->mr->elements)[j];

            if(mr->c.transforms) {
                instances += mr->c.transforms->size;
            }
        }

        if(instances) {
            multidraws++;
            draws += instances;
        }
    }

    if(multidraws == 1) multidraws = 0;

    rs->_draws = draws;
    rs->_multidraws = multidraws;

    total_draws += draws;
    total_multidraw_draws += multidraws;
}


static void scene_graph_prepass_anim(void * anim_, void * arg0)
{
    (void)arg0;
    PigeonAnimationState * anim = anim_;

    anim->_first_bone_index = total_bones;
    total_bones += anim->model_asset->bones_count;
}

static void scene_graph_prepass(PigeonWGIShadowParameters shadows[4])
{
    total_draws = total_multidraw_draws = total_bones = 0;
    // TODO job system: each of these can be a job.
    pigeon_object_pool_for_each(&pigeon_pool_rs, scene_graph_prepass_rs, NULL);
    pigeon_object_pool_for_each(&pigeon_pool_anim, scene_graph_prepass_anim, NULL);

    // lights & shadow

    unsigned int light_index = 0;
    for(unsigned int i = 0; i < pigeon_lights.size && light_index < 4; i++) {
        PigeonLight * l = ((PigeonLight**)pigeon_lights.elements)[i];
        if(!l || !l->c.transforms) continue;

        for(unsigned int j = 0; j < l->c.transforms->size && light_index < 4; j++, light_index++) {
            PigeonTransform * t = ((PigeonTransform**)l->c.transforms->elements)[j];

            pigeon_scene_calculate_world_matrix(t);
            if(l->shadow_resolution) {
                memcpy(shadows[light_index].inv_view_matrix, t->world_transform_cache, 64);
                shadows[light_index].resolution = l->shadow_resolution;
                shadows[light_index].near_plane = l->shadow_near;
                shadows[light_index].far_plane = l->shadow_far;
                shadows[light_index].sizeX = l->shadow_size_x;
                shadows[light_index].sizeY = l->shadow_size_y;
            }
            else {
                shadows[light_index].resolution = 0;            
            }
        }        
    }
    total_lights = light_index;

    
}

static unsigned int current_draw_index;
static unsigned int current_multidraw_index;
static unsigned int current_bone_index;

static void set_object_uniform(PigeonModelMaterial* model, PigeonMaterialRenderer* mr,
    PigeonTransform* t)
{
    assert(current_draw_index < draw_objects.size);
    PigeonWGIDrawObject * data = &((PigeonWGIDrawObject*)draw_objects.elements)[current_draw_index++];


    
	/* Mesh metadata */

	memcpy(data->position_min, model->model_asset->mesh_meta.bounds_min, 3 * 4);
	memcpy(data->position_range, model->model_asset->mesh_meta.bounds_range, 3 * 4);

	/* Object data */

	data->ssao_intensity = 1.35f;

    pigeon_scene_calculate_world_matrix(t);

	memcpy(data->model, t->world_transform_cache, 64);

	pigeon_wgi_get_normal_model_matrix(data->model, data->normal_model_matrix);

	glm_mat4_mul(scene_uniform_data.view, data->model, data->view_model);
	glm_mat4_mul(scene_uniform_data.proj_view, data->model, data->proj_view_model[0]);

	/* Material data */

    memcpy(data->colour, mr->colour, 3 * 4);
    memcpy(data->under_colour, mr->under_colour, 3 * 4);
    data->alpha_channel_usage = mr->use_transparency ?
        (mr->use_under_colour ? PIGEON_WGI_ALPHA_CHANNEL_TRANSPARENCY : 
            PIGEON_WGI_ALPHA_CHANNEL_UNDER_COLOUR)
        : PIGEON_WGI_ALPHA_CHANNEL_UNUSED;
    

    if (mr->diffuse_bind_point != UINT32_MAX)
    {
        data->texture_sampler_index_plus1 = mr->diffuse_bind_point+1;
        data->texture_index = (float)mr->diffuse_layer;
        data->texture_uv_base_and_range[0] = 0;
        data->texture_uv_base_and_range[1] = 0;
        data->texture_uv_base_and_range[2] = 1;
        data->texture_uv_base_and_range[3] = 1;
    }
    else
    {
        data->texture_sampler_index_plus1 = 0;
    }
    if (mr->nmap_bind_point != UINT32_MAX)
    {
        data->normal_map_sampler_index_plus1 = mr->nmap_bind_point+1;
        data->normal_map_index = (float)mr->nmap_layer;
        data->normal_map_uv_base_and_range[0] = 0;
        data->normal_map_uv_base_and_range[1] = 0;
        data->normal_map_uv_base_and_range[2] = 1;
        data->normal_map_uv_base_and_range[3] = 1;
    }
    else
    {
        data->normal_map_sampler_index_plus1 = 0;
    }

	data->first_bone_index = mr->animation_state ? mr->animation_state->_first_bone_index : UINT32_MAX;
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

static void set_object_bones(void* a_, void * arg0)
{
    (void)arg0;
    PigeonAnimationState* a = a_;

    assert(a->_first_bone_index == current_bone_index);
    
    if(a->animation_index < 0) {
        for(unsigned int i = 0; i < a->model_asset->bones_count; i++) {
            assert(current_bone_index < bone_matrix_array.size);
            PigeonWGIBoneMatrix * bone_matrix = &((PigeonWGIBoneMatrix*)bone_matrix_array.elements)[current_bone_index++];
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
        return;
    }

    if((unsigned)a->animation_index >= a->model_asset->animations_count) {
        assert(false);
        return;
    }

    PigeonWGIAnimationMeta* animation = &a->model_asset->animations[a->animation_index];

    double frame = (pigeon_wgi_get_time_seconds_double() - a->animation_start_time) 
        * (double)animation->fps;

    unsigned int bones_per_frame = a->model_asset->bones_count;

    unsigned int subr_i = 0;
    for(;; subr_i++) {
        if(!a->model_asset->mesh_meta.attribute_types[subr_i]) break;
    }
    if(a->model_asset->mesh_meta.index_count > 0) subr_i++;
    subr_i += (unsigned) a->animation_index;

    PigeonWGIBoneData * bone_data = a->model_asset->subresources[subr_i].decompressed_data;

    for(unsigned int i = 0; i < a->model_asset->bones_count; i++) {
        assert(current_bone_index < bone_matrix_array.size);
        PigeonWGIBoneMatrix * bone_matrix = &((PigeonWGIBoneMatrix*)bone_matrix_array.elements)[current_bone_index++];

        unsigned int frame0 = ((unsigned)frame) % animation->frame_count;
        unsigned int frame1 = ((unsigned)ceil(frame)) % animation->frame_count;
        float frame_interp = (float)fmod(frame - floor(frame), 1.0f);

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
}

static void set_draw_data_(void * rs_, void * arg0)
{
    (void)arg0;
    PigeonRenderState * rs = rs_;

    if(!rs->_draws) return;
    assert(rs->models && rs->models->size);

    DrawCall* dc = pigeon_array_list_add(&draw_calls, 1);
    dc->pipeline = rs->pipeline;
    dc->mesh = rs->mesh;
    dc->draw_object_index = current_draw_index;
    dc->draw_object_count = rs->_draws;
    dc->multidraw_index = rs->_multidraws == 0 ? UINT32_MAX : current_multidraw_index;
    dc->multidraw_count = rs->_multidraws;

    dc->count = 0;



    unsigned int draw_index = 0;

    for(unsigned int i = 0; i < rs->models->size; i++) {
        PigeonModelMaterial* model = ((PigeonModelMaterial**)rs->models->elements)[i];

        if(!model->mr) continue;

        unsigned int instances = 0;
        for(unsigned int j = 0; j < model->mr->size; j++) {
            PigeonMaterialRenderer* mr = ((PigeonMaterialRenderer**)model->mr->elements)[j];
            
            if(mr->c.transforms) {
                for(unsigned int k = 0; k < mr->c.transforms->size; k++, instances++) {
                    PigeonTransform* t = ((PigeonTransform**)mr->c.transforms->elements)[k];
                    set_object_uniform(model, mr, t);
                }
            }   
        }

        if(instances) {
            if(!rs->_multidraws) {
                assert(!dc->count);

                dc->start_vertex = model->model_asset->mesh_meta.multimesh_start_vertex;
                dc->instances = instances;
                dc->first = model->model_asset->mesh_meta.multimesh_start_index
                        + model->model_asset->materials[model->material_index].first;
                dc->count = model->model_asset->materials[model->material_index].count;
            }
            else {
                pigeon_wgi_multidraw_draw(
                    model->model_asset->mesh_meta.multimesh_start_vertex,
                    instances,
                    model->model_asset->mesh_meta.multimesh_start_index
                        + model->model_asset->materials[model->material_index].first, 
                    model->model_asset->materials[model->material_index].count,
                    draw_index
                );
                current_multidraw_index++;
            }
            draw_index += instances;
        }

    }
}

static PIGEON_ERR_RET set_draw_data(bool debug_disable_ssao)
{
    ASSERT_R1(!pigeon_array_list_resize(&draw_objects, total_draws));
    ASSERT_R1(!pigeon_array_list_resize(&bone_matrix_array, total_bones));
    ASSERT_R1(!pigeon_array_list_resize(&draw_calls, 0));

    current_draw_index = current_multidraw_index = current_bone_index = 0;

    // scene data

	unsigned int window_width, window_height;
	pigeon_wgi_get_window_dimensions(&window_width, &window_height);

    pigeon_scene_calculate_world_matrix(camera);
    glm_mat4_inv(camera->world_transform_cache, scene_uniform_data.view);

	pigeon_wgi_perspective(scene_uniform_data.proj, 45, (float)window_width / (float)window_height);

	glm_mat4_mul(scene_uniform_data.proj, scene_uniform_data.view, scene_uniform_data.proj_view);
	scene_uniform_data.viewport_size[0] = (float)window_width;
	scene_uniform_data.viewport_size[1] = (float)window_height;
	scene_uniform_data.one_pixel_x = 1.0f / (float)window_width;
	scene_uniform_data.one_pixel_y = 1.0f / (float)window_height;
	scene_uniform_data.time = pigeon_wgi_get_time_seconds();
	scene_uniform_data.ambient[0] = 0.3f;
	scene_uniform_data.ambient[1] = 0.3f;
	scene_uniform_data.ambient[2] = 0.3f;
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
                        
            pigeon_scene_calculate_world_matrix(t);
            if(!l->shadow_resolution) {
                vec4 dir = {0, 0, 1, 0};
                glm_mat4_mulv(t->world_transform_cache, dir, dir);
                memcpy(ldata->neg_direction, dir, 3*4);                
            }

            ldata->is_shadow_caster = false;
            memcpy(ldata->intensity, l->intensity, 3*4);
        }
    }
    assert(light_index == total_lights);


    // object data
    pigeon_object_pool_for_each(&pigeon_pool_rs, set_draw_data_, NULL);

    ASSERT_R1(current_multidraw_index == total_multidraw_draws);
    ASSERT_R1(current_draw_index == total_draws);

    return 0;
}

static void draw_call(PigeonWGICommandBuffer * command_buffer, DrawCall * dc)
{
    if(dc->multidraw_count == 0) {
        pigeon_wgi_draw(command_buffer, dc->pipeline, dc->mesh, 
            dc->start_vertex, dc->draw_object_index, dc->instances, dc->first, dc->count);
    }
    else {
        pigeon_wgi_multidraw_submit(
            command_buffer,
            dc->pipeline,
            dc->mesh,
            dc->multidraw_index,
            dc->multidraw_count,
            dc->draw_object_index,
            dc->draw_object_count
        );
    }
}

static PIGEON_ERR_RET render_frame(PigeonWGICommandBuffer * command_buffer,
    PigeonWGIPipeline* skybox_pipeline)
{
	ASSERT_R1(!pigeon_wgi_start_command_buffer(command_buffer));

    if(!skybox_pipeline) {
        for(unsigned int i = 0; i < draw_calls.size; i++) {
            DrawCall* dc = &((DrawCall*)draw_calls.elements)[i];
            draw_call(command_buffer, dc);
        }
    }
    else {
        // opaque objects
        for(unsigned int i = 0; i < draw_calls.size; i++) {
            DrawCall* dc = &((DrawCall*)draw_calls.elements)[i];
            if(dc->pipeline->transparent) continue;
            draw_call(command_buffer, dc);
        }

        pigeon_wgi_draw_without_mesh(command_buffer, skybox_pipeline, 3);

        // transparent objects
        for(unsigned int i = 0; i < draw_calls.size; i++) {
            DrawCall* dc = &((DrawCall*)draw_calls.elements)[i];
            if(!dc->pipeline->transparent) continue;
            draw_call(command_buffer, dc);
        }
    }

	ASSERT_R1(!pigeon_wgi_end_command_buffer(command_buffer));
	return 0;
}

PIGEON_ERR_RET pigeon_draw_frame(PigeonTransform * camera_, bool debug_disable_ssao, PigeonWGIPipeline* skybox_pipeline)
{
    camera = camera_;
	PigeonWGIShadowParameters shadows[4] = {{0}};

    scene_graph_prepass(shadows);


    ASSERT_R1(!pigeon_wgi_start_frame(total_draws, total_multidraw_draws, shadows, total_bones));

    ASSERT_R1(!set_draw_data(debug_disable_ssao));

    current_bone_index = 0;
    pigeon_object_pool_for_each(&pigeon_pool_anim, set_object_bones, NULL);

    ASSERT_R1(!pigeon_wgi_set_uniform_data(&scene_uniform_data, draw_objects.elements, total_draws,
        bone_matrix_array.elements, total_bones));


    ASSERT_R1(!render_frame(pigeon_wgi_get_depth_command_buffer(), NULL));

    for(unsigned int i = 0; i < 4; i++) {
        if(shadows[i].resolution)
            ASSERT_R1(!render_frame(pigeon_wgi_get_shadow_command_buffer(i), NULL));
    }

    ASSERT_R1(!render_frame(pigeon_wgi_get_light_pass_command_buffer(), NULL));
    ASSERT_R1(!render_frame(pigeon_wgi_get_render_command_buffer(), skybox_pipeline));

    return 0;
}
