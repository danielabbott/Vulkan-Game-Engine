#pragma once

#include "image_formats.h"
#include <stdbool.h>
#include <pigeon/util.h>
#include <pigeon/wgi/wgi.h>

struct PigeonWGICommandBuffer;

typedef struct PigeonWGITextureMeta
{
    unsigned int width;
    unsigned int height;
    PigeonWGIImageFormat format;
    bool has_mip_maps;

    // The compressed texture data is always stored in the same order as these variables
    // Mipmap chains are stored together, smallest mip size is 4x4 pixels (this includes uncompressed formats)
    // e.g. 16x16 RGB image: bc1_level0 bc1_level1, bc1_level2, bc5_level0, bc5_level1, bc5_level2

    bool has_bc1;
    bool has_bc3;
    bool has_bc5;
    bool has_bc7;
    bool has_etc1;
    bool has_etc2;
    bool has_etc2_alpha;
} PigeonWGITextureMeta;


typedef struct PigeonWGIGridTextureGrid {
    void* tiles[8*8]; // Points to texture object. What that object is is up to the calling code.
    unsigned int used_tiles;
} PigeonWGIGridTextureGrid;

ERROR_RETURN_TYPE pigeon_wgi_allocate_empty_grids(PigeonWGIGridTextureGrid**, unsigned int * grids_count, unsigned int tiles);


typedef struct PigeonWGITextureGridPosition {
	unsigned int grid_i, grid_x, grid_y, grid_w, grid_h;
} PigeonWGITextureGridPosition;

// Start with pigeon_wgi_minimum_texture_grids_needed + 1 empty grids
// tiles_width and tiles_height are size of texture in 512x512 tiles
ERROR_RETURN_TYPE pigeon_wgi_add_texture_to_grid(PigeonWGIGridTextureGrid **, unsigned int* grids_count,
    void * texture, // Can be anything.
    unsigned int tiles_width, unsigned int tiles_height, PigeonWGITextureGridPosition*,
    bool allow_add_new_grids);


struct PigeonWGIGridTextureData;

typedef struct PigeonWGIGridTexture {
    struct PigeonWGIGridTextureData * data;
    bool host_memory_image; // If true, not using staging buffer

    void * mapping;
    unsigned int mapping_offset;

    // unsigned int * width_height;
    // unsigned int * grids;
    PigeonWGIImageFormat format; // Either RGBA_U8_*, RG_U8_LINEAR or a compressed format
} PigeonWGIGridTexture;

// mapping is set to a host-accessible write-only memory mapping
ERROR_RETURN_TYPE pigeon_wgi_create_grid_texture(PigeonWGIGridTexture*, uint32_t grids, PigeonWGIImageFormat,
    PigeonWGICommandBuffer*);

// 512 * 512 * bytes per pixel
// For mip map level 0
uint32_t pigeon_wgi_get_grid_texture_tile_size(PigeonWGIImageFormat format);

// Returns pointer to memcpy (or zstd decompress) texture data to
// Write (pigeon_wgi_get_grid_texture_tile_size() * grid_position.w * grid_position.h) bytes
// There are 8 mip map levels (0 to 7 inclusive). All must be stored.
// Width&height halve with each mip level, data size in bytes quarters
// Adds transfer commands to command buffer
void* pigeon_wgi_grid_texture_upload(PigeonWGIGridTexture*, 
    PigeonWGITextureGridPosition, unsigned int w, unsigned int h,
    PigeonWGICommandBuffer*);

// Call when finished calling pigeon_wgi_grid_texture_upload
void pigeon_wgi_grid_texture_transition(PigeonWGIGridTexture* grid_texture, PigeonWGICommandBuffer* cmd_buf);

void pigeon_wgi_grid_texture_unmap(PigeonWGIGridTexture*);

void pigeon_wgi_grid_texture_transfer_done(PigeonWGIGridTexture*); // Call this when the transfer is done *on the GPU*
void pigeon_wgi_destroy_grid_texture(PigeonWGIGridTexture*);

// Mip maps go down to 2x2
uint32_t pigeon_wgi_get_texture_size(unsigned int width, unsigned int height, PigeonWGIImageFormat format, bool mip_maps);

// ** Binding points are per-frame-in-flight
void pigeon_wgi_bind_grid_texture(unsigned int binding_point, PigeonWGIGridTexture*);
