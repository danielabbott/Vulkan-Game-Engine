#include <pigeon/wgi/wgi.h>
#include <pigeon/wgi/pipeline.h>
#include <pigeon/util.h>
#include <pigeon/wgi/input.h>
#include <pigeon/wgi/mesh.h>
#include <pigeon/asset.h>
#include <cglm/mat4.h>
#include <cglm/clipspace/persp_rh_zo.h>
#include <cglm/euler.h>
#include <cglm/affine.h>


#define NUM_MODELS 2

#define ASSET_PATH(x) ("build/test_assets/models/" x ".asset")
#define ASSET_PATH2(x) ("build/standard_assets/models/" x ".asset")
#define ASSET_PATH3(x) ("build/test_assets/models/" x ".data")
#define ASSET_PATH4(x) ("build/standard_assets/models/" x ".data")

const char *model_file_paths[NUM_MODELS][2] = {
	{ASSET_PATH2("cube.blend"), ASSET_PATH4("cube.blend")},
	{ASSET_PATH("alien.blend"), ASSET_PATH3("alien.blend")},
};

#undef ASSET_PATH

PigeonAsset model_assets[NUM_MODELS];

typedef struct Mat
{
	// UINT32_MAX if not used, otherwise index into texture_assets and texture_asset_grid_positions
	uint32_t texture_index;
	uint32_t normal_index;

	uint32_t index; // index into uniform array
} Mat;

Mat *model_materials[NUM_MODELS]; // 1 for each material in each model
unsigned int total_material_count;

unsigned int texture_assets_count;
PigeonAsset *texture_assets;

unsigned int tiles_needed;
PigeonWGIGridTexture grid_texture;
PigeonWGIGridTextureGrid *grid_texture_grids;
unsigned int grid_texture_grids_count;

unsigned int normal_tiles_needed;
PigeonWGIGridTexture normal_grid_texture;
PigeonWGIGridTextureGrid *normal_grid_texture_grids;
unsigned int normal_grid_texture_grids_count;

PigeonWGITextureGridPosition *texture_asset_grid_positions; // One for each texture asset

typedef struct GO
{
	unsigned int model_index;
	vec3 position;
	vec3 scale;
	vec3 rotation;
	vec3 colour;
} GO;

#define NUM_GAME_OBJECTS 3
GO game_objects[NUM_GAME_OBJECTS] = {
	{0, {0, -0.05f, 0}, {10000, 0.1f, 10000}, {0, 0, 0}, {0.7f, 0.7f, 0.7f}},
	{0, {-3, 0.5f, 1}, {1, 1, 1}, {0, -0.2f, 0}, {0.7f, 0.7f, 0.7f}},
	{1, {0, 0, -4}, {1, 1, 1}, {0, 0.1f, 0}, {0.7f, 0.7f, 0.7f}},
};

PigeonWGIMultiMesh mesh;

PigeonWGIPipeline skybox_pipeline;
PigeonWGIPipeline render_pipeline;

bool mouse_grabbed;
int last_mouse_x = -1;
int last_mouse_y = -1;

/* Creates window and vulkan context */
static ERROR_RETURN_TYPE start(void)
{
	PigeonWindowParameters window_parameters = {
		.width = 800,
		.height = 600,
		.window_mode = PIGEON_WINDOW_MODE_WINDOWED,
		.title = "Test 1"};

	if (pigeon_wgi_init(window_parameters, true))
	{
		pigeon_wgi_deinit();
		return 1;
	}

	return 0;
}

static ERROR_RETURN_TYPE load_texture_asset(const char *asset_name, PigeonAsset *asset, bool normals)
{
	const char *prefix = "build/test_assets/textures/";
	const size_t prefix_len = strlen(prefix);
	const size_t asset_name_len = strlen(asset_name);
	const char *suffix = ".asset";
	const size_t suffix_len = strlen(suffix);

	char *meta_file_path = malloc(prefix_len + asset_name_len + suffix_len + 1);
	ASSERT_1(meta_file_path);

	memcpy(meta_file_path, prefix, prefix_len);
	memcpy(&meta_file_path[prefix_len], asset_name, asset_name_len);
	memcpy(&meta_file_path[prefix_len + asset_name_len], suffix, suffix_len + 1);

	ASSERT_1(!pigeon_load_asset_meta(asset, meta_file_path));
	ASSERT_1(asset->type == PIGEON_ASSET_TYPE_IMAGE);
	if (normals)
	{
		ASSERT_1(asset->texture_meta.format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR && asset->texture_meta.has_bc5);
	}
	else
	{
		ASSERT_1(asset->texture_meta.format == PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB && asset->texture_meta.has_bc1);
	}
	ASSERT_1(asset->texture_meta.width % 512 == 0);
	ASSERT_1(asset->texture_meta.height % 512 == 0);
	ASSERT_1(asset->texture_meta.has_mip_maps);

	char *data_file_path = meta_file_path;
	memcpy(&data_file_path[prefix_len + asset_name_len], ".data", 6);

	ASSERT_1(!pigeon_load_asset_data(asset, data_file_path));

	unsigned int t_w = (asset->texture_meta.width + 511) / 512;
	unsigned int t_h = (asset->texture_meta.height + 511) / 512;
	if (normals)
		normal_tiles_needed += t_w * t_h;
	else
		tiles_needed += t_w * t_h;

	return 0;
}

static ERROR_RETURN_TYPE load_model_assets(void)
{
	texture_assets_count = 0;
	for (unsigned int i = 0; i < NUM_MODELS; i++)
	{
		ASSERT_1(!pigeon_load_asset_meta(&model_assets[i], model_file_paths[i][0]));
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
		ASSERT_1(model_assets[i].type == PIGEON_ASSET_TYPE_MODEL);
		ASSERT_1(model_assets[i].mesh_meta.attribute_types[0] == PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED &&
			model_assets[i].mesh_meta.attribute_types[1] == PIGEON_WGI_VERTEX_ATTRIBUTE_NORMAL &&
			model_assets[i].mesh_meta.attribute_types[2] == PIGEON_WGI_VERTEX_ATTRIBUTE_TANGENT &&
			model_assets[i].mesh_meta.attribute_types[3] == PIGEON_WGI_VERTEX_ATTRIBUTE_UV &&
			!model_assets[i].mesh_meta.attribute_types[4] &&
			!model_assets[i].mesh_meta.big_indices
		);
		ASSERT_1(!pigeon_load_asset_data(&model_assets[i], model_file_paths[i][1]));
	}
	return 0;
}

static ERROR_RETURN_TYPE load_texture_assets()
{
	texture_assets = calloc(texture_assets_count, sizeof *texture_assets);
	ASSERT_1(texture_assets);

	texture_asset_grid_positions = malloc(texture_assets_count * sizeof *texture_asset_grid_positions);

	unsigned int tex_i = 0;
	unsigned int material_index = 0;
	for (unsigned int i = 0; i < NUM_MODELS; i++)
	{
		total_material_count += model_assets[i].materials_count;
		model_materials[i] = calloc(model_assets[i].materials_count, sizeof *model_materials[i]);
		ASSERT_1(model_materials[i]);

		for (unsigned int j = 0; j < model_assets[i].materials_count; j++)
		{
			model_materials[i][j].index = material_index++;
			if (model_assets[i].materials[j].texture)
			{
				model_materials[i][j].texture_index = tex_i;
				ASSERT_1(!load_texture_asset(model_assets[i].materials[j].texture, &texture_assets[tex_i],
											 false));

				tex_i++;
			}
			else
			{
				model_materials[i][j].texture_index = UINT32_MAX;
			}
			if (model_assets[i].materials[j].normal_map_texture)
			{
				model_materials[i][j].normal_index = tex_i;
				ASSERT_1(!load_texture_asset(model_assets[i].materials[j].normal_map_texture,
											 &texture_assets[tex_i], true));

				tex_i++;
			}
			else
			{
				model_materials[i][j].normal_index = UINT32_MAX;
			}
		}
	}
	return 0;
}

static ERROR_RETURN_TYPE load_assets(void)
{
	ASSERT_1(!load_model_assets());

	// This assumes that textures are not shared between models

	if (!texture_assets_count)
	{
		return 0;
	}

	ASSERT_1(!load_texture_assets());
	return 0;
}

static int texture_asset_compare(const void *i0_, const void *i1_)
{
	const unsigned int *i0 = i0_;
	const unsigned int *i1 = i1_;

	PigeonAsset *a0 = &texture_assets[*i0];
	PigeonAsset *a1 = &texture_assets[*i1];

	unsigned int sz0 = a0->texture_meta.width * a0->texture_meta.height;
	unsigned int sz1 = a1->texture_meta.width * a1->texture_meta.height;

	return (int)sz1 - (int)sz0;
}

static ERROR_RETURN_TYPE populate_grids(void)
{
	ASSERT_1(!pigeon_wgi_allocate_empty_grids(&grid_texture_grids, &grid_texture_grids_count, tiles_needed));
	ASSERT_1(!pigeon_wgi_allocate_empty_grids(&normal_grid_texture_grids, &normal_grid_texture_grids_count, normal_tiles_needed));

	unsigned int *asset_sorted_indices = malloc(texture_assets_count * sizeof *asset_sorted_indices);
	ASSERT_1(asset_sorted_indices);

	for (unsigned int i = 0; i < texture_assets_count; i++)
	{
		asset_sorted_indices[i] = i;
	}

	qsort(asset_sorted_indices, texture_assets_count, sizeof *asset_sorted_indices, texture_asset_compare);

	for (unsigned int j = 0; j < texture_assets_count; j++)
	{
		unsigned int i = asset_sorted_indices[j];

		PigeonAsset *asset = &texture_assets[i];
		unsigned int w = (asset->texture_meta.width + 511) / 512;
		unsigned int h = (asset->texture_meta.height + 511) / 512;
		if (asset->texture_meta.format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR)
		{
			ASSERT_1(!pigeon_wgi_add_texture_to_grid(&normal_grid_texture_grids, &normal_grid_texture_grids_count,
													 asset, w, h, &texture_asset_grid_positions[i], true));
		}
		else
		{
			ASSERT_1(!pigeon_wgi_add_texture_to_grid(&grid_texture_grids, &grid_texture_grids_count,
													 asset, w, h, &texture_asset_grid_positions[i], true));
		}
	}
	free(asset_sorted_indices);
	return 0;
}

// Runs in first frame
static ERROR_RETURN_TYPE create_grid_texture(PigeonWGICommandBuffer *cmd)
{
	ASSERT_1(!populate_grids());

	ASSERT_1(!pigeon_wgi_create_grid_texture(&grid_texture, grid_texture_grids_count,
											 PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB, cmd));
	ASSERT_1(!pigeon_wgi_create_grid_texture(&normal_grid_texture, normal_grid_texture_grids_count,
											 PIGEON_WGI_IMAGE_FORMAT_BC5, cmd));

	for (unsigned int i = 0; i < texture_assets_count; i++)
	{
		PigeonAsset *asset = &texture_assets[i];

		PigeonWGIImageFormat format = asset->texture_meta.format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR ? PIGEON_WGI_IMAGE_FORMAT_BC5 : PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB;

		ASSERT_1(pigeon_wgi_get_texture_size(asset->texture_meta.width, asset->texture_meta.height,
											 format, true) == asset->compression[1].decompressed_data_length);

		PigeonWGIGridTexture *grid_tex = asset->texture_meta.format == PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR ? &normal_grid_texture : &grid_texture;

		void *dst = pigeon_wgi_grid_texture_upload(grid_tex, texture_asset_grid_positions[i],
												   asset->texture_meta.width, asset->texture_meta.height, cmd);

		ASSERT_1(!pigeon_decompress_asset(asset, dst, 1));
	}
	pigeon_wgi_grid_texture_transition(&normal_grid_texture, cmd);
	pigeon_wgi_grid_texture_transition(&grid_texture, cmd);

	pigeon_wgi_grid_texture_unmap(&normal_grid_texture);
	pigeon_wgi_grid_texture_unmap(&grid_texture);

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
	for (unsigned int i = 0; i < NUM_MODELS; i++)
	{
		pigeon_free_asset(&model_assets[i]);
	}
}

static ERROR_RETURN_TYPE create_meshes(void)
{
	uint64_t size = 0;
    unsigned int vertex_count = 0;
	unsigned int index_count = 0;

	for (unsigned int i = 0; i < NUM_MODELS; i++)
	{
		uint64_t sz = pigeon_wgi_mesh_meta_size_requirements(&model_assets[i].mesh_meta);

		uint64_t full_size = 0;
		for(unsigned int j = 0; j < PIGEON_ASSET_MAX_SUBRESOURCES; j++) {
			full_size += model_assets[i].compression[j].decompressed_data_length;
		}
		ASSERT_1(full_size == sz);

		model_assets[i].mesh_meta.multimesh_start_vertex = vertex_count;
		model_assets[i].mesh_meta.multimesh_start_index = index_count;
		vertex_count += model_assets[i].mesh_meta.vertex_count;
		index_count += model_assets[i].mesh_meta.index_count ?
			model_assets[i].mesh_meta.index_count :
			model_assets[i].mesh_meta.vertex_count;
	}

	uint8_t *mapping;
	ASSERT_1(!pigeon_wgi_create_multimesh(&mesh,
		model_assets[0].mesh_meta.attribute_types, vertex_count, index_count, false,
		&size, (void **)&mapping));

	PigeonWGIVertexAttributeType * attributes = model_assets[0].mesh_meta.attribute_types;
	uint64_t offset = 0;
	unsigned int j = 0;
	for (; j < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; j++) {
		if(!attributes[j]) break;

		for (unsigned int i = 0; i < NUM_MODELS; i++) {
			uint64_t sz = pigeon_wgi_get_vertex_attribute_type_size(attributes[j]) * model_assets[i].mesh_meta.vertex_count;
			ASSERT_1(sz == model_assets[i].compression[j].decompressed_data_length);
			ASSERT_1(!pigeon_decompress_asset(&model_assets[i], &mapping[offset], j));
			offset += sz;
		}
	}
	for (unsigned int i = 0; i < NUM_MODELS; i++) {
		if(j < model_assets[i].compression[j].decompressed_data_length) {
			ASSERT_1(!pigeon_decompress_asset(&model_assets[i], &mapping[offset], j));
			offset += 2 * model_assets[i].mesh_meta.index_count;
		}
		else {
			// Flat shaded model- create indices
			for(unsigned int k = 0; k < model_assets[i].mesh_meta.vertex_count; k++) {
				mapping[offset++] = (uint8_t)k;
				mapping[offset++] = (uint8_t)(k>>8);
			}
		}
	}
	assert(offset == size);

	ASSERT_1(!pigeon_wgi_multimesh_unmap(&mesh));
	mapping = NULL;

	return 0;
}

static void upload_meshes(PigeonWGICommandBuffer *cmd)
{
	pigeon_wgi_upload_multimesh(cmd, &mesh);
}

static void destroy_meshes(void)
{
	if (mesh.staged_buffer)
		pigeon_wgi_destroy_multimesh(&mesh);
}

static ERROR_RETURN_TYPE create_pipeline(PigeonWGIPipeline *pipeline,
										 const char *vs_shader_path, const char *vs_depth_only_shader_path, const char *fs_shader_path,
										 PigeonWGIPipelineConfig *config)
{
	bool has_depth_only = vs_depth_only_shader_path != NULL;

	unsigned long vs_spv_len = 0, fs_spv_len = 0, vs_depth_spv_len = 0;

	uint32_t *vs_spv = (uint32_t *)load_file(vs_shader_path, 0, &vs_spv_len);
	ASSERT__1(vs_spv, "Error loading vertex shader");

	uint32_t *fs_spv = (uint32_t *)load_file(fs_shader_path, 0, &fs_spv_len);
	if (!fs_spv)
	{
		ERRLOG("Error loading fragment shader");
		free(vs_spv);
		return 1;
	}

	uint32_t *vs_depth_spv = NULL;

	if (has_depth_only)
	{
		vs_depth_spv = (uint32_t *)load_file(vs_depth_only_shader_path, 0, &vs_depth_spv_len);
		if (!vs_depth_spv)
		{
			ERRLOG("Error loading depth-only vertex shader");
			free(vs_spv);
			free(fs_spv);
			return 1;
		}
	}

	PigeonWGIShader vs = { 0 }, vs_depth = { 0 }, fs = { 0 };
	if (pigeon_wgi_create_shader(&vs, vs_spv, (uint32_t)vs_spv_len, PIGEON_WGI_SHADER_TYPE_VERTEX))
	{
		free(vs_spv);
		if (has_depth_only)
			free(vs_depth_spv);
		free(fs_spv);
		return 1;
	}
	if (pigeon_wgi_create_shader(&fs, fs_spv, (uint32_t)fs_spv_len, PIGEON_WGI_SHADER_TYPE_FRAGMENT))
	{
		free(vs_spv);
		if (has_depth_only)
			free(vs_depth_spv);
		free(fs_spv);
		pigeon_wgi_destroy_shader(&vs);
		return 1;
	}

	if (has_depth_only)
	{
		if (pigeon_wgi_create_shader(&vs_depth, vs_depth_spv, (uint32_t)vs_depth_spv_len, PIGEON_WGI_SHADER_TYPE_VERTEX))
		{
			free(vs_spv);
			free(vs_depth_spv);
			free(fs_spv);
			pigeon_wgi_destroy_shader(&vs);
			pigeon_wgi_destroy_shader(&fs);
			return 1;
		}
	}

	int err = pigeon_wgi_create_pipeline(pipeline, &vs,
										 has_depth_only ? &vs_depth : NULL,
										 &fs, config);

	pigeon_wgi_destroy_shader(&vs);
	if (has_depth_only)
		pigeon_wgi_destroy_shader(&vs_depth);
	pigeon_wgi_destroy_shader(&fs);
	free(vs_spv);
	if (has_depth_only)
		free(vs_depth_spv);
	free(fs_spv);

	ASSERT_1(!err);
	return 0;
}

static ERROR_RETURN_TYPE verify_model_vertex_attribs(void)
{
	// Check all models have the same attributes
	for (unsigned int i = 1; i < NUM_MODELS; i++)
	{
		if (memcmp(model_assets[i].mesh_meta.attribute_types,
				   model_assets[0].mesh_meta.attribute_types,
				   sizeof model_assets[0].mesh_meta.attribute_types) != 0)
		{
			ASSERT__1(false, "All models must have same vertex attributes");
		}
	}
	return 0;
}

static ERROR_RETURN_TYPE create_pipelines(void)
{
#ifdef NDEBUG
#define SHADER_PATH(x) ("build/release/standard_assets/shaders/" x ".spv")
#else
#define SHADER_PATH(x) ("build/debug/standard_assets/shaders/" x ".spv")
#endif

	PigeonWGIPipelineConfig config = {0};

	ASSERT_1(!create_pipeline(&skybox_pipeline,
							  SHADER_PATH("skybox.vert"), NULL, SHADER_PATH("skybox.frag"), &config));

	ASSERT_1(!verify_model_vertex_attribs());

	config.depth_write = true;
	config.depth_test = true;
	// config.cull_mode = PIGEON_WGI_CULL_MODE_BACK;
	memcpy(config.vertex_attributes, model_assets[0].mesh_meta.attribute_types,
		   sizeof config.vertex_attributes);

	ASSERT_1(!create_pipeline(&render_pipeline,
							  SHADER_PATH("object.vert"), SHADER_PATH("object.vert.depth"), SHADER_PATH("object.frag"), &config));

	return 0;

#undef SHADER_PATH
}

static void destroy_pipelines(void)
{
	if (skybox_pipeline.pipeline)
		pigeon_wgi_destroy_pipeline(&skybox_pipeline);
	if (render_pipeline.pipeline)
		pigeon_wgi_destroy_pipeline(&render_pipeline);
}

static ERROR_RETURN_TYPE render_frame(PigeonWGICommandBuffer *command_buffer, bool depth_only, unsigned int multidraw_draw_count)
{
	assert(command_buffer);

	ASSERT_1(!pigeon_wgi_start_command_buffer(command_buffer));

	if (!depth_only) {
		pigeon_wgi_draw_without_mesh(command_buffer, &skybox_pipeline, 3);
	}

	if (depth_only) {
		unsigned int draw_calls = 0;
		for (unsigned int i = 0; i < NUM_MODELS; i++) {
			PigeonAsset *model = &model_assets[i];

			for (unsigned int j = 0; j < model->materials_count; j++) {
				unsigned int obj_count = 0;
				for (unsigned int k = 0; k < NUM_GAME_OBJECTS; k++) {
					if (game_objects[k].model_index == i) obj_count++;
				}			

				pigeon_wgi_multidraw_draw(model->mesh_meta.multimesh_start_vertex,
					obj_count, model->mesh_meta.multimesh_start_index + model->materials[j].first, model->materials[j].count);
				draw_calls += obj_count;	
			}
		}
		ASSERT_1(draw_calls == multidraw_draw_count);
		pigeon_wgi_multidraw_submit(command_buffer, &render_pipeline, &mesh, 0, multidraw_draw_count, 0);
	}
	else {
		pigeon_wgi_multidraw_submit(command_buffer, &render_pipeline, &mesh, 0, multidraw_draw_count, 0);
	}

	ASSERT_1(!pigeon_wgi_end_command_buffer(command_buffer));
	return 0;
}

static void set_uniforms(PigeonWGISceneUniformData *scene_uniform_data, vec2 rotation, vec3 position,
						 PigeonWGIDrawCallObject *draw_call_objects, unsigned int total_draw_calls)
{
#ifdef NDEBUG
	(void)total_draw_calls;
#endif

	unsigned int window_width, window_height;
	pigeon_wgi_get_window_dimensions(&window_width, &window_height);

	vec3 inverted_angles = {-rotation[0], rotation[1], 0.0f};
	vec3 inverted_position = {-position[0], -position[1], -position[2]};

	glm_mat4_identity(scene_uniform_data->view);
	glm_translate(scene_uniform_data->view, inverted_position);

	mat4 rot_mat;
	glm_euler_xyz(inverted_angles, rot_mat);
	glm_mat4_mul(rot_mat, scene_uniform_data->view, scene_uniform_data->view);

	pigeon_wgi_perspective(scene_uniform_data->proj, 45, (float)window_width / (float)window_height, 0.1f);

	glm_mat4_mul(scene_uniform_data->proj, scene_uniform_data->view, scene_uniform_data->proj_view);
	scene_uniform_data->viewport_size[0] = (float)window_width;
	scene_uniform_data->viewport_size[1] = (float)window_height;
	scene_uniform_data->one_pixel_x = 1.0f / (float)window_width;
	scene_uniform_data->one_pixel_y = 1.0f / (float)window_height;
	scene_uniform_data->time = pigeon_wgi_get_time_seconds();
	scene_uniform_data->ambient[0] = 1.7f;
	scene_uniform_data->ambient[1] = 1.7f;
	scene_uniform_data->ambient[2] = 1.7f;
	// scene_uniform_data->ambient[0] = 0.05f;
	// scene_uniform_data->ambient[1] = 0.05f;
	// scene_uniform_data->ambient[2] = 0.05f;

	scene_uniform_data->number_of_lights = 1;
	PigeonWGILight *light = &scene_uniform_data->lights[0];
	light->intensity[0] = 12;
	light->intensity[1] = 12;
	light->intensity[2] = 12;
	// light->intensity[0] = 0.1f;
	// light->intensity[1] = 0.1f;
	// light->intensity[2] = 0.1f;
	// light->intensity[0] = 15;
	// light->intensity[1] = 15;
	// light->intensity[2] = 15;
	light->neg_direction[0] = 0.19245f;
	light->neg_direction[1] = 0.96225f;
	light->neg_direction[2] = 0.19245f;
	light->is_shadow_caster = 0;

	unsigned int draw_call_index = 0;
	for (unsigned int i = 0; i < NUM_MODELS; i++)
	{
		PigeonAsset *model_asset = &model_assets[i];
		for (unsigned int j = 0; j < model_asset->materials_count; j++)
		{
			for (unsigned int k = 0; k < NUM_GAME_OBJECTS; k++)
			{
				if (game_objects[k].model_index != i)
					continue;

				PigeonWGIDrawCallObject *data = &draw_call_objects[draw_call_index++];

				/* Mesh metadata */
				memcpy(data->position_min, model_asset->mesh_meta.bounds_min, 3 * 4);
				memcpy(data->position_range, model_asset->mesh_meta.bounds_range, 3 * 4);

				/* Object data */

				data->ssao_intensity = 45;

				glm_mat4_identity(data->model);
				glm_scale(data->model, game_objects[k].scale);

				glm_euler_xyz(game_objects[k].rotation, rot_mat);
				glm_mat4_mul(rot_mat, data->model, data->model);

				mat4 translation;
				glm_translate_make(translation, game_objects[k].position);
				glm_mat4_mul(translation, data->model, data->model);

				pigeon_wgi_get_normal_model_matrix(data->model, data->normal_model_matrix);

				glm_mat4_mul(scene_uniform_data->view, data->model, data->view_model);
				glm_mat4_mul(scene_uniform_data->proj_view, data->model, data->proj_view_model);

				/* Material data */

				memcpy(data->colour, model_asset->materials[j].colour, 3 * 4);

				unsigned int texture_asset_index = model_materials[i][j].texture_index;
				unsigned int normal_asset_index = model_materials[i][j].normal_index;

				if (texture_asset_index != UINT32_MAX)
				{
					PigeonWGITextureGridPosition *p = &texture_asset_grid_positions[texture_asset_index];
					data->texture_sampler_index_plus1 = 1;
					data->texture_index = (float)p->grid_i;
					data->texture_uv_base_and_range[0] = (float)p->grid_x / 8.0f;
					data->texture_uv_base_and_range[1] = (float)p->grid_y / 8.0f;
					data->texture_uv_base_and_range[2] = (float)p->grid_w / 8.0f;
					data->texture_uv_base_and_range[3] = (float)p->grid_h / 8.0f;
				}
				else
				{
					data->texture_sampler_index_plus1 = 0;
				}
				if (normal_asset_index != UINT32_MAX)
				{
					PigeonWGITextureGridPosition *p = &texture_asset_grid_positions[normal_asset_index];
					data->normal_map_sampler_index_plus1 = 2;
					data->normal_map_index = (float)p->grid_i;
					data->normal_map_uv_base_and_range[0] = (float)p->grid_x / 8.0f;
					data->normal_map_uv_base_and_range[1] = (float)p->grid_y / 8.0f;
					data->normal_map_uv_base_and_range[2] = (float)p->grid_w / 8.0f;
					data->normal_map_uv_base_and_range[3] = (float)p->grid_h / 8.0f;
				}
				else
				{
					data->normal_map_sampler_index_plus1 = 0;
				}
			}
		}
	}
	assert(total_draw_calls == draw_call_index);
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

	float speed = pigeon_wgi_is_key_down(PIGEON_WGI_KEY_RIGHT_SHIFT) ? 30.0f : 5.0f;

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

static unsigned int get_total_draw_calls(void)
{
	unsigned total_draw_calls = 0;
	for (unsigned int i = 0; i < NUM_MODELS; i++)
	{
		PigeonAsset *model = &model_assets[i];
		for (unsigned int k = 0; k < NUM_GAME_OBJECTS; k++)
		{
			if (game_objects[k].model_index == i)
			{
				total_draw_calls += model->materials_count;
			}
		}
	}
	return total_draw_calls;
}

static ERROR_RETURN_TYPE recreate_swapchain(void)
{
	while (true) {
		int err = pigeon_wgi_recreate_swapchain();
		if (err == 2)
		{
			pigeon_wgi_wait_events();
			continue;
		}
		else if (err)
		{
			return 1;
		}
		break;
	}
	return 0;
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
	printf("\tSSAO and Shadow Maps: %f\n", values[PIGEON_WGI_TIMER_SSAO_AND_SHADOW_DONE] - values[PIGEON_WGI_TIMER_DEPTH_PREPASS_DONE]);
	printf("\tSSAO blur: %f\n", values[PIGEON_WGI_TIMER_SSAO_BLUR_DONE] - values[PIGEON_WGI_TIMER_SSAO_AND_SHADOW_DONE]);
	printf("\tRender: %f\n", values[PIGEON_WGI_TIMER_RENDER_DONE] - values[PIGEON_WGI_TIMER_SSAO_BLUR_DONE]);
	printf("\tBloom Downsample: %f\n", values[PIGEON_WGI_TIMER_BLOOM_DOWNSAMPLE_DONE] - values[PIGEON_WGI_TIMER_RENDER_DONE]);
	printf("\tBloom Gaussian Blur: %f\n", values[PIGEON_WGI_TIMER_BLOOM_GAUSSIAN_BLUR_DONE] - values[PIGEON_WGI_TIMER_BLOOM_DOWNSAMPLE_DONE]);
	printf("\tPost Process: %f\n", values[PIGEON_WGI_TIMER_POST_PROCESS_DONE] - values[PIGEON_WGI_TIMER_BLOOM_GAUSSIAN_BLUR_DONE]);
	printf("Total: %f\n", values[PIGEON_WGI_TIMER_POST_PROCESS_DONE] - values[PIGEON_WGI_TIMER_START]);

	frame_counter = 0;
	memset(values, 0, sizeof values);

}

// TODO ERROR_RETURN_TYPE
static void game_loop(void)
{
	unsigned total_draw_calls = get_total_draw_calls();

	PigeonWGISceneUniformData scene_uniform_data = {0};
	PigeonWGIDrawCallObject *draw_call_objects = calloc(total_draw_calls, sizeof *draw_call_objects);
	if (!draw_call_objects)
		return;

	vec2 rotation = {0};
	vec3 position = {0, 1.7f, 0};

	float last_frame_time = pigeon_wgi_get_time_seconds() - 1 / 60.0f;

	unsigned int frame_number = 0;
	while (!pigeon_wgi_close_requested() && !pigeon_wgi_is_key_down(PIGEON_WGI_KEY_ESCAPE))
	{
		double delayed_timer_values[PIGEON_WGI_TIMERS_COUNT];

		int start_frame_err = pigeon_wgi_start_frame(true, total_draw_calls, total_draw_calls, delayed_timer_values);
		if (start_frame_err == 3)
		{
			if(recreate_swapchain()) return;
			start_frame_err = pigeon_wgi_start_frame(true, total_draw_calls, total_draw_calls, delayed_timer_values);
		}
		if (start_frame_err)
		{
			ERRLOG("start frame error");
			return;
		}

		if(delayed_timer_values[0] > 0.0 || delayed_timer_values[PIGEON_WGI_TIMER_POST_PROCESS_DONE] > 0.0)
			print_timer_stats(delayed_timer_values);

		float time_now = pigeon_wgi_get_time_seconds();
		float delta_time = time_now - last_frame_time;
		last_frame_time = time_now;

		game_objects[1].rotation[1] = fmodf(game_objects[1].rotation[1] + delta_time, 2.0f * 3.1315f);

		fps_camera_input(delta_time, rotation, position);

		set_uniforms(&scene_uniform_data, rotation, position, draw_call_objects, total_draw_calls);
		if (pigeon_wgi_set_uniform_data(&scene_uniform_data, draw_call_objects, total_draw_calls))
			return;

		PigeonWGICommandBuffer *upload_command_buffer = pigeon_wgi_get_upload_command_buffer();
		PigeonWGICommandBuffer *depth_command_buffer = pigeon_wgi_get_depth_command_buffer();
		PigeonWGICommandBuffer *render_command_buffer = pigeon_wgi_get_render_command_buffer();

		if (frame_number == 0)
		{
			if (pigeon_wgi_start_command_buffer(upload_command_buffer))
				return;
			upload_meshes(upload_command_buffer);
			if (create_grid_texture(upload_command_buffer))
				return;
			if (pigeon_wgi_end_command_buffer(upload_command_buffer))
				return;
		}
		else if (frame_number == 1)
		{
			pigeon_wgi_grid_texture_transfer_done(&normal_grid_texture);
			pigeon_wgi_grid_texture_transfer_done(&grid_texture);
			pigeon_wgi_multimesh_transfer_done(&mesh);
		}

		// TODO don't rebind every time if possible-
		// ^ still need to bind 2+ times though (once for each possible frame in flight)
		pigeon_wgi_bind_grid_texture(0, &grid_texture);
		pigeon_wgi_bind_grid_texture(1, &normal_grid_texture);

		if (render_frame(depth_command_buffer, true, total_draw_calls))
			return;
		if (render_frame(render_command_buffer, false, total_draw_calls))
			return;

		int present_frame_error = pigeon_wgi_present_frame();
		if (present_frame_error == 2)
		{
			if (recreate_swapchain()) return;
		}
		else if (present_frame_error)
		{
			ERRLOG("start frame error");
			return;
		}

		// printf("%d\n", frames);
		frame_number++;
	}
}

static void mouse_callback(PigeonWGIMouseEvent e)
{
	if (e.button == PIGEON_WGI_MOUSE_BUTTON_RIGHT && !e.pressed)
	{
		mouse_grabbed = !mouse_grabbed;
		pigeon_wgi_set_cursor_type(mouse_grabbed ? PIGEON_WGI_CURSOR_TYPE_FPS_CAMERA : PIGEON_WGI_CURSOR_TYPE_NORMAL);
		if(mouse_grabbed) last_mouse_x = last_mouse_y = -1;
	}
}

int main(void)
{
	if (start())
	{
		return 1;
	}

	pigeon_wgi_set_mouse_button_callback(mouse_callback);

	if (load_assets())
	{
		free_assets();
		return 1;
	}

	if (create_meshes())
	{
		destroy_meshes();
		free_assets();
		return 1;
	}

	if (create_pipelines())
	{
		destroy_pipelines();
		destroy_meshes();
		free_assets();
		return 1;
	}

	game_loop();

	pigeon_wgi_wait_idle();
	destroy_pipelines();
	destroy_meshes();
	free_assets();

	return 0;
}
