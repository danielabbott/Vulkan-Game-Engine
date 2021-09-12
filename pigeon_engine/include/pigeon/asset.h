#pragma once

#include <pigeon/wgi/mesh.h>
#include <pigeon/wgi/textures.h>
#include <pigeon/wgi/material.h>
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
    PIGEON_ASSET_SUBRESOURCE_TYPE_NONE, // generic raw data
    PIGEON_ASSET_SUBRESOURCE_TYPE_ZSTD, // zstd compressed generic data
    PIGEON_ASSET_SUBRESOURCE_TYPE_OGG_FILE,
} PigeonAssetSubresourceType;

typedef struct PigeonAssetSubresource {
    PigeonAssetSubresourceType type;

    uint32_t original_file_data_offset;
    
    void * compressed_data;
    bool compressed_data_was_mallocd;
    uint32_t compressed_data_length;

    void * decompressed_data;
    bool decompressed_data_was_mallocd;
    uint32_t decompressed_data_length;
} PigeonAssetSubresource;

typedef struct PigeonAsset {
    PigeonAssetType type;

    char * name;


    #define PIGEON_ASSET_MAX_SUBRESOURCES (PIGEON_WGI_MAX_VERTEX_ATTRIBUTES+1)
    PigeonAssetSubresource subresources[PIGEON_ASSET_MAX_SUBRESOURCES];

    // If not NULL, was malloc'd
    void * original_data;


    union {
        // Type-specific data
        // Pointers in these structs are always malloc'd and copied to

        // Models
        struct {
            PigeonWGIMeshMeta mesh_meta;

            unsigned int materials_count;
            PigeonWGIMaterialImport* materials;
        };

        // Textures
        PigeonWGITextureMeta texture_meta;

        // Audio
        PigeonAudioMeta audio_meta;
    };

} PigeonAsset;

// 'meta file' is the .asset text file
ERROR_RETURN_TYPE pigeon_load_asset_meta(PigeonAsset *, const char * meta_file_path);

// Loads ZSTD compressed data (or decompressed if available)
ERROR_RETURN_TYPE pigeon_load_asset_data(PigeonAsset *, const char * data_file_path);

// Decompresses data into given buffer
// buffer must be >= asset->subresources[i].decompressed_data_length
ERROR_RETURN_TYPE pigeon_decompress_asset(PigeonAsset *, void * buffer, unsigned int i);

// Use this if the asset is not compressed to fread the data into buffer
// buffer must be >= raw_data_length
// ERROR_RETURN_TYPE pigeon_load_decompressed(PigeonAsset *, void * buffer);

void pigeon_free_asset(PigeonAsset *);


// TODO PigeonAssetPackage

// ERROR_RETURN_TYPE pigeon_load_assets(const char * meta_file_path, PigeonAsset **, unsigned int * data_file_count, void ** data_files);
