#include <pigeon/scene/transform.h>
#include <pigeon/scene/scene.h>
#include <pigeon/scene/mesh_renderer.h>
#include <pigeon/scene/light.h>
#include <pigeon/scene/audio.h>
#include <pigeon/pigeon.h>
#include <pigeon/assert.h>
#include <pigeon/asset.h>
#include <pigeon/misc.h>
#include <pigeon/wgi/input.h>
#include <pigeon/wgi/pipeline.h>
#include <pigeon/wgi/swapchain.h>
#include <string.h>
#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
    #define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <cglm/quat.h>
#include <cglm/affine.h>
#include <cglm/euler.h>

#ifdef NDEBUG
	#define SHADER_PATH_PREFIX "build/release/standard_assets/shaders/"
#else
	#define SHADER_PATH_PREFIX "build/debug/standard_assets/shaders/"
#endif

PigeonWGIVertexAttributeType static_mesh_attribs[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES] = {
	PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED,
	PIGEON_WGI_VERTEX_ATTRIBUTE_UV_FLOAT,
	PIGEON_WGI_VERTEX_ATTRIBUTE_NORMAL,
	PIGEON_WGI_VERTEX_ATTRIBUTE_TANGENT
};

PigeonWGIVertexAttributeType skinned_mesh_attribs[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES] = {
	PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED,
	PIGEON_WGI_VERTEX_ATTRIBUTE_BONE,
	PIGEON_WGI_VERTEX_ATTRIBUTE_UV_FLOAT,
	PIGEON_WGI_VERTEX_ATTRIBUTE_NORMAL,
	PIGEON_WGI_VERTEX_ATTRIBUTE_TANGENT
};

#define AUDIO_ASSET_COUNT 1
#define ASSET_PATH(x) ("build/test_assets/audio/" x ".asset")
#define ASSET_PATH2(x) ("build/test_assets/audio/" x ".data")

const char *audio_file_paths[AUDIO_ASSET_COUNT][2] = {
	{ASSET_PATH("pigeon.ogg"), ASSET_PATH2("pigeon.ogg")},
};

#undef ASSET_PATH
#undef ASSET_PATH2

PigeonAsset audio_assets[AUDIO_ASSET_COUNT];
PigeonAudioBufferID audio_buffers[AUDIO_ASSET_COUNT];

#define MODEL_ASSET_COUNT 3
#define ASSET_PATH(x) ("build/test_assets/models/" x ".asset")
#define ASSET_PATH2(x) ("build/standard_assets/models/" x ".asset")
#define ASSET_PATH3(x) ("build/test_assets/models/" x ".data")
#define ASSET_PATH4(x) ("build/standard_assets/models/" x ".data")

const char *model_file_paths[MODEL_ASSET_COUNT][2] = {
	{ASSET_PATH2("cube.blend"), ASSET_PATH4("cube.blend")},
	{ASSET_PATH("pete.blend"), ASSET_PATH3("pete.blend")},
	{ASSET_PATH2("sphere.blend"), ASSET_PATH4("sphere.blend")},
};

#undef ASSET_PATH

PigeonAsset model_assets[MODEL_ASSET_COUNT];

unsigned int texture_assets_count;
PigeonAsset *texture_assets;

typedef struct TextureIndices
{
	// UINT32_MAX if not used, otherwise index into texture_assets and texture_locations
	uint32_t texture_index;
	uint32_t normal_index;
} TextureIndices;

TextureIndices *model_texture_indices[MODEL_ASSET_COUNT]; // 1 for each material in each model

typedef struct ArrayTexture
{
	struct ArrayTexture *next;
	PigeonWGIArrayTexture t;
} ArrayTexture;

ArrayTexture *array_texture_0 = NULL; // LINKED LIST

typedef struct TextureLocation
{
	PigeonWGIArrayTexture *texture;
	unsigned int bind_point;
	unsigned int layer;
	unsigned int subresource_index;
} TextureLocation;

TextureLocation *texture_locations; // One for each texture asset

PigeonWGIMultiMesh mesh;
PigeonWGIMultiMesh mesh_skinned; // includes bone data

PigeonWGIPipeline skybox_pipeline;
PigeonWGIPipeline render_pipeline;
PigeonWGIPipeline render_pipeline_transparent;
PigeonWGIPipeline render_pipeline_skinned;
PigeonWGIPipeline render_pipeline_skinned_transparent;

PigeonRenderState *rs_static_opaque;
PigeonRenderState *rs_static_transparent;
PigeonRenderState *rs_skinned_opaque;
PigeonRenderState *rs_skinned_transparent;

PigeonTransform *t_floor;
PigeonTransform *t_character;
PigeonTransform *t_spinning_cube;
PigeonTransform *t_light;
PigeonTransform *t_camera;
PigeonTransform *t_sphere;
PigeonTransform *t_pigeon;

PigeonModelMaterial *model_cube;
PigeonModelMaterial **model_character;
PigeonModelMaterial *model_sphere;

PigeonMaterialRenderer *mr_floor;
PigeonMaterialRenderer *mr_spinning_cube;
PigeonMaterialRenderer *mr_sphere;
PigeonMaterialRenderer **mr_character;

PigeonAnimationState *as_character;
PigeonLight * light;
PigeonAudioPlayer * audio_pigeon;

bool mouse_grabbed;
int last_mouse_x = -1;
int last_mouse_y = -1;

bool debug_disable_ssao = false, debug_disable_bloom = false;

double start_time = 0;

static void key_callback(PigeonWGIKeyEvent e)
{
	if (e.key == PIGEON_WGI_KEY_1 && !e.pressed)
	{
		debug_disable_ssao = !debug_disable_ssao;
	}
	if (e.key == PIGEON_WGI_KEY_2 && !e.pressed)
	{
		debug_disable_bloom = !debug_disable_bloom;
		pigeon_wgi_set_bloom_intensity(debug_disable_bloom ? 0 : 1);
	}

	if(e.key == PIGEON_WGI_KEY_0 && !e.pressed && AUDIO_ASSET_COUNT) {
		pigeon_audio_player_play(audio_pigeon, audio_buffers[0]);
	}
}

static void mouse_callback(PigeonWGIMouseEvent e)
{
	if (e.button == PIGEON_WGI_MOUSE_BUTTON_RIGHT && !e.pressed)
	{
		mouse_grabbed = !mouse_grabbed;
		pigeon_wgi_set_cursor_type(mouse_grabbed ? PIGEON_WGI_CURSOR_TYPE_FPS_CAMERA : PIGEON_WGI_CURSOR_TYPE_NORMAL);
		if (mouse_grabbed)
			last_mouse_x = last_mouse_y = -1;
	}
}

static PIGEON_ERR_RET start(void)
{
	pigeon_init();

	PigeonWindowParameters window_parameters = {
		.width = 1280,
		.height = 720,
		.window_mode = PIGEON_WINDOW_MODE_WINDOWED,
		.title = "Test 1"};

	PigeonWGIRenderConfig cfg = {0};
	cfg.ssao = true;
	cfg.bloom = true;
	cfg.shadow_casting_lights = 1;
	cfg.shadow_blur_passes = 2;
	if (pigeon_wgi_init(window_parameters, true, false, cfg, 0.1f, 1000.0f))
	{
		pigeon_wgi_deinit();
		return 1;
	}

	pigeon_wgi_set_key_callback(key_callback);
	pigeon_wgi_set_mouse_button_callback(mouse_callback);

	if (pigeon_audio_init())
	{
		pigeon_wgi_deinit();
		return 1;
	}

	start_time = pigeon_wgi_get_time_seconds_double();

	return 0;
}

static PIGEON_ERR_RET load_model_assets(void)
{
	texture_assets_count = 0;
	for (unsigned int i = 0; i < MODEL_ASSET_COUNT; i++)
	{
		ASSERT_R1(!pigeon_load_asset_meta(&model_assets[i], model_file_paths[i][0]));
		for (unsigned int j = 0; j < model_assets[i].materials_count; j++)
		{
			if (model_assets[i].materials[j].texture)
			{
				texture_assets_count++;
			}
			if (model_assets[i].materials[j].normal_map_texture)
			{
				texture_assets_count++;
			}
		}
		ASSERT_R1(model_assets[i].type == PIGEON_ASSET_TYPE_MODEL);

		ASSERT_R1(
			memcmp(static_mesh_attribs, model_assets[i].mesh_meta.attribute_types,
				sizeof static_mesh_attribs) == 0 ||
			memcmp(skinned_mesh_attribs, model_assets[i].mesh_meta.attribute_types,
				sizeof static_mesh_attribs) == 0);

		ASSERT_R1(!pigeon_load_asset_data(&model_assets[i], model_file_paths[i][1]));
	}
	return 0;
}

static PIGEON_ERR_RET load_texture_asset(const char *asset_name, PigeonAsset *asset, bool normals)
{
	const char *prefix = "build/test_assets/textures/";
	const size_t prefix_len = strlen(prefix);
	const size_t asset_name_len = strlen(asset_name);
	const char *suffix = ".asset";
	const size_t suffix_len = strlen(suffix);

	char *meta_file_path = malloc(prefix_len + asset_name_len + suffix_len + 1);
	ASSERT_R1(meta_file_path);

	memcpy(meta_file_path, prefix, prefix_len);
	memcpy(&meta_file_path[prefix_len], asset_name, asset_name_len);
	memcpy(&meta_file_path[prefix_len + asset_name_len], suffix, suffix_len + 1);

	ASSERT_R1(!pigeon_load_asset_meta(asset, meta_file_path));
	ASSERT_R1(asset->type == PIGEON_ASSET_TYPE_IMAGE);
	if (normals)
	{
		ASSERT_R1(asset->texture_meta.format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR);
	}
	else
	{
		ASSERT_R1(asset->texture_meta.format == PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB);
	}
	ASSERT_R1(asset->texture_meta.width % 4 == 0);
	ASSERT_R1(asset->texture_meta.height % 4 == 0);
	ASSERT_R1(asset->texture_meta.has_mip_maps);

	char *data_file_path = meta_file_path;
	memcpy(&data_file_path[prefix_len + asset_name_len], ".data", 6);

	ASSERT_R1(!pigeon_load_asset_data(asset, data_file_path));

	return 0;
}

static PIGEON_ERR_RET load_texture_assets()
{
	texture_assets = calloc(texture_assets_count, sizeof *texture_assets);
	ASSERT_R1(texture_assets);

	texture_locations = malloc(texture_assets_count * sizeof *texture_locations);

	unsigned int tex_i = 0;
	for (unsigned int i = 0; i < MODEL_ASSET_COUNT; i++)
	{
		model_texture_indices[i] = calloc(model_assets[i].materials_count, sizeof *model_texture_indices[i]);
		ASSERT_R1(model_texture_indices[i]);

		for (unsigned int j = 0; j < model_assets[i].materials_count; j++)
		{
			if (model_assets[i].materials[j].texture)
			{
				model_texture_indices[i][j].texture_index = tex_i;
				ASSERT_R1(!load_texture_asset(model_assets[i].materials[j].texture, &texture_assets[tex_i],
											  false));

				tex_i++;
			}
			else
			{
				model_texture_indices[i][j].texture_index = UINT32_MAX;
			}
			if (model_assets[i].materials[j].normal_map_texture)
			{
				model_texture_indices[i][j].normal_index = tex_i;
				ASSERT_R1(!load_texture_asset(model_assets[i].materials[j].normal_map_texture,
											  &texture_assets[tex_i], true));

				tex_i++;
			}
			else
			{
				model_texture_indices[i][j].normal_index = UINT32_MAX;
			}
		}
	}
	return 0;
}


static PIGEON_ERR_RET populate_array_textures(void)
{
	for (unsigned int i = 0; i < texture_assets_count; i++)
	{
		PigeonAsset *asset = &texture_assets[i];

		PigeonWGIImageFormat base_format = asset->texture_meta.format;
		PigeonWGIImageFormat true_format = base_format;
		unsigned int subresource_index = 0;
		unsigned int chosen_subresource = 0;

		if(base_format == PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB) {
			if (asset->texture_meta.has_bc1) {
				subresource_index++;
				if(pigeon_wgi_bc1_optimal_available()) {
					chosen_subresource = subresource_index;
					true_format = PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB;
				}
			}
			if (asset->texture_meta.has_bc3) {
				subresource_index++;
				if(pigeon_wgi_bc3_optimal_available()) {
					chosen_subresource = subresource_index;
					true_format = PIGEON_WGI_IMAGE_FORMAT_BC3_SRGB;
				}
			}
			if (asset->texture_meta.has_bc7) {
				subresource_index++;
				if(pigeon_wgi_bc7_optimal_available()) {
					chosen_subresource = subresource_index;
					true_format = PIGEON_WGI_IMAGE_FORMAT_BC7_SRGB;
				}
			}
			if (asset->texture_meta.has_etc1) {
				subresource_index++;
				if(pigeon_wgi_etc1_optimal_available()) {
					chosen_subresource = subresource_index;
					true_format = PIGEON_WGI_IMAGE_FORMAT_ETC1_LINEAR;
				}
				if(pigeon_wgi_etc2_optimal_available()) {
					chosen_subresource = subresource_index;
					true_format = PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB;
				}
			}
			if (asset->texture_meta.has_etc2) {
				subresource_index++;
				if(pigeon_wgi_etc2_optimal_available()) {
					chosen_subresource = subresource_index;
					true_format = PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB;
				}
			}
			if (asset->texture_meta.has_etc2_alpha) {
				subresource_index++;
				if(pigeon_wgi_etc2_rgba_optimal_available()) {
					chosen_subresource = subresource_index;
					true_format = PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB_ALPHA_SRGB;
				}
			}
		}
		if(base_format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR && asset->texture_meta.has_bc5
			&& pigeon_wgi_bc5_optimal_available()) {
			chosen_subresource = 1;
			true_format = PIGEON_WGI_IMAGE_FORMAT_BC5;
		}

		ArrayTexture * array_texture = NULL;
		unsigned int texture_index = 0;
		if(array_texture_0) {
			ArrayTexture * arr = array_texture_0;
			while(1) {
				if(arr->t.format == true_format && arr->t.width == asset->texture_meta.width
					&& arr->t.height == asset->texture_meta.height
					&& arr->t.mip_maps == asset->texture_meta.has_mip_maps)
				{
					array_texture = arr;
					break;
				}

				if(!arr->next) break;

				arr = arr->next;
				texture_index++;
			}
			if(!array_texture) {
				arr->next = calloc(1, sizeof *arr->next);
				ASSERT_R1(arr->next);
				array_texture = arr->next;
				texture_index++;
			}
		}
		else {
			array_texture_0 = calloc(1, sizeof *array_texture_0);
			ASSERT_R1(array_texture_0);
			array_texture = array_texture_0;
		}

		array_texture->t.format = true_format;
		array_texture->t.width = asset->texture_meta.width;
		array_texture->t.height = asset->texture_meta.height;
		array_texture->t.mip_maps = asset->texture_meta.has_mip_maps;
		array_texture->t.layers++;

		texture_locations[i].texture = &array_texture->t;
		texture_locations[i].bind_point = texture_index;
		texture_locations[i].layer = array_texture->t.layers-1;
		texture_locations[i].subresource_index = chosen_subresource;
	}
	return 0;
}

// Runs in first frame
static PIGEON_ERR_RET create_array_textures(PigeonWGICommandBuffer *cmd)
{

	ArrayTexture * arr = array_texture_0;
	while(arr) {
		ASSERT_R1(!pigeon_wgi_create_array_texture(&arr->t, arr->t.width, arr->t.height,
			arr->t.layers, arr->t.format, arr->t.mip_maps ? 0 : 1, cmd));

		
		unsigned int layer = 0;
		for (unsigned int i = 0; i < texture_assets_count; i++)
		{
			if(texture_locations[i].texture == &arr->t) {
				if(pigeon_wgi_array_texture_upload_method() == 1) {
					void *dst = pigeon_wgi_array_texture_upload1(&arr->t, layer++, cmd);
					ASSERT_R1(!pigeon_decompress_asset(&texture_assets[i], dst, texture_locations[i].subresource_index));
				}
				else {
					unsigned int subr = texture_locations[i].subresource_index;
					ASSERT_R1(texture_assets[i].subresources[subr].decompressed_data_length ==
						pigeon_wgi_get_array_texture_layer_size(&arr->t));
					ASSERT_R1(!pigeon_decompress_asset(&texture_assets[i], NULL, subr));
					ASSERT_R1(!pigeon_wgi_array_texture_upload2(&arr->t, layer++, 
						texture_assets[i].subresources[subr].decompressed_data));
				}
			}
		}

		pigeon_wgi_array_texture_transition(&arr->t, cmd);
		pigeon_wgi_array_texture_unmap(&arr->t);

		arr = arr->next;
	}


	return 0;
}

static void destroy_array_textures(void)
{
	ArrayTexture * arr = array_texture_0;
	while(arr) {
		pigeon_wgi_destroy_array_texture(&arr->t);
		arr = arr->next;
	}
}


static PIGEON_ERR_RET load_animation_assets(void)
{

	for (unsigned int j = 0; j < MODEL_ASSET_COUNT; j++)
	{
		PigeonAsset *a = &model_assets[j];
		unsigned int i = 0;
		for (;; i++)
		{
			if (!a->mesh_meta.attribute_types[i])
				break;
		}
		if (a->mesh_meta.index_count > 0)
			i++;

		for (; i < PIGEON_ASSET_MAX_SUBRESOURCES; i++)
		{
			if (a->subresources[i].decompressed_data_length)
			{
				ASSERT_R1(!pigeon_decompress_asset(a, NULL, i));
			}
			else
				break;
		}
	}

	return 0;
}

static PIGEON_ERR_RET load_audio_assets(void)
{
	if (!AUDIO_ASSET_COUNT)
		return 0;
	ASSERT_R1(!pigeon_audio_create_buffers(AUDIO_ASSET_COUNT, audio_buffers));

	for (unsigned int i = 0; i < AUDIO_ASSET_COUNT; i++)
	{
		ASSERT_R1(!pigeon_load_asset_meta(&audio_assets[i], audio_file_paths[i][0]));
		ASSERT_R1(audio_assets[i].type == PIGEON_ASSET_TYPE_AUDIO);
		ASSERT_R1(!pigeon_load_asset_data(&audio_assets[i], audio_file_paths[i][1]));
		ASSERT_R1(!pigeon_decompress_asset(&audio_assets[i], NULL, 0));
		ASSERT_R1(!pigeon_audio_upload(audio_buffers[i], audio_assets[i].audio_meta,
									   audio_assets[i].subresources[0].decompressed_data,
									   audio_assets[i].subresources[0].decompressed_data_length));
		pigeon_free_asset(&audio_assets[i]);
	}

	return 0;
}

static PIGEON_ERR_RET load_assets(void)
{
	ASSERT_R1(!load_model_assets());
	ASSERT_R1(!load_animation_assets());
	ASSERT_R1(!load_audio_assets());
	ASSERT_R1(!load_texture_assets());
	return 0;
}

static void free_assets(void)
{
	if (texture_assets)
	{
		for (unsigned int i = 0; i < texture_assets_count; i++)
		{
			pigeon_free_asset(&texture_assets[i]);
		}
		free(texture_assets);
	}
	for (unsigned int i = 0; i < MODEL_ASSET_COUNT; i++)
	{
		pigeon_free_asset(&model_assets[i]);
	}
	for (unsigned int i = 0; i < AUDIO_ASSET_COUNT; i++)
	{
		pigeon_free_asset(&audio_assets[i]);
	}
}

static void audio_cleanup(void)
{
	if (AUDIO_ASSET_COUNT && audio_buffers[0])
		pigeon_audio_destroy_buffers(AUDIO_ASSET_COUNT, audio_buffers);
	pigeon_audio_deinit();
}

static PIGEON_ERR_RET create_multimesh(PigeonWGIMultiMesh *mm, bool big_indices, bool skinned)
{
	uint64_t size = 0;
	unsigned int vertex_count = 0;
	unsigned int index_count = 0;
	unsigned int first_model_index = 0;

	for (unsigned int i = 0; i < MODEL_ASSET_COUNT; i++)
	{
		if (!model_assets[i].mesh_meta.vertex_count || (model_assets[i].bones_count > 0) != skinned)
			continue;
		first_model_index = i;

		uint64_t sz = pigeon_wgi_mesh_meta_size_requirements(&model_assets[i].mesh_meta);

		uint64_t full_size = 0;
		for (unsigned int j = 0; j < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; j++)
		{
			full_size += model_assets[i].subresources[j].decompressed_data_length;
			if (!model_assets[i].mesh_meta.attribute_types[j])
				break;
		}
		ASSERT_R1(full_size == sz);

		model_assets[i].mesh_meta.multimesh_start_vertex = vertex_count;
		model_assets[i].mesh_meta.multimesh_start_index = index_count;
		vertex_count += model_assets[i].mesh_meta.vertex_count;
		index_count += model_assets[i].mesh_meta.index_count ? model_assets[i].mesh_meta.index_count : model_assets[i].mesh_meta.vertex_count;
	}

	uint8_t *mapping_vertices;
	uint16_t *mapping_indices;
	ASSERT_R1(!pigeon_wgi_create_multimesh(mm,
		model_assets[first_model_index].mesh_meta.attribute_types, vertex_count, index_count, big_indices,
		&size, (void **)&mapping_vertices, (void **)&mapping_indices));

	// Load vertex attributes

	PigeonWGIVertexAttributeType *attributes = model_assets[first_model_index].mesh_meta.attribute_types;
	uint64_t offset = 0;
	unsigned int j = 0;
	for (; j < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; j++)
	{
		if (!attributes[j])
			break;

		for (unsigned int i = 0; i < MODEL_ASSET_COUNT; i++)
		{
			if (!model_assets[i].mesh_meta.vertex_count || (model_assets[i].bones_count > 0) != skinned)
				continue;

			uint64_t sz = pigeon_wgi_get_vertex_attribute_type_size(attributes[j]) * model_assets[i].mesh_meta.vertex_count;
			ASSERT_R1(sz == model_assets[i].subresources[j].decompressed_data_length);
			ASSERT_R1(!pigeon_decompress_asset(&model_assets[i], &mapping_vertices[offset], j));
			offset += sz;
		}
	}
	assert(offset == mm->index_data_offset);

	// Load indices

	offset = 0;
	for (unsigned int i = 0; i < MODEL_ASSET_COUNT; i++)
	{
		if (!model_assets[i].mesh_meta.vertex_count || (model_assets[i].bones_count > 0) != skinned)
			continue;

		if (j < model_assets[i].subresources[j].decompressed_data_length)
		{
			if (model_assets[i].mesh_meta.big_indices == big_indices)
			{
				ASSERT_R1(!pigeon_decompress_asset(&model_assets[i], &mapping_indices[offset], j));
				offset += big_indices ? 2 : 1 * model_assets[i].mesh_meta.index_count;
			}
			else
			{
				ASSERT_R1(big_indices);

				// Convert indices from u16 to u32

				uint16_t *u16 = malloc(2 * model_assets[i].mesh_meta.index_count);
				ASSERT_R1(u16);

				ASSERT_R1(!pigeon_decompress_asset(&model_assets[i], u16, j));

				for (unsigned int k = 0; k < model_assets[i].mesh_meta.index_count; k++)
				{
					mapping_indices[offset++] = u16[k];
					mapping_indices[offset++] = 0;
				}
				free(u16);
			}
		}
		else
		{
			// Flat shaded model- create indices
			for (unsigned int k = 0; k < model_assets[i].mesh_meta.vertex_count; k++)
			{
				mapping_indices[offset++] = (uint16_t)k;
				if (big_indices)
				{
					mapping_indices[offset++] = (uint16_t)(k >> 16);
				}
			}
		}
	}
	assert(offset == (mm->big_indices ? 2 : 1) * mm->index_count);

	ASSERT_R1(!pigeon_wgi_multimesh_unmap(mm));

	return 0;
}

static void destroy_multimeshes(void)
{
	if (mesh.staged_buffer)
		pigeon_wgi_destroy_multimesh(&mesh);
	if (mesh_skinned.staged_buffer)
		pigeon_wgi_destroy_multimesh(&mesh_skinned);
}

static PIGEON_ERR_RET create_multimeshes(void)
{
	ASSERT_R1(!create_multimesh(&mesh, true, false));
	ASSERT_R1(!create_multimesh(&mesh_skinned, false, true));
	return 0;
}

static PIGEON_ERR_RET create_skybox_pipeline_glsl(void)
{
	PigeonWGIShader vs = {0}, fs = {0};

	ASSERT_R1 (!pigeon_wgi_create_shader2(&vs, "skybox.vert", PIGEON_WGI_SHADER_TYPE_VERTEX, NULL, PIGEON_WGI_RENDER_STAGE_RENDER));

	if (pigeon_wgi_create_shader2(&fs, "skybox.frag", PIGEON_WGI_SHADER_TYPE_FRAGMENT, NULL, PIGEON_WGI_RENDER_STAGE_RENDER))
	{
		pigeon_wgi_destroy_shader(&vs);
		ASSERT_R1(false);
	}

	int err = pigeon_wgi_create_skybox_pipeline(&skybox_pipeline, &vs, &fs);

	pigeon_wgi_destroy_shader(&vs);
	pigeon_wgi_destroy_shader(&fs);

	ASSERT_R1(!err);
	return 0;
}

static PIGEON_ERR_RET create_skybox_pipeline(void)
{
	if(!pigeon_wgi_accepts_spirv()) return create_skybox_pipeline_glsl();

	unsigned long vs_spv_len = 0, fs_spv_len = 0;

	uint32_t *vs_spv = (uint32_t *)pigeon_load_file(SHADER_PATH_PREFIX "skybox.vert.spv", 0, &vs_spv_len);
	ASSERT_LOG_R1(vs_spv, "Error loading vertex shader");

	uint32_t *fs_spv = (uint32_t *)pigeon_load_file(SHADER_PATH_PREFIX "skybox.frag.spv", 0, &fs_spv_len);
	if (!fs_spv)
	{
		free(vs_spv);
		ASSERT_LOG_R1(false, "Error loading fragment shader");
	}

	PigeonWGIShader vs = {0}, fs = {0};

	if (pigeon_wgi_create_shader(&vs, vs_spv, (uint32_t)vs_spv_len, PIGEON_WGI_SHADER_TYPE_VERTEX))
	{
		free(vs_spv);
		free(fs_spv);
		ASSERT_R1(false);
	}

	if (pigeon_wgi_create_shader(&fs, fs_spv, (uint32_t)fs_spv_len, PIGEON_WGI_SHADER_TYPE_FRAGMENT))
	{
		pigeon_wgi_destroy_shader(&vs);
		free(vs_spv);
		free(fs_spv);
		ASSERT_R1(false);
	}

	int err = pigeon_wgi_create_skybox_pipeline(&skybox_pipeline, &vs, &fs);

	pigeon_wgi_destroy_shader(&vs);
	pigeon_wgi_destroy_shader(&fs);
	free(vs_spv);
	free(fs_spv);

	ASSERT_R1(!err);
	return 0;
}

static void create_pipeline_cleanup(uint32_t *spv_data[6], PigeonWGIShader shaders[6])
{
	for (unsigned int i = 0; i < 6; i++)
	{
		if (spv_data[i])
			free(spv_data[i]);
		if (shaders[i].shader)
			pigeon_wgi_destroy_shader(&shaders[i]);
	}
}


static PIGEON_ERR_RET create_pipeline(PigeonWGIPipeline * pipeline,
	const char * shader_paths[6][2], PigeonWGIRenderStage shader_stages[6], 
	PigeonWGIPipelineConfig * config, bool skinned, bool transparent)
{

	PigeonWGIShaderType shader_types[6] = {
		PIGEON_WGI_SHADER_TYPE_VERTEX,
		PIGEON_WGI_SHADER_TYPE_VERTEX,
		PIGEON_WGI_SHADER_TYPE_VERTEX,
		PIGEON_WGI_SHADER_TYPE_FRAGMENT,
		PIGEON_WGI_SHADER_TYPE_FRAGMENT,
		PIGEON_WGI_SHADER_TYPE_FRAGMENT
	};

	unsigned long spv_lengths[6] = {0};
	uint32_t * spv_data[6] = {0};
	PigeonWGIShader shaders[6] = {{0}};


	#define CHECK(x) \
		if(!(x)) { \
			create_pipeline_cleanup(spv_data, shaders); \
			ASSERT_R1(false); \
		}

	for(unsigned int i = 0; i < 6; i++) {
		if(shader_paths[i][0]) {
			if(pigeon_wgi_accepts_spirv()) {
				spv_data[i] = (uint32_t *)pigeon_load_file(shader_paths[i][1], 0, &spv_lengths[i]);
				CHECK(spv_data[i]);
				CHECK(!pigeon_wgi_create_shader(&shaders[i], spv_data[i], (uint32_t)spv_lengths[i], shader_types[i]));
			}
			else {
				CHECK(!pigeon_wgi_create_shader2(&shaders[i], shader_paths[i][0], shader_types[i], config, shader_stages[i]));
			}
		}	
	}

	int err = pigeon_wgi_create_pipeline(pipeline, &shaders[0], &shaders[1], &shaders[2],
		shaders[3].shader ? &shaders[3] : NULL, &shaders[4], &shaders[5], config, skinned, transparent);

	create_pipeline_cleanup(spv_data, shaders);

	ASSERT_R1(!err);

	return 0;
}


static PIGEON_ERR_RET create_pipelines(void)
{
	ASSERT_R1(!create_skybox_pipeline());

	PigeonWGIPipelineConfig config = {0};
	config.depth_test = true;

	config.depth_write = true;
	config.depth_test = true;
	config.cull_mode = PIGEON_WGI_CULL_MODE_BACK;

	#define FULL_PATH(x) SHADER_PATH_PREFIX x ".spv"
	#define PATHS(x) {x, SHADER_PATH_PREFIX x ".spv"}

	const char *shader_paths[6][2] = {
		PATHS("object.vert.depth"),
		PATHS("object.vert.light"),
		PATHS("object.vert"),
		{NULL, NULL},
		PATHS("object_light.frag"),
		PATHS("object.frag")};

	PigeonWGIRenderStage shader_stages[6] = {
		PIGEON_WGI_RENDER_STAGE_DEPTH,
		PIGEON_WGI_RENDER_STAGE_LIGHT,
		PIGEON_WGI_RENDER_STAGE_RENDER,
		PIGEON_WGI_RENDER_STAGE_DEPTH,
		PIGEON_WGI_RENDER_STAGE_LIGHT,
		PIGEON_WGI_RENDER_STAGE_RENDER
	};

	memcpy(config.vertex_attributes, static_mesh_attribs, sizeof config.vertex_attributes);
	ASSERT_R1(!create_pipeline(&render_pipeline, shader_paths, shader_stages, &config, false, false));

	const char *shader_paths_skinned[6][2] = {
		PATHS("object.vert.skinned.depth"),
		PATHS("object.vert.skinned.light"),
		PATHS("object.vert.skinned"),
		{NULL, NULL},
		PATHS("object_light.frag"),
		PATHS("object.frag")};

	memcpy(config.vertex_attributes, skinned_mesh_attribs, sizeof config.vertex_attributes);
	ASSERT_R1(!create_pipeline(&render_pipeline_skinned, shader_paths_skinned, shader_stages, &config, true, false));

	config.blend_function = PIGEON_WGI_BLEND_NORMAL;
	config.cull_mode = PIGEON_WGI_CULL_MODE_NONE;
	shader_paths[0][0] = "object.vert.depth_alpha";
	shader_paths[0][1] = FULL_PATH("object.vert.depth_alpha");
	shader_paths[3][0] = "object_depth_alpha.frag";
	shader_paths[3][1] = FULL_PATH("object_depth_alpha.frag");

	memcpy(config.vertex_attributes, static_mesh_attribs, sizeof config.vertex_attributes);
	ASSERT_R1(!create_pipeline(&render_pipeline_transparent, shader_paths, shader_stages, &config, false, true));

	shader_paths_skinned[0][0] = "object.vert.skinned.depth_alpha";
	shader_paths_skinned[0][1] = FULL_PATH("object.vert.skinned.depth_alpha");
	shader_paths_skinned[3][0] = "object_depth_alpha.frag";
	shader_paths_skinned[3][1] = FULL_PATH("object_depth_alpha.frag");
	memcpy(config.vertex_attributes, skinned_mesh_attribs, sizeof config.vertex_attributes);
	ASSERT_R1(!create_pipeline(&render_pipeline_skinned_transparent, shader_paths_skinned, shader_stages, &config, true, true));

	return 0;

}

static void destroy_pipelines(void)
{
	pigeon_wgi_destroy_pipeline(&skybox_pipeline);
	pigeon_wgi_destroy_pipeline(&render_pipeline);
	pigeon_wgi_destroy_pipeline(&render_pipeline_skinned);
	pigeon_wgi_destroy_pipeline(&render_pipeline_skinned_transparent);
	pigeon_wgi_destroy_pipeline(&render_pipeline_transparent);
}

static void set_camera_transform(vec2 rotation, vec3 position)
{
	vec3 angles = {rotation[0], -rotation[1], 0.0f};

	glm_euler_zyx(angles, t_camera->matrix_transform);

	mat4 t_mat;
	glm_translate_make(t_mat, position);
	glm_mat4_mul(t_mat, t_camera->matrix_transform, t_camera->matrix_transform);
	pigeon_invalidate_world_transform(t_camera);
}

static void print_timer_stats(double delayed_timer_values[PIGEON_WGI_TIMERS_COUNT])
{
	static double values[PIGEON_WGI_TIMERS_COUNT];
	for(unsigned int i = 0; i < PIGEON_WGI_TIMERS_COUNT; i++)
		values[i] += delayed_timer_values[i] - delayed_timer_values[0];

	static unsigned int frame_counter;
	frame_counter++;

	if(frame_counter < 300) {
		return;
	}

	for(unsigned int i = 0; i < PIGEON_WGI_TIMERS_COUNT; i++)
		values[i] /= 300.0;

	printf("Render time statistics (300-frame average):\n");
	printf("\tUpload: %f\n", values[PIGEON_WGI_TIMER_UPLOAD_DONE]);
	printf("\tDepth Prepass: %f\n", values[PIGEON_WGI_TIMER_DEPTH_PREPASS_DONE] - values[PIGEON_WGI_TIMER_UPLOAD_DONE]);
	printf("\tShadow Maps: %f\n", values[PIGEON_WGI_TIMER_SHADOW_MAPS_DONE] - values[PIGEON_WGI_TIMER_DEPTH_PREPASS_DONE]);
	printf("\tLight Pass: %f\n", values[PIGEON_WGI_TIMER_LIGHT_PASS_DONE] - values[PIGEON_WGI_TIMER_SHADOW_MAPS_DONE]);
	printf("\tLight Blur: %f\n", values[PIGEON_WGI_TIMER_LIGHT_GAUSSIAN_BLUR_DONE] - values[PIGEON_WGI_TIMER_LIGHT_PASS_DONE]);
	printf("\tRender: %f\n", values[PIGEON_WGI_TIMER_RENDER_DONE] - values[PIGEON_WGI_TIMER_LIGHT_GAUSSIAN_BLUR_DONE]);
	printf("\tBloom Downsample: %f\n", values[PIGEON_WGI_TIMER_BLOOM_DOWNSAMPLE_DONE] - values[PIGEON_WGI_TIMER_RENDER_DONE]);
	printf("\tBloom Gaussian Blur: %f\n", values[PIGEON_WGI_TIMER_BLOOM_GAUSSIAN_BLUR_DONE] - values[PIGEON_WGI_TIMER_BLOOM_DOWNSAMPLE_DONE]);
	printf("\tPost Process: %f\n", values[PIGEON_WGI_TIMER_POST_PROCESS_DONE] - values[PIGEON_WGI_TIMER_BLOOM_GAUSSIAN_BLUR_DONE]);
	printf("Total: %f\n", values[PIGEON_WGI_TIMER_POST_PROCESS_DONE] - values[PIGEON_WGI_TIMER_START]);

	frame_counter = 0;
	memset(values, 0, sizeof values);

}


static void fps_camera_mouse_input(vec2 rotation)
{
	if (!mouse_grabbed)
		return;


	int mouse_x, mouse_y;
	pigeon_wgi_get_mouse_position(&mouse_x, &mouse_y);

	if (last_mouse_x == -1)
	{
		last_mouse_x = mouse_x;
		last_mouse_y = mouse_y;
	}

	rotation[0] -= (float)(mouse_y - last_mouse_y) * 0.001f;
	rotation[1] += (float)(mouse_x - last_mouse_x) * 0.001f;

	last_mouse_x = mouse_x;
	last_mouse_y = mouse_y;

	if (rotation[0] > glm_rad(80.0f))
	{
		rotation[0] = glm_rad(80.0f);
	}
	else if (rotation[0] < glm_rad(-80.0f))
	{
		rotation[0] = glm_rad(-80.0f);
	}
}

static void fps_camera_input(float delta_time, vec2 rotation, vec3 position)
{
	fps_camera_mouse_input(rotation);

	mat4 rotation_matrix;
	vec3 angles = {0, -rotation[1], 0.0f};
	glm_euler_zyx(angles, rotation_matrix);

	vec3 forwards = {0, 0, -1};
	glm_vec3_rotate_m4(rotation_matrix, forwards, forwards);

	vec3 right = {1, 0, 0};
	glm_vec3_rotate_m4(rotation_matrix, right, right);

	float speed = pigeon_wgi_is_key_down(PIGEON_WGI_KEY_RIGHT_SHIFT) ? 30.0f : 
		(pigeon_wgi_is_key_down(PIGEON_WGI_KEY_SLASH) ? 1.0f : 5.0f);

	if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_W))
	{
		for (unsigned int i = 0; i < 3; i++)
			position[i] += forwards[i] * speed * delta_time;
	}
	else if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_S))
	{
		for (unsigned int i = 0; i < 3; i++)
			position[i] += -forwards[i] * speed * delta_time;
	}

	if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_D))
	{
		for (unsigned int i = 0; i < 3; i++)
			position[i] += right[i] * speed * delta_time;
	}
	else if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_A))
	{
		for (unsigned int i = 0; i < 3; i++)
			position[i] += -right[i] * speed * delta_time;
	}

	if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_SPACE))
	{
		position[1] += speed * delta_time;
	}
	else if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_LEFT_SHIFT))
	{
		position[1] -= speed * delta_time;
	}
}


static PIGEON_ERR_RET game_loop(void)
{

	float last_frame_time = pigeon_wgi_get_time_seconds() - 1 / 60.0f;

	float last_fps_output_time = pigeon_wgi_get_time_seconds();
	unsigned int fps_frame_counter = 0;
	unsigned int frame_number = 0;

	vec2 rotation = {0};
	vec3 position = {0, 1.7f, 0};
	vec3 position_prev = {0, 1.7f, 0};


	while (!pigeon_wgi_close_requested() && !pigeon_wgi_is_key_down(PIGEON_WGI_KEY_ESCAPE))
	{
		double delayed_timer_values[PIGEON_WGI_TIMERS_COUNT];


		ASSERT_R1(!pigeon_wgi_next_frame_wait(delayed_timer_values));

		
		if(delayed_timer_values[0] > 0.0 || delayed_timer_values[PIGEON_WGI_TIMER_POST_PROCESS_DONE] > 0.0)
			print_timer_stats(delayed_timer_values);

		float time_now = pigeon_wgi_get_time_seconds();
		float delta_time = time_now - last_frame_time;
		last_frame_time = time_now;

		if(time_now - last_fps_output_time >= 1.0f) {
			printf("FPS: %u\n", fps_frame_counter);
			fps_frame_counter = 0;
			last_fps_output_time = time_now;
		}
		else {
			fps_frame_counter++;
		}


		// game logic


		float cube_rotation = fmodf((float) (time_now - start_time), 2.0f * 3.1315f);
		mat4 identity = GLM_MAT4_IDENTITY_INIT;
		glm_rotate_y(identity, cube_rotation, t_spinning_cube->matrix_transform);
		mat4 spin_cube_translation_mat4;
		vec3 spin_cube_translation_vec3 = {-3.0f, 0.5f, 0.1f};
		glm_translate_make(spin_cube_translation_mat4, spin_cube_translation_vec3);
		glm_mat4_mul(spin_cube_translation_mat4, t_spinning_cube->matrix_transform, t_spinning_cube->matrix_transform);
		pigeon_invalidate_world_transform(t_spinning_cube);


		memcpy(position_prev, position, sizeof(vec3));
		fps_camera_input(delta_time, rotation, position);
		set_camera_transform(rotation, position);

		PigeonWGICommandBuffer *upload_command_buffer = pigeon_wgi_get_upload_command_buffer();

		if (frame_number == 0)
		{
			ASSERT_R1(!pigeon_wgi_start_command_buffer(upload_command_buffer));
			pigeon_wgi_upload_multimesh(upload_command_buffer, &mesh);
			pigeon_wgi_upload_multimesh(upload_command_buffer, &mesh_skinned);
			ASSERT_R1(!create_array_textures(upload_command_buffer));
			ASSERT_R1(!pigeon_wgi_end_command_buffer(upload_command_buffer));
		}
		else if (frame_number == 1)
		{
			pigeon_wgi_multimesh_transfer_done(&mesh);
			pigeon_wgi_multimesh_transfer_done(&mesh_skinned);
		}	

		
		ArrayTexture * arr = array_texture_0;
		unsigned int array_texture_index = 0;
		while(arr) {
			if(frame_number == 1)
				pigeon_wgi_array_texture_transfer_done(&arr->t);
			pigeon_wgi_bind_array_texture(array_texture_index++, &arr->t);
			arr = arr->next;
		}


		ASSERT_R1(!pigeon_draw_frame(t_camera, debug_disable_ssao, &skybox_pipeline));
		ASSERT_R1(!pigeon_update_scene_audio(t_camera));


		ASSERT_R1(!pigeon_wgi_present_frame());

		frame_number++;
	}

	return 0;
}

int main(void)
{
	ASSERT_R1(!start());
	ASSERT_R1(!load_assets());
	ASSERT_R1(!create_multimeshes());
	ASSERT_R1(!create_pipelines());
	ASSERT_R1(!populate_array_textures());

	t_floor = pigeon_create_transform(NULL);
	t_character = pigeon_create_transform(NULL);
	t_spinning_cube = pigeon_create_transform(NULL);
	t_light = pigeon_create_transform(NULL);
	t_camera = pigeon_create_transform(NULL);
	t_sphere = pigeon_create_transform(NULL);
	t_pigeon = pigeon_create_transform(NULL);
	ASSERT_R1(t_floor && t_character && t_spinning_cube && t_light && t_camera && t_sphere && t_pigeon);

	t_floor->transform_type = PIGEON_TRANSFORM_TYPE_SRT;
	t_floor->scale[0] = 10000;
	t_floor->scale[1] = 0.1f;
	t_floor->scale[2] = 10000;
	t_floor->translation[1] = -0.05f;

	t_sphere->transform_type = PIGEON_TRANSFORM_TYPE_SRT;
	t_sphere->scale[0] = 1;
	t_sphere->scale[1] = 1;
	t_sphere->scale[2] = 1;
	t_sphere->translation[0] = 2;
	t_sphere->translation[1] = 0.5;
	t_sphere->translation[2] = -1;

	t_character->transform_type = PIGEON_TRANSFORM_TYPE_SRT;
	t_character->scale[0] = 1;
	t_character->scale[1] = 1;
	t_character->scale[2] = 1;
	t_character->translation[2] = -4;

	t_light->transform_type = PIGEON_TRANSFORM_TYPE_SRT;
	t_light->scale[0] = 1;
	t_light->scale[1] = 1;
	t_light->scale[2] = 1;
	t_light->translation[0] = 0;
	t_light->translation[1] = 10;
	t_light->translation[2] = -3;
	t_light->rotation[0] = 0.819152f;
	t_light->rotation[3] = -0.5735764f;


	// set each frame
	t_spinning_cube->transform_type = PIGEON_TRANSFORM_TYPE_MATRIX;
	t_camera->transform_type = PIGEON_TRANSFORM_TYPE_MATRIX;


	light = pigeon_create_light();
	ASSERT_R1(light);
	light->intensity[0] = light->intensity[1] = light->intensity[2] = 7;
	light->shadow_resolution = 2048;
	light->shadow_near = 6;
	light->shadow_far = 13;
	light->shadow_size_x = 5;
	light->shadow_size_y = 5;
	ASSERT_R1(!pigeon_join_transform_and_component(t_light, &light->c));


	rs_static_opaque = pigeon_create_render_state(&mesh, &render_pipeline);
	rs_static_transparent = pigeon_create_render_state(&mesh, &render_pipeline_transparent);
	rs_skinned_opaque = pigeon_create_render_state(&mesh_skinned, &render_pipeline_skinned);
	rs_skinned_transparent = pigeon_create_render_state(&mesh_skinned, &render_pipeline_skinned_transparent);

	ASSERT_R1(rs_static_opaque);
	ASSERT_R1(rs_static_transparent);
	ASSERT_R1(rs_skinned_opaque);
	ASSERT_R1(rs_skinned_transparent);

	model_cube = pigeon_create_model_renderer(&model_assets[0], 0);
	model_sphere = pigeon_create_model_renderer(&model_assets[2], 0);
	ASSERT_R1(model_cube && model_sphere);

	model_character = malloc(sizeof *model_character * model_assets[1].materials_count);
	ASSERT_R1(model_character);

	ASSERT_R1(!pigeon_join_rs_model(rs_static_opaque, model_cube));
	ASSERT_R1(!pigeon_join_rs_model(rs_static_opaque, model_sphere));

	mr_floor = pigeon_create_material_renderer(model_cube);
	mr_spinning_cube = pigeon_create_material_renderer(model_cube);
	mr_sphere = pigeon_create_material_renderer(model_sphere);
	ASSERT_R1(mr_floor && mr_spinning_cube && mr_sphere);

	mr_spinning_cube->colour[0] = 1.1f;
	mr_spinning_cube->colour[1] = 0.5f;
	mr_spinning_cube->colour[2] = 1.15f;

	mr_sphere->colour[0] = 0.15f;
	mr_sphere->colour[1] = 0.5f;
	mr_sphere->colour[2] = 2.1f;

	mr_character = malloc(sizeof *mr_character * model_assets[1].materials_count);
	ASSERT_R1(mr_character);

	as_character = pigeon_create_animation_state(&model_assets[1]);
	ASSERT_R1(as_character);
	as_character->animation_start_time = pigeon_wgi_get_time_seconds_double();
	as_character->animation_index = 1;

	audio_pigeon = pigeon_create_audio_player();
	ASSERT_R1(audio_pigeon);
	ASSERT_R1(!pigeon_join_transform_and_component(t_pigeon, &audio_pigeon->c));


	for(unsigned int i = 0; i < model_assets[1].materials_count; i++) {
		model_character[i] = pigeon_create_model_renderer(&model_assets[1], i);
		ASSERT_R1(model_character[i]);


		mr_character[i] = pigeon_create_material_renderer(model_character[i]);
		ASSERT_R1(mr_character[i]);


		TextureIndices * ti = &model_texture_indices[1][i];

		if(ti->texture_index != UINT32_MAX) {
			TextureLocation * tl = &texture_locations[ti->texture_index];
			mr_character[i]->diffuse_bind_point = tl->bind_point;
			mr_character[i]->diffuse_layer = tl->layer;
			if(texture_assets[ti->texture_index].texture_meta.has_alpha)
				mr_character[i]->use_transparency = true;
		}
		if(ti->normal_index != UINT32_MAX) {
			TextureLocation * tl = &texture_locations[ti->normal_index];
			mr_character[i]->nmap_bind_point = tl->bind_point;
			mr_character[i]->nmap_layer = tl->layer;			
		}

		if(mr_character[i]->use_transparency) {
			ASSERT_R1(!pigeon_join_rs_model(rs_skinned_transparent, model_character[i]));
		} else {
			ASSERT_R1(!pigeon_join_rs_model(rs_skinned_opaque, model_character[i]));
		}

		ASSERT_R1(!pigeon_join_mr_anim(mr_character[i], as_character));

		ASSERT_R1(!pigeon_join_transform_and_component(t_character, &mr_character[i]->c));
	}

	ASSERT_R1(!pigeon_join_transform_and_component(t_floor, &mr_floor->c));
	ASSERT_R1(!pigeon_join_transform_and_component(t_spinning_cube, &mr_spinning_cube->c));
	ASSERT_R1(!pigeon_join_transform_and_component(t_sphere, &mr_sphere->c));

	int err = game_loop();



	pigeon_wgi_wait_idle();

	pigeon_destroy_light(light);
	pigeon_destroy_animation_state(as_character);
	pigeon_destroy_audio_player(audio_pigeon);

	pigeon_destroy_material_renderer(mr_floor);
	pigeon_destroy_material_renderer(mr_spinning_cube);
	
	for(unsigned int i = 0; i < model_assets[1].materials_count; i++) {
		pigeon_destroy_material_renderer(mr_character[i]);
		pigeon_destroy_model_renderer(model_character[i]);
	}

	pigeon_destroy_model_renderer(model_cube);

	pigeon_destroy_render_state(rs_static_opaque);
	pigeon_destroy_render_state(rs_static_transparent);
	pigeon_destroy_render_state(rs_skinned_opaque);
	pigeon_destroy_render_state(rs_skinned_transparent);

	destroy_array_textures();
	destroy_pipelines();
	destroy_multimeshes();
	free_assets();
	audio_cleanup();
	pigeon_wgi_deinit();
	pigeon_deinit();

	return err;
	return 0;
}
