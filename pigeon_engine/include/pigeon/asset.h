#pragma once

#include <pigeon/wgi/mesh.h>
#include <pigeon/wgi/textures.h>
#include <pigeon/wgi/material.h>
#include <pigeon/wgi/animation.h>
#include <pigeon/audio/audio.h>
#include <pigeon/util.h>

typedef enum {
    PIGEON_ASSET_TYPE_UNKNOWN,
    PIGEON_ASSET_TYPE_NONE, // Binary data or text
    PIGEON_ASSET_TYPE_MODEL,
    PIGEON_ASSET_TYPE_IMAGE,
    PIGEON_ASSET_TYPE_AUDIO
} PigeonAssetType;

typedef enum {
    PIGEON_ASSET_SUBRESOURCE_TYPE_UNCOMPRESSED, // generic raw data
    PIGEON_ASSET_SUBRESOURCE_TYPE_ZSTD, // zstd compressed generic data
    PIGEON_ASSET_SUBRESOURCE_TYPE_OGG_FILE,
} PigeonAssetSubresourceType;

typedef struct PigeonAssetSubresource {
    PigeonAssetSubresourceType type;

    uint32_t original_file_data_offset;

    uint32_t compressed_data_length;
    uint32_t decompressed_data_length;
    
    void * compressed_data;
    void * decompressed_data;

    bool compressed_data_was_mallocd;
    bool decompressed_data_was_mallocd;
} PigeonAssetSubresource;

typedef struct PigeonAsset {
    PigeonAssetType type;

    char * name;

    unsigned int subresource_count;
    PigeonAssetSubresource* subresources;

    // If not NULL, was malloc'd
    void * original_data;


    union {
        // Type-specific data
        // Pointers in these structs are all malloc allocations

        // Models
        struct {
            PigeonWGIMeshMeta mesh_meta;

            unsigned int materials_count;
            PigeonWGIMaterialImport* materials;

            unsigned int bones_count;
            PigeonWGIBone* bones;

            unsigned int animations_count;
            PigeonWGIAnimationMeta* animations;
        };

        // Textures
        PigeonWGITextureMeta texture_meta;

        // Audio
        PigeonAudioMeta audio_meta;
    };

} PigeonAsset;

// TODO object pool for assets

// 'meta file' is the .asset text file
PIGEON_ERR_RET pigeon_load_asset_meta(PigeonAsset *, const char * meta_file_path);

// Loads ZSTD compressed data (or decompressed if available)
PIGEON_ERR_RET pigeon_load_asset_data(PigeonAsset *, const char * data_file_path);

// Decompresses data into given buffer
// buffer must be >= asset->subresources[i].decompressed_data_length
PIGEON_ERR_RET pigeon_decompress_asset(PigeonAsset *, void * buffer, unsigned int i);

// Use this if the asset is not compressed to fread the data into buffer
// buffer must be >= raw_data_length
// PIGEON_ERR_RET pigeon_load_decompressed(PigeonAsset *, void * buffer);

void pigeon_free_asset(PigeonAsset *);


// TODO PigeonAssetPackage

// PIGEON_ERR_RET pigeon_load_assets(const char * meta_file_path, PigeonAsset **, unsigned int * data_file_count, void ** data_files);
