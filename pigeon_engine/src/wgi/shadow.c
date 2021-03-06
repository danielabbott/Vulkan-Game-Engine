#include <pigeon/wgi/wgi.h>
#include <pigeon/wgi/vulkan/framebuffer.h>
#include "singleton.h"
#include <cglm/cam.h>
#include <string.h>
#include <pigeon/assert.h>
#include <cglm/affine.h>
#include <stdlib.h>


static int shadow_compare(const void *i0_, const void *i1_)
{
	const unsigned int *i0 = i0_;
	const unsigned int *i1 = i1_;

	PigeonWGIShadowParameters *p0 = &singleton_data.shadow_parameters[*i0];
	PigeonWGIShadowParameters *p1 = &singleton_data.shadow_parameters[*i1];

	return (int)p1->resolution - (int)p0->resolution;
}

static PIGEON_ERR_RET validate(void)
{
    for(unsigned int i = 0; i < 4; i++) {
        PigeonWGIShadowParameters *p = &singleton_data.shadow_parameters[i];
        if(!p->resolution) continue;

        ASSERT_R1(p->resolution % 2 == 0);
        ASSERT_R1(p->resolution <= 4096);
        ASSERT_R1(p->near_plane > 0);
        ASSERT_R1(p->sizeX > 0);
        ASSERT_R1(p->sizeY > 0);

        float x = p->sizeX;
        float y = p->sizeY;
        mat4 ortho;
        glm_ortho_rh_zo(-x, x, y, -y, p->far_plane, p->near_plane, ortho);

        mat4 view_matrix;
        glm_mat4_inv(p->inv_view_matrix, view_matrix);

        glm_mat4_mul(ortho, view_matrix, p->proj_view);

        p->framebuffer_index = -1;
    }
    return 0;
}


static PIGEON_ERR_RET create_shadow_framebuffer(PigeonWGIShadowParameters * p, unsigned int framebuffer_index)
{
    ASSERT_R1(!pigeon_wgi_create_framebuffer_images(&singleton_data.shadow_images[framebuffer_index],
        PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32, p->resolution, p->resolution, false, false, true));

    if(VULKAN) {
        ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.shadow_framebuffers[framebuffer_index],
            &singleton_data.shadow_images[framebuffer_index].image_view, NULL, &singleton_data.rp_depth));
    }
    else {
        ASSERT_R1(!pigeon_opengl_create_framebuffer(&singleton_data.gl.shadow_framebuffers[framebuffer_index],
            &singleton_data.shadow_images[framebuffer_index].gltex2d, NULL));
    }
    return 0;
}

PIGEON_ERR_RET pigeon_wgi_assign_shadow_framebuffers(void)
{
    ASSERT_R1(!validate());

    unsigned int shadow_param_indices_sorted[4] = {0, 1, 2, 3};
    qsort(shadow_param_indices_sorted, sizeof shadow_param_indices_sorted[0], 4, shadow_compare);

    memset(singleton_data.shadow_framebuffer_assigned, 0, sizeof singleton_data.shadow_framebuffer_assigned);

    // Assign framebuffer from already allocated framebuffers

    for(unsigned int param_index_ = 0; param_index_ < 4; param_index_++) {
	    PigeonWGIShadowParameters *p = &singleton_data.shadow_parameters[shadow_param_indices_sorted[param_index_]];
        if(!p->resolution) break;

        // Find the smallest framebuffer that is not assigned
        int chosen_framebuffer_index = -1;
        unsigned int chosen_framebuffer_width = 99999;
        unsigned int chosen_framebuffer_height = 99999;

        for(unsigned int i = 0; i < 4; i++) {
            if(VULKAN && !singleton_data.shadow_framebuffers->vk_framebuffer) continue;
            if(OPENGL && !singleton_data.gl.shadow_framebuffers->id) continue;
            if(singleton_data.shadow_framebuffer_assigned[i]) continue;

            bool match = false;

            if(VULKAN) {
                match = singleton_data.shadow_images[i].image.width >= p->resolution &&
                singleton_data.shadow_images[i].image.width < chosen_framebuffer_width &&
                singleton_data.shadow_images[i].image.height >= p->resolution &&
                singleton_data.shadow_images[i].image.height < chosen_framebuffer_height;
            }
            else {
                match = singleton_data.shadow_images[i].gltex2d.width >= p->resolution &&
                singleton_data.shadow_images[i].gltex2d.width < chosen_framebuffer_width &&
                singleton_data.shadow_images[i].gltex2d.height >= p->resolution &&
                singleton_data.shadow_images[i].gltex2d.height < chosen_framebuffer_height;
            }

            if(match)
            {
                if(chosen_framebuffer_index >= 0) singleton_data.shadow_framebuffer_assigned[chosen_framebuffer_index] = false;

                chosen_framebuffer_index = (int)i;
                singleton_data.shadow_framebuffer_assigned[i] = true;

                if(VULKAN) {
                    chosen_framebuffer_width = singleton_data.shadow_images[i].image.width;
                    chosen_framebuffer_height = singleton_data.shadow_images[i].image.height;
                }
                else {
                    chosen_framebuffer_width = singleton_data.shadow_images[i].gltex2d.width;
                    chosen_framebuffer_height = singleton_data.shadow_images[i].gltex2d.height;
                }
            }
        }
        p->framebuffer_index = chosen_framebuffer_index;
    }

    // Delete unassigned
    for(unsigned int i = 0; i < 4; i++) {
        if(VULKAN) {
            if(singleton_data.shadow_framebuffers[i].vk_framebuffer && !singleton_data.shadow_framebuffer_assigned[i]) {
                pigeon_vulkan_destroy_framebuffer(&singleton_data.shadow_framebuffers[i]);
                pigeon_vulkan_destroy_image_view(&singleton_data.shadow_images[i].image_view);
                pigeon_vulkan_free_memory(&singleton_data.shadow_images[i].memory);
                pigeon_vulkan_destroy_image(&singleton_data.shadow_images[i].image);
            }
        }
        else {
            if(singleton_data.gl.shadow_framebuffers[i].id && !singleton_data.shadow_framebuffer_assigned[i]) {
                pigeon_opengl_destroy_framebuffer(&singleton_data.gl.shadow_framebuffers[i]);
                pigeon_opengl_destroy_texture(&singleton_data.shadow_images[i].gltex2d);
            }
        }
    }

    // Create new framebuffers

    for(unsigned int param_index_ = 0; param_index_ < 4; param_index_++) {
	    PigeonWGIShadowParameters *p = &singleton_data.shadow_parameters[shadow_param_indices_sorted[param_index_]];
        if(!p->resolution || p->framebuffer_index >= 0) break;

        for(unsigned int i = 0; i < 4; i++) {
            if(singleton_data.shadow_framebuffer_assigned[i]) continue;
            singleton_data.shadow_framebuffer_assigned[i] = true;
            p->framebuffer_index = (int)i;

            ASSERT_R1(!create_shadow_framebuffer(p, i));
            break;
        }

    }

    return 0;
}

void pigeon_wgi_set_object_shadow_mvp_uniform(PigeonWGIDrawObject * data, mat4 model_matrix)
{
    for(unsigned int i = 0; i < 4; i++) {
        PigeonWGIShadowParameters * p = &singleton_data.shadow_parameters[i];

        if(p->resolution)
            glm_mat4_mul(p->proj_view, model_matrix, data->proj_view_model[1+i]);
        else
            memset(data->proj_view_model[1+i], 0, 64);
    }
}

void pigeon_wgi_set_shadow_uniforms(PigeonWGISceneUniformData* data)
{
    for(unsigned int i = 0; i < 4; i++) {
        PigeonWGIShadowParameters * p = &singleton_data.shadow_parameters[i];
        if(!p->resolution) continue;
        
        data->lights[i].shadow_pixel_offset = 1.0f / (float)p->resolution;

        memcpy(data->lights[i].shadow_proj_view, p->proj_view, 64);
        data->lights[i].is_shadow_caster = 1.0f;

        vec4 dir = {0, 0, 1, 0};
        glm_mat4_mulv(p->inv_view_matrix, dir, dir);

        data->lights[i].neg_direction[0] = dir[0];
        data->lights[i].neg_direction[1] = dir[1];
        data->lights[i].neg_direction[2] = dir[2];

        vec4 position = {0, 0, 0, 1};
        glm_mat4_mulv(p->inv_view_matrix, position, position);

        data->lights[i].world_position[0] = position[0];
        data->lights[i].world_position[1] = position[1];
        data->lights[i].world_position[2] = position[2];
        data->lights[i].light_type = 0;
    }
}
