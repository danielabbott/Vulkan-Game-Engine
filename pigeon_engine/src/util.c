#include <pigeon/misc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void* pigeon_load_file(const char* file, unsigned int extra, unsigned long * file_size) {
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

	void* data = malloc(fsize + extra);
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


