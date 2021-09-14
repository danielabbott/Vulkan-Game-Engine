#include <pigeon/wgi/wgi.h>
#include <pigeon/wgi/vulkan/framebuffer.h>
#include "singleton.h"
#include <cglm/cam.h>

// code taken from cglm, changed so z = z-1
static void ortho_rh_z_1_0(float left, float right,
                float bottom,  float top,
                float nearZ, float farZ,
                mat4  dest) {

  glm_ortho_rh_zo(left, right,
                bottom,  top,
                nearZ, farZ,
                 dest);

    mat4 fix = {0};
    fix[0][0] = 1;
    fix[1][1] = 1;
    fix[2][2] = -1;
    fix[3][2] = 1;
    fix[3][3] = 1;

    glm_mat4_mul(fix, dest, dest);

}


static int shadow_compare(const void *i0_, const void *i1_)
{
	const unsigned int *i0 = i0_;
	const unsigned int *i1 = i1_;

	PigeonWGIShadowParameters *p0 = &singleton_data.shadow_parameters[*i0];
	PigeonWGIShadowParameters *p1 = &singleton_data.shadow_parameters[*i1];

	return (int)p1->resolution - (int)p0->resolution;
}

static ERROR_RETURN_TYPE validate(void)
{
    for(unsigned int i = 0; i < 4; i++) {
        PigeonWGIShadowParameters *p = &singleton_data.shadow_parameters[i];
        if(!p->resolution) continue;

        ASSERT_1(p->resolution % 2 == 0);
        ASSERT_1(p->resolution <= 4096);
        ASSERT_1(p->near_plane > 0);
        ASSERT_1(p->sizeX > 0);
        ASSERT_1(p->sizeY > 0);

        float x = p->sizeX;
        float y = p->sizeY;
        mat4 ortho;
        ortho_rh_z_1_0(-x, x, y, -y, p->near_plane, p->far_plane, ortho);

        glm_mat4_mul(ortho, p->view_matrix, p->proj_view);

        p->framebuffer_index = -1;
    }
    return 0;
}


static ERROR_RETURN_TYPE create_shadow_framebuffer(PigeonWGIShadowParameters * p, unsigned int framebuffer_index)
{
    ASSERT_1(!pigeon_wgi_create_framebuffer_images(&singleton_data.shadow_images[framebuffer_index],
        PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24, p->resolution, p->resolution, false, false));

    ASSERT_1(!create_framebuffer(&singleton_data.shadow_framebuffers[framebuffer_index],
        &singleton_data.shadow_images[framebuffer_index].image_view, NULL, &singleton_data.rp_depth));
    return 0;
}

ERROR_RETURN_TYPE pigeon_wgi_assign_shadow_framebuffers(void)
{
    ASSERT_1(!validate());

    unsigned int shadow_param_indices_sorted[4] = {0, 1, 2, 3};
    qsort(shadow_param_indices_sorted, sizeof shadow_param_indices_sorted[0], 4, shadow_compare);

    memset(singleton_data.shadow_framebuffer_assigned, 0, sizeof singleton_data.shadow_framebuffer_assigned);

    // Assign framebuffer from already allocated framebuffers

    for(unsigned int param_index_ = 0; param_index_ < 4; param_index_++) {
	    PigeonWGIShadowParameters *p = &singleton_data.shadow_parameters[shadow_param_indices_sorted[param_index_]];
        p->framebuffer_index = -1;
        if(!p->resolution) break;

        // Find the smallest framebuffer that is not assigned
        int chosen_framebuffer_index = -1;
        unsigned int chosen_framebuffer_width = 99999;
        unsigned int chosen_framebuffer_height = 99999;

        for(unsigned int i = 0; i < 4; i++) {
            if(!singleton_data.shadow_framebuffers->vk_framebuffer) continue;
            if(singleton_data.shadow_framebuffer_assigned[i]) continue;

            if(singleton_data.shadow_images[i].image.width >= p->resolution &&
                singleton_data.shadow_images[i].image.width < chosen_framebuffer_width &&
                singleton_data.shadow_images[i].image.height >= p->resolution &&
                singleton_data.shadow_images[i].image.height < chosen_framebuffer_height)
            {
                if(chosen_framebuffer_index >= 0) singleton_data.shadow_framebuffer_assigned[chosen_framebuffer_index] = false;

                chosen_framebuffer_index = (int)i;
                singleton_data.shadow_framebuffer_assigned[i] = true;
                chosen_framebuffer_width = singleton_data.shadow_images[i].image.width;
                chosen_framebuffer_height = singleton_data.shadow_images[i].image.height;
            }
        }
        p->framebuffer_index = chosen_framebuffer_index;
    }

    // Delete unassigned
    for(unsigned int i = 0; i < 4; i++) {
        if(singleton_data.shadow_framebuffers[i].vk_framebuffer && !singleton_data.shadow_framebuffer_assigned[i]) {
            pigeon_vulkan_destroy_framebuffer(&singleton_data.shadow_framebuffers[i]);
            pigeon_vulkan_destroy_image_view(&singleton_data.shadow_images[i].image_view);
            pigeon_vulkan_free_memory(&singleton_data.shadow_images[i].memory);
            pigeon_vulkan_destroy_image(&singleton_data.shadow_images[i].image);
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

            ASSERT_1(!create_shadow_framebuffer(p, i));
            break;
        }

    }

    return 0;
}

void pigeon_wgi_set_shadow_uniforms(PigeonWGISceneUniformData* data, PigeonWGIDrawCallObject * draw_calls, 
    unsigned int num_draw_calls)
{
    for(unsigned int i = 0; i < 4; i++) {
        PigeonWGIShadowParameters * p = &singleton_data.shadow_parameters[i];
        if(!p->resolution) continue;
        
        data->lights[i].shadow_pixel_offset = 1.0f / (float)p->resolution;
        memcpy(data->lights[i].shadow_proj_view, p->proj_view, 64);
        data->lights[i].is_shadow_caster = 1.0f;

        mat4 inv_view;
        glm_mat4_inv(p->view_matrix, inv_view);

        vec4 dir = {0, 0, 1, 0};
        glm_mat4_mulv(inv_view, dir, dir);

        data->lights[i].neg_direction[0] = dir[0];
        data->lights[i].neg_direction[1] = dir[1];
        data->lights[i].neg_direction[2] = dir[2];
    }

    for(unsigned int i = 0; i < num_draw_calls; i++) {
        for(unsigned int j = 0; j < 4; j++) {
            glm_mat4_mul(data->lights[j].shadow_proj_view, draw_calls[i].model, 
                draw_calls[i].proj_view_model[1+j]);
        }
    }
}
