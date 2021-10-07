#include <pigeon/util.h>


size_t* load_file(const char* file, unsigned int extra, unsigned long * file_size) {
	FILE* f = fopen(file, "rb");
	if (!f) {
		fprintf(stderr, "File not found: %s\n", file);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long fsize_ = ftell(f);
	rewind(f);

	if (fsize_ < 1) {
		fclose(f);
		return NULL;
	}

	size_t fsize = (size_t)fsize_;

	size_t* data = malloc(fsize + extra);
	if (!data) {
		fclose(f);
		return NULL;
	}

	size_t r = fread(data, fsize, 1, f);
	fclose(f);

	if (r != 1) {
		free(data);
		return NULL;
	}

	memset(&((char*)data)[fsize], 0, extra);
	*file_size = fsize;

	return data;

}

unsigned int round_up(unsigned int x, unsigned int round)
{
    return ((x + round-1) / round) * round;
}
