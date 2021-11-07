#include <pigeon/asset.h>
#include <pigeon/assert.h>
#include <pigeon/misc.h>
#include <config_parser.h>
#include <zstd.h>
#include <stb_vorbis.c>

enum {
    KEY_ASSET,
    KEY_TYPE,
    KEY_DECOMPRESSED_SIZE,
    KEY_SUBRESOURCE_COUNT,
    KEY_SUBRESOURCES,

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
    KEY_SPECULAR,
    KEY_BONES_COUNT,
    KEY_BONE,
    KEY_HEAD,
    KEY_TAIL,
    KEY_PARENT,
    KEY_FRAME_RATE,
    KEY_ANIMATIONS_COUNT,
    KEY_ANIMATION,
    KEY_LOOPS,
    KEY_FRAMES,

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

    KEY_SAMPLE_RATE,
    KEY_CHANNELS,

    KEY__MAX
};

const char * keys[] = {
    "ASSET",
    "TYPE",
    "DECOMPRESSED-SIZE",
    "SUBRESOURCE-COUNT",
    "SUBRESOURCES",

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
    "SPECULAR",
    "BONES-COUNT",
    "BONE",
    "HEAD",
    "TAIL",
    "PARENT",
    "FRAME-RATE",
    "ANIMATIONS-COUNT",
    "ANIMATION",
    "LOOPS",
    "FRAMES",

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

    "SAMPLE-RATE",
    "CHANNELS",
};

static PIGEON_ERR_RET copy_asset_name(PigeonAsset * asset, const char * meta_file_path)
{
    size_t l = strlen(meta_file_path);
    ASSERT_R1(l > 7 && l < 1000);
    ASSERT_R1(memcmp(&meta_file_path[l-6], ".asset", 6) == 0);

    int i = (int)l-6-1;
    while(i && meta_file_path[i] != '/') {
        i--;
    }
    i++;

    asset->name = malloc(l-(unsigned)i-6+1);
    ASSERT_R1(asset->name);

    memcpy(asset->name, &meta_file_path[i], l-(unsigned)i-6);
    
    asset->name[l-(unsigned)i-6] = 0;

    return 0;
}

PIGEON_ERR_RET pigeon_load_asset_meta(PigeonAsset * asset, const char * meta_file_path)
{
    ASSERT_R1(asset);

    ASSERT_R1(!copy_asset_name(asset, meta_file_path));

    unsigned long file_size;
    char* input = (char*)pigeon_load_file(meta_file_path, 1, &file_size);
    ASSERT_R1(input);
    if(!input) {
        free(asset->name);
        asset->name = NULL;
        ASSERT_R1(false);
    }

    char * input_ = input;

    ASSERT_R1(KEY__MAX*sizeof(*keys) == sizeof keys);

    #define CLEANUP() pigeon_free_asset(asset);

    bool got_indices_type = false;
    bool got_vertex_attributes = false;
    bool got_bounds_min = false;
    bool got_bounds_range = false;

    int material_index = -1;
    int bone_index = -1;
    int animation_index = -1;

    float fps = 0;

    const char * value;
    int key;
    while(input && *input) {
        char * old_input_value = input;
        key = parser_match_key_get_value(&input, keys, KEY__MAX, &value);

        #define CHECK_TYPE(a_type) if(asset->type != a_type){ \
            fprintf(stderr, "%s not valid for asset type\n", keys[key]); ASSERT_R1(false);}

        if (key == KEY_TYPE) {
            if(word_matches(value, "MODEL")) {
                asset->type = PIGEON_ASSET_TYPE_MODEL;
            }
            else if(word_matches(value, "IMAGE")) {
                asset->type = PIGEON_ASSET_TYPE_IMAGE;
            }
            else if(word_matches(value, "AUDIO")) {
                asset->type = PIGEON_ASSET_TYPE_AUDIO;
            }
            else if(word_matches(value, "NONE") || word_matches(value, "TEXT")
                || word_matches(value, "BINARY"))
            {
                asset->type = PIGEON_ASSET_TYPE_NONE;
            }
            else {
                fprintf(stderr, "Unrecognised asset type: %s\n", value);
                ASSERT_R1(false);
            }
        }
        else if (key == KEY_SUBRESOURCE_COUNT) {
            long int c = strtol(value, NULL, 10);
            if(c < 0 || c > 10000) {
                fprintf(stderr, "Invalid SUBRESOURCE-COUNT: %li\n", c);
                ASSERT_R1(false);
            }

            asset->subresource_count = (unsigned int) c;
            asset->subresources = calloc((unsigned) c, sizeof *asset->subresources);
            ASSERT_R1(asset->subresources);
        }
        else if (key == KEY_SUBRESOURCES) {
            uint32_t file_offset = 0;
            for(unsigned int i = 0; i < asset->subresource_count && *value; i++) {
                if(word_matches(value, "ZSTD")) {
                    asset->subresources[i].type = PIGEON_ASSET_SUBRESOURCE_TYPE_ZSTD;
                }
                else if(word_matches(value, "NONE")) {
                    asset->subresources[i].type = PIGEON_ASSET_SUBRESOURCE_TYPE_UNCOMPRESSED;
                }
                else if(word_matches(value, "OGG")) {
                    asset->subresources[i].type = PIGEON_ASSET_SUBRESOURCE_TYPE_OGG_FILE;
                }
                else {
                    fputs("Unknown subresource type\n", stderr);
                    ASSERT_R1(false);
                }
                value += 4;

                #define ERR() ASSERT_LOG_R1(false, "Invalid COMPRESSION value(s)");

                while(*value == ' ' || *value == '\t') value++;
                if(!*value) ERR();

                asset->subresources[i].original_file_data_offset = file_offset;

                long int sz_ = strtol(value, (char**)&value, 10);
                if(sz_ <= 0 || sz_ > UINT32_MAX) ERR();
                uint32_t sz = (uint32_t)sz_;


                if(asset->subresources[i].type == PIGEON_ASSET_SUBRESOURCE_TYPE_UNCOMPRESSED) {
                    asset->subresources[i].decompressed_data_length = sz;
                }
                else if(asset->subresources[i].type == PIGEON_ASSET_SUBRESOURCE_TYPE_OGG_FILE) {
                    asset->subresources[i].compressed_data_length = sz;
                }
                else {
                    asset->subresources[i].decompressed_data_length = sz;
                    if(*value != '>') ERR();
                    value++;

                    sz_ = strtol(value, NULL, 10);
                    if(sz_ <= 0 || sz_ > UINT32_MAX) ERR();
                    sz = (uint32_t)sz_;

                    asset->subresources[i].compressed_data_length = sz;
                }
                file_offset += sz;
                #undef ERR
                
                
                while(*value && *value != ' ' && *value != '\t') value++;
                while(*value == ' ' || *value == '\t') value++;
                if(!*value) break;
            }
        }
        else if (key == KEY_VERTEX_COUNT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);
            
            long int c = strtol(value, NULL, 10);
            if(c < 0 || c > UINT32_MAX) {
                fprintf(stderr, "Invalid VERTEX-COUNT: %li\n", c);
                ASSERT_R1(false);
            }
            asset->mesh_meta.vertex_count = (uint32_t)c;
        }
        else if (key == KEY_INDICES_COUNT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);

            long int c = strtol(value, NULL, 10);
            if(c < 0 || c > UINT32_MAX) {
                fprintf(stderr, "Invalid INDICES-COUNT: %li\n", c);
                ASSERT_R1(false);
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
                ASSERT_R1(false);
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
                else if(word_matches(value, "POSITION-NORMALIZED")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED;
                else if(word_matches(value, "COLOUR")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR;
                else if(word_matches(value, "COLOUR-RGBA8")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR_RGBA8;
                else if(word_matches(value, "COLOR")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR;
                else if(word_matches(value, "COLOR-RGBA8")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR_RGBA8;
                else if(word_matches(value, "NORMAL")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_NORMAL;
                else if(word_matches(value, "TANGENT")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_TANGENT;
                else if(word_matches(value, "UV")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_UV;
                else if(word_matches(value, "UV-FLOAT")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_UV_FLOAT;
                else if(word_matches(value, "BONE")) type = PIGEON_WGI_VERTEX_ATTRIBUTE_BONE;
                else {
                    fprintf(stderr, "Unrecognised vertex attribute\n");
                    ASSERT_R1(false);
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
            ASSERT_R1(*value);
            asset->mesh_meta.bounds_min[1] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            ASSERT_R1(*value);
            asset->mesh_meta.bounds_min[2] = strtof(value, (char**)&value);
        }
        else if (key == KEY_BOUNDS_RANGE) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);
            got_bounds_range = true;

            asset->mesh_meta.bounds_range[0] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            ASSERT_R1(*value);
            asset->mesh_meta.bounds_range[1] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            ASSERT_R1(*value);
            asset->mesh_meta.bounds_range[2] = strtof(value, NULL);

            ASSERT_LOG_R1(asset->mesh_meta.bounds_range[0] > 0.0f &&
                asset->mesh_meta.bounds_range[1] > 0.0f &&
                asset->mesh_meta.bounds_range[2] > 0.0f, "Invalid vertex position range");
        }
        else if (key == KEY_MATERIALS_COUNT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);

            ASSERT_LOG_R1(!asset->materials_count, "Multiple MATERIALS-COUNT");

            long int c = strtol(value, NULL, 10);
            if(c < 0 || c > 999) {
                fprintf(stderr, "Invalid MATERIALS-COUNT: %li\n", c);
                ASSERT_R1(false);
            }
            asset->materials_count = (unsigned int)c;
            asset->materials = calloc((unsigned)c, sizeof *asset->materials);
            material_index = -1;
        }
        else if (key == KEY_MATERIAL) {
            ++material_index;
            #define MATERIAL_CHECKS() \
                CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL); \
                ASSERT_LOG_R1(asset->materials_count, "Missing MATERIALS-COUNT"); \
                ASSERT_LOG_R1((unsigned)material_index < asset->materials_count, "Too many materials");
            MATERIAL_CHECKS();

            char * name = copy_string_to_whitespace_to_malloc(value);
            ASSERT_R1(name);
            asset->materials[material_index].name = name;
        }
        else if (key == KEY_COLOUR || key == KEY_COLOR) {
            MATERIAL_CHECKS();

            asset->materials[material_index].colour[0] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            ASSERT_R1(*value);
            asset->materials[material_index].colour[1] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            ASSERT_R1(*value);
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
                ASSERT_R1(false);
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
                ASSERT_R1(false);
            }
            asset->materials[material_index].count = (uint32_t)c;
        }
        else if (key == KEY_TEXTURE) {
            MATERIAL_CHECKS();

            char * s = copy_string_to_whitespace_to_malloc(value);
            ASSERT_R1(*s);
            asset->materials[material_index].texture = s;
        }
        else if (key == KEY_NORMAL_MAP) {
            MATERIAL_CHECKS();

            char * s = copy_string_to_whitespace_to_malloc(value);
            ASSERT_R1(*s);
            asset->materials[material_index].normal_map_texture = s;
        }
        else if (key == KEY_SPECULAR) {
            MATERIAL_CHECKS();
            asset->materials[material_index].specular = strtof(value, (char**)&value);
        }
        #undef MATERIAL_CHECKS

        else if (key == KEY_BONES_COUNT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL); 

            long int c = strtol(value, NULL, 10);
            if(c < 0 || c > 256)
            {
                fprintf(stderr, "Invalid number of bones: %li\n", c);
                ASSERT_R1(false);
            }
            asset->bones_count = (unsigned)c;
            asset->bones = calloc((unsigned)c, sizeof *asset->bones);
            bone_index = -1;
        }
        else if (key == KEY_BONE) {
            ++bone_index;
            #define BONE_CHECKS() \
                CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL); \
                ASSERT_LOG_R1(asset->bones_count, "Missing BONES-COUNT"); \
                ASSERT_LOG_R1((unsigned)bone_index < asset->bones_count, "Too many bones");
            BONE_CHECKS();

            // char * name = copy_string_to_whitespace_to_malloc(value);
            // if(!name) ERR();
            // asset->bones[bone_index].name = name;
            asset->bones[bone_index].parent_index = -1;
        }
        else if (key == KEY_HEAD) {
            BONE_CHECKS();

            asset->bones[bone_index].head[0] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            ASSERT_R1(*value);
            asset->bones[bone_index].head[1] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            ASSERT_R1(*value);
            asset->bones[bone_index].head[2] = strtof(value, NULL);
        }
        else if (key == KEY_TAIL) {
            BONE_CHECKS();

            asset->bones[bone_index].tail[0] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            ASSERT_R1(*value);
            asset->bones[bone_index].tail[1] = strtof(value, (char**)&value);
            while(*value == ' ' || *value == '\t') value++;
            ASSERT_R1(*value);
            asset->bones[bone_index].tail[2] = strtof(value, NULL);
        }
        else if (key == KEY_PARENT) {
            BONE_CHECKS();

            long int p = strtol(value, NULL, 10);
            if(p >= asset->bones_count) {
                fprintf(stderr, "Invalid bone parent index: %li\n", p);
                ASSERT_R1(false);
            }
            if(p < 0 || p == bone_index) p = -1;

            asset->bones[bone_index].parent_index = (int)p;
        }
        #undef BONE_CHECKS
        else if (key == KEY_FRAME_RATE) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL);

            fps = strtof(value, NULL);
            if(fps < 0.001f || fps > 1000.0f)
            {
                fprintf(stderr, "Invalid frame-rate: %f\n", fps);
                ASSERT_R1(false);
            }
        }
        else if (key == KEY_ANIMATIONS_COUNT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL); 

            long int c = strtol(value, NULL, 10);
            if(c < 0 || c > 9999)
            {
                fprintf(stderr, "Invalid number of animations: %li\n", c);
                ASSERT_R1(false);
            }
            asset->animations_count = (unsigned)c;
            asset->animations = calloc((unsigned)c, sizeof *asset->animations);
            animation_index = -1;
        }
        else if (key == KEY_ANIMATION) {
            ++animation_index;
            #define ANIMATION_CHECKS() \
                CHECK_TYPE(PIGEON_ASSET_TYPE_MODEL); \
                ASSERT_LOG_R1(asset->animations_count, "Missing ANIMATIONS-COUNT"); \
                ASSERT_LOG_R1((unsigned)animation_index < asset->animations_count, "Too many animations");
            ANIMATION_CHECKS();

            char * name = copy_string_to_whitespace_to_malloc(value);
            ASSERT_R1(name);
            asset->animations[animation_index].name = name;
            asset->animations[animation_index].fps = fps;
        }
        else if (key == KEY_LOOPS) {
            ANIMATION_CHECKS();

            if(word_matches(value, "YES")) {
                asset->animations[animation_index].loops = true;
            }
            else if(word_matches(value, "NO")) {
                asset->animations[animation_index].loops = false;
            }
            else {
                ASSERT_R1(false);
            }
        }
        else if (key == KEY_FRAMES) {
            ANIMATION_CHECKS();

            long int f = strtol(value, NULL, 10);
            if(f <= 0 || f > 1000000)
            {
                fprintf(stderr, "Invalid number of frames: %li\n", f);
                ASSERT_R1(false);
            }
            asset->animations[animation_index].frame_count = (unsigned)f;
        }
        #undef ANIMATION_CHECKS
        


        else if (key == KEY_WIDTH) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            long int w = strtol(value, NULL, 10);
            if(w < 0 || w > 32768) {
                fprintf(stderr, "Invalid WIDTH: %li\n", w);
                ASSERT_R1(false);
            }
            asset->texture_meta.width = (uint32_t)w;
        }
        else if (key == KEY_HEIGHT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            long int h = strtol(value, NULL, 10);
            if(h < 0 || h > 32768) {
                fprintf(stderr, "Invalid HEIGHT: %li\n", h);
                ASSERT_R1(false);
            }
            asset->texture_meta.height = (uint32_t)h;
        }
        else if (key == KEY_IMAGE_FORMAT) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_IMAGE);
            if(word_matches(value, "RGBA")) {
                asset->texture_meta.format = PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB;
                asset->texture_meta.has_alpha = true;
            }
            else if(word_matches(value, "RGB")) {
                asset->texture_meta.format = PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB;
            }
            else if(word_matches(value, "RG")) {
                asset->texture_meta.format = PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR;
            }
            else {
                fprintf(stderr, "Invalid IMAGE-FORMAT: %s\n", value);
                ASSERT_R1(false);
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
        else if (key == KEY_SAMPLE_RATE) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_AUDIO);
            long int s = strtol(value, NULL, 10);
            if(s <= 0 || s > 192000) {
                fprintf(stderr, "Invalid SAMPLE-RATE: %li\n", s);
                ASSERT_R1(false);
            }
            asset->audio_meta.sample_rate = (unsigned int)s;
        }
        else if (key == KEY_CHANNELS) {
            CHECK_TYPE(PIGEON_ASSET_TYPE_AUDIO);
            long int c = strtol(value, NULL, 10);
            if(c != 1 && c != 2) {
                fprintf(stderr, "Invalid CHANNELS: %li\n", c);
                ASSERT_R1(false);
            }
            asset->audio_meta.channels = (unsigned int)c;
        }

        else {
            if(input) {
                // Invalid key

                input = old_input_value;
                while(*input && *input != ' ' && *input != '\t') input++;
                *input = 0;
                fprintf(stderr, "Unrecognised asset configuration key: %s\n", old_input_value);
                ASSERT_R1(false);
            }
            else {
                // End of string
                break;
            }
        }
        #undef CHECK_TYPE
    }
    free(input_);

    ASSERT_LOG_R1(asset->type, "Asset type unspecified");

    if(asset->type == PIGEON_ASSET_TYPE_MODEL) {
        ASSERT_R1(!asset->animations_count || (unsigned)(animation_index+1) == asset->animations_count);
        ASSERT_R1(!asset->bones_count || (unsigned)(bone_index+1) == asset->bones_count);
        ASSERT_R1(!asset->materials_count || (unsigned)(material_index+1) == asset->materials_count);

        ASSERT_LOG_R1(!asset->mesh_meta.index_count || got_indices_type, "Missing INDICES-TYPE");
        ASSERT_LOG_R1(!asset->mesh_meta.vertex_count || got_vertex_attributes, "Missing VERTEX-ATTRIBUTES");


        uint64_t offset = 0;
        bool contains_normalised_position = false;
        for(unsigned int i = 0; i < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; i++) {
            PigeonWGIVertexAttributeType type = asset->mesh_meta.attribute_types[i];
            if(!type) break;
            else if (type == PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED) {
                contains_normalised_position = true;
            }

            offset += pigeon_wgi_get_vertex_attribute_type_size(type) * asset->mesh_meta.vertex_count;
        }

        if(contains_normalised_position) {
            ASSERT_LOG_R1(got_bounds_range && got_bounds_min, 
                "Normalised position attribute requires BOUNDS-MINIMUM and BOUNDS-RANGE");
        }

    }
    else if (asset->type == PIGEON_ASSET_TYPE_IMAGE) {
        ASSERT_LOG_R1(asset->texture_meta.width, "Missing WIDTH");
        ASSERT_LOG_R1(asset->texture_meta.height, "Missing HEIGHT");
        ASSERT_LOG_R1(asset->texture_meta.format, "Missing FORMAT");
    }

#undef CLEANUP

    return 0;
}

PIGEON_ERR_RET pigeon_load_asset_data(PigeonAsset * asset, const char * data_file_path)
{
    ASSERT_R1(asset && asset->type);

    unsigned long size;
    asset->original_data = (char*)pigeon_load_file(data_file_path, 0, &size);
    uint8_t * data = asset->original_data;

    ASSERT_R1(asset->original_data);

    for(unsigned int i = 0; i < asset->subresource_count; i++) {
        switch(asset->subresources[i].type) {
            case PIGEON_ASSET_SUBRESOURCE_TYPE_UNCOMPRESSED:
                asset->subresources[i].decompressed_data = &data[asset->subresources[i].original_file_data_offset];
                break;
            case PIGEON_ASSET_SUBRESOURCE_TYPE_ZSTD:
            case PIGEON_ASSET_SUBRESOURCE_TYPE_OGG_FILE:
                asset->subresources[i].compressed_data = &data[asset->subresources[i].original_file_data_offset];
                break;
        }
    }

    return 0;
}


PIGEON_ERR_RET pigeon_decompress_asset(PigeonAsset * asset, void * buffer, unsigned int i)
{
    ASSERT_R1(asset && i < asset->subresource_count);

    PigeonAssetSubresource * subr = &asset->subresources[i];

    if(buffer && subr->decompressed_data) {
        memcpy(buffer, subr->decompressed_data, subr->decompressed_data_length);
        return 0;
    }

    if(subr->type == PIGEON_ASSET_SUBRESOURCE_TYPE_OGG_FILE) {
        ASSERT_R1(!buffer);

        int channels, sample_rate;
        short * audio_data;
        int sample_count = stb_vorbis_decode_memory(subr->compressed_data, (int)subr->compressed_data_length, 
            &channels, &sample_rate, &audio_data);
        ASSERT_R1(sample_count && channels >= 1 && channels <= 2);

        asset->audio_meta.channels = (unsigned int) channels;
        asset->audio_meta.sample_rate = (unsigned int) sample_rate;
        subr->decompressed_data = audio_data;
        subr->decompressed_data_length = (unsigned)sample_count * (unsigned)channels * 2;
        subr->decompressed_data_was_mallocd = true;

        return 0;
    }

    if(!buffer) {
        if(subr->decompressed_data) return 0;

        subr->decompressed_data = malloc(subr->decompressed_data_length);
        subr->decompressed_data_was_mallocd = true;

        buffer = subr->decompressed_data;
    }

    ASSERT_R1(subr->compressed_data);
    

    if (subr->type == PIGEON_ASSET_SUBRESOURCE_TYPE_ZSTD) {
        size_t output_bytes_count = ZSTD_decompress(
            buffer, subr->decompressed_data_length, 
            subr->compressed_data, subr->compressed_data_length);

        if(ZSTD_isError(output_bytes_count)) {
            fprintf(stderr, "ZSTD error: %s\n", ZSTD_getErrorName(output_bytes_count));
            ASSERT_R1(false);
        }
    }
    else {
        ASSERT_R1(false);
    }

    return 0;
}

void pigeon_free_asset(PigeonAsset * asset)
{
    assert(asset);


    free_if(asset->name);
    free_if(asset->original_data);

    if(asset->subresources) {
        for(unsigned int i = 0; i < asset->subresource_count; i++) {
            if(asset->subresources[i].compressed_data_was_mallocd) {
                free2(asset->subresources[i].compressed_data);
            }
            if(asset->subresources[i].decompressed_data_was_mallocd) {
                free2(asset->subresources[i].decompressed_data);
            }
        }
        free2(asset->subresources);
    }


    if(asset->type == PIGEON_ASSET_TYPE_MODEL) {
        if(asset->materials_count && asset->materials) {
            for(unsigned int i = 0; i < asset->materials_count; i++) {
                free_if(asset->materials[i].name);
                free_if(asset->materials[i].texture);
                free_if(asset->materials[i].normal_map_texture);
            }

            free2(asset->materials);
        }
        asset->materials_count = 0;


        if(asset->bones_count && asset->bones) {
            for(unsigned int i = 0; i < asset->bones_count; i++) {
                free_if(asset->bones[i].name);
            }

            free2(asset->bones);
        }
        asset->bones_count = 0;


        if(asset->animations_count && asset->animations) {
            for(unsigned int i = 0; i < asset->animations_count; i++) {
                free_if(asset->animations[i].name);
            }

            free2(asset->animations);
        }
        asset->animations_count = 0;
    }

}
