
# -- C CONFIG --

# Run with make DEBUG=1 to enable debugging and disable optimisations
DEBUG ?= 0

# Run with make CC=clang to force usage of clang
CC ?= clang

CFLAGS_COMMON = -std=c11 -MMD -Wall -Wextra -pedantic \
-Wshadow -Wno-missing-field-initializers -Werror=implicit-function-declaration \
-Wmissing-prototypes -Wimplicit-fallthrough \
-Wunused-macros -Wcast-align \
-Wformat-security -Wundef -Wconversion -Werror=unused-result\
-Iconfig_parser -fstack-protector -I pigeon_engine/include -isystem deps


ifeq ($(CC), clang)
CFLAGS_COMMON += -Wshorten-64-to-32 -Wconditional-uninitialized -Wimplicit-int-conversion \
-Wimplicit-float-conversion -Wimplicit-int-float-conversion -Wno-newline-eof 
endif


CFLAGS = $(CFLAGS_COMMON)

LDFLAGS = -lopenal -lglfw -lzstd -lvulkan -lX11 -lXi -lpthread -lm -lc -fuse-ld=lld

# -- GLSL CONFIG --

GLSLC ?= glslc
GLSLCFLAGS = -O -MD


# -- BUILD-MODE-SPECIFIC OPTIONS --


ifeq ($(DEBUG), 1)
	BUILD_DIR += build/debug
	CFLAGS += -O0 -DDEBUG -g
	LDFLAGS += -O0 -g
	GLSLCFLAGS += -O0 -DDEBUG
else
	BUILD_DIR = build/release
	CFLAGS += -O3 -DNDEBUG -g0
	LDFLAGS += -O3 -g0 -Wl,-s -flto
	GLSLCFLAGS += -O -DNDEBUG
endif



# -- SOURCE & OBJECT FILES (CODE & ASSETS) --

SOURCES_GLSL=$(wildcard standard_assets/shaders/*.glsl) $(wildcard test_assets/shaders/*.glsl)
SOURCES_VERT=$(wildcard standard_assets/shaders/*.vert) $(wildcard test_assets/shaders/*.vert)
SOURCES_FRAG=$(wildcard standard_assets/shaders/*.frag) $(wildcard test_assets/shaders/*.frag)
OBJECTS_GLSL=$(SOURCES_VERT:%=$(BUILD_DIR)/%.spv) $(SOURCES_FRAG:%=$(BUILD_DIR)/%.spv) \
$(BUILD_DIR)/standard_assets/shaders/object.vert.depth.spv \
$(BUILD_DIR)/standard_assets/shaders/object.vert.depth_alpha.spv \
$(BUILD_DIR)/standard_assets/shaders/object.vert.light.spv \
$(BUILD_DIR)/standard_assets/shaders/gaussian_light.frag.1.spv \
$(BUILD_DIR)/standard_assets/shaders/gaussian_light.frag.3.spv \
$(BUILD_DIR)/standard_assets/shaders/gaussian_light.frag.4.spv



SOURCES = $(wildcard pigeon_engine/src/*.c) $(wildcard pigeon_engine/src/wgi/*.c) \
$(wildcard pigeon_engine/src/wgi/vulkan/*.c) $(wildcard pigeon_engine/src/audio/*.c) \
$(wildcard tests/1/*.c) config_parser/parser.c
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)


DEPS = $(OBJECTS:%.o=%.d) $(OBJECTS_GLSL:%=%.d)

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

ASSET_FILES = $(ASSET_FILES_TEXTURES) $(ASSET_FILES_MODELS) $(ASSET_FILES_AUDIO)

# -- RULES --

all: tests

shaders: $(OBJECTS_GLSL)

IMAGE_ASSET_CONVERTER_DEPS = image_asset_converter/converter.c config_parser/parser.c

$(BUILD_DIR)/image_asset_converter: $(IMAGE_ASSET_CONVERTER_DEPS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ -lzstd -lm

AUDIO_ASSET_CONVERTER_DEPS = audio_asset_converter/converter.c

$(BUILD_DIR)/audio_asset_converter: $(AUDIO_ASSET_CONVERTER_DEPS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ -lm


ifeq ($(DEBUG), 1)
tests: $(BUILD_DIR)/test1 $(OBJECTS_GLSL) $(ASSET_FILES) build/debug/parser_test
else
tests: $(BUILD_DIR)/test1 $(OBJECTS_GLSL) $(ASSET_FILES)
endif

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o $@


$(BUILD_DIR)/%.spv: %
	@mkdir -p $(@D)
	$(GLSLC) $(GLSLCFLAGS) $< -o $@

$(BUILD_DIR)/standard_assets/shaders/object.vert.depth.spv: standard_assets/shaders/object.vert
	@mkdir -p $(@D)
	$(GLSLC) -DOBJECT_DEPTH $(GLSLCFLAGS) $< -o $@

$(BUILD_DIR)/standard_assets/shaders/object.vert.depth_alpha.spv: standard_assets/shaders/object.vert
	@mkdir -p $(@D)
	$(GLSLC) -DOBJECT_DEPTH_ALPHA $(GLSLCFLAGS) $< -o $@

$(BUILD_DIR)/standard_assets/shaders/object.vert.light.spv: standard_assets/shaders/object.vert
	@mkdir -p $(@D)
	$(GLSLC) -DOBJECT_LIGHT $(GLSLCFLAGS) $< -o $@

$(BUILD_DIR)/standard_assets/shaders/gaussian_light.frag.1.spv: standard_assets/shaders/gaussian_light.frag
	@mkdir -p $(@D)
	$(GLSLC) -DCOLOUR_TYPE_R $(GLSLCFLAGS) $< -o $@

$(BUILD_DIR)/standard_assets/shaders/gaussian_light.frag.3.spv: standard_assets/shaders/gaussian_light.frag
	@mkdir -p $(@D)
	$(GLSLC) -DCOLOUR_TYPE_RGB $(GLSLCFLAGS) $< -o $@

$(BUILD_DIR)/standard_assets/shaders/gaussian_light.frag.4.spv: standard_assets/shaders/gaussian_light.frag
	@mkdir -p $(@D)
	$(GLSLC) -DCOLOUR_TYPE_RGBA $(GLSLCFLAGS) $< -o $@



-include $(DEPS)

$(BUILD_DIR)/test1: $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

build/debug/parser_test: $(wildcard config_parser/*.c)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_COMMON) -DDEBUG -g -O0 $^ -o $@


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


clean_spv:
	-rm build/debug/standard_assets/shaders/*.spv
	-rm build/release/standard_assets/shaders/*.spv

clean_assets:
	-rm -r build/standard_assets/*
	-rm -r build/test_assets/*

clean_code:
	-rm -r build/debug
	-rm -r build/release

clean:
	-rm -r build/*

