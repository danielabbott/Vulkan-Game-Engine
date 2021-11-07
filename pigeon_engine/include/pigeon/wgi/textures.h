#pragma once

#include "image_formats.h"
#include <stdbool.h>
#include <pigeon/util.h>
#include <pigeon/wgi/wgi.h>


typedef struct PigeonWGITextureMeta
{
    unsigned int width;
    unsigned int height;
    PigeonWGIImageFormat format;
    bool has_alpha;
    bool has_mip_maps;

    // The compressed texture data is always stored in the same order as these variables
    // Mipmap chains are stored together, smallest mip size is 4x4 pixels (this includes uncompressed formats)
    // e.g. 16x16 RGB image: bc1_level0 bc1_level1, bc1_level2, etc1_level0, etc1_level1, etc1_level2

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

PIGEON_ERR_RET pigeon_wgi_allocate_empty_grids(PigeonWGIGridTextureGrid**, unsigned int * grids_count, unsigned int tiles);


typedef struct PigeonWGITextureGridPosition {
	unsigned int grid_i, grid_x, grid_y, grid_w, grid_h;
} PigeonWGITextureGridPosition;

// Start with pigeon_wgi_minimum_texture_grids_needed + 1 empty grids
// tiles_width and tiles_height are size of texture in 512x512 tiles
PIGEON_ERR_RET pigeon_wgi_add_texture_to_grid(PigeonWGIGridTextureGrid **, unsigned int* grids_count,
    void * texture, // Can be anything.
    unsigned int tiles_width, unsigned int tiles_height, PigeonWGITextureGridPosition*,
    bool allow_add_new_grids);

// Mip maps go down to 4x4
uint32_t pigeon_wgi_get_texture_size(unsigned int width, unsigned int height, PigeonWGIImageFormat format, bool mip_maps);




typedef struct PigeonWGIArrayTexture {
    struct PigeonWGIArrayTextureData * data;

    unsigned int width, height, layers;
    PigeonWGIImageFormat format; // Either RGBA_U8_*, RG_U8_LINEAR or a compressed format
    unsigned int mip_maps;

    void * mapping;
    unsigned int mapping_offset;
} PigeonWGIArrayTexture;

PIGEON_ERR_RET pigeon_wgi_create_array_texture(PigeonWGIArrayTexture*, 
    uint32_t width, uint32_t height, uint32_t layers, PigeonWGIImageFormat,
    unsigned int mip_maps // 0 = auto, 1 = no mipmapping
);

uint32_t pigeon_wgi_get_array_texture_layer_size(PigeonWGIArrayTexture const*);

// If this returns 1, use pigeon_wgi_array_texture_upload1
// If this returns 2, use pigeon_wgi_array_texture_upload2
int pigeon_wgi_array_texture_upload_method(void);

// Transition image to transfer_dst
void pigeon_wgi_start_array_texture_upload(PigeonWGIArrayTexture*);

// upload function copy (or the caller copies) pigeon_wgi_get_array_texture_layer_size() bytes

// Call this once for each layer and memcpy image data (full mip chain) to returned pointer
void* pigeon_wgi_array_texture_upload1(PigeonWGIArrayTexture*, 
    unsigned int layer);

// Call this once for each layer, this does the memcpy
PIGEON_ERR_RET pigeon_wgi_array_texture_upload2(PigeonWGIArrayTexture*, 
    unsigned int layer, void *);

// Call when finished calling pigeon_wgi_array_texture_upload1
void pigeon_wgi_array_texture_transition(PigeonWGIArrayTexture* array_texture);

void pigeon_wgi_array_texture_unmap(PigeonWGIArrayTexture*);

void pigeon_wgi_array_texture_transfer_done(PigeonWGIArrayTexture*); // Call this when the transfer is done *on the GPU*
void pigeon_wgi_destroy_array_texture(PigeonWGIArrayTexture*);

// ** Binding points are per-frame-in-flight
void pigeon_wgi_bind_array_texture(unsigned int binding_point, PigeonWGIArrayTexture*);


typedef struct PigeonWGIGridTexture {
    PigeonWGIArrayTexture array_texture;
} PigeonWGIGridTexture;

// mapping is set to a host-accessible write-only memory mapping
PIGEON_ERR_RET pigeon_wgi_create_grid_texture(PigeonWGIGridTexture*, uint32_t grids, PigeonWGIImageFormat,
    bool mip_maps);

// 512 * 512 * bytes per pixel * 1.33333ish
uint32_t pigeon_wgi_get_grid_texture_tile_size(PigeonWGIImageFormat format, bool mip_maps);

// Returns pointer to memcpy (or zstd decompress) texture data to
// Write (pigeon_wgi_get_grid_texture_tile_size() * grid_position.w * grid_position.h) bytes
// There are 8 mip map levels (0 to 7 inclusive). All must be stored.
// Width&height halve with each mip level, data size in bytes quarters
// Adds transfer commands to command buffer
void* pigeon_wgi_grid_texture_upload(PigeonWGIGridTexture*, 
    PigeonWGITextureGridPosition, unsigned int w, unsigned int h);

// Call when finished calling pigeon_wgi_grid_texture_upload
void pigeon_wgi_grid_texture_transition(PigeonWGIGridTexture* grid_texture);

void pigeon_wgi_grid_texture_unmap(PigeonWGIGridTexture*);

void pigeon_wgi_grid_texture_transfer_done(PigeonWGIGridTexture*); // Call this when the transfer is done *on the GPU*
void pigeon_wgi_destroy_grid_texture(PigeonWGIGridTexture*);

// ** Binding points are per-frame-in-flight
void pigeon_wgi_bind_grid_texture(unsigned int binding_point, PigeonWGIGridTexture*);

