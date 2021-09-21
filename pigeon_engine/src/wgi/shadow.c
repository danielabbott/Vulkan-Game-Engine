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


static ERROR_RETURN_TYPE validate(void)
{
    for(unsigned int i = 0; i < 4; i++) {
        PigeonWGIShadowParameters *p = &singleton_data.shadow_parameters[i];
        if(!p->resolution) continue;

        ASSERT_1(p->resolution % 2 == 0);
        ASSERT_1(p->resolution <= 4096);
        ASSERT_1(p->resolution >= 16);
        ASSERT_1(p->near_plane > 0);
        ASSERT_1(p->sizeX > 0);
        ASSERT_1(p->sizeY > 0);

        float x = p->sizeX;
        float y = p->sizeY;
        mat4 ortho;
        ortho_rh_z_1_0(-x, x, y, -y, p->near_plane, p->far_plane, ortho);

        glm_mat4_mul(ortho, p->view_matrix, p->proj_view);
    }
    return 0;
}


static ERROR_RETURN_TYPE check_fb_size(void)
{
    unsigned int min_w=0, min_h=0;

    for(unsigned int i = 0; i < 4; i++) {
        if(singleton_data.shadow_parameters[i].resolution > min_w) {
            min_w = singleton_data.shadow_parameters[i].resolution;
        }
        if(singleton_data.shadow_parameters[i].resolution > min_h) {
            min_h = singleton_data.shadow_parameters[i].resolution;
        }
    }

    if(!min_w || !min_h) return 0;

    if(!singleton_data.shadow_map_image.image.vk_image || 
        (min_w > singleton_data.shadow_map_image.image.width
        || min_h > singleton_data.shadow_map_image.image.height))
    {
        if(singleton_data.shadow_map_image.image.vk_image) {
            // Delete

            pigeon_vulkan_destroy_framebuffer(&singleton_data.shadow_map_framebuffer);            
            pigeon_vulkan_destroy_image_view(&singleton_data.shadow_map_image.image_view);
            pigeon_vulkan_free_memory(&singleton_data.shadow_map_image.memory);
            pigeon_vulkan_destroy_image(&singleton_data.shadow_map_image.image);
        }

        ASSERT_1(!pigeon_wgi_create_framebuffer_images(&singleton_data.shadow_map_image,
            PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32, min_w, min_h, false, false));

        ASSERT_1(!pigeon_vulkan_create_framebuffer(&singleton_data.shadow_map_framebuffer,
            &singleton_data.shadow_map_image.image_view, NULL, &singleton_data.rp_depth));
    }

    return 0;
}

// TODO rename
ERROR_RETURN_TYPE pigeon_wgi_assign_shadow_framebuffers(void)
{
    ASSERT_1(!validate());
    ASSERT_1(!check_fb_size());

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
