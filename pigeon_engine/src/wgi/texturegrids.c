// * untested *
#include "tex.h"
#include <stdlib.h>

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

ERROR_RETURN_TYPE pigeon_wgi_create_grid_texture(PigeonWGIGridTexture* grid_texture, 
    uint32_t grids, PigeonWGIImageFormat format, PigeonWGICommandBuffer* cmd_buf, bool mip_maps)
{
    ASSERT_1(grid_texture && cmd_buf);
    ASSERT_1(format == PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB || format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR
    || (format >= PIGEON_WGI_IMAGE_FORMAT__FIRST_COMPRESSED_FORMAT &&
        format <= PIGEON_WGI_IMAGE_FORMAT__LAST_COMPRESSED_FORMAT));

    ASSERT_1(pigeon_wgi_create_array_texture(&grid_texture->array_texture, 
        4096, 4096, grids, format, mip_maps ? 8 : 1, cmd_buf));
    return 0;
}

uint32_t pigeon_wgi_get_grid_texture_tile_size(PigeonWGIImageFormat format, bool mip_maps)
{
    if(mip_maps)
        return ((512*512+256*256+128*128+64*64+32*32+16*16+8*8+4*4)/16)
            * pigeon_image_format_bytes_per_4x4_block(format);
    else
        return ((512*512)/16)
            * pigeon_image_format_bytes_per_4x4_block(format);
}

void* pigeon_wgi_grid_texture_upload(PigeonWGIGridTexture* grid_texture, 
    PigeonWGITextureGridPosition position, unsigned int w, unsigned int h, PigeonWGICommandBuffer* cmd_buf)
{
    assert(w % 512 == 0);
    assert(h % 512 == 0);
    assert((w+511) / 512 == position.grid_w);
    assert((h+511) / 512 == position.grid_h);
    assert(position.grid_x + position.grid_w <= 8);
    assert(position.grid_y + position.grid_h <= 8);

    uint64_t buffer_offset = grid_texture->array_texture.mapping_offset;
    
    grid_texture->array_texture.mapping_offset += position.grid_w * position.grid_h * 
        pigeon_wgi_get_grid_texture_tile_size(grid_texture->array_texture.format, grid_texture->array_texture.mip_maps > 1);
    
    pigeon_vulkan_transfer_buffer_to_image(&cmd_buf->command_pool, 0,
        &grid_texture->array_texture.data->staging_buffer, buffer_offset,
        &grid_texture->array_texture.data->image, position.grid_i, position.grid_x*512, position.grid_y*512,
        w,h, grid_texture->array_texture.mip_maps ? 8 : 1);    

    return &((uint8_t*)grid_texture->array_texture.mapping)[buffer_offset];
}

void pigeon_wgi_grid_texture_transition(PigeonWGIGridTexture* grid_texture, PigeonWGICommandBuffer* cmd_buf)
{
    pigeon_wgi_array_texture_transition(&grid_texture->array_texture, cmd_buf);
}

void pigeon_wgi_grid_texture_unmap(PigeonWGIGridTexture* grid_texture)
{
    pigeon_wgi_array_texture_unmap(&grid_texture->array_texture);
}

void pigeon_wgi_grid_texture_transfer_done(PigeonWGIGridTexture* grid_texture)
{
    pigeon_wgi_array_texture_transfer_done(&grid_texture->array_texture);
}

void pigeon_wgi_destroy_grid_texture(PigeonWGIGridTexture* grid_texture)
{
    pigeon_wgi_destroy_array_texture(&grid_texture->array_texture);
}

void pigeon_wgi_bind_grid_texture(unsigned int binding_point, PigeonWGIGridTexture* grid_texture)
{
    pigeon_wgi_bind_array_texture(binding_point, &grid_texture->array_texture);
}
