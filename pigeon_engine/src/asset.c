#include <pigeon/asset.h>
#include <pigeon/util.h>
#include <parser.h>
#include <zstd.h>

enum {
    KEY_ASSET,
    KEY_TYPE,
    KEY_DECOMPRESSED_SIZE,
    KEY_COMPRESSION,

    KEY_VERTEX_COUNT,
    KEY_BOUNDS_MINIMUM,
    KEY_BOUNDS_RANGE,
    KEY_VERTEX_ATTRIBUTES,
    KEY_INDICES_TYPE,
    KEY_INDICES_COUNT,
    KEY_MATERIALS_COUNT,
    KEY_MATERIAL,
    KEY_COLOUR,
    KEY_COLOR,
    KEY_FLAT_COLOUR,
    KEY_FLAT_COLOR,
    KEY_FIRST,
    KEY_COUNT,
    KEY_TEXTURE,
    KEY_NORMAL_MAP,

    KEY_WIDTH,
    KEY_HEIGHT,
    KEY_IMAGE_FORMAT,
    KEY_MIP_MAPS,
    KEY_BC1,
    KEY_BC3,
    KEY_BC5,
    KEY_BC7,
    KEY_ETC1,
    KEY_ETC2,
    KEY_ETC2_ALPHA,


    KEY__MAX
};

const char * keys[] = {
    "ASSET",
    "TYPE",
    "DECOMPRESSED-SIZE",
    "COMPRESSION",

    "VERTEX-COUNT",
    "BOUNDS-MINIMUM",
    "BOUNDS-RANGE",
    "VERTEX-ATTRIBUTES",
    "INDICES-TYPE",
    "INDICES-COUNT",
    "MATERIALS-COUNT",
    "MATERIAL",
    "COLOUR",
    "COLOR",
    "FLAT-COLOUR",
    "FLAT-COLOR",
    "FIRST",
    "COUNT",
    "TEXTURE",
    "NORMAL-MAP",

    "WIDTH",
    "HEIGHT",
    "IMAGE-FORMAT",
    "MIP-MAPS",
    "BC1",
    "BC3",
    "BC5",
    "BC7",
    "ETC1",
    "ETC2",
    "ETC2-ALPHA",
};

static ERROR_RETURN_TYPE copy_asset_name(PigeonAsset * asset, const char * meta_file_path)
{
    size_t l = strlen(meta_file_path);
    ASSERT_1(l > 7 && l < 1000);
    ASSERT_1(memcmp(&meta_file_path[l-6], ".asset", 6) == 0);

    int i = (int)l-6-1;
    while(i && meta_file_path[i] != '/') {
        i--;
    }
    i++;

    asset->name = malloc(l-(unsigned)i-6+1);
    ASSERT_1(asset->name);

    memcpy(asset->name, &meta_file_path[i], l-(unsigned)i-6);
    
    asset->name[l-(unsigned)i-6] = 0;

    return 0;
}

ERROR_RETURN_TYPE pigeon_load_asset_meta(PigeonAsset * asset, const char * meta_file_path)
{
    ASSERT_1(asset);

    ASSERT_1(!copy_asset_name(asset, meta_file_path));

    unsigned long file_size;
    char* input = (char*)load_file(meta_file_path, 1, &file_size);
    ASSERT_1(input);
    if(!input) {
        free(asset->name);
        asset->name = NULL;
        ASSERT_1(false);
    }

    char * input_ = input;

    ASSERT_1(KEY__MAX*sizeof(*keys) == sizeof keys);

    #define ERR() {free(asset->name); asset->name = NULL; free(input_);  ASSERT_1(false);}

    bool got_indices_type = false;
    bool got_vertex_attributes = false;
    bool got_bounds_min = false;
    bool got_bounds_range = false;

    int material_index = -1;

    const char * value;
    int key;
    while(input && *input) {
        char * old_input_value = input;
        key = parser_match_key_get_value(&input, keys, KEY__MAX, &value);

        #define CHECK_TYPE(a_type) if(asset->type != a_type){ \
            fprintf(stderr, "%s not valid for asset type\n", keys[key]); ERR();}

        if (key == KEY_TYPE) {
            if(word_matches(value, "MODEL")) {
                asset->type = PIGEON_ASSET_TYPE_MODEL;
            }
            else if(word_matches(value, "IMAGE")) {
                asset->type = PIGEON_ASSET_TYPE_IMAGE;
            }
            else if(word_matches(value, "NONE") || word_matches(value, "TEXT")
                || word_matches(value, "BINARY"))
            {
                asset->type = PIGEON_ASSET_TYPE_NONE;
            }
            else {
                fprintf(stderr, "Unrecognised asset type: %s\n", value);
                ERR();
            }
        }
        else if (key == KEY_COMPRESSION) {
            uint32_t file_offset = 0;
            for(unsigned int i = 0; i < PIGEON_ASSET_MAX_SUBRESOURCES && *value; i++) {
                if(word_matches(value, "ZSTD")) {
                    asset->compression[i].type = PIGEON_ASSET_COMPRESSION_TYPE_ZSTD;
                }
                else if(word_matches(value, "NONE")) {
                    asset->compression[i].type = PIGEON_ASSET_COMPRESSION_TYPE_NONE;
                }
                else {
                    fputs("Unknown compression type\n", stderr);
                    ERR();
                }
                value += 4;

                #define ERR2() \
                    {fputs("Invalid COMPRESSION value(s)\n", stderr); \
                    ERR();}

                while(*value == ' ' || *value == '\t') value++;
                if(!*value) ERR2();

                asset->compression[i].original_file_data_offset = file_offset;

                long int sz_ = strtol(value, (char**)&value, 10);
                if(sz_ <= 0 || sz_ > UINT32_MAX) ERR2();
                uint32_t sz = (uint32_t)sz_;


                if(asset->compression[i].type == PIGEON_ASSET_COMPRESSION_TYPE_ZSTD) {
                    asset->compression[i].decompressed_data_length = sz;
                    if(*value != '>') ERR2();
                    value++;

                    sz_ = strtol(value, NULL, 10);
                    if(sz_ <= 0 || sz_ > UINT32_MAX) ERR2();
                    sz = (uint32_t)sz_;

                    asset->compression[i].compressed_data_length = sz;
                }
                else {
                    asset->compression[i].decompressed_data_length = sz;
                }
                file_offset += sz;
                
                
                while(*value && *value != ' ' && *value != '\t') value++;
                while(*value == ' ' || *value == '\t') value++;
                if(!*value) break;
                #undef ERR2
            }
        }
        else if (key == KEY_VERTEX_COUNT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);
            
            long int c = strtol(value, NULL, 10);
            if(c < 0 || c > UINT32_MAX) {
                fprintf(stderr, "Invalid VERTEX-COUNT: %li\n", c);
                ERR();
            }
            asset->mesh_meta.vertex_count = (uint32_t)c;
        }
        else if (key == KEY_INDICES_COUNT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);

            long int c = strtol(value, NULL, 10);
            if(c < 0 || c > UINT32_MAX) {
                fprintf(stderr, "Invalid INDICES-COUNT: %li\n", c);
                ERR();
            }
            asset->mesh_meta.index_count = (uint32_t)c;
        }
        else if (key == KEY_INDICES_TYPE) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);

            got_indices_type = true;
            if(word_matches(value, "U32")) {
                asset->mesh_meta.big_indices = true;
            }
            else if(word_matches(value, "U16")) {
                asset->mesh_meta.big_indices = false;
            }
            else {
                fputs("Expected U32 or U16 for INDICES-TYPE\n", stderr);
                ERR();
            }
        }
        else if (key == KEY_VERTEX_ATTRIBUTES) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);
            got_vertex_attributes = true;


            unsigned int i = 0;
            for(; i < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES && *value; i++) {
                PigeonWGIVertexAttributeType type;
                if(word_matches(value, "POSITION")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION;
                else if(word_matches(value, "POSITION-2D")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION2D;
                else if(word_matches(value, "POSITION-NORMALISED")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED;
                
                // Incorrect spellings are also accepted
                else if(word_matches(value, "POSITION-NORMALIZED")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED;
               
                else if(word_matches(value, "COLOUR")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR;
                else if(word_matches(value, "COLOUR-RGBA8")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR_RGBA8;
                else if(word_matches(value, "COLOR")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR;
                else if(word_matches(value, "COLOR-RGBA8")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR_RGBA8;
                else if(word_matches(value, "NORMAL")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_NORMAL;
                else if(word_matches(value, "TANGENT")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_TANGENT;
                else if(word_matches(value, "UV")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_UV;
                else if(word_matches(value, "BONE2")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_BONE2;
                else {
                    fprintf(stderr, "Unrecognised vertex attribute\n");
                    ERR();
                }

                asset->mesh_meta.attribute_types[i] = type;

                while(*value && *value != ' ' && *value != '\t') value++;
                while(*value == ' ' || *value == '\t') value++;
            }
            if(i < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES) asset->mesh_meta.attribute_types[i] = PIGEON_WGI_VERTEX_ATTRIBUTE_NONE;
        }
        else if (key == KEY_BOUNDS_MINIMUM) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);
            got_bounds_min = true;

            asset->mesh_meta.bounds_min[0] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            if(!*value) ERR();
            asset->mesh_meta.bounds_min[1] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            if(!*value) ERR();
            asset->mesh_meta.bounds_min[2] = strtof(value, (char**)&value);
        }
        else if (key == KEY_BOUNDS_RANGE) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);
            got_bounds_range = true;

            asset->mesh_meta.bounds_range[0] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            if(!*value) ERR();
            asset->mesh_meta.bounds_range[1] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            if(!*value) ERR();
            asset->mesh_meta.bounds_range[2] = strtof(value, NULL);

            if(asset->mesh_meta.bounds_range[0] <= 0.0f ||
                asset->mesh_meta.bounds_range[1] <= 0.0f ||
                asset->mesh_meta.bounds_range[2] <= 0.0f)
            {
                fputs("Invalid vertex position range\n", stderr);
                ERR();
            }
        }
        else if (key == KEY_MATERIALS_COUNT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);

            if(asset->materials_count) {
                fputs("Multiple MATERIALS-COUNT\n", stderr);
                ERR();
            }

            long int c = strtol(value, NULL, 10);
            if(c < 0 || c > 999) {
                fprintf(stderr, "Invalid MATERIALS-COUNT: %li\n", c);
                ERR();
            }
            asset->materials_count = (unsigned int)c;
            asset->materials = calloc((unsigned)c, sizeof *asset->materials);
            material_index = -1;
        }
        else if (key == KEY_MATERIAL) {
            ++material_index;
            #define MATERIAL_CHECKS() \
                CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL); \
                if(!asset->materials_count) { \
                    fputs("Missing MATERIALS-COUNT\n", stderr); \
                    ERR(); \
                } \
                if((unsigned)material_index >= asset->materials_count) { \
                    fputs("Too many materials\n", stderr); \
                    ERR(); \
                }
            MATERIAL_CHECKS();

            char * name = copy_string_to_whitespace_to_malloc(value);
            if(!name) ERR();
            asset->materials[material_index].name = name;
        }
        else if (key == KEY_COLOUR || key == KEY_COLOR) {
            MATERIAL_CHECKS();

            asset->materials[material_index].colour[0] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            if(!*value) ERR();
            asset->materials[material_index].colour[1] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            if(!*value) ERR();
            asset->materials[material_index].colour[2] = strtof(value, NULL);
        }
        else if (key == KEY_FLAT_COLOUR || key == KEY_FLAT_COLOR) {
            // Unused.
        }
        else if (key == KEY_FIRST) {
            MATERIAL_CHECKS();

            long int f = strtol(value, NULL, 10);
            if(f < 0 || 
                (asset->mesh_meta.index_count && 
                    (f+asset->materials[material_index].count) > asset->mesh_meta.index_count) ||
                (!asset->mesh_meta.index_count && 
                    (f+asset->materials[material_index].count)  > asset->mesh_meta.vertex_count))
            {
                fprintf(stderr, "Invalid first vertex/index: %li\n", f);
                ERR();
            }
            asset->materials[material_index].first = (uint32_t)f;
        }
        else if (key == KEY_COUNT) {
            MATERIAL_CHECKS();

            long int c = strtol(value, NULL, 10);
            if(c < 0 || 
                (asset->mesh_meta.index_count && 
                    (asset->materials[material_index].first+c) > asset->mesh_meta.index_count) ||
                (!asset->mesh_meta.index_count && 
                    (asset->materials[material_index].first+c) > asset->mesh_meta.vertex_count))
            {
                fprintf(stderr, "Invalid number of vertices/indices: %li\n", c);
                ERR();
            }
            asset->materials[material_index].count = (uint32_t)c;
        }
        else if (key == KEY_TEXTURE) {
            MATERIAL_CHECKS();

            char * s = copy_string_to_whitespace_to_malloc(value);
            if(!s) ERR();
            asset->materials[material_index].texture = s;
        }
        else if (key == KEY_NORMAL_MAP) {
            MATERIAL_CHECKS();


            char * s = copy_string_to_whitespace_to_malloc(value);
            if(!s) ERR();
            asset->materials[material_index].normal_map_texture = s;
        }
        else if (key == KEY_WIDTH) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            long int w = strtol(value, NULL, 10);
            if(w < 0 || w > 32768) {
                fprintf(stderr, "Invalid WIDTH: %li\n", w);
                ERR();
            }
            asset->texture_meta.width = (uint32_t)w;
        }
        else if (key == KEY_HEIGHT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            long int h = strtol(value, NULL, 10);
            if(h < 0 || h > 32768) {
                fprintf(stderr, "Invalid HEIGHT: %li\n", h);
                ERR();
            }
            asset->texture_meta.height = (uint32_t)h;
        }
        else if (key == KEY_IMAGE_FORMAT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            if(word_matches(value, "RGBA") || word_matches(value, "RGB")) {
                asset->texture_meta.format = PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB;
            }
            else if(word_matches(value, "RG")) {
                asset->texture_meta.format = PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR;
            }
            else {
                fprintf(stderr, "Invalid IMAGE-FORMAT: %s\n", old_input_value);
                ERR();
            }
        }
        else if (key == KEY_MIP_MAPS) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            asset->texture_meta.has_mip_maps = true;
        }
        else if (key == KEY_BC1) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            asset->texture_meta.has_bc1 = true;
        }
        else if (key == KEY_BC3) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            asset->texture_meta.has_bc3 = true;
        }
        else if (key == KEY_BC5) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            asset->texture_meta.has_bc5 = true;
        }
        else if (key == KEY_BC7) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            asset->texture_meta.has_bc7 = true;
        }
        else if (key == KEY_BC1) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            asset->texture_meta.has_bc1 = true;
        }
        else if (key == KEY_ETC1) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            asset->texture_meta.has_etc1 = true;
        }
        else if (key == KEY_ETC2) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            asset->texture_meta.has_etc2 = true;
        }
        else if (key == KEY_ETC2_ALPHA) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            asset->texture_meta.has_etc2_alpha = true;
        }
        else {
            if(input) {
                // Invalid key

                input = old_input_value;
                while(*input && *input != ' ' && *input != '\t') input++;
                *input = 0;
                fprintf(stderr, "Unrecognised asset configuration key: %s\n", old_input_value);
                ERR();
            }
            else {
                // End of string
                break;
            }
        }
        #undef CHECK_TYPE
    }
    #undef ERR
    free(input_);

    if(asset->type == PIGEON_ASSET_TYPE_UNKNOWN) {
        fputs("Asset type unspecified\n", stderr);
        return 1;
    }

    if(asset->type == PIGEON_ASSET_TYPE_MODEL) {
        if(!asset->mesh_meta.vertex_count) {
            fputs("Missing vertex count\n", stderr);
            return 1;
        }

        if(asset->mesh_meta.index_count && !got_indices_type) {
            fputs("Missing INDICES-TYPE\n", stderr);
            return 1;
        }

        if(!got_vertex_attributes) {
            fputs("Missing VERTEX-ATTRIBUTES\n", stderr);
            return 1;
        }

        uint64_t offset = 0;
        bool contains_normalised_position = false;
        for(unsigned int i = 0; i < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; i++) {
            PigeonWGIVertexAttributeType type = asset->mesh_meta.attribute_types[i];
            if(!type) break;
            else if (type == PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED) {
                contains_normalised_position = true;
            }

            asset->mesh_meta.attribute_offsets[i] = offset;
            offset += pigeon_wgi_get_vertex_attribute_type_size(type) * asset->mesh_meta.vertex_count;
        }

        if(contains_normalised_position) {
            if(!got_bounds_range || !got_bounds_min) {
                fputs("Normalised position attribute requires BOUNDS-MINIMUM and BOUNDS-RANGE\n", stderr);
                return 1;
            }
        }
    }
    else if (asset->type == PIGEON_ASSET_TYPE_IMAGE) {
        if(!asset->texture_meta.width) {
            fputs("Missing WIDTH\n", stderr);
            return 1;
        }
        if(!asset->texture_meta.height) {
            fputs("Missing HEIGHT\n", stderr);
            return 1;
        }
        if(!asset->texture_meta.format) {
            fputs("Missing FORMAT\n", stderr);
            return 1;
        }
    }



    return 0;
}

ERROR_RETURN_TYPE pigeon_load_asset_data(PigeonAsset * asset, const char * data_file_path)
{
    ASSERT_1(asset && asset->type);

    unsigned long size;
    asset->original_data = (char*)load_file(data_file_path, 0, &size);
    uint8_t * data = asset->original_data;

    ASSERT_1(asset->original_data);

    for(unsigned int i = 0; i < sizeof asset->compression / sizeof *asset->compression; i++) {
        if(asset->compression[i].type == PIGEON_ASSET_COMPRESSION_TYPE_NONE) {
            asset->compression[i].decompressed_data = &data[asset->compression[i].original_file_data_offset];
        }
        else if(asset->compression[i].type == PIGEON_ASSET_COMPRESSION_TYPE_ZSTD) {
            asset->compression[i].compressed_data = &data[asset->compression[i].original_file_data_offset];
        }
        else {
            ASSERT_1(false);
        }
    }

    return 0;
}


ERROR_RETURN_TYPE pigeon_decompress_asset(PigeonAsset * asset, void * buffer, unsigned int i)
{
    ASSERT_1(asset);

    PigeonAssetCompression * comp = &asset->compression[i];
    

    if(comp->decompressed_data) {
        memcpy(buffer, comp->decompressed_data, comp->decompressed_data_length);
    }
    else if (comp->compressed_data && comp->type == PIGEON_ASSET_COMPRESSION_TYPE_ZSTD) {
        size_t output_bytes_count = ZSTD_decompress(
            buffer, comp->decompressed_data_length, 
            comp->compressed_data, comp->compressed_data_length);

        if(ZSTD_isError(output_bytes_count)) {
            fprintf(stderr, "ZSTD error: %s\n", ZSTD_getErrorName(output_bytes_count));
            ASSERT_1(false);
        }
    }
    else {
        ASSERT_1(false);
    }

    return 0;
}

void pigeon_free_asset(PigeonAsset * asset)
{
    assert(asset);

    if(asset->name) {
        free(asset->name);
        asset->name = NULL;
    }

    if(asset->original_data) {
        free(asset->original_data);
        asset->original_data = NULL;
    }

    for(unsigned int i = 0; i < sizeof asset->compression / sizeof *asset->compression; i++) {
        if(asset->compression[i].compressed_data_was_mallocd) {
            free(asset->compression[i].compressed_data);
            asset->compression[i].compressed_data = NULL;
        }
        if(asset->compression[i].decompressed_data_was_mallocd) {
            free(asset->compression[i].decompressed_data);
            asset->compression[i].decompressed_data = NULL;
        }
    }


    if(asset->type == PIGEON_ASSET_TYPE_MODEL) {
        if(asset->materials_count && asset->materials) {
            for(unsigned int i = 0; i < asset->materials_count; i++) {
                free(asset->materials[i].name);
                if(asset->materials[i].texture) free(asset->materials[i].texture);
                if(asset->materials[i].normal_map_texture) free(asset->materials[i].normal_map_texture);
            }

            free(asset->materials);
        }
        asset->materials = NULL;
        asset->materials_count = 0;
    }

}
