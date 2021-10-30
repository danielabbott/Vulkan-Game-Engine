#include "tex.h"
#include <pigeon/assert.h>



PIGEON_ERR_RET pigeon_wgi_create_array_texture(PigeonWGIArrayTexture* array_texture, 
    uint32_t width, uint32_t height, uint32_t layers, PigeonWGIImageFormat format, unsigned int mip_maps,
    PigeonWGICommandBuffer* cmd_buf)
{
    ASSERT_R1(array_texture && cmd_buf && width >= 4 && width <= 16384 && height >= 4 && height <= 16384);
    ASSERT_R1(layers && layers <= 1024);
    ASSERT_R1(format == PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB || format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR
    || (format >= PIGEON_WGI_IMAGE_FORMAT__FIRST_COMPRESSED_FORMAT &&
        format <= PIGEON_WGI_IMAGE_FORMAT__LAST_COMPRESSED_FORMAT));

    array_texture->data = calloc(1, sizeof *array_texture->data);
    ASSERT_R1(array_texture->data);

    if(!mip_maps) {
        unsigned int w = width, h = height;
        while(w >= 4 && h >= 4) {
            mip_maps += 1;
            w /= 2;
            h /= 2;
        }
    }
    

    array_texture->format = format;
    array_texture->width = width;
    array_texture->height = height;
    array_texture->layers = layers;
    array_texture->mip_maps = mip_maps;

    if(VULKAN) {
        if(pigeon_vulkan_create_texture_with_dedicated_memory(&array_texture->data->image,
            &array_texture->data->image_memory, &array_texture->data->image_view,
            format, width, height, layers, mip_maps, true))
        {
            // Cannot create device local. Fall back to host memory
            fputs("Failed to create device local image. Multitexture is falling back to host memory\n", stderr);

            if(pigeon_vulkan_create_texture_with_dedicated_memory(&array_texture->data->image,
                &array_texture->data->image_memory, &array_texture->data->image_view,
                format, width, height, layers, mip_maps, false))
            {
                free(array_texture->data);
                return 1;
            }
        }


        if(pigeon_vulkan_create_staging_buffer_with_dedicated_memory(&array_texture->data->staging_buffer,
            &array_texture->data->staging_buffer_memory, 
            pigeon_wgi_get_array_texture_layer_size(array_texture) * layers, &array_texture->mapping))
        {
            pigeon_vulkan_destroy_image_view(&array_texture->data->image_view);
            pigeon_vulkan_destroy_image(&array_texture->data->image);
            pigeon_vulkan_free_memory(&array_texture->data->image_memory);
            free(array_texture->data);
            array_texture->data = NULL;
            return 1;
        }

        pigeon_vulkan_transition_image_to_transfer_dst(&cmd_buf->command_pool, 0, &array_texture->data->image);
    }

    else if (OPENGL) {
        if(pigeon_opengl_create_texture(&array_texture->data->gl.texture, format, 
            width, height, layers, mip_maps)) 
        {
            free(array_texture->data);
            array_texture->data = NULL;
            return 1;
        }

        pigeon_opengl_set_texture_sampler(&array_texture->data->gl.texture,
            true, true, false, false, true);
    }

    return 0;

}

uint32_t pigeon_wgi_get_array_texture_layer_size(PigeonWGIArrayTexture const* array_texture)
{
    ASSERT_R0(array_texture->mip_maps && array_texture->width >= 4 && array_texture->height >= 4);

    uint32_t size = 0;
    unsigned int w = array_texture->width, h = array_texture->height;

        
    for(unsigned int mip = 0; mip < array_texture->mip_maps; mip++) {
        size += w*h;
        w /= 2;
        h /= 2;
    }

    return (size * pigeon_image_format_bytes_per_4x4_block(array_texture->format))/16;

}

int pigeon_wgi_array_texture_upload_method(void)
{
    return VULKAN ? 1 : 2;
}

void* pigeon_wgi_array_texture_upload1(PigeonWGIArrayTexture* array_texture, 
    unsigned int layer, PigeonWGICommandBuffer* cmd_buf)
{
    ASSERT_R0(VULKAN);

    uint64_t buffer_offset = array_texture->mapping_offset;
    
    array_texture->mapping_offset += pigeon_wgi_get_array_texture_layer_size(array_texture);
    
    pigeon_vulkan_transfer_buffer_to_image(&cmd_buf->command_pool, 0,
        &array_texture->data->staging_buffer, buffer_offset,
        &array_texture->data->image, layer, 0, 0,
        array_texture->width, array_texture->height, array_texture->mip_maps);    

    return &((uint8_t*)array_texture->mapping)[buffer_offset];
}

PIGEON_ERR_RET pigeon_wgi_array_texture_upload2(PigeonWGIArrayTexture* array_texture, 
    unsigned int layer, void * data)
{
    ASSERT_R1(OPENGL && array_texture && layer < array_texture->layers && data);

    uint64_t offset = 0;
    unsigned int w = array_texture->width, h = array_texture->height;

    unsigned block_size = pigeon_image_format_bytes_per_4x4_block(array_texture->format);

	for(unsigned int mip = 0; mip < array_texture->mip_maps; mip++) {
        unsigned int sz = (w*h * block_size) / 16;
        ASSERT_R1(!pigeon_opengl_upload_texture_mip(&array_texture->data->gl.texture, 
            mip, layer, (void *) ((uintptr_t)data + offset), sz));


        offset += sz;
        w /= 2;
        h /= 2;
    }

    return 0;
}

void pigeon_wgi_array_texture_transition(PigeonWGIArrayTexture* array_texture, PigeonWGICommandBuffer* cmd_buf)
{
    if(VULKAN)
        pigeon_vulkan_transition_transfer_dst_to_shader_read(&cmd_buf->command_pool, 0, &array_texture->data->image);
}

void pigeon_wgi_array_texture_unmap(PigeonWGIArrayTexture* array_texture)
{
    if(VULKAN) {
        assert(array_texture && array_texture->data && array_texture->data->staging_buffer.vk_buffer);
        pigeon_vulkan_unmap_memory(&array_texture->data->staging_buffer_memory);
    }
}

void pigeon_wgi_array_texture_transfer_done(PigeonWGIArrayTexture* array_texture)
{
    if(VULKAN) {
        assert(array_texture && array_texture->data && array_texture->data->staging_buffer.vk_buffer);

        pigeon_vulkan_destroy_buffer(&array_texture->data->staging_buffer);
        pigeon_vulkan_free_memory(&array_texture->data->staging_buffer_memory);
    }
}

void pigeon_wgi_destroy_array_texture(PigeonWGIArrayTexture* array_texture)
{
    assert(array_texture);

    if(!array_texture->data) return;

    if(VULKAN) {
        pigeon_vulkan_destroy_image_view(&array_texture->data->image_view);
        pigeon_vulkan_destroy_image(&array_texture->data->image);
        pigeon_vulkan_free_memory(&array_texture->data->image_memory);
        pigeon_vulkan_destroy_buffer(&array_texture->data->staging_buffer);
        pigeon_vulkan_free_memory(&array_texture->data->staging_buffer_memory);
    }
    else {
        pigeon_opengl_destroy_texture(&array_texture->data->gl.texture);
    }
    free(array_texture->data);
    array_texture->data = NULL;
}

void pigeon_wgi_bind_array_texture(unsigned int binding_point, PigeonWGIArrayTexture* array_texture)
{
    assert(array_texture && array_texture->data);

    if(VULKAN) {
        PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
        pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 5, binding_point, 
            &array_texture->data->image_view, &singleton_data.texture_sampler);
    }
    else {
        singleton_data.gl.bound_textures[binding_point] = array_texture;
    }
}
