#include <stdio.h>
#include <stb_vorbis.c>

const char * sound_file_path;
const char * output_asset_file_path;
unsigned int file_size;
stb_vorbis_info info;

static int parse_arguments(int argc, const char ** argv)
{
    if(argc != 3) {
        fprintf(stderr, "Usage: %s [INPUT_FILE] [OUTPUT_FILE]\n", argv[0]);
        return 1;
    }


    sound_file_path = argv[1];
    output_asset_file_path = argv[2];
    return 0;
}


static int get_ogg_meta(void)
{
    int error = 0;
    stb_vorbis * vorbis = stb_vorbis_open_filename(sound_file_path, &error, NULL);
    if(error || !vorbis) {
        fprintf(stderr, "stb_vorbis error: %i\n", error);
        return 1;
    }
    file_size = vorbis->stream_len;
    info = stb_vorbis_get_info(vorbis);
    stb_vorbis_close(vorbis);

    return 0;
}

static int write_asset_file(void)
{
    FILE * f = fopen(output_asset_file_path, "w");
    if(!f) return 1;

    if(fprintf(f, "TYPE AUDIO\nSAMPLE-RATE %u\nCHANNELS %u\nSUBREGIONS OGG %u", 
        info.sample_rate, info.channels, file_size) <= 0) return 1;
    fclose(f);
    return 0;
}

int main(int argc, const char ** argv)
{
    if(parse_arguments(argc, argv)) return 1;
    if(get_ogg_meta()) return 1;
    if(write_asset_file()) return 1;
    return 0;
}
