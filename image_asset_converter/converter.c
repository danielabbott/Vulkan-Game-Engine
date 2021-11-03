#include <stdio.h>
#include <string.h>
#include <zstd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <config_parser.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>
#define STB_DXT_IMPLEMENTATION
#include <stb_dxt.h>

// To enable BC7, set BC7E_OBJ on the makefile command line (points to _avx.obj)
#ifdef BC7E
#include <bc7e.h>

struct bc7e_compress_block_params bc7_params;
#endif

typedef enum {
    RGB,
    RGBA,
    NormalMap
} OutputFormat;

// bool no_compressed_formats;
char * import_file_path;
const char * image_file_path;
const char * output_asset_file_path;
char * output_data_file_path;

OutputFormat output_format = RGB;
bool generate_mipmap = true;
bool use_bc1_3 = true;
bool use_bc5 = false;

#ifdef BC7E
bool use_bc7 = true;
#else
bool use_bc7 = false;
#endif



static int parse_arguments(int argc, const char ** argv)
{
    if(argc != 3) {
        fprintf(stderr, "Usage: %s [INPUT_FILE] [OUTPUT_FILE]\n", argv[0]);
        return 1;
    }


    image_file_path = argv[1];
    output_asset_file_path = argv[2];

    const size_t output_asset_file_path_length = strlen(output_asset_file_path);

    if(output_asset_file_path_length < 7 || 
            memcmp(&output_asset_file_path[output_asset_file_path_length-6], ".asset", 6) != 0) {
        fprintf(stderr, "Output file path must be a .asset file, got %s\n", output_asset_file_path);
        return 1;
    }

    output_data_file_path = malloc(output_asset_file_path_length);
    memcpy(output_data_file_path, output_asset_file_path, output_asset_file_path_length-5);

    output_data_file_path[output_asset_file_path_length-5] = 'd';
    output_data_file_path[output_asset_file_path_length-4] = 'a';
    output_data_file_path[output_asset_file_path_length-3] = 't';
    output_data_file_path[output_asset_file_path_length-2] = 'a';
    output_data_file_path[output_asset_file_path_length-1] = 0;


    const size_t image_file_path_len = strlen(image_file_path);
    import_file_path = malloc(image_file_path_len + 8);
    if(!import_file_path) return 1;
    memcpy(import_file_path, image_file_path, image_file_path_len);
    memcpy(&import_file_path[image_file_path_len], ".import", 8);
    return 0;
}


static int parse_import_file(void)
{
    FILE * f = fopen(import_file_path, "r");
    if(!f) {
        fprintf(stderr, "Error opening file: %s\nAssuming RGB format\n", import_file_path);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long sz_ = ftell(f);
    if(sz_ < 1) return 0; // Empty file, use defaults
    rewind(f);

    size_t sz = (size_t)sz_;

    char *string = malloc(sz + 1);
    if(!string || fread(string, sz, 1, f) != 1) {
        fprintf(stderr, "Error reading file: %s\n", import_file_path);
        return 1;
    }
    fclose(f);
    string[sz] = 0;

    enum {
        KEY_FORMAT,
        KEY_MIPMAP,
        KEY_NO_BC1_OR_3,
        KEY_NO_BC5,
        KEY_NO_BC7,
    };

    const char * keys[] = {
        "FORMAT",
        "CREATE-MIPMAP",
        "NO-BC1-OR-3",
        "NO-BC5",
        "NO-BC7"
    };
    

    char * input = string;
    const char * value;
    int key;
    while(input && *input) {
        key = parser_match_key_get_value(&input, keys, sizeof keys / sizeof(*keys), &value);

        if(key == KEY_FORMAT) {
            if(word_matches(value, "RGB")) {
                output_format = RGB;
                use_bc5 = false;
            }
            else if(word_matches(value, "RGBA")) {
                output_format = RGBA;
                use_bc5 = false;
            }
            else if(word_matches(value, "NORMAL-MAP")) {
                output_format = NormalMap;
                use_bc1_3 = false;
                use_bc7 = false;
                use_bc5 = true;
            }
            else {
                fputs("Unknown image format\n", stderr);
                return 1;
            }
        }
        else if(key == KEY_MIPMAP) {
            generate_mipmap = true;
        } 
        else if(key == KEY_NO_BC1_OR_3) {
            use_bc1_3 = false;
        }  
        else if(key == KEY_NO_BC5) {
            use_bc5 = false;
        }  
        else if(key == KEY_NO_BC7) {
            use_bc7 = false;
        }    
        else {
            fputs("Unrecognised image configuration key\n", stderr);
            return 1;
        }    
    }


    return 0;
}

static int load_image_file(void ** data, int * image_width, int * image_height)
{
    int image_bytes_per_channel;
    *data = stbi_load(image_file_path, image_width, image_height, &image_bytes_per_channel, 4);
    if(!*data) {
        fprintf(stderr, "Error loading image file %s: %s\n", image_file_path, stbi_failure_reason());
        return 1;
    }

    if(*image_width % 4 != 0 && *image_height % 4 != 0) {
        fputs("Image dimensions must be multiples of 4\n", stderr);
        return 1;
    }

    if(image_bytes_per_channel != 3 && image_bytes_per_channel != 4) {
        fputs( "Image must be 24-bpp or 32-bpp\n", stderr);
        printf("image_bytes_per_channel %d\n", image_bytes_per_channel);
        return 1;
    }

    if(image_bytes_per_channel == 4 && output_format == RGB && use_bc1_3) {
        // Set alpha to 1.0. This is needed for the DXT(BC1) compression

        uint8_t* image_u8 = *data;
        image_u8 += 3;

        for(unsigned int i = 0; i < (unsigned)(*image_width) * (unsigned)(*image_height); i++) {
            image_u8[i*4] = 0xff;
        }
    }

    return 0;
}

static void convert_rgba8_to_rg8(void * rgba8, void * uncompressed_data, unsigned int pixels)
{
    uint16_t * rg_u16 = (uint16_t*)uncompressed_data;
    uint16_t * rgba_u16 = rgba8;

    for(unsigned int i = 0; i < pixels; i++) {
        rg_u16[i] = rgba_u16[i*2];
    }
}

// Includes level 0 (original image)
static void get_mip_map_requirements(unsigned int width, unsigned int height, unsigned int bytes_per_4x4_block,
    bool use_mip_maps,
    unsigned int* levels, unsigned int* bytes_total)
{
    if(!use_mip_maps) {
        *levels = 1;
        *bytes_total = ((width*height)/16) * bytes_per_4x4_block;
        return;
    }

    *levels = *bytes_total = 0;
    while(width >= 4 && height >= 4) {
        *levels += 1;
        *bytes_total += ((width*height)/16) * bytes_per_4x4_block;

        width /= 2;
        height /= 2;
    }
}

static int alloc_memory_and_convert_raw(
    unsigned int image_width, unsigned int image_height,
    void ** raw_data,
    void ** uncompressed_data,
    unsigned int * uncompressed_data_size,
    void * lossy_compressed_data[2],
    unsigned int lossy_compressed_data_size[2],
    unsigned int * mip_levels
)
{
    if(output_format == NormalMap) {
        get_mip_map_requirements(image_width, image_height, 32, generate_mipmap,
            mip_levels, uncompressed_data_size);
        *lossy_compressed_data_size = use_bc5 ? *uncompressed_data_size/2 : 0;
    }
    else {
        get_mip_map_requirements(image_width, image_height, 64, generate_mipmap,
            mip_levels, uncompressed_data_size);

        lossy_compressed_data_size[0] = lossy_compressed_data_size[1] = 0;

        unsigned int i = 0;

        if(use_bc1_3) {
            if(output_format == RGBA) {
                lossy_compressed_data_size[0] = *uncompressed_data_size/4;
            }
            else {
                lossy_compressed_data_size[0] = *uncompressed_data_size/8;
            }
            i=1;
        }

        if(use_bc7) {
            lossy_compressed_data_size[i] = *uncompressed_data_size/4;
        }
    }

    if(output_format == NormalMap) {
        *uncompressed_data = malloc(*uncompressed_data_size);
        if(!*uncompressed_data) return 1;

        convert_rgba8_to_rg8(*raw_data, *uncompressed_data, image_width * image_height);
        free(*raw_data);
    }
    else {
        *uncompressed_data = realloc(*raw_data, *uncompressed_data_size);
        if(!*uncompressed_data) return 1;
    }
    *raw_data = NULL;

    for(unsigned int i = 0; i < 2; i++) {
        if(lossy_compressed_data_size[i]) {
            lossy_compressed_data[i] = malloc(lossy_compressed_data_size[i]);
            if(!lossy_compressed_data[i]) return 1;
        }
    }

    return 0;
}

// mip_width, x, y are for the dst (smaller) mip level
static void mip_downscale(uint8_t * mip_dst, uint8_t * mip_src, unsigned int mip_width,
    unsigned int x, unsigned int  y, unsigned int bytes_per_pixel)
{
    unsigned int components[4] = {0};

    for(unsigned int y_ = y*2; y_ < y*2+2; y_++) {
        for(unsigned int x_ = x*2; x_ < x*2+2; x_++) {
            uint8_t * src = &mip_src[(y_* mip_width*2 + x_)*bytes_per_pixel];
            for(unsigned int component_index = 0; component_index < bytes_per_pixel; component_index++) {
                components[component_index] += src[component_index];
            }
        }
    }

    uint8_t * dst = &mip_dst[(y*mip_width + x)*bytes_per_pixel];
    dst[0] = (uint8_t)(components[0]/4);
    dst[1] = (uint8_t)(components[1]/4);

    if(bytes_per_pixel > 2) {
        dst[2] = (uint8_t)(components[2]/4);
        dst[3] = (uint8_t)(components[3]/4);
    }
}

static void generate_uncompressed_mipmap(unsigned int image_width, unsigned int image_height,
    void * uncompressed_data)
{
    unsigned int mip_width = image_width, mip_height = image_height;
    uint8_t * mip_dst = (uint8_t*)uncompressed_data;
    uint8_t * mip_src = (uint8_t*)uncompressed_data;
    unsigned int bytes_per_pixel = output_format == NormalMap ? 2 : 4;

    if(generate_mipmap) {
        while(mip_width >= 8 && mip_height >= 8) {
            mip_src = mip_dst;
            mip_dst = &mip_dst[mip_width*mip_height*bytes_per_pixel];
            mip_width /= 2;
            mip_height /= 2;


            // Downscale
            for(unsigned int y = 0; y < mip_height; y++) {
                for(unsigned int x = 0; x < mip_width; x++) {
                    mip_downscale(mip_dst, mip_src, mip_width, x, y, bytes_per_pixel);
                }
            }
        }
    }
}

static void generate_lossy_compressed_mip_non_bc7(void * uncompressed_data, void * lossy_compressed_data,
    unsigned int width, unsigned int height)
{
    unsigned int bytes_per_pixel = output_format == NormalMap ? 2 : 4;
    unsigned int bytes_per_compressed_4x4_block = output_format == RGB ? 8 : 16;

    uint8_t* dst = (uint8_t *)lossy_compressed_data;
    uint8_t* src = (uint8_t *)uncompressed_data;

    for(unsigned int y = 0; y < height; y += 4) {
        for(unsigned int x = 0; x < width; x += 4) {
            // Gather pixel data
            uint8_t block_data[64]; // 64 = maximum size
            memcpy(&block_data[0], &src[(y*width + x) * bytes_per_pixel], bytes_per_pixel*4);
            memcpy(&block_data[bytes_per_pixel*4], &src[((y+1)*width + x) * bytes_per_pixel], bytes_per_pixel*4);
            memcpy(&block_data[bytes_per_pixel*4*2], &src[((y+2)*width + x) * bytes_per_pixel], bytes_per_pixel*4);
            memcpy(&block_data[bytes_per_pixel*4*3], &src[((y+3)*width + x) * bytes_per_pixel], bytes_per_pixel*4);

            // Compress

            if(output_format == NormalMap) {
                stb_compress_bc5_block(dst, block_data);
            }
            else {
                stb_compress_dxt_block(dst, block_data, output_format == RGBA ? 1 : 0, STB_DXT_HIGHQUAL);
            }

            dst += bytes_per_compressed_4x4_block;
        }
    }
}


static int generate_lossy_compressed(void * uncompressed_data_, void * lossy_compressed_data_[2],
    unsigned int width_, unsigned int height_)
{
    void * lossy_compressed_data[2];
    lossy_compressed_data[0] = lossy_compressed_data_[0];
    lossy_compressed_data[1] = lossy_compressed_data_[1];

    for(unsigned int i = 0; i < 2; i++) {
        unsigned int width = width_, height = height_;
        void * uncompressed_data = uncompressed_data_;

        unsigned int bytes_per_pixel = output_format == NormalMap ? 2 : 4;
        unsigned int bytes_per_compressed_4x4_block;
        bool is_bc7 = false;

        if(!i) {
            if(output_format == NormalMap) {
                if(!use_bc5) break;
                bytes_per_compressed_4x4_block = 16;
            }
            else {
                if(!use_bc1_3 && !use_bc7) break;
                if(use_bc1_3) {
                    bytes_per_compressed_4x4_block = output_format == RGB ? 8 : 16;
                }
                else {
                    is_bc7 = true;
                    bytes_per_compressed_4x4_block = 16;
                }
            }
        }
        else {
            if(!use_bc7 || !use_bc1_3) break;
            is_bc7 = true;
            bytes_per_compressed_4x4_block = 16;
        }

        if(!lossy_compressed_data[i]) continue;

        void * bc7_block_data = NULL;
        if(is_bc7) {
            bc7_block_data = malloc(width * height * 4);
            if(!bc7_block_data) return 1;
        }

        while(width >= 4 && height >= 4) {
            if(is_bc7) {
#ifdef BC7E
                unsigned int blocks = (width * height) / 16;
                // Copy image data into appropriate format

                uint32_t * block_data = bc7_block_data;
                const uint32_t * src = uncompressed_data;

                for(unsigned int y = 0; y < height; y += 4) {
                    for(unsigned int x = 0; x < width; x += 4) {
                        // Gather pixel data
                        memcpy(&block_data[0], &src[(y*width + x)], 4*4);
                        memcpy(&block_data[4], &src[((y+1)*width + x)], 4*4);
                        memcpy(&block_data[4*2], &src[((y+2)*width + x)], 4*4);
                        memcpy(&block_data[4*3], &src[((y+3)*width + x)], 4*4);

                        block_data += 16;
                    }
                }
                
                bc7e_compress_blocks_avx(blocks, (uint64_t *)lossy_compressed_data[i],
                    (const uint32_t *) bc7_block_data, &bc7_params);
#endif
            }
            else {
                generate_lossy_compressed_mip_non_bc7(uncompressed_data, lossy_compressed_data[i], width, height);
            }

            uncompressed_data = (void *)((uintptr_t)uncompressed_data + width*height*bytes_per_pixel);
            lossy_compressed_data[i] = (void *)((uintptr_t)lossy_compressed_data[i] + 
                ((width*height)/16)*bytes_per_compressed_4x4_block);
            width /= 2;
            height /= 2;
        }
        if(is_bc7) free(bc7_block_data);
    }
    return 0;
}

static int do_zstd_compress(FILE * f, void * data_in, unsigned int data_in_size, bool * zstd,
    unsigned int * zstd_compressed_length)
{
    size_t zstd_max_length = ZSTD_compressBound(data_in_size);
    void * zstd_compressed_data = malloc(zstd_max_length);
    if(!zstd_compressed_data) return 1;

    size_t output_bytes = ZSTD_compress(zstd_compressed_data, zstd_max_length,
        data_in, data_in_size, 3);

    void * to_write = data_in;
    unsigned int to_write_size = data_in_size;

    if(ZSTD_isError(output_bytes) || output_bytes*10 >= data_in_size*9  || output_bytes > UINT32_MAX) {
        fputs("Compression failure, outputting uncompressed data\n", stderr);
        free(zstd_compressed_data);
        zstd_compressed_data = NULL;
        *zstd = false;
        *zstd_compressed_length = 0;
    }
    else {
        to_write = zstd_compressed_data;
        to_write_size = (uint32_t)output_bytes;
        *zstd = true;
        *zstd_compressed_length = (uint32_t)output_bytes;
    }

    size_t written = fwrite(to_write, to_write_size, 1, f);
    if(written != 1) {
        fputs("Error writing to file\n", stderr);
        return 1;
    }

    return 0;   
}

static int save_files(void * uncompressed_data, unsigned int uncompressed_data_length,
        void * lossy_compressed_data[2], unsigned int lossy_compressed_data_size_bytes[2],
    unsigned int image_width, unsigned int image_height)
{
    FILE * data_file = fopen(output_data_file_path, "wb");
    FILE * asset_file = fopen(output_asset_file_path, "w");
    if(!data_file) {
        fprintf(stderr, "Error opening output data file: %s\n", output_data_file_path);
        return 1;
    }
    if(!asset_file) {
        fprintf(stderr, "Error opening output asset file: %s\n", output_asset_file_path);
        return 1;
    }

    bool uncompressed_uses_zstd = false;
    bool lossy_compressed_uses_zstd[2] = {0};

    unsigned int uncompressed_data_length_zstd_length=0;
    unsigned int lossy_compressed_data_length_zstd_length[2] = {0};

    if(do_zstd_compress(data_file, uncompressed_data, uncompressed_data_length, &uncompressed_uses_zstd,
        &uncompressed_data_length_zstd_length)) return 1;
    
    unsigned int subr_count = 1;
    for(unsigned int i = 0; i < 2; i++) {
        if(lossy_compressed_data_size_bytes[i]) {
            if(do_zstd_compress(data_file, lossy_compressed_data[i], lossy_compressed_data_size_bytes[i], 
                &lossy_compressed_uses_zstd[i], &lossy_compressed_data_length_zstd_length[i])) return 1;
            subr_count++;
        }
        else break;
    }


    fprintf(asset_file, 
        "#ASSET %s\nTYPE IMAGE\nWIDTH %d\nHEIGHT %d\nIMAGE-FORMAT %s\n",
        image_file_path,
        image_width,
        image_height,
        output_format == NormalMap ? "RG" : (output_format == RGB ? "RGB" : "RGBA")
    );

    fprintf(asset_file, "SUBRESOURCE-COUNT %u\n", subr_count);

    fputs("SUBRESOURCES ", asset_file);
    if(uncompressed_uses_zstd) {
        fprintf(asset_file, "ZSTD %u>%u", uncompressed_data_length, uncompressed_data_length_zstd_length);
    }
    else {
        fprintf(asset_file, "NONE %u", uncompressed_data_length);
    }
    for(unsigned int i = 0; i < 2; i++) {
        if(!lossy_compressed_data_size_bytes[i]) break;

        if(lossy_compressed_uses_zstd[i]) {
            fprintf(asset_file, " ZSTD %u>%u", lossy_compressed_data_size_bytes[i], 
                lossy_compressed_data_length_zstd_length[i]);
        }
        else {
            fprintf(asset_file, " NONE %u", lossy_compressed_data_size_bytes[i]);
        }
    }
    fputc('\n', asset_file);

    if(generate_mipmap) {
        fputs("MIP-MAPS\n", asset_file);
    }

    if(use_bc5) fputs("BC5\n", asset_file);
    if(use_bc1_3) {
        if (output_format == RGBA) 
            fputs("BC3\n", asset_file);
        else 
            fputs("BC1\n", asset_file);
    }
    if(use_bc7) fputs("BC7\n", asset_file);

    fputc('\n', asset_file);

    fclose(data_file);
    fclose(asset_file);

    return 0;
}


int main(int argc, const char ** argv)
{
    if(parse_arguments(argc, argv)) return 1;
    if(parse_import_file()) return 1;

    #ifdef BC7E
    if(use_bc7) {
        bc7e_compress_block_init_avx();
        if(output_format == RGBA)
            bc7e_compress_block_params_init_veryfast_avx(&bc7_params, true);
        else
            bc7e_compress_block_params_init_ultrafast_avx(&bc7_params, true);
    }
    #endif

    unsigned int image_width, image_height;
    void * raw_data;

    if(load_image_file(&raw_data, (int*)&image_width, (int*)&image_height)) return 1;

    void * uncompressed_data = NULL;
    unsigned int uncompressed_data_size = 0;

    void * lossy_compressed_data[2] = {0};
    unsigned int lossy_compressed_data_size_bytes[2] = {0};

    unsigned int mip_levels = 0;

    if(alloc_memory_and_convert_raw(image_width, image_height, &raw_data,
        &uncompressed_data, &uncompressed_data_size,
        lossy_compressed_data, lossy_compressed_data_size_bytes, &mip_levels)) return 1;
    
    raw_data = NULL;


    generate_uncompressed_mipmap(image_width, image_height, uncompressed_data);


    if(generate_lossy_compressed(uncompressed_data, lossy_compressed_data, image_width, image_height)) return 1;


    if(save_files(uncompressed_data, uncompressed_data_size, 
        lossy_compressed_data, lossy_compressed_data_size_bytes,
        image_width, image_height)) 
    {
        remove(output_asset_file_path);
        remove(output_data_file_path);

        return 1;
    }
    return 0;
}
