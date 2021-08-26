#include <pigeon/wgi/textures.h>
#include <pigeon/util.h>
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/vulkan/buffer.h>
#include <stdlib.h>
#include "singleton.h"


typedef struct PigeonWGIGridTextureData
{
    PigeonVulkanMemoryAllocation image_memory;
    PigeonVulkanImage image;
    PigeonVulkanImageView image_view;

    PigeonVulkanMemoryAllocation staging_buffer_memory;
    PigeonVulkanBuffer staging_buffer;
} PigeonWGIGridTextureData;

// TODO replace 8 with GRID_SIZE and 4096 with IMAGE_WIDTH_HEIGHT
// #define GRID_SIZE 8
// #define IMAGE_WIDTH_HEIGHT (GRID_SIZE*512)

ERROR_RETURN_TYPE pigeon_wgi_allocate_empty_grids(PigeonWGIGridTextureGrid** grid_ptr, unsigned int * grids_count, 
    unsigned int tiles)
{
    unsigned int tiles_per_grid = 8*8;
    unsigned int grids = (tiles+tiles_per_grid-1) / tiles_per_grid;

    *grid_ptr = calloc(grids, sizeof(PigeonWGIGridTextureGrid));
    ASSERT_1(*grid_ptr);
    *grids_count = grids;

    return 0;
}

static bool grid_region_empty(void ** tiles, unsigned int w, unsigned h, unsigned int x, unsigned int y)
{
    for(unsigned int y_ = 0; y_ < h; y_++) {
        for(unsigned int x_ = 0; x_ < w; x_++) {
            if(tiles[(y+y_)*8 + x+x_]) {
                return false;
            }
        }
    }
    return true;
}

ERROR_RETURN_TYPE pigeon_wgi_add_texture_to_grid(PigeonWGIGridTextureGrid ** grids_, unsigned int* grids_count,
    void * texture,
    unsigned int tiles_width, unsigned int tiles_height, PigeonWGITextureGridPosition* position,
    bool allow_add_new_grids)
{
    ASSERT_1(tiles_width > 0 && tiles_width <= 8 && tiles_height > 0 && tiles_height <= 8);
    ASSERT_1(grids_ && grids_count && position);

    PigeonWGIGridTextureGrid * grids = *grids_;

    if(*grids_count) ASSERT_1(grids);

    for(unsigned int i = 0; i < *grids_count; i++) {
        if(grids[i].used_tiles == 8*8) continue;

        for(unsigned int y = 0; y <= 8-tiles_height; y++) {
            for(unsigned int x = 0; x <= 8-tiles_width; x++) {
                if(grid_region_empty(grids[i].tiles, tiles_width, tiles_height, x, y)) {
                    for(unsigned int y_ = 0; y_ < tiles_height; y_++) {
                        for(unsigned int x_ = 0; x_ < tiles_width; x_++) {
                            grids[i].tiles[(y+y_)*8 + x+x_] = texture;
                        }
                    }
                    grids[i].used_tiles += tiles_width*tiles_height;
                    assert(grids[i].used_tiles <= 8*8);

                    position->grid_i = i;
                    position->grid_x = x;
                    position->grid_y = y;
                    position->grid_w = tiles_width;
                    position->grid_h = tiles_height;
                    return 0;
                }
            }
        }
    }

    if(!allow_add_new_grids) {
        return 1;
    }


    unsigned int new_grids_count = *grids_count + 1;
    grids = realloc(grids, new_grids_count * sizeof *grids);

    ASSERT_1(grids);

    memset(&grids[new_grids_count-1], 0, sizeof *grids);

    *grids_ = grids;
    *grids_count = new_grids_count;

    return pigeon_wgi_add_texture_to_grid(grids_, grids_count,
        texture, tiles_width, tiles_height, position, false);
}

static ERROR_RETURN_TYPE create_image(PigeonWGIGridTexture* grid_texture, PigeonWGIImageFormat format,
    unsigned int grids, bool device_local)
{
    PigeonVulkanMemoryRequirements memory_req;

	ASSERT_1 (!pigeon_vulkan_create_image(
		&grid_texture->data->image,
		format,
		4096, 4096, grids, 8,
		false, false,
		true, false,
		false, device_local,
		&memory_req
	))

	PigeonVulkanMemoryTypePerferences preferences = { 0 };
	preferences.device_local = device_local ? PIGEON_VULKAN_MEMORY_TYPE_MUST : PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT;
	preferences.host_visible = device_local ? PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT : PIGEON_VULKAN_MEMORY_TYPE_MUST;
	preferences.host_coherent = device_local ? PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT : PIGEON_VULKAN_MEMORY_TYPE_PREFERED;
	preferences.host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT;

	if (pigeon_vulkan_allocate_memory_dedicated(&grid_texture->data->image_memory, memory_req, preferences,
        &grid_texture->data->image, NULL)) {
        pigeon_vulkan_destroy_image(&grid_texture->data->image);
        ASSERT_1(false);
    }

	if (pigeon_vulkan_image_bind_memory_dedicated(&grid_texture->data->image, &grid_texture->data->image_memory)) {
        pigeon_vulkan_destroy_image(&grid_texture->data->image);
        pigeon_vulkan_free_memory(&grid_texture->data->image_memory);
        ASSERT_1(false);
    }

	if (pigeon_vulkan_create_image_view(&grid_texture->data->image_view, &grid_texture->data->image, true)) {
        pigeon_vulkan_destroy_image(&grid_texture->data->image);
        pigeon_vulkan_free_memory(&grid_texture->data->image_memory);
        ASSERT_1(false);
    }
    return 0;
}

static ERROR_RETURN_TYPE create_staging_buffer(PigeonWGIGridTexture* grid_texture, PigeonWGIImageFormat format,
    unsigned int grids)
{
    PigeonVulkanBufferUsages usages = {0};
    usages.transfer_src = true;

    uint64_t size = pigeon_wgi_get_grid_texture_tile_size(format) * 8*8 * grids;
    
	PigeonVulkanMemoryRequirements mem_req;
	ASSERT_1(!pigeon_vulkan_create_buffer(&grid_texture->data->staging_buffer, size, usages, &mem_req));

	PigeonVulkanMemoryTypePerferences preferences = { 0 };
	preferences.device_local = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT;
	preferences.host_visible = PIGEON_VULKAN_MEMORY_TYPE_MUST;
	preferences.host_coherent = PIGEON_VULKAN_MEMORY_TYPE_PREFERED;
	preferences.host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT;

	if (pigeon_vulkan_allocate_memory_dedicated(&grid_texture->data->staging_buffer_memory, mem_req, 
        preferences, NULL, &grid_texture->data->staging_buffer)) 
    {
		pigeon_vulkan_destroy_buffer(&grid_texture->data->staging_buffer);
		return 1;
	}

	if(pigeon_vulkan_buffer_bind_memory_dedicated(&grid_texture->data->staging_buffer, 
        &grid_texture->data->staging_buffer_memory) ||
        pigeon_vulkan_map_memory(&grid_texture->data->staging_buffer_memory, &grid_texture->mapping)) 
    {
		pigeon_vulkan_destroy_buffer(&grid_texture->data->staging_buffer);
		pigeon_vulkan_free_memory(&grid_texture->data->staging_buffer_memory);
		return 1;
	}
	return 0;
}

ERROR_RETURN_TYPE pigeon_wgi_create_grid_texture(PigeonWGIGridTexture* grid_texture, 
    uint32_t grids, PigeonWGIImageFormat format, PigeonWGICommandBuffer* cmd_buf)
{
    ASSERT_1(grid_texture && cmd_buf);
    ASSERT_1(format == PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB || format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR
    || (format >= PIGEON_WGI_IMAGE_FORMAT__FIRST_COMPRESSED_FORMAT &&
        format <= PIGEON_WGI_IMAGE_FORMAT__LAST_COMPRESSED_FORMAT));

    grid_texture->data = calloc(1, sizeof *grid_texture->data);
    ASSERT_1(grid_texture->data);

    if(create_image(grid_texture, format, grids, true)) {
        // Cannot create device local. Fall back to host memory
        fputs("Failed to create device local image. Multitexture is falling back to host memory\n", stderr);
        grid_texture->host_memory_image = true;

        if(create_image(grid_texture, format, grids, false)) {
            free(grid_texture->data);
            return 1;
        }
    }

    if(create_staging_buffer(grid_texture, format, grids)) {
        pigeon_vulkan_destroy_image_view(&grid_texture->data->image_view);
        pigeon_vulkan_destroy_image(&grid_texture->data->image);
        pigeon_vulkan_free_memory(&grid_texture->data->image_memory);
        return 1;
    }
    
    pigeon_vulkan_transition_image_to_transfer_dst(&cmd_buf->command_pool, 0, &grid_texture->data->image);

    grid_texture->format = format;

    return 0;
}

uint32_t pigeon_wgi_get_grid_texture_tile_size(PigeonWGIImageFormat format)
{
    return ((512*512+256*256+128*128+64*64+32*32+16*16+8*8+4*4)/16)
    * pigeon_image_format_bytes_per_4x4_block(format);
}

void* pigeon_wgi_grid_texture_upload(PigeonWGIGridTexture* grid_texture, 
    PigeonWGITextureGridPosition position, unsigned int w, unsigned int h, PigeonWGICommandBuffer* cmd_buf)
{
    assert((w+511) / 512 == position.grid_w);
    assert((h+511) / 512 == position.grid_h);
    assert(position.grid_x + position.grid_w <= 8);
    assert(position.grid_y + position.grid_h <= 8);

    uint64_t buffer_offset = grid_texture->mapping_offset;
    
    grid_texture->mapping_offset += position.grid_w * position.grid_h * 
        pigeon_wgi_get_grid_texture_tile_size(grid_texture->format);
    
    pigeon_vulkan_transfer_buffer_to_image(&cmd_buf->command_pool, 0,
        &grid_texture->data->staging_buffer, buffer_offset,
        &grid_texture->data->image, position.grid_i, position.grid_x*512, position.grid_y*512,
        w,h, 8);    

    return &((uint8_t*)grid_texture->mapping)[buffer_offset];
}

void pigeon_wgi_grid_texture_transition(PigeonWGIGridTexture* grid_texture, PigeonWGICommandBuffer* cmd_buf)
{

    pigeon_vulkan_transition_transfer_dst_to_shader_read(&cmd_buf->command_pool, 0, &grid_texture->data->image);
}

void pigeon_wgi_grid_texture_unmap(PigeonWGIGridTexture* grid_texture)
{
    assert(grid_texture && grid_texture->data && grid_texture->data->staging_buffer.vk_buffer);
    pigeon_vulkan_unmap_memory(&grid_texture->data->staging_buffer_memory);
}

void pigeon_wgi_grid_texture_transfer_done(PigeonWGIGridTexture* grid_texture)
{
    assert(grid_texture && grid_texture->data && grid_texture->data->staging_buffer.vk_buffer);

    pigeon_vulkan_destroy_buffer(&grid_texture->data->staging_buffer);
    pigeon_vulkan_free_memory(&grid_texture->data->staging_buffer_memory);
}

void pigeon_wgi_destroy_grid_texture(PigeonWGIGridTexture* grid_texture)
{
    if(grid_texture->data) {
        if(grid_texture->data->image_view.vk_image_view)
            pigeon_vulkan_destroy_image_view(&grid_texture->data->image_view);

        if(grid_texture->data->image.vk_image)
            pigeon_vulkan_destroy_image(&grid_texture->data->image);

        if(grid_texture->data->image_memory.vk_device_memory) 
            pigeon_vulkan_free_memory(&grid_texture->data->image_memory);

        if(grid_texture->data->staging_buffer.size) 
            pigeon_vulkan_destroy_buffer(&grid_texture->data->staging_buffer);

		if(grid_texture->data->staging_buffer_memory.vk_device_memory) 
            pigeon_vulkan_free_memory(&grid_texture->data->staging_buffer_memory);


        free(grid_texture->data);
    }
    else {
        assert(false);
    }
}

void pigeon_wgi_bind_grid_texture(unsigned int binding_point, PigeonWGIGridTexture* grid_texture)
{
    assert(grid_texture && grid_texture->data);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, binding_point, 
        &grid_texture->data->image_view, &singleton_data.texture_sampler);
}
