# Example build command:
# clear && make CC=clang DEBUG=1 -s -j`nproc` && build/debug/test1



# -- GENERAL CONFIG --

# Run with make DEBUG=1 to enable debugging and disable optimisations
DEBUG ?= 0

ifeq ($(DEBUG), 1)
	BUILD_DIR = build/debug
else
	BUILD_DIR = build/release
endif


# -- C CONFIG --

# Run with make CC=clang to force usage of clang
CC ?= clang

# If 1 then liburing must be available
ENABLE_IO_URING ?= 0

CFLAGS = -std=gnu11 -MMD -Wall -Wextra \
-Wshadow -Wno-missing-field-initializers -Werror=implicit-function-declaration \
-Wmissing-prototypes -Wimplicit-fallthrough \
-Wunused-macros -Wcast-align -Werror=incompatible-pointer-types \
-Wformat-security -Wundef -Werror=unused-result \
-Werror=int-conversion \
-Iconfig_parser -fstack-protector -I pigeon_engine/include -isystem deps


ifeq ($(CC), clang)
CFLAGS += -Wshorten-64-to-32 -Wconditional-uninitialized -Wimplicit-int-conversion \
-Wimplicit-float-conversion -Wimplicit-int-float-conversion -Wno-newline-eof -Wconversion
else
CFLAGS += -Wno-sign-conversion
endif

ifeq ($(ENABLE_IO_URING), 1)
CFLAGS += -DENABLE_IO_URING
endif

LDFLAGS = -lcrypto -lssl -lopenal -lglfw -lzstd -lvulkan -lX11 -lXi -lpthread -ldl -lm -lc -fuse-ld=lld

ifeq ($(ENABLE_IO_URING), 1)
LDFLAGS := $(LDFLAGS) -luring
endif



# -- C BUILD-MODE-SPECIFIC OPTIONS --

ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -DDEBUG -g
	LDFLAGS += -O0 -g
else
	CFLAGS += -O3 -DNDEBUG -g0
	LDFLAGS += -O3 -g0 -Wl,-s -flto
endif




# -- GLSL CONFIG --

GLSLC ?= glslc
GLSLCFLAGS = -MD






# -- SOURCE & OBJECT FILES (BOTH CODE & ASSETS) --

# glsl

SOURCES_GLSL=$(wildcard standard_assets/shaders/*.glsl) $(wildcard test_assets/shaders/*.glsl)
SOURCES_VERT=$(wildcard standard_assets/shaders/*.vert) $(wildcard test_assets/shaders/*.vert)
SOURCES_FRAG=$(wildcard standard_assets/shaders/*.frag) $(wildcard test_assets/shaders/*.frag)

OBJECTS_GLSL=$(SOURCES_VERT:%=$(BUILD_DIR)/%.spv) $(SOURCES_FRAG:%=$(BUILD_DIR)/%.spv) \
$(SOURCES_GLSL:%=build/%) \
$(SOURCES_VERT:%=build/%) \
$(SOURCES_FRAG:%=build/%)


# c

CONFIG_PARSER_SOURCES = $(wildcard config_parser/*.c)

SOURCES_C_NO_TESTS = $(wildcard pigeon_engine/src/*.c) $(wildcard pigeon_engine/src/wgi/*.c) \
$(wildcard pigeon_engine/src/wgi/vulkan/*.c) $(wildcard pigeon_engine/src/wgi/opengl/*.c) \
$(wildcard pigeon_engine/src/scene/*.c) $(wildcard pigeon_engine/src/audio/*.c) \
$(wildcard pigeon_engine/src/job_system/*.c) $(wildcard pigeon_engine/src/io/*.c) \
$(CONFIG_PARSER_SOURCES) deps/glad.c

OBJECTS_C_NO_TESTS = $(SOURCES_C_NO_TESTS:%.c=$(BUILD_DIR)/%.o)

SOURCES_ALL_C = $(SOURCES_C_NO_TESTS) $(wildcard tests/1/*.c) \
$(wildcard tests/network/*.c) $(wildcard tests/unit_tests/*.c)

OBJECTS_ALL_C = $(SOURCES_ALL_C:%.c=$(BUILD_DIR)/%.o)

DEPS = $(OBJECTS_ALL_C:%.o=%.d) $(OBJECTS_GLSL:%=%.d)

ASSETS_SRC_IMAGES = \
$(wildcard test_assets/textures/*.jpg) \
$(wildcard test_assets/textures/*.jpeg) \
$(wildcard test_assets/textures/*.png) \
$(wildcard test_assets/textures/*.tga) \
$(wildcard test_assets/textures/*.bmp) \
$(wildcard test_assets/textures/*.psd) \
$(wildcard test_assets/textures/*.gif) \
$(wildcard test_assets/textures/*.hdr) \
$(wildcard test_assets/textures/*.pic) \
$(wildcard test_assets/textures/*.pnm) \
$(wildcard standard_assets/textures/*.jpg) \
$(wildcard standard_assets/textures/*.jpeg) \
$(wildcard standard_assets/textures/*.png) \
$(wildcard standard_assets/textures/*.tga) \
$(wildcard standard_assets/textures/*.bmp) \
$(wildcard standard_assets/textures/*.psd) \
$(wildcard standard_assets/textures/*.gif) \
$(wildcard standard_assets/textures/*.hdr) \
$(wildcard standard_assets/textures/*.pic) \
$(wildcard standard_assets/textures/*.pnm)

ASSETS_SRC_MODELS = $(wildcard test_assets/models/*.blend) \
$(wildcard standard_assets/models/*.blend)

ASSETS_SRC_AUDIO = $(wildcard test_assets/audio/*.ogg) \
$(wildcard standard_assets/audio/*.ogg)

ASSET_FILES_TEXTURES = $(ASSETS_SRC_IMAGES:%=build/%.asset)
ASSET_FILES_MODELS = $(ASSETS_SRC_MODELS:%=build/%.asset)
ASSET_FILES_AUDIO = $(ASSETS_SRC_AUDIO:%=build/%.asset)

ASSET_FILES = $(ASSET_FILES_TEXTURES) $(ASSET_FILES_MODELS) $(ASSET_FILES_AUDIO) build/standard_assets/ca-certificates.crt




# -- RULES --

all: tests

# tests

tests: $(BUILD_DIR)/test1 $(BUILD_DIR)/nettest $(BUILD_DIR)/unit_tests $(OBJECTS_GLSL) $(ASSET_FILES)


$(BUILD_DIR)/test1: $(OBJECTS_C_NO_TESTS) $(BUILD_DIR)/tests/1/test1.o
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/nettest: $(OBJECTS_C_NO_TESTS) $(BUILD_DIR)/tests/network/test_network_client.o $(BUILD_DIR)/tests/network/test_network_server.o
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/unit_tests: $(OBJECTS_C_NO_TESTS) $(BUILD_DIR)/tests/unit_tests/unit_tests.o
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# c

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o $@

# shaders

shaders: $(OBJECTS_GLSL)

$(BUILD_DIR)/%.spv: %
	@mkdir -p $(@D)
	$(GLSLC) $(GLSLCFLAGS) $< -o $@

build/standard_assets/shaders/%: standard_assets/shaders/%
	@mkdir -p $(@D)
	ln -sf ../../../$< $@

# tools

IMAGE_ASSET_CONVERTER_DEPS_C = $(wildcard image_asset_converter/*.c)
IMAGE_ASSET_CONVERTER_DEPS_OBJ = $(CONFIG_PARSER_SOURCES:%.c=$(BUILD_DIR)/%.o)
IMAGE_ASSET_CONVERTER_DEPS = $(IMAGE_ASSET_CONVERTER_DEPS_C) $(IMAGE_ASSET_CONVERTER_DEPS_OBJ)
IMAGE_ASSET_CONVERTER_CFLAGS = $(CFLAGS)

ifneq ($(BC7E_OBJ),)
	IMAGE_ASSET_CONVERTER_CFLAGS += -DBC7E
endif

$(BUILD_DIR)/image_asset_converter: $(IMAGE_ASSET_CONVERTER_DEPS) $(BC7E_OBJ)
	@mkdir -p $(@D)
	$(CC) $(IMAGE_ASSET_CONVERTER_CFLAGS) $^ -o $@ -lzstd -lm


AUDIO_ASSET_CONVERTER_DEPS = $(wildcard audio_asset_converter/*.c)

$(BUILD_DIR)/audio_asset_converter: $(AUDIO_ASSET_CONVERTER_DEPS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ -lm




# assets

build/standard_assets/ca-certificates.crt: standard_assets/ca-certificates.crt
	ln -sf ../../$< $@


$(ASSET_FILES_TEXTURES): build/%.asset: % %.import $(IMAGE_ASSET_CONVERTER_DEPS) | $(BUILD_DIR)/image_asset_converter
	@mkdir -p $(@D)
	$(BUILD_DIR)/image_asset_converter $< $@


$(ASSET_FILES_MODELS): build/%.asset: % %.import blender_export.py
	@mkdir -p $(@D)
	blender $< --background --python-exit-code 1 --python blender_export.py -- $@


$(ASSET_FILES_AUDIO): build/%.asset: % $(AUDIO_ASSET_CONVERTER_DEPS) | $(BUILD_DIR)/audio_asset_converter
	@mkdir -p $(@D)
	ln -sf ../../../$< $(patsubst %.asset,%.data,$@)
	$(BUILD_DIR)/audio_asset_converter $< $@


-include $(DEPS)

clean_shaders:
	-rm build/debug/standard_assets/shaders/*.spv
	-rm build/release/standard_assets/shaders/*.spv
	-rm build/standard_assets/shaders/*

clean_assets:
	-rm -r build/standard_assets/*
	-rm -r build/test_assets/*

clean_code:
	-rm -r build/debug
	-rm -r build/release

clean:
	-rm -r build/*

