#include <pigeon/assert.h>
#include <pigeon/asset.h>
#include <pigeon/misc.h>
#include <pigeon/pigeon.h>
#include <pigeon/scene/audio.h>
#include <pigeon/scene/light.h>
#include <pigeon/scene/mesh_renderer.h>
#include <pigeon/scene/scene.h>
#include <pigeon/scene/transform.h>
#include <pigeon/wgi/input.h>
#include <pigeon/wgi/pipeline.h>
#include <pigeon/wgi/swapchain.h>
#include <string.h>
#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <cglm/affine.h>
#include <cglm/cam.h>
#include <cglm/euler.h>
#include <cglm/quat.h>
#include <time.h>

#ifdef NDEBUG
#define SHADER_PATH_PREFIX "build/release/standard_assets/shaders/"
#else
#define SHADER_PATH_PREFIX "build/debug/standard_assets/shaders/"
#endif

#define AUDIO_ASSET_COUNT 1
#define MODEL_ASSET_COUNT 3

const char* asset_paths[MODEL_ASSET_COUNT + AUDIO_ASSET_COUNT] = {

#define ASSET_PATH(x) ("build/test_assets/audio/" x ".asset")

	ASSET_PATH("pigeon.ogg"),

#undef ASSET_PATH
#undef ASSET_PATH2

#define ASSET_PATH(x) ("build/test_assets/models/" x ".asset")
#define ASSET_PATH2(x) ("build/standard_assets/models/" x ".asset")

	ASSET_PATH2("cube.blend"),
	ASSET_PATH("pete.blend"),
	ASSET_PATH2("sphere.blend"),

#undef ASSET_PATH
#undef ASSET_PATH2
};

PigeonAudioBufferID audio_buffers[AUDIO_ASSET_COUNT];

PigeonArrayList meshes; // PigeonWGIMultiMesh
PigeonArrayList textures; // PigeonWGIArrayTexture

PigeonWGIPipeline skybox_pipeline;
PigeonWGIPipeline render_pipeline;
PigeonWGIPipeline render_pipeline_transparent;
PigeonWGIPipeline render_pipeline_skinned;
PigeonWGIPipeline render_pipeline_skinned_transparent;

PigeonRenderState* rs_static_opaque;
PigeonRenderState* rs_static_transparent;
PigeonRenderState* rs_skinned_opaque;
PigeonRenderState* rs_skinned_transparent;

PigeonTransform* t_floor;
PigeonTransform* t_wall;
PigeonTransform* t_character;
PigeonTransform* t_spinning_cube;
PigeonTransform* t_light;
PigeonTransform* t_camera;
PigeonTransform* t_sphere;
PigeonTransform* t_pigeon;

#define NUMBER_OF_CUBES 1000
PigeonTransform** many_cubes_transforms;

PigeonModelMaterial* model_cube;
PigeonModelMaterial** model_character;
PigeonModelMaterial* model_sphere;

PigeonMaterialRenderer* mr_white_cuboid;
PigeonMaterialRenderer* mr_spinning_cube;
PigeonMaterialRenderer* mr_sphere;
PigeonMaterialRenderer** mr_character;

PigeonAnimationState* as_character;
PigeonLight* light;
PigeonLight* light2;
PigeonAudioPlayer* audio_pigeon;

PigeonWGIRenderConfig render_config;

bool mouse_grabbed;
int last_mouse_x = -1;
int last_mouse_y = -1;

double start_time = 0;

static void key_callback(PigeonWGIKeyEvent e)
{
	if (e.key == PIGEON_WGI_KEY_1 && !e.pressed) {
		render_config.ssao = !render_config.ssao;
	}
	if (e.key == PIGEON_WGI_KEY_2 && !e.pressed) {
		render_config.bloom = !render_config.bloom;
	}

	if (e.key == PIGEON_WGI_KEY_0 && !e.pressed && AUDIO_ASSET_COUNT) {
		pigeon_audio_player_play(audio_pigeon, audio_buffers[0]);
	}
}

static void mouse_callback(PigeonWGIMouseEvent e)
{
	if (e.button == PIGEON_WGI_MOUSE_BUTTON_RIGHT && !e.pressed) {
		mouse_grabbed = !mouse_grabbed;
		pigeon_wgi_set_cursor_type(mouse_grabbed ? PIGEON_WGI_CURSOR_TYPE_FPS_CAMERA : PIGEON_WGI_CURSOR_TYPE_NORMAL);
		if (mouse_grabbed)
			last_mouse_x = last_mouse_y = -1;
	}
}

static PIGEON_ERR_RET start(void)
{
	ASSERT_R1(!pigeon_init());

	PigeonWindowParameters window_parameters
		= { .width = 1280, .height = 720, .window_mode = PIGEON_WINDOW_MODE_WINDOWED, .title = "Test 1" };

	render_config.ssao = true;
	render_config.bloom = true;
	if (pigeon_wgi_init(window_parameters, true, false, render_config, 0.1f, 1000.0f)) {
		pigeon_wgi_deinit();
		return 1;
	}

	pigeon_wgi_set_key_callback(key_callback);
	pigeon_wgi_set_mouse_button_callback(mouse_callback);

	(void)pigeon_audio_init();

	start_time = pigeon_wgi_get_time_seconds_double();

	return 0;
}

static unsigned int audio_buffer_index;
static PIGEON_ERR_RET load_audio_assets(PigeonAsset* a)
{
	if (a->type == PIGEON_ASSET_TYPE_AUDIO) {
		ASSERT_R1(!pigeon_decompress_asset(a, NULL, 0));
		ASSERT_R1(!pigeon_audio_upload(audio_buffers[audio_buffer_index++], a->audio_meta,
			a->subresources[0].decompressed_data, a->subresources[0].decompressed_data_length));
		pigeon_free_asset(a);
	}
	return 0;
}

static PIGEON_ERR_RET load_assets(void)
{
	// Load assets into object pool
	ASSERT_R1(!pigeon_load_assets(asset_paths, MODEL_ASSET_COUNT + AUDIO_ASSET_COUNT));

	if (AUDIO_ASSET_COUNT) {
		ASSERT_R1(!pigeon_audio_create_buffers(AUDIO_ASSET_COUNT, audio_buffers));
	}

	ASSERT_R1(!pigeon_object_pool_for_each3(&pigeon_asset_object_pool, load_audio_assets, AUDIO_ASSET_COUNT, false));

	ASSERT_R1(!pigeon_allocate_meshes(&meshes));
	ASSERT_R1(!pigeon_allocate_array_textures(&textures));

	return 0;
}

static void free_assets(void)
{
	for (unsigned int i = 0; i < textures.size; i++) {
		pigeon_wgi_destroy_array_texture(&((PigeonWGIArrayTexture*)textures.elements)[i]);
	}
	for (unsigned int i = 0; i < meshes.size; i++) {
		pigeon_wgi_destroy_multimesh(&((PigeonWGIMultiMesh*)meshes.elements)[i]);
	}

	pigeon_free_all_assets();
}

static PIGEON_ERR_RET create_skybox_pipeline_glsl(void)
{
	PigeonWGIShader vs = { 0 }, fs = { 0 };

	ASSERT_R1(!pigeon_wgi_create_shader2(&vs, "skybox.vert", PIGEON_WGI_SHADER_TYPE_VERTEX, NULL));

	if (pigeon_wgi_create_shader2(&fs, "skybox.frag", PIGEON_WGI_SHADER_TYPE_FRAGMENT, NULL)) {
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
	if (!pigeon_wgi_accepts_spirv())
		return create_skybox_pipeline_glsl();

	unsigned long vs_spv_len = 0, fs_spv_len = 0;

	uint32_t* vs_spv = (uint32_t*)pigeon_load_file(SHADER_PATH_PREFIX "skybox.vert.spv", 0, &vs_spv_len);
	ASSERT_LOG_R1(vs_spv, "Error loading vertex shader");

	uint32_t* fs_spv = (uint32_t*)pigeon_load_file(SHADER_PATH_PREFIX "skybox.frag.spv", 0, &fs_spv_len);
	if (!fs_spv) {
		free(vs_spv);
		ASSERT_LOG_R1(false, "Error loading fragment shader");
	}

	PigeonWGIShader vs = { 0 }, fs = { 0 };

	if (pigeon_wgi_create_shader(&vs, vs_spv, (uint32_t)vs_spv_len, PIGEON_WGI_SHADER_TYPE_VERTEX)) {
		free(vs_spv);
		free(fs_spv);
		ASSERT_R1(false);
	}

	if (pigeon_wgi_create_shader(&fs, fs_spv, (uint32_t)fs_spv_len, PIGEON_WGI_SHADER_TYPE_FRAGMENT)) {
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

static void create_pipeline_cleanup(uint32_t* spv_data[4], PigeonWGIShader shaders[4])
{
	for (unsigned int i = 0; i < 4; i++) {
		if (spv_data[i])
			free(spv_data[i]);
		if (shaders[i].shader)
			pigeon_wgi_destroy_shader(&shaders[i]);
	}
}

static PIGEON_ERR_RET create_pipeline(PigeonWGIPipeline* pipeline, const char* shader_paths[4][2],
	PigeonWGIPipelineConfig* config, bool skinned, bool transparent)
{

	PigeonWGIShaderType shader_types[4] = { PIGEON_WGI_SHADER_TYPE_VERTEX, PIGEON_WGI_SHADER_TYPE_VERTEX,
		PIGEON_WGI_SHADER_TYPE_FRAGMENT, PIGEON_WGI_SHADER_TYPE_FRAGMENT };

	unsigned long spv_lengths[4] = { 0 };
	uint32_t* spv_data[4] = { 0 };
	PigeonWGIShader shaders[4] = { { { 0 } } };

#define CHECK(x)                                                                                                       \
	if (!(x)) {                                                                                                        \
		create_pipeline_cleanup(spv_data, shaders);                                                                    \
		ASSERT_R1(false);                                                                                              \
	}

	for (unsigned int i = 0; i < 4; i++) {
		if (shader_paths[i][0]) {
			if (pigeon_wgi_accepts_spirv()) {
				spv_data[i] = (uint32_t*)pigeon_load_file(shader_paths[i][1], 0, &spv_lengths[i]);
				CHECK(spv_data[i]);
				CHECK(!pigeon_wgi_create_shader(&shaders[i], spv_data[i], (uint32_t)spv_lengths[i], shader_types[i]));
			} else {
				CHECK(!pigeon_wgi_create_shader2(&shaders[i], shader_paths[i][0], shader_types[i], config));
			}
		}
	}

	int err = pigeon_wgi_create_pipeline(pipeline, &shaders[0], &shaders[1], shaders[2].shader ? &shaders[2] : NULL,
		&shaders[3], config, skinned, transparent);

	create_pipeline_cleanup(spv_data, shaders);

	ASSERT_R1(!err);

	return 0;
}

// TODO create a set of pipelines per multimesh
static PIGEON_ERR_RET create_pipelines(void)
{
	ASSERT_R1(!create_skybox_pipeline());

	PigeonWGIPipelineConfig config = { 0 };
	config.depth_test = true;

	config.depth_write = true;
	config.depth_test = true;
	config.cull_mode = PIGEON_WGI_CULL_MODE_BACK;

#define FULL_PATH(x) SHADER_PATH_PREFIX x ".spv"
#define PATHS(x)                                                                                                       \
	{                                                                                                                  \
		x, SHADER_PATH_PREFIX x ".spv"                                                                                 \
	}

	const char* shader_paths[4][2] = { { "object.vert", FULL_PATH("object.depth.vert") }, PATHS("object.vert"),
		{ NULL, NULL }, PATHS("object.frag") };

	memcpy(config.vertex_attributes, static_mesh_attribs, sizeof config.vertex_attributes);
	ASSERT_R1(!create_pipeline(&render_pipeline, shader_paths, &config, false, false));

	const char* shader_paths_skinned[4][2] = { { "object.vert", FULL_PATH("object.skinned.depth.vert") },
		{ "object.vert", FULL_PATH("object.skinned.vert") }, { NULL, NULL }, PATHS("object.frag") };

	memcpy(config.vertex_attributes, skinned_mesh_attribs, sizeof config.vertex_attributes);
	ASSERT_R1(!create_pipeline(&render_pipeline_skinned, shader_paths_skinned, &config, true, false));

	config.blend_function = PIGEON_WGI_BLEND_NORMAL;
	shader_paths[0][1] = FULL_PATH("object.depth_alpha.vert");
	shader_paths[2][0] = "object_depth_alpha.frag";
	shader_paths[2][1] = FULL_PATH("object_depth_alpha.frag");

	memcpy(config.vertex_attributes, static_mesh_attribs, sizeof config.vertex_attributes);
	ASSERT_R1(!create_pipeline(&render_pipeline_transparent, shader_paths, &config, false, true));

	shader_paths_skinned[0][1] = FULL_PATH("object.skinned.depth_alpha.vert");
	shader_paths_skinned[2][0] = "object_depth_alpha.frag";
	shader_paths_skinned[2][1] = FULL_PATH("object_depth_alpha.frag");
	memcpy(config.vertex_attributes, skinned_mesh_attribs, sizeof config.vertex_attributes);
	ASSERT_R1(!create_pipeline(&render_pipeline_skinned_transparent, shader_paths_skinned, &config, true, true));

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
	vec3 angles = { rotation[0], -rotation[1], 0.0f };

	glm_euler_zyx(angles, t_camera->matrix_transform);

	mat4 t_mat;
	glm_translate_make(t_mat, position);
	glm_mat4_mul(t_mat, t_camera->matrix_transform, t_camera->matrix_transform);
	pigeon_invalidate_world_transform(t_camera);
}

static void print_timer_stats(double delayed_timer_values[PIGEON_WGI_RENDER_STAGE__COUNT], double cpu_frame_time)
{
	static double values[PIGEON_WGI_RENDER_STAGE__COUNT];
	for (unsigned int i = 0; i < PIGEON_WGI_RENDER_STAGE__COUNT; i++)
		values[i] += delayed_timer_values[i];

	static double cpu_frame_time_sum = 0;
	cpu_frame_time_sum += cpu_frame_time;

	static unsigned int frame_counter;
	frame_counter++;

	if (frame_counter < 300) {
		return;
	}

	double total = 0;

	for (unsigned int i = 0; i < PIGEON_WGI_RENDER_STAGE__COUNT; i++) {
		values[i] /= 300.0;
		total += values[i];
	}

	printf("Render time statistics (300-frame average):\n");
	printf("\tUpload: %f\n", values[PIGEON_WGI_RENDER_STAGE_UPLOAD]);
	printf("\tShadow Map 0: %f\n", values[PIGEON_WGI_RENDER_STAGE_SHADOW0]);
	if (values[PIGEON_WGI_RENDER_STAGE_SHADOW1] > 0.001)
		printf("\tShadow Map 1: %f\n", values[PIGEON_WGI_RENDER_STAGE_SHADOW1]);
	if (values[PIGEON_WGI_RENDER_STAGE_SHADOW2] > 0.001)
		printf("\tShadow Map 2: %f\n", values[PIGEON_WGI_RENDER_STAGE_SHADOW2]);
	if (values[PIGEON_WGI_RENDER_STAGE_SHADOW3] > 0.001)
		printf("\tShadow Map 3: %f\n", values[PIGEON_WGI_RENDER_STAGE_SHADOW3]);
	printf("\tDepth Prepass: %f\n", values[PIGEON_WGI_RENDER_STAGE_DEPTH]);
	if (values[PIGEON_WGI_RENDER_STAGE_SSAO] > 0.001)
		printf("\tSSAO: %f\n", values[PIGEON_WGI_RENDER_STAGE_SSAO]);
	printf("\tRender: %f\n", values[PIGEON_WGI_RENDER_STAGE_RENDER]);
	if (values[PIGEON_WGI_RENDER_STAGE_BLOOM] > 0.001)
		printf("\tBloom Blur: %f\n", values[PIGEON_WGI_RENDER_STAGE_BLOOM]);
	printf("\tPost Process & GUI: %f\n", values[PIGEON_WGI_RENDER_STAGE_POST_AND_UI]);
	printf("GPU time: %f\n", total);
	printf("CPU time: %f\n", cpu_frame_time_sum / 300.0);

	frame_counter = 0;
	cpu_frame_time_sum = 0;
	memset(values, 0, sizeof values);
}

static void fps_camera_mouse_input(vec2 rotation)
{
	if (!mouse_grabbed)
		return;

	int mouse_x, mouse_y;
	pigeon_wgi_get_mouse_position(&mouse_x, &mouse_y);

	if (last_mouse_x == -1) {
		last_mouse_x = mouse_x;
		last_mouse_y = mouse_y;
	}

	rotation[0] -= (float)(mouse_y - last_mouse_y) * 0.001f;
	rotation[1] += (float)(mouse_x - last_mouse_x) * 0.001f;

	last_mouse_x = mouse_x;
	last_mouse_y = mouse_y;

	if (rotation[0] > glm_rad(80.0f)) {
		rotation[0] = glm_rad(80.0f);
	} else if (rotation[0] < glm_rad(-80.0f)) {
		rotation[0] = glm_rad(-80.0f);
	}
}

static void fps_camera_input(float delta_time, vec2 rotation, vec3 position)
{
	fps_camera_mouse_input(rotation);

	mat4 rotation_matrix;
	vec3 angles = { 0, -rotation[1], 0.0f };
	glm_euler_zyx(angles, rotation_matrix);

	vec3 forwards = { 0, 0, -1 };
	glm_vec3_rotate_m4(rotation_matrix, forwards, forwards);

	vec3 right = { 1, 0, 0 };
	glm_vec3_rotate_m4(rotation_matrix, right, right);

	float speed = pigeon_wgi_is_key_down(PIGEON_WGI_KEY_RIGHT_SHIFT)
		? 30.0f
		: (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_SLASH) ? 1.0f : 5.0f);

	if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_W)) {
		for (unsigned int i = 0; i < 3; i++)
			position[i] += forwards[i] * speed * delta_time;
	} else if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_S)) {
		for (unsigned int i = 0; i < 3; i++)
			position[i] += -forwards[i] * speed * delta_time;
	}

	if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_D)) {
		for (unsigned int i = 0; i < 3; i++)
			position[i] += right[i] * speed * delta_time;
	} else if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_A)) {
		for (unsigned int i = 0; i < 3; i++)
			position[i] += -right[i] * speed * delta_time;
	}

	if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_SPACE)) {
		position[1] += speed * delta_time;
	} else if (pigeon_wgi_is_key_down(PIGEON_WGI_KEY_LEFT_SHIFT)) {
		position[1] -= speed * delta_time;
	}
}

static PIGEON_ERR_RET game_loop(void)
{

	float last_frame_time = pigeon_wgi_get_time_seconds() - 1 / 60.0f;

	float last_fps_output_time = pigeon_wgi_get_time_seconds();
	unsigned int fps_frame_counter = 0;
	unsigned int frame_number = 0;

	vec2 rotation = { 0 };
	vec3 position = { 0, 1.7f, 1 };
	vec3 position_prev = { 0, 1.7f, 0 };

	double cpu_frame_time = 1 / 60.0;

	while (!pigeon_wgi_close_requested() && !pigeon_wgi_is_key_down(PIGEON_WGI_KEY_ESCAPE)) {
		double delayed_timer_values[PIGEON_WGI_RENDER_STAGE__COUNT];

		ASSERT_R1(!pigeon_wgi_next_frame_wait(delayed_timer_values));

		if (delayed_timer_values[PIGEON_WGI_RENDER_STAGE__LAST] > 0.001)
			print_timer_stats(delayed_timer_values, cpu_frame_time);

		float time_now = pigeon_wgi_get_time_seconds();
		float start_frame_time = time_now;
		float delta_time = time_now - last_frame_time;
		last_frame_time = time_now;

		if (time_now - last_fps_output_time >= 1.0f) {
			printf("FPS: %u\n", fps_frame_counter);
			fps_frame_counter = 0;
			last_fps_output_time = time_now;
		} else {
			fps_frame_counter++;
		}

		// game logic

		float cube_rotation = fmodf((float)(time_now - start_time), 2.0f * 3.1315f);
		mat4 identity = GLM_MAT4_IDENTITY_INIT;
		glm_rotate_y(identity, cube_rotation, t_spinning_cube->matrix_transform);
		mat4 spin_cube_translation_mat4;
		vec3 spin_cube_translation_vec3 = { -3.0f, 0.5f, 0.1f };
		glm_translate_make(spin_cube_translation_mat4, spin_cube_translation_vec3);
		glm_mat4_mul(spin_cube_translation_mat4, t_spinning_cube->matrix_transform, t_spinning_cube->matrix_transform);
		pigeon_invalidate_world_transform(t_spinning_cube);

		memcpy(position_prev, position, sizeof(vec3));
		fps_camera_input(delta_time, rotation, position);
		set_camera_transform(rotation, position);

		PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();

		pigeon_bind_array_textures(&textures);

		ASSERT_R1(!pigeon_update_scene_audio(t_camera));
		pigeon_wgi_set_active_render_config(render_config);

		// Scene graph pre-pass, ready uniform buffers, configure lighting
		ASSERT_R1(!pigeon_prepare_draw_frame(t_camera));

		// Upload stage is required, even if nothing is uploaded
		ASSERT_R1(!pigeon_wgi_start_record(PIGEON_WGI_RENDER_STAGE_UPLOAD));

		if (frame_number == 0) {
			for (unsigned int i = 0; i < meshes.size; i++) {
				pigeon_wgi_upload_multimesh(&((PigeonWGIMultiMesh*)meshes.elements)[i]);
			}
			for (unsigned int i = 0; i < textures.size; i++) {
				pigeon_wgi_array_texture_upload_all(&((PigeonWGIArrayTexture*)textures.elements)[i]);
			}
		} else if (frame_number == sc_info.image_count) { // upload has definitely finished
			// Free staging buffers
			
			for (unsigned int i = 0; i < meshes.size; i++) {
				pigeon_wgi_multimesh_transfer_done(&((PigeonWGIMultiMesh*)meshes.elements)[i]);
			}
			for (unsigned int i = 0; i < textures.size; i++) {
				pigeon_wgi_array_texture_transfer_done(&((PigeonWGIArrayTexture*)textures.elements)[i]);
			}
		}

		ASSERT_R1(!pigeon_wgi_end_record(PIGEON_WGI_RENDER_STAGE_UPLOAD));

		ASSERT_R1(!pigeon_draw_frame(&skybox_pipeline));

		ASSERT_R1(!pigeon_wgi_submit_frame());

		cpu_frame_time = (pigeon_wgi_get_time_seconds() - start_frame_time) * 1000.0;

		frame_number++;
	}

	return 0;
}

int main(void)
{
	srand((unsigned)time(NULL));
	ASSERT_R1(!start());
	ASSERT_R1(!load_assets());
	ASSERT_R1(!create_pipelines());

	t_floor = pigeon_create_transform(NULL);
	t_wall = pigeon_create_transform(NULL);
	t_character = pigeon_create_transform(NULL);
	t_spinning_cube = pigeon_create_transform(NULL);
	t_light = pigeon_create_transform(NULL);
	t_camera = pigeon_create_transform(NULL);
	t_sphere = pigeon_create_transform(NULL);
	t_pigeon = pigeon_create_transform(NULL);
	many_cubes_transforms = malloc(sizeof *many_cubes_transforms * NUMBER_OF_CUBES);
	ASSERT_R1(t_floor && t_character && t_spinning_cube && t_light && t_camera && t_sphere && t_pigeon
		&& many_cubes_transforms);

	for (unsigned int i = 0; i < NUMBER_OF_CUBES; i++) {
		many_cubes_transforms[i] = pigeon_create_transform(NULL);
		many_cubes_transforms[i]->transform_type = PIGEON_TRANSFORM_TYPE_SRT;
		many_cubes_transforms[i]->scale[0] = 0.5f;
		many_cubes_transforms[i]->scale[1] = 0.5f;
		many_cubes_transforms[i]->scale[2] = 0.5f;

		many_cubes_transforms[i]->translation[0] = ((float)rand() / (float)(RAND_MAX)-0.5f) * 100.0f;
		many_cubes_transforms[i]->translation[1] = (float)rand() / (float)(RAND_MAX)*20.0f;
		many_cubes_transforms[i]->translation[2] = ((float)rand() / (float)(RAND_MAX)-0.5f) * 100.0f;
	}

	t_floor->transform_type = PIGEON_TRANSFORM_TYPE_SRT;
	t_floor->scale[0] = 10000;
	t_floor->scale[1] = 0.1f;
	t_floor->scale[2] = 10000;
	t_floor->translation[1] = -0.05f;

	t_wall->transform_type = PIGEON_TRANSFORM_TYPE_SRT;
	t_wall->scale[0] = 3;
	t_wall->scale[1] = 4;
	t_wall->scale[2] = 0.3f;
	t_wall->translation[0] = 3;
	t_wall->translation[2] = 3;

	t_sphere->transform_type = PIGEON_TRANSFORM_TYPE_SRT;
	t_sphere->scale[0] = 0.1f;
	t_sphere->scale[1] = 0.1f;
	t_sphere->scale[2] = 0.1f;
	t_sphere->translation[0] = 3;
	t_sphere->translation[1] = 1;
	t_sphere->translation[2] = 2;

	t_character->transform_type = PIGEON_TRANSFORM_TYPE_SRT;
	t_character->scale[0] = 1;
	t_character->scale[1] = 1;
	t_character->scale[2] = 1;
	t_character->translation[2] = -3;

	t_light->transform_type = PIGEON_TRANSFORM_TYPE_MATRIX;
	{
		vec3 position = { 0, 6, 6 };
		vec3 centre = { 0 };
		vec3 up = { 0, 1, 0 };
		glm_lookat(position, centre, up, t_light->matrix_transform);
		glm_mat4_inv(t_light->matrix_transform, t_light->matrix_transform);
	}

	// set each frame
	t_spinning_cube->transform_type = PIGEON_TRANSFORM_TYPE_MATRIX;
	t_camera->transform_type = PIGEON_TRANSFORM_TYPE_MATRIX;

	light = pigeon_create_light();
	ASSERT_R1(light);
	light->intensity[0] = light->intensity[1] = light->intensity[2] = 1.3f;
	light->shadow_resolution = 1024;
	light->shadow_near = 3;
	light->shadow_far = 16;
	light->shadow_size_x = 6;
	light->shadow_size_y = 6;
	ASSERT_R1(!pigeon_join_transform_and_component(t_light, &light->c));

	pigeon_wgi_set_brightness(1);

	light2 = pigeon_create_light();
	ASSERT_R1(light2);
	light2->type = PIGEON_LIGHT_TYPE_POINT;
	light2->intensity[0] = 0.15f * 0.3f;
	light2->intensity[1] = 0.5f * 0.3f;
	light2->intensity[2] = 2.1f * 0.3f;
	ASSERT_R1(!pigeon_join_transform_and_component(t_sphere, &light2->c));

	rs_static_opaque = pigeon_create_render_state(&mesh, &render_pipeline);
	rs_static_transparent = pigeon_create_render_state(&mesh, &render_pipeline_transparent);
	rs_skinned_opaque = pigeon_create_render_state(&mesh_skinned, &render_pipeline_skinned);
	rs_skinned_transparent = pigeon_create_render_state(&mesh_skinned, &render_pipeline_skinned_transparent);

	ASSERT_R1(rs_static_opaque);
	ASSERT_R1(rs_static_transparent);
	ASSERT_R1(rs_skinned_opaque);
	ASSERT_R1(rs_skinned_transparent);

	model_cube = pigeon_create_model_renderer(pigeon_object_pool_get(&pigeon_asset_object_pool, AUDIO_ASSET_COUNT+0), 0);
	model_sphere = pigeon_create_model_renderer(pigeon_object_pool_get(&pigeon_asset_object_pool, AUDIO_ASSET_COUNT+2), 0);
	ASSERT_R1(model_cube && model_sphere);

	PigeonAsset* character_model_asset = pigeon_object_pool_get(&pigeon_asset_object_pool, AUDIO_ASSET_COUNT+0);

	model_character = malloc(sizeof *model_character * character_model_asset->materials_count);
	ASSERT_R1(model_character);

	ASSERT_R1(!pigeon_join_rs_model(rs_static_opaque, model_cube));
	ASSERT_R1(!pigeon_join_rs_model(rs_static_opaque, model_sphere));

	mr_white_cuboid = pigeon_create_material_renderer(model_cube);
	mr_spinning_cube = pigeon_create_material_renderer(model_cube);
	mr_sphere = pigeon_create_material_renderer(model_sphere);
	ASSERT_R1(mr_white_cuboid && mr_spinning_cube && mr_sphere);

	mr_white_cuboid->specular_intensity = 0;

	mr_spinning_cube->colour[0] = 1.1f;
	mr_spinning_cube->colour[1] = 0.5f;
	mr_spinning_cube->colour[2] = 1.15f;
	mr_spinning_cube->specular_intensity = 0;

	mr_sphere->colour[0] = 0.15f;
	mr_sphere->colour[1] = 0.5f;
	mr_sphere->colour[2] = 2.1f;
	mr_sphere->luminosity = 12;

	mr_character = malloc(sizeof *mr_character * character_model_asset->materials_count);
	ASSERT_R1(mr_character);

	as_character = pigeon_create_animation_state(character_model_asset);
	ASSERT_R1(as_character);
	as_character->animation_start_time = pigeon_wgi_get_time_seconds_double();
	as_character->animation_index = 1;

	audio_pigeon = pigeon_create_audio_player();
	ASSERT_R1(audio_pigeon);
	ASSERT_R1(!pigeon_join_transform_and_component(t_pigeon, &audio_pigeon->c));

	for (unsigned int i = 0; i < character_model_asset->materials_count; i++) {
		model_character[i] = pigeon_create_model_renderer(character_model_asset, i);
		ASSERT_R1(model_character[i]);

		mr_character[i] = pigeon_create_material_renderer(model_character[i]);
		ASSERT_R1(mr_character[i]);

		TextureIndices* ti = &model_texture_indices[1][i];

		if (ti->texture_index != UINT32_MAX) {
			TextureLocation* tl = &texture_locations[ti->texture_index];
			mr_character[i]->diffuse_bind_point = tl->bind_point;
			mr_character[i]->diffuse_layer = tl->layer;
			if (texture_assets[ti->texture_index].texture_meta.has_alpha)
				mr_character[i]->use_transparency = true;
		}
		if (ti->normal_index != UINT32_MAX) {
			TextureLocation* tl = &texture_locations[ti->normal_index];
			mr_character[i]->nmap_bind_point = tl->bind_point;
			mr_character[i]->nmap_layer = tl->layer;
		}

		if (mr_character[i]->use_transparency) {
			ASSERT_R1(!pigeon_join_rs_model(rs_skinned_transparent, model_character[i]));
		} else {
			ASSERT_R1(!pigeon_join_rs_model(rs_skinned_opaque, model_character[i]));
		}

		ASSERT_R1(!pigeon_join_mr_anim(mr_character[i], as_character));

		ASSERT_R1(!pigeon_join_transform_and_component(t_character, &mr_character[i]->c));
	}

	ASSERT_R1(!pigeon_join_transform_and_component(t_floor, &mr_white_cuboid->c));
	ASSERT_R1(!pigeon_join_transform_and_component(t_wall, &mr_white_cuboid->c));
	ASSERT_R1(!pigeon_join_transform_and_component(t_spinning_cube, &mr_spinning_cube->c));
	ASSERT_R1(!pigeon_join_transform_and_component(t_sphere, &mr_sphere->c));

	for (unsigned int i = 0; i < NUMBER_OF_CUBES; i++) {
		ASSERT_R1(!pigeon_join_transform_and_component(many_cubes_transforms[i], &mr_spinning_cube->c));
	}

	int err = game_loop();

	pigeon_wgi_wait_idle();

	pigeon_destroy_light(light);
	pigeon_destroy_animation_state(as_character);
	pigeon_destroy_audio_player(audio_pigeon);

	pigeon_destroy_material_renderer(mr_white_cuboid);
	pigeon_destroy_material_renderer(mr_spinning_cube);

	for (unsigned int i = 0; i < character_model_asset->materials_count; i++) {
		pigeon_destroy_material_renderer(mr_character[i]);
		pigeon_destroy_model_renderer(model_character[i]);
	}

	pigeon_destroy_model_renderer(model_cube);

	pigeon_destroy_render_state(rs_static_opaque);
	pigeon_destroy_render_state(rs_static_transparent);
	pigeon_destroy_render_state(rs_skinned_opaque);
	pigeon_destroy_render_state(rs_skinned_transparent);

	destroy_pipelines();
	free_assets();
	audio_cleanup();
	pigeon_wgi_deinit();
	pigeon_deinit();

	return err;
	return 0;
}
