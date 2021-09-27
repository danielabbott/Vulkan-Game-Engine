#include "tex.h"



ERROR_RETURN_TYPE pigeon_wgi_create_array_texture(PigeonWGIArrayTexture* array_texture, 
    uint32_t width, uint32_t height, uint32_t layers, PigeonWGIImageFormat format, unsigned int mip_maps,
    PigeonWGICommandBuffer* cmd_buf)
{
    ASSERT_1(array_texture && cmd_buf && width && width <= 16384 && height && height <= 16384 && layers && layers <= 1024);
    ASSERT_1(format == PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB || format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR
    || (format >= PIGEON_WGI_IMAGE_FORMAT__FIRST_COMPRESSED_FORMAT &&
        format <= PIGEON_WGI_IMAGE_FORMAT__LAST_COMPRESSED_FORMAT));

    array_texture->data = calloc(1, sizeof *array_texture->data);
    ASSERT_1(array_texture->data);

    if(!mip_maps) {
        unsigned int w = width, h = height;
        while(w >= 4 && h >= 4) {
            mip_maps += 1;
            w /= 2;
            h /= 2;
        }
    }

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

    array_texture->format = format;
    array_texture->width = width;
    array_texture->height = height;
    array_texture->layers = layers;
    array_texture->mip_maps = mip_maps;


    if(pigeon_vulkan_create_staging_buffer_with_dedicated_memory(&array_texture->data->staging_buffer,
        &array_texture->data->staging_buffer_memory, 
        pigeon_wgi_get_array_texture_layer_size(array_texture) * layers, &array_texture->mapping))
    {
        pigeon_vulkan_destroy_image_view(&array_texture->data->image_view);
        pigeon_vulkan_destroy_image(&array_texture->data->image);
        pigeon_vulkan_free_memory(&array_texture->data->image_memory);
        free(array_texture->data);
        return 1;
    }
    
    pigeon_vulkan_transition_image_to_transfer_dst(&cmd_buf->command_pool, 0, &array_texture->data->image);

    return 0;

}

uint32_t pigeon_wgi_get_array_texture_layer_size(PigeonWGIArrayTexture const* array_texture)
{
    uint32_t size = 0;
    unsigned int w = array_texture->width, h = array_texture->height;

    if(array_texture->mip_maps) {
        while(w >= 4 && h >= 4) {
            size += w*h;
            w /= 2;
            h /= 2;
        }
    }
    else {
        size = w*h;
    }

    return (size * pigeon_image_format_bytes_per_4x4_block(array_texture->format))/16;

}

void* pigeon_wgi_array_texture_upload(PigeonWGIArrayTexture* array_texture, 
    unsigned int layer, PigeonWGICommandBuffer* cmd_buf)
{
    uint64_t buffer_offset = array_texture->mapping_offset;
    
    array_texture->mapping_offset += pigeon_wgi_get_array_texture_layer_size(array_texture);
    
    pigeon_vulkan_transfer_buffer_to_image(&cmd_buf->command_pool, 0,
        &array_texture->data->staging_buffer, buffer_offset,
        &array_texture->data->image, layer, 0, 0,
        array_texture->width, array_texture->height, array_texture->mip_maps);    

    return &((uint8_t*)array_texture->mapping)[buffer_offset];
}

void pigeon_wgi_array_texture_transition(PigeonWGIArrayTexture* array_texture, PigeonWGICommandBuffer* cmd_buf)
{
    pigeon_vulkan_transition_transfer_dst_to_shader_read(&cmd_buf->command_pool, 0, &array_texture->data->image);
}

void pigeon_wgi_array_texture_unmap(PigeonWGIArrayTexture* array_texture)
{
    assert(array_texture && array_texture->data && array_texture->data->staging_buffer.vk_buffer);
    pigeon_vulkan_unmap_memory(&array_texture->data->staging_buffer_memory);
}

void pigeon_wgi_array_texture_transfer_done(PigeonWGIArrayTexture* array_texture)
{
    assert(array_texture && array_texture->data && array_texture->data->staging_buffer.vk_buffer);

    pigeon_vulkan_destroy_buffer(&array_texture->data->staging_buffer);
    pigeon_vulkan_free_memory(&array_texture->data->staging_buffer_memory);
}

void pigeon_wgi_destroy_array_texture(PigeonWGIArrayTexture* array_texture)
{
    if(array_texture->data) {
        if(array_texture->data->image_view.vk_image_view)
            pigeon_vulkan_destroy_image_view(&array_texture->data->image_view);

        if(array_texture->data->image.vk_image)
            pigeon_vulkan_destroy_image(&array_texture->data->image);

        if(array_texture->data->image_memory.vk_device_memory) 
            pigeon_vulkan_free_memory(&array_texture->data->image_memory);

        if(array_texture->data->staging_buffer.vk_buffer) 
            pigeon_vulkan_destroy_buffer(&array_texture->data->staging_buffer);

		if(array_texture->data->staging_buffer_memory.vk_device_memory) 
            pigeon_vulkan_free_memory(&array_texture->data->staging_buffer_memory);


        free(array_texture->data);
    }
    else {
        assert(false);
    }
}

void pigeon_wgi_bind_array_texture(unsigned int binding_point, PigeonWGIArrayTexture* array_texture)
{
    assert(array_texture && array_texture->data);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, binding_point, 
        &array_texture->data->image_view, &singleton_data.texture_sampler);
    pigeon_vulkan_set_descriptor_texture(&objects->light_pass_descriptor_pool, 0, 4, binding_point, 
        &array_texture->data->image_view, &singleton_data.texture_sampler);
}
