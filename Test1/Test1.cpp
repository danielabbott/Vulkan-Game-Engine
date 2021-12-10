#include <WGI/Window.h>
#include <thread>
#include <WGI/Input.h>
#include <ErrorPopup.h>
#include <stdexcept>
#include <FileIO.h>
#include "IncludeSpdlog.h"
#include <Pigeon.h>
#include <Component.h>
#include <Transform.h>
#include <Asset.h>
#include <MeshRenderer.h>
#include <WGI/Input.h>
#include <WGI/WGI.h>
#include <WGI/Shader.h>
#include <thread>
#include <iostream>
#include <Ref.h>


using namespace std;


void frame_time_stats();

// Microseconds
static vector<uint32_t> cpu_time_values;

static float pre_render_time_sum;
static float render_time_sum;
static float post_render_time_sum;
static float full_gpu_time_sum;
static unsigned int gpu_time_divider;


// TODO this is a stub implementation
class Job {
public:
	enum class State {
		READY, RUNNING, WAITING, DONE
	};
private:
	State state = State::DONE;
public:
	void wait() {
	}
};
class JobSystem {
public:
	Ref<Job> new_job(int pri, function<void()> f, vector<Ref<Job>> && dependencies = vector<Ref<Job>>{}) {
		f();
		return Ref<Job>::make();
	}
};

void test1() {
	auto wgi = WGI::WGI("Test1", WGI::GraphicsSettings { 
		WGI::WindowDimensions(800, 600), WGI::Window::FullscreenMode::Windowed, WGI::GPUPreference::Integrated 
	});


	struct Object {
		string mesh_file_name;
		glm::mat4 position;

		glm::vec3 colour;

		Object(string mesh_file_name_,	glm::mat4 position_, glm::vec3 colour_)
		:mesh_file_name(mesh_file_name_), position(position_), colour(colour_)
		{}
	};

	struct ObjectMesh {
		Ref<WGI::Mesh> mesh;
		WGI::MeshMeta mesh_meta;
	};

	unordered_map<string, ObjectMesh> meshes;

	auto objects = array<Object, 2> {
		Object (
			"alien",
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f)),
			glm::vec3(0.52f, 0.5f, 0.52f)
		),
		Object (
			"cube",
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.05f, 0.0f)) 
			* glm::scale(glm::vec3(100.0f, 0.1f, 100.0f)),
			glm::vec3(0.7f, 0.7f, 0.7f)
		)
	};

	WGI::PipelineConfig render_config;

	for(Object & obj : objects)  {
		// Create mesh from asset

		Asset model_asset("../ExampleAssets/Models/" + obj.mesh_file_name + ".json");
		model_asset.load_from_disk();

		auto& mesh_data = meshes[obj.mesh_file_name];

		mesh_data.mesh_meta = model_asset.get_mesh_meta();
		mesh_data.mesh = Ref<WGI::Mesh>::make(wgi, mesh_data.mesh_meta);
		model_asset.decompress(mesh_data.mesh->create_staging_buffer(model_asset.get_decompressed_size()));
		render_config.vertex_attributes = mesh_data.mesh_meta.vertex_attributes;

		// obj.render_config.cull_mode = WGI::CullMode::None;
	}


	/*
	TODO
	unsigned int buffer_space_needed = model_asset.get_decompressed_size() 
		+ WGI::get_buffer_alignment()
		+ model_asset2.get_decompressed_size();
	WGI::Heap staging_buffer_heap(wgi, WGI::Heap::Type::Buffer, buffer_space_needed, WGI::Heap::AllocatorType::Stack);
	WGI::Heap device_buffer_heap(wgi, WGI::Heap::Type::Buffer, buffer_space_needed, WGI::Heap::AllocatorType::Stack);

	mesh = Ref<WGI::Mesh>::make(wgi, mesh_meta);
	mesh2 = Ref<WGI::Mesh>::make(wgi, mesh_meta2);
	model_asset.decompress(mesh->create_staging_buffer(heap, model_asset.get_decompressed_size()));
	model_asset2.decompress(mesh2->create_staging_buffer(heap, model_asset.get_decompressed_size()));

	*/

	Ref<WGI::PipelineInstance> pipeline_instance;
	{
		auto vs_depth = WGI::Shader(wgi, "../StandardAssets/Shaders/object.vert.depth.spv", WGI::Shader::Type::Vertex);
		auto vs = WGI::Shader(wgi, "../StandardAssets/Shaders/object.vert.spv", WGI::Shader::Type::Vertex);
		auto fs = WGI::Shader(wgi, "../StandardAssets/Shaders/object.frag.spv", WGI::Shader::Type::Fragment);

		// TODO function that caches these
		// TODO would be good if shaders examined vertex inputs and validated them against render_config
		pipeline_instance = Ref<WGI::PipelineInstance>::make(wgi, render_config, vs_depth, vs, fs);
	}

	Ref<WGI::PipelineInstance> skybox_pipeline_instance;
	{
		auto vs = WGI::Shader(wgi, "../StandardAssets/Shaders/skybox.vert.spv", WGI::Shader::Type::Vertex);
		auto fs = WGI::Shader(wgi, "../StandardAssets/Shaders/skybox.frag.spv", WGI::Shader::Type::Fragment);
		skybox_pipeline_instance = Ref<WGI::PipelineInstance>::make(wgi, render_config, vs, fs);
	}



	{
		WGI::FrameObject& frame = wgi.start_frame(0);
		auto& cmdbuf_upload = frame.get_upload_cmd_buf();

		
		for(Object & obj : objects)  {
			auto& mesh_data = meshes[obj.mesh_file_name];
			cmdbuf_upload.mesh_upload(mesh_data.mesh);
		}

		wgi.present();
	}

	glm::vec2 rotation(0.0f,0.0f);
	glm::vec3 position(1.0f, 2.0f, 0.0f);
	float cube_rotation = 0;

	float last_frame_time = WGI::get_time_seconds();

	wgi.ssao_parameter = 0.00033559382;
	float ssao_intensity = 15.7f;
	bool disable_ssao = false;

	bool mouse_grabbed = false;

	auto last_mouse_position = wgi.get_window()->get_mouse_position();

	auto& lights = wgi.edit_lights();
	lights.emplace_back(
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 4.0f, 10.0f)) *
		glm::rotate(glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::radians(70.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		glm::vec3(1.2f, 1.2f, 1.0f), true
	);
	lights[0].shadow_map_resolution = 1024;
	lights[0].shadow_map_far_plane = 6;
	lights[0].shadow_map_near_plane = -5;
	lights.emplace_back(
		glm::rotate(glm::radians(-40.0f), glm::vec3(0.0f, 1.0f, 0.0f))*
		glm::rotate(glm::radians(-70.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		glm::vec3(0.4f, 0.0f, 0.0f), false
	);


	JobSystem job_system;
	while (!wgi.should_terminate()) {
		WGI::WindowVisibilityState visible = wgi.process_input();
		// WGI::WindowVisibilityState visible = wgi.wait_for_input();
		while(visible == WGI::WindowVisibilityState::Minimised) {
			visible = wgi.wait_for_input();
		}

		if (wgi.get_window()->key_is_down(WGI::Key::ESCAPE)) {
			break;
		}

		frame_time_stats();
		float now = WGI::get_time_seconds();
		float delta_time = now - last_frame_time;

		
		auto mouse_position = wgi.get_window()->get_mouse_position();
		auto mouse_delta = pair<int,int>(mouse_position.first - last_mouse_position.first, mouse_position.second - last_mouse_position.second);

		if (wgi.get_window()->key_is_down(WGI::Key::PERIOD)) {
			wgi.ssao_parameter *= 1.0 + delta_time;
			spdlog::info("wgi.ssao_parameter {}", wgi.ssao_parameter);
		}
		else if (wgi.get_window()->key_is_down(WGI::Key::COMMA)) {
			wgi.ssao_parameter /= 1.0 + delta_time;
			spdlog::info("wgi.ssao_parameter {}", wgi.ssao_parameter);
		}

		if (wgi.get_window()->key_is_down(WGI::Key::RIGHT_BRACKET)) {
			ssao_intensity *= 1.0 + delta_time;
			spdlog::info("ssao_intensity {}", ssao_intensity);
		}
		else if (wgi.get_window()->key_is_down(WGI::Key::LEFT_BRACKET)) {
			ssao_intensity /= 1.0 + delta_time;
			spdlog::info("ssao_intensity {}", ssao_intensity);
		}

		{
			auto& key_events = wgi.get_window()->get_key_events();
			for(auto e : key_events) {
				if(e.key == WGI::Key::N1 && e.pressed) {
					disable_ssao = !disable_ssao;
				}
			}
			key_events.clear();
		}

		{
			int scroll;
			auto& mouse_events = wgi.get_window()->get_mouse_events(scroll);
			for(auto e : mouse_events) {
				if(e.button == WGI::MouseButton::Forward && e.pressed) {
					spdlog::info("Forward :))");
				}
				if(e.button == WGI::MouseButton::Right && !e.pressed) {
					mouse_grabbed = !mouse_grabbed;
					wgi.get_window()->grab_mouse(mouse_grabbed);
				}
			}
			mouse_events.clear();
		}

		if(mouse_grabbed) {
			rotation.y -= mouse_delta.first * delta_time * 0.1;
			rotation.x -= mouse_delta.second * delta_time * 0.1;

			if(rotation.x > glm::radians(80.0f)) {
				rotation.x = glm::radians(80.0f);
			}
			else if(rotation.x < glm::radians(-80.0f)) {
				rotation.x = glm::radians(-80.0f);
			}
		}

		{
			cube_rotation += delta_time*2;
			objects[4].position = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 0.5f, 0.0f))
			* glm::rotate(cube_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		}


		glm::vec3 forwards = glm::rotate(glm::mat4(1.0f), rotation[1], glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::rotate(glm::mat4(1.0f), rotation[0], glm::vec3(1.0f, 0.0f, 0.0f))
		* glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		glm::vec3 right = glm::rotate(glm::mat4(1.0f), rotation[1], glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::rotate(glm::mat4(1.0f), rotation[0], glm::vec3(1.0f, 0.0f, 0.0f))
		* glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

		float speed = wgi.get_window()->key_is_down(WGI::Key::LEFT_SHIFT) ? 30.0f : 5.0f;
		if (wgi.get_window()->key_is_down(WGI::Key::W)) {
			position += forwards * delta_time * speed;
		}
		else if (wgi.get_window()->key_is_down(WGI::Key::S)) {
			position -= forwards * delta_time * speed;
		}
		if (wgi.get_window()->key_is_down(WGI::Key::A)) {
			position -= right * delta_time * speed;
		}
		else if (wgi.get_window()->key_is_down(WGI::Key::D)) {
			position += right * delta_time * speed;
		}

		auto window_size = wgi.get_window()->get_dimensions();


		

		auto& frame = wgi.start_frame(objects.size());

		auto cpu_time0 = chrono::high_resolution_clock::now();

		auto& scene_uniform_data = frame.edit_scene_uniform_data();
		auto object_uniform_data = frame.edit_object_uniform_data();

		auto [_, shadow0_proj, shadow0_view] = frame.edit_shadow_scene_uniform_data(0);
		auto shadow0_object_uniform_data = frame.edit_shadow_object_uniform_data(0);

		scene_uniform_data.ambient = glm::vec3(0.6f, 0.65f, 0.8f);

		auto proj = WGI::get_coord_system_switch_matrix() * glm::perspective (glm::radians(45.0f), 
					window_size.width / static_cast<float>(window_size.height), 
					0.1f, 100.0f);
		// const float light_size = 10.0f;
		// auto proj = WGI::get_coord_system_switch_matrix() * glm::ortho(-light_size, light_size, -light_size, light_size, 0.0001f, 10000.0f);
		scene_uniform_data.proj = proj;

		auto view = glm::rotate(glm::mat4(1.0f), -rotation[0], glm::vec3(1.0f, 0.0f, 0.0f))
		*glm::rotate(glm::mat4(1.0f), -rotation[1], glm::vec3(0.0f, 1.0f, 0.0f))
		*glm::translate(glm::mat4(1.0f), -position);
		
		scene_uniform_data.view = view;

		scene_uniform_data.proj_view = proj*view;
		scene_uniform_data.time = WGI::get_time_seconds();

		const auto set_obj_data = [disable_ssao, ssao_intensity, &meshes](
			WGI::ObjectUniformData & d, Object & obj,
			glm::mat4 proj_, glm::mat4 view_
		) {
			d.ssao_intensity = disable_ssao ? 0 : ssao_intensity;

			auto& mesh_data = meshes[obj.mesh_file_name];
			if(mesh_data.mesh_meta.has_position_bounds()) {
				d.position_min = glm::vec4(mesh_data.mesh_meta.get_position_bounds_minimum(), 0.0f);
				d.position_range = glm::vec4(mesh_data.mesh_meta.get_position_bounds_range(), 0.0f);
			}

			d.model = obj.position;
			d.view_model = view_*obj.position;
			d.proj_view_model = proj_*view_*obj.position;

			d.colour = obj.colour;
		};

		unsigned int obj_index = 0;
		for(Object & obj : objects)  {
			set_obj_data(object_uniform_data[obj_index], obj, proj, view);
			set_obj_data(shadow0_object_uniform_data[obj_index], obj, shadow0_proj, shadow0_view);
			obj_index++;
		}
		

		auto& cmdbuf_render = frame.get_render_cmd_buf();
		auto& cmdbuf_depth = frame.get_depth_cmd_buf();
		auto& cmdbuf_shadow0 = frame.get_shadow_cmd_buf(0);

		const auto do_draw = [&objects, &pipeline_instance, &meshes](WGI::CmdBuffer & ctx) {
			unsigned int obj_index = 0;
			for(Object & obj : objects)  {
				auto& mesh_data = meshes[obj.mesh_file_name];
				ctx.draw_object(mesh_data.mesh, pipeline_instance, obj_index);
				obj_index++;
			}
		};

		// Order doesn't matter here.

		auto j1 = job_system.new_job(0, [&cmdbuf_depth, do_draw] {
			do_draw(cmdbuf_depth);
		});
		auto j2 = job_system.new_job(0, [&cmdbuf_render, do_draw, &skybox_pipeline_instance] {
			do_draw(cmdbuf_render);
			cmdbuf_render.draw_object(nullptr, skybox_pipeline_instance, -1);
		});
		auto j3 = job_system.new_job(0, [&cmdbuf_shadow0, do_draw] {
			do_draw(cmdbuf_shadow0);
		});


		j1->wait();
		j2->wait();
		j3->wait();

		auto stats = wgi.present();

		if(stats.total_time > 0.0) {
			pre_render_time_sum += stats.pre_render_time;
			render_time_sum += stats.render_time;
			post_render_time_sum += stats.post_render_time;
			full_gpu_time_sum += stats.total_time;
			gpu_time_divider++;
		}

		auto cpu_time1 = chrono::high_resolution_clock::now();

		cpu_time_values.push_back(chrono::duration_cast<chrono::microseconds>(cpu_time1 - cpu_time0).count());

		last_frame_time = now;
		last_mouse_position = mouse_position;
	}
	wgi.destroy();

}

void frame_time_stats()
{
	static unsigned int frame_counter = 0;
	static double last_fps_output_time = WGI::get_time_seconds();
	static double last_frame_time = WGI::get_time_seconds();
	static double frame_time_sum = 0;

	frame_counter++;

	double now = WGI::get_time_seconds();

	frame_time_sum += now - last_frame_time;
	last_frame_time = now;

	if (now - last_fps_output_time >= 1.0) {
		uint32_t cpu_time_sum = 0;
		for(auto x : cpu_time_values) {
			cpu_time_sum += x;
		}
		double cpu_time = (cpu_time_sum / static_cast<double>(cpu_time_values.size()) / 1000.0);
		cpu_time_values.clear();

		
		pre_render_time_sum /= static_cast<double>(gpu_time_divider);
		render_time_sum /= static_cast<double>(gpu_time_divider);
		post_render_time_sum /= static_cast<double>(gpu_time_divider);
		full_gpu_time_sum /= static_cast<double>(gpu_time_divider);
		
		spdlog::debug("FPS: {:02.1f}; Frame Time: {:02.2f}ms; CPU time: {:02.2f}ms; Pre-render GPU time: {:02.2f}ms; Render GPU time: {:02.2f}ms; Post render GPU time: {:02.2f}ms; GPU time: {:02.2f}ms",
			frame_counter / (now - last_fps_output_time),
			(1000.0 * frame_time_sum) / static_cast<double>(frame_counter),
			cpu_time,
			pre_render_time_sum, render_time_sum, post_render_time_sum, full_gpu_time_sum
		);

		gpu_time_divider = 0;
		pre_render_time_sum = render_time_sum = post_render_time_sum = full_gpu_time_sum = 0;

		last_fps_output_time = now;
		frame_counter = 0;
		frame_time_sum = 0;
	}
}

int main() {
	enable_debug_logging();

	#ifdef NDEBUG
	try {
		test1();
	}
	catch (exception const& e) {
		show_error_popup(e.what());
		assert(false);
	}

	#else
	spdlog::set_level(spdlog::level::trace);
	test1();
	#endif



	return 0;
}
