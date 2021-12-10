#include "WGI.h"
#include "../Vulkan/Device.h"
#include "../Vulkan/Swapchain.h"
#include "../Vulkan/CommandPool.h"
#include "../Vulkan/Fence.h"
#include "../Vulkan/RenderPass.h"
#include "../Vulkan/Pipeline.h"
#include "../Vulkan/DescriptorSets.h"
#include "../Vulkan/AutoBuffer.h"
#include "../Vulkan/Sampler.h"
#include "../Vulkan/QueryPool.h"
#include "Input.h"
#include "Mesh.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Shader.h"

#include <iostream> // TODO

using namespace std;



namespace WGI {

	uint64_t get_time_micro()
	{
		return static_cast<uint64_t>(glfwGetTime() * 1000000.0);
	}

	uint32_t get_time_millis()
	{
		return static_cast<uint32_t>(glfwGetTime() * 1000.0);
	}

	float get_time_seconds()
	{
		return static_cast<float>(glfwGetTime());
	}
	double get_time_seconds_double()
	{
		return glfwGetTime();
	}

	WGI::WGI(WindowTitle window_title, GraphicsSettings graphics)
	{
		window = Ref<Window>::make(graphics.window_dimensions, window_title, graphics.fullscreen_mode);
		window_dimensions = window->get_dimensions();


		vulkan_device = make_unique<Vulkan::Device>(window, graphics.gpu_preference == GPUPreference::Dedicated);

		Vulkan::RenderPass::CreateInfo render_pass_info;

		// depth
		render_pass_info.depends_on_transfer = true;
		render_pass_info.depth = Vulkan::RenderPass::DepthMode::Keep;

		depth_render_pass = make_unique<Vulkan::RenderPass>(*vulkan_device, render_pass_info);

		// ssao
		render_pass_info.depends_on_transfer = false;
		render_pass_info.depth = Vulkan::RenderPass::DepthMode::None;
		render_pass_info.colour_image = ImageFormat::U8;
		render_pass_info.colour_image_is_swapchain = false;
		render_pass_info.clear_colour_image = false;

		ssao_render_pass = make_unique<Vulkan::RenderPass>(*vulkan_device, render_pass_info);

		// ssao blur

		ssao_blur_render_pass = make_unique<Vulkan::RenderPass>(*vulkan_device, render_pass_info);

		// render
		render_pass_info.depth = Vulkan::RenderPass::DepthMode::ReadOnly;
		render_pass_info.colour_image = ImageFormat::RGBALinearF16;
		render_pass_info.clear_colour_image = false;

		main_render_pass = make_unique<Vulkan::RenderPass>(*vulkan_device, render_pass_info);

		// post
		render_pass_info.depth = Vulkan::RenderPass::DepthMode::None;
		render_pass_info.colour_image = Vulkan::get_swapchain_format();
		render_pass_info.colour_image_is_swapchain = true;
		render_pass_info.clear_colour_image = false;

		post_render_pass = make_unique<Vulkan::RenderPass>(*vulkan_device, render_pass_info);


		vector<Vulkan::DescriptorLayout::DescriptorType> d_types {
			Vulkan::DescriptorLayout::DescriptorType(Vulkan::DescriptorLayout::DescriptorType::Type::Uniform),
			Vulkan::DescriptorLayout::DescriptorType(Vulkan::DescriptorLayout::DescriptorType::Type::Texture),
			Vulkan::DescriptorLayout::DescriptorType(Vulkan::DescriptorLayout::DescriptorType::Type::Texture)
		};
		d_types[1].vertex_shader_accessible = false; // Don't need depth lookups in vertex shader
		d_types[2].vertex_shader_accessible = false; // Don't need shadows in vertex shader
		d_types[2].array_elements = 4; // Maximum number of shadow-casting lights
		descriptor_layout = make_unique<Vulkan::DescriptorLayout>(
			*vulkan_device,
			ArrayPointer<Vulkan::DescriptorLayout::DescriptorType> (d_types.data(), d_types.size())
		);

		d_types.resize(1);
		d_types[0] = Vulkan::DescriptorLayout::DescriptorType(Vulkan::DescriptorLayout::DescriptorType::Type::Uniform);
		d_types[0].fragment_shader_accessible = false; // No fragment shaders for shadows
		shadow_descriptor_layout = make_unique<Vulkan::DescriptorLayout>(
			*vulkan_device,
			ArrayPointer<Vulkan::DescriptorLayout::DescriptorType> (d_types.data(), d_types.size())
		);

		d_types.resize(1);
		d_types[0] = Vulkan::DescriptorLayout::DescriptorType(Vulkan::DescriptorLayout::DescriptorType::Type::Texture);
		d_types[0].vertex_shader_accessible = false;
		post_and_ssao_descriptor_layout = make_unique<Vulkan::DescriptorLayout>(
			*vulkan_device,
			ArrayPointer<Vulkan::DescriptorLayout::DescriptorType> (d_types.data(), d_types.size())
		);

		
		hdr_and_depth_sampler = Ref<Vulkan::Sampler>::make(*vulkan_device, Vulkan::Sampler::Type::PostProcess);
		ssao_image_sampler = Ref<Vulkan::Sampler>::make(*vulkan_device, Vulkan::Sampler::Type::SSAO);
		shadow_sampler = Ref<Vulkan::Sampler>::make(*vulkan_device, Vulkan::Sampler::Type::Shadow);

		create_swapchain_();


		auto vs = Shader(*this, "../StandardAssets/Shaders/post.vert.spv", Shader::Type::Vertex);
		auto fs = Shader(*this, "../StandardAssets/Shaders/post.frag.spv", Shader::Type::Fragment);

		PipelineConfig render_config;
		render_config.cull_mode = CullMode::None;
		render_config.depth_test = render_config.depth_write = false;

		post_pipeline = Ref<PipelineInstance>::make(*this, render_config, vs, fs, 16, *post_render_pass, *post_and_ssao_descriptor_layout);
		
		auto ssao_vs = Shader(*this, "../StandardAssets/Shaders/ssao.vert.spv", Shader::Type::Vertex);
		auto ssao_fs = Shader(*this, "../StandardAssets/Shaders/ssao.frag.spv", Shader::Type::Fragment);

		ssao_pipeline = Ref<PipelineInstance>::make(*this, render_config, ssao_vs, ssao_fs, 20, *ssao_render_pass, *post_and_ssao_descriptor_layout);
		
		auto ssao_blur_vs = Shader(*this, "../StandardAssets/Shaders/ssao_blur.vert.spv", Shader::Type::Vertex);
		auto ssao_blur_fs = Shader(*this, "../StandardAssets/Shaders/ssao_blur.frag.spv", Shader::Type::Fragment);

		ssao_blur_pipeline = Ref<PipelineInstance>::make(*this, render_config, ssao_blur_vs, ssao_blur_fs, 16, *ssao_blur_render_pass, *post_and_ssao_descriptor_layout);
		

		{

			// Create 1x1 image in host memory (using preinitialised layout)

			MemoryCriteria mem_criteria;
			mem_criteria.device_local = MemoryTypePerference::Prefered;
			mem_criteria.host_visible = MemoryTypePerference::Must;
			mem_criteria.host_coherent = MemoryTypePerference::DontCare;
			mem_criteria.host_cached = MemoryTypePerference::DontCare;

			white_1px_image = Ref<Vulkan::Image>::make(*vulkan_device, ImageFormat::RGBALinearU8, 1, 1, mem_criteria, true);


			// Allocate memory

			auto memory = Ref<Vulkan::MemoryAllocation>::make(*vulkan_device, mem_criteria);

			auto data = memory->map();
			data[0] = data[1] = data[2] = data[3] = 0xff;
			memory->flush();
			memory->unmap();

			white_1px_image->bind_memory(memory, MemoryRegion(0, mem_criteria.size));

			white_1px_image_view = Ref<Vulkan::ImageView>::make(white_1px_image);

			// Transition image

			Vulkan::CommandPool pool(*vulkan_device, vulkan_device->get_general_queue(), true, false);
			pool.start_submit(nullopt, nullopt);
			pool.transition_host_image_to_general_layout(*white_1px_image);
			pool.submit();
			pool.wait();
		}
	}

	void WGI::create_swapchain_()
	{
		wait_idle();

		swapchain = Ref<Vulkan::Swapchain>::make(*vulkan_device);

		frames.clear();	

		unsigned int n = image_count();
		for(unsigned int i = 0; i < n; i++) {
			frames.push_back(make_unique<FrameObject>(*this, swapchain->get_image_view(i)));
		}
		
		window_dimensions = window->get_dimensions();

	}

	bool WGI::should_terminate() const
	{
		return window->should_close();
	}

	WindowVisibilityState WGI::poll_or_wait_for_input(function<void()> f)
	{
		auto old_window_dimensions = window_dimensions;
		f();
		window_dimensions = window->get_dimensions();

		if(window_dimensions.invalid()) {
			return WindowVisibilityState::Minimised;
		}

		if(window_dimensions != old_window_dimensions) {
			create_swapchain_();
		}
		return WindowVisibilityState::Visible;
	}


	WindowVisibilityState WGI::process_input()
	{
		return poll_or_wait_for_input(::WGI::poll_input);
	}

	WindowVisibilityState WGI::wait_for_input()
	{
		return poll_or_wait_for_input(::WGI::wait_for_input);
	}

	WGI::~WGI() {
		vulkan_device->vulkan_wait_idle();
	}

	void WGI::destroy() {
		vulkan_device->vulkan_wait_idle();
		frames.clear();
	}

	void WGI::next_frame_()
	{
		frame_number++;

		try {
			this_frame_swapchain_index = swapchain->next_image();
		}
		catch (Vulkan::RecreateSwapchainException&) {
			create_swapchain_();
			this_frame_swapchain_index = swapchain->next_image();
		}
	}

	unique_ptr<Vulkan::CommandPool> FrameObject::take_secondary_command_pool_()
	{
		if(secondary_command_pools.size()) {
			auto p = move(secondary_command_pools.back());
			secondary_command_pools.pop_back();
			return p;
		}
		auto & d = wgi.get_vulkan_device_();
		return make_unique<Vulkan::CommandPool>(d, d.get_general_queue(), true, true);
	}

	void FrameObject::return_secondary_command_pool_(unique_ptr<Vulkan::CommandPool> p)
	{
		secondary_command_pools.push_back(move(p));
	}
	
	unsigned int WGI::image_count() const
	{
		return swapchain->get_number_of_images();
	}

	// TODO put frame and render objects in separate cpp file

	FrameObject::FrameObject(WGI& wgi_, Ref<Vulkan::ImageView> swapchain_image_view)
	: wgi(wgi_)
	{
		auto & vulkan_device = wgi.get_vulkan_device_();

		primary_command_pool = make_unique<Vulkan::CommandPool>(vulkan_device, vulkan_device.get_general_queue(), true, false);

		vector<reference_wrapper<const Vulkan::DescriptorLayout>> layouts;
		layouts.push_back(wgi.get_descriptor_layout_());

		descriptor_pool = make_unique<Vulkan::DescriptorPool>(vulkan_device, 
			ArrayPointer<const reference_wrapper<const Vulkan::DescriptorLayout>>(layouts.data(), layouts.size()));


		layouts[0] = wgi.get_ssao_descriptor_layout_();
		ssao_descriptor_pool = make_unique<Vulkan::DescriptorPool>(vulkan_device, 
			ArrayPointer<const reference_wrapper<const Vulkan::DescriptorLayout>>(layouts.data(), layouts.size()));


		ssao_blur_descriptor_pool = make_unique<Vulkan::DescriptorPool>(vulkan_device, 
			ArrayPointer<const reference_wrapper<const Vulkan::DescriptorLayout>>(layouts.data(), layouts.size()));



		layouts[0] = wgi.get_hdr_image_descriptor_layout_();
		post_process_descriptor_pool = make_unique<Vulkan::DescriptorPool>(vulkan_device, 
			ArrayPointer<const reference_wrapper<const Vulkan::DescriptorLayout>>(layouts.data(), layouts.size()));

		MemoryCriteria framebuffer_mem_criteria;
        framebuffer_mem_criteria.device_local = MemoryTypePerference::Must;
        framebuffer_mem_criteria.host_visible = MemoryTypePerference::PreferedNot;
        framebuffer_mem_criteria.host_coherent = MemoryTypePerference::PreferedNot;
        framebuffer_mem_criteria.host_cached = MemoryTypePerference::PreferedNot;

		{
			depth_buffer_image = Ref<Vulkan::Image>::make(vulkan_device, ImageFormat::DepthF32, wgi.get_swapchain_()->get_width(), wgi.get_swapchain_()->get_height(), framebuffer_mem_criteria);
			auto depth_memory = Ref<Vulkan::MemoryAllocation>::make(vulkan_device, framebuffer_mem_criteria);
			depth_buffer_image->bind_memory(depth_memory, MemoryRegion(0, framebuffer_mem_criteria.size));

			depth_buffer_image_view = Ref<Vulkan::ImageView>::make(depth_buffer_image);
		}


		{
			hdr_colour_image = Ref<Vulkan::Image>::make(vulkan_device, ImageFormat::RGBALinearF16, wgi.get_swapchain_()->get_width(), wgi.get_swapchain_()->get_height(), framebuffer_mem_criteria);
			auto hdr_colour_memory = Ref<Vulkan::MemoryAllocation>::make(vulkan_device, framebuffer_mem_criteria);
			hdr_colour_image->bind_memory(hdr_colour_memory, MemoryRegion(0, framebuffer_mem_criteria.size));

			hdr_colour_image_view = Ref<Vulkan::ImageView>::make(hdr_colour_image);
		}


		{
			ssao_image = Ref<Vulkan::Image>::make(vulkan_device, ImageFormat::U8, wgi.get_swapchain_()->get_width(), wgi.get_swapchain_()->get_height(), framebuffer_mem_criteria);
			auto ssao_memory = Ref<Vulkan::MemoryAllocation>::make(vulkan_device, framebuffer_mem_criteria);
			ssao_image->bind_memory(ssao_memory, MemoryRegion(0, framebuffer_mem_criteria.size));
			
			ssao_image_view = Ref<Vulkan::ImageView>::make(ssao_image);
		}

		{
			ssao_blur_image = Ref<Vulkan::Image>::make(vulkan_device, ImageFormat::U8, wgi.get_swapchain_()->get_width(), wgi.get_swapchain_()->get_height(), framebuffer_mem_criteria);
			auto ssao_blur_memory = Ref<Vulkan::MemoryAllocation>::make(vulkan_device, framebuffer_mem_criteria);
			ssao_blur_image->bind_memory(ssao_blur_memory, MemoryRegion(0, framebuffer_mem_criteria.size));
			
			ssao_blur_image_view = Ref<Vulkan::ImageView>::make(ssao_blur_image);
		}



		framebuffer_depth = make_unique<Vulkan::Framebuffer>(wgi.get_depth_renderpass_(), nullptr, depth_buffer_image_view);
		framebuffer_hdr_render = make_unique<Vulkan::Framebuffer>(wgi.get_main_renderpass_(), hdr_colour_image_view, depth_buffer_image_view);
		framebuffer_post = make_unique<Vulkan::Framebuffer>(wgi.get_post_renderpass_(), swapchain_image_view, nullptr);
		framebuffer_ssao = make_unique<Vulkan::Framebuffer>(wgi.get_ssao_renderpass_(), ssao_image_view, nullptr);
		framebuffer_ssao_blur = make_unique<Vulkan::Framebuffer>(wgi.get_ssao_renderpass_(), ssao_blur_image_view, nullptr);


		post_process_descriptor_pool->update_descriptor(0, 0, hdr_colour_image_view, wgi.get_hdr_image_sampler_());
		ssao_descriptor_pool->update_descriptor(0, 0, depth_buffer_image_view, wgi.get_depth_sampler_());
		ssao_blur_descriptor_pool->update_descriptor(0, 0, ssao_image_view, wgi.get_ssao_image_sampler_());
		descriptor_pool->update_descriptor(0, 1, ssao_blur_image_view, wgi.get_ssao_image_sampler_());

		query_pool = make_unique<Vulkan::QueryPool>(vulkan_device, Vulkan::QueryPool::Type::Timer, 4);
	}

	void FrameObject::set_scene_uniform_data(SceneUniformData & scene_uniform_data, unsigned int viewport_width, unsigned int viewport_height)
	{
		scene_uniform_data.time = get_time_seconds();

		float vw = static_cast<float>(viewport_width);
		float vh = static_cast<float>(viewport_height);

		scene_uniform_data.one_pixel_x = 1.0f / vw;
		scene_uniform_data.one_pixel_y = 1.0f / vh;
		scene_uniform_data.viewport_size = glm::vec2(vw, vh);

		auto& lights = wgi.edit_lights();

		if(lights.size() > 4) {
			lights.resize(4);
		}

		scene_uniform_data.number_of_lights = lights.size();

		for(unsigned int i = 0; i < lights.size(); i++) {
			scene_uniform_data.lights[i].neg_direction = -(lights[i].matrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
			scene_uniform_data.lights[i].is_shadow_caster = lights[i].casts_shadows ? 1.0f : 0.0f;
			scene_uniform_data.lights[i].intensity = lights[i].intensity;
			scene_uniform_data.lights[i].shadow_proj_view = lights[i].get_projection_matrix() * glm::inverse(lights[i].matrix);
			scene_uniform_data.lights[i].shadow_pixel_offset = 0.5f / static_cast<float>(lights[i].shadow_map_resolution);
		}
	}

	SceneUniformData & FrameObject::edit_scene_uniform_data()
	{
		auto const& swapchain = *wgi.get_swapchain_();	
		auto& scene_uniform_data = *reinterpret_cast<SceneUniformData*>(uniform_buffer->map_for_staging().data());
		set_scene_uniform_data(scene_uniform_data, swapchain.get_width(), swapchain.get_height());
		
		return scene_uniform_data;
	}

	ArrayPointer<ObjectUniformData> FrameObject::edit_object_uniform_data()
	{
		unsigned int n = (uniform_buffer->get_buffer().get_size() - sizeof(SceneUniformData)) / sizeof(ObjectUniformData);
		auto p = reinterpret_cast<ObjectUniformData*>(&uniform_buffer->map_for_staging().data()[sizeof(SceneUniformData
			)]);
		return ArrayPointer<ObjectUniformData>(p,n);	
	}

	glm::mat4 Light::get_projection_matrix() const
	{
		const float light_size = 15.0f;
		return get_coord_system_switch_matrix() * glm::ortho(-light_size, light_size, -light_size, light_size, shadow_map_near_plane, shadow_map_far_plane)
					* glm::scale(glm::vec3(1.0f,1.0f,-1.0f)); // I don't know why the Z needs to be flipped but it works.
	}

	tuple<SceneUniformData &, glm::mat4, glm::mat4> FrameObject::edit_shadow_scene_uniform_data(unsigned int light_index)
	{
		assert__(light_index <= wgi.edit_lights().size(), "No such light");
		auto const& light = wgi.edit_lights()[light_index];

		for(auto& so: shadow_objects) {
			if(so.light_index == light_index) {
				auto& buffer = so.uniform_buffer;
				auto& scene_uniform_data = *reinterpret_cast<SceneUniformData*>(buffer->map_for_staging().data());
				set_scene_uniform_data(scene_uniform_data, light.shadow_map_resolution, light.shadow_map_resolution);

				auto proj = light.get_projection_matrix();
				
				auto view = glm::inverse(light.matrix);
				scene_uniform_data.proj = proj;
				scene_uniform_data.view = view;
				scene_uniform_data.proj_view = proj*view;

				return {scene_uniform_data, proj, view};
			}
		}
		assert_(false);
		// return reinterpret_cast<SceneUniformData*>(nullptr);
	}

	ArrayPointer<ObjectUniformData> FrameObject::edit_shadow_object_uniform_data(unsigned int light_index)
	{
		assert__(light_index <= wgi.edit_lights().size(), "No such light");

		for(auto& so: shadow_objects) {
			if(so.light_index == light_index) {
				auto& buffer = so.uniform_buffer;
				unsigned int n = (buffer->get_buffer().get_size() - sizeof(SceneUniformData)) / sizeof(ObjectUniformData);
				auto p = reinterpret_cast<ObjectUniformData*>(
					&buffer->map_for_staging().data()[sizeof(SceneUniformData)]
				);
				return ArrayPointer<ObjectUniformData>(p,n);
			}
		}
		assert_(false);
		return ArrayPointer<ObjectUniformData>(nullptr, 0);
	}

	
	void FrameObject::start_(unsigned int num_objects)
	{
		if(num_objects > 16384) {
			throw runtime_error("Too many objects");
		}

		clear_secondary_command_buffers();

		meshes.clear();
		pipelines.clear();

		frame_number_modulo = wgi.get_frame_number_modulo();

		get_depth_cmd_buf();
		get_render_cmd_buf();

		auto& lights = wgi.edit_lights();

		if(lights.size() > 4) {
			lights.resize(4);
		}

		struct LightPtr {
			reference_wrapper<Light> light;
			unsigned int index;
		};

		vector<LightPtr> light_ptrs;
		light_ptrs.reserve(lights.size());
		{
			unsigned int i = 0;
			for(Light & l : lights) {
				light_ptrs.push_back(LightPtr{l, i++});
			}
		}

		sort (light_ptrs.begin(), light_ptrs.end(), [](LightPtr& a, LightPtr& b){
			if(!a.light.get().casts_shadows) {
				return true;
			}
			if(!b.light.get().casts_shadows) {
				return false;
			}
			return a.light.get().shadow_map_resolution < b.light.get().shadow_map_resolution;
		});

		// Assign shadow objects

		vector<bool> shadow_obj_assigned(shadow_objects.size());
		vector<bool> light_assigned(lights.size());

		
		for(LightPtr& lptr: light_ptrs) {
			if(lptr.light.get().casts_shadows) {
				// Find most suitable unassigned shadow object

				unsigned int i = 0;
				unsigned int smallest_resolution = 999999;
				unsigned int chosen_index = 999;
				for(auto & s : shadow_objects) {
					if(!shadow_obj_assigned[i] and s.resolution >= lptr.light.get().shadow_map_resolution
							and s.resolution < smallest_resolution) {
						smallest_resolution = s.resolution;
						chosen_index = i;
					}
					i++;
				}

				i = 0;
				for(auto & so : shadow_objects) {
					if(i == chosen_index) {
						so.light_index = lptr.index;
						light_assigned[lptr.index] = true;
						shadow_obj_assigned[chosen_index] = true;
					}

					i++;
				}
			}
			else {
				descriptor_pool->update_descriptor(0, 2, wgi.get_white_1px_image_view_(), wgi.get_shadow_sampler_(), lptr.index);
			}
		}

		// Remove unassigned

		{
			unsigned int j = 0;
			for (auto it = shadow_objects.begin(); it != shadow_objects.end(); ++it) {
				if(!shadow_obj_assigned[j]) {
					shadow_objects.erase(it);
				}
				else {
					j++;
				}
			}
		}

		// Add shadow objects for light that are still without
		for(unsigned int i = 0; i < lights.size(); i++) {
			if(!light_assigned[i] and lights[i].casts_shadows and lights[i].shadow_map_resolution) {
				auto& shadow_obj = shadow_objects.emplace_back();
				shadow_obj.light_index = i;

				MemoryCriteria framebuffer_mem_criteria;
				framebuffer_mem_criteria.device_local = MemoryTypePerference::Must;
				framebuffer_mem_criteria.host_visible = MemoryTypePerference::PreferedNot;
				framebuffer_mem_criteria.host_coherent = MemoryTypePerference::PreferedNot;
				framebuffer_mem_criteria.host_cached = MemoryTypePerference::PreferedNot;

				shadow_obj.resolution = lights[i].shadow_map_resolution;

				
				shadow_obj.image = Ref<Vulkan::Image>::make(wgi.get_vulkan_device_(), ImageFormat::DepthF32, 
					lights[i].shadow_map_resolution, lights[i].shadow_map_resolution,
					framebuffer_mem_criteria);
				auto memory = Ref<Vulkan::MemoryAllocation>::make(wgi.get_vulkan_device_(), framebuffer_mem_criteria);
				shadow_obj.image->bind_memory(memory, MemoryRegion(0, framebuffer_mem_criteria.size));

				shadow_obj.image_view = Ref<Vulkan::ImageView>::make(shadow_obj.image);

				shadow_obj.framebuffer = make_unique<Vulkan::Framebuffer>(wgi.get_shadow_renderpass_(), nullptr, shadow_obj.image_view);
				
				
				vector<reference_wrapper<const Vulkan::DescriptorLayout>> layouts;
				layouts.push_back(wgi.get_shadow_descriptor_layout_());

				shadow_obj.descriptor_pool = make_unique<Vulkan::DescriptorPool>(wgi.get_vulkan_device_(), 
					ArrayPointer<const reference_wrapper<const Vulkan::DescriptorLayout>>(layouts.data(), layouts.size()));
			}
		}

		// Bind texture for unused lights
		for(unsigned int i = lights.size(); i < 4; i++) {
			descriptor_pool->update_descriptor(0, 2, wgi.get_white_1px_image_view_(), wgi.get_shadow_sampler_(), i);
		}

		
		unsigned uniform_min_size = num_objects*sizeof(ObjectUniformData) + sizeof(SceneUniformData);
		Vulkan::BufferUsages uniform_buffer_usages;
		uniform_buffer_usages.uniforms = true;

		for(auto& s : shadow_objects) {
			create_cmd_buf_(s.cmd_buf, RenderStage::Shadow, 
				wgi.get_shadow_renderpass_(), *s.framebuffer, *s.descriptor_pool, s.resolution, s.resolution);

			
			if(!s.uniform_buffer or uniform_buffer->get_buffer().get_size() < uniform_min_size) {
				s.uniform_buffer = Ref<Vulkan::AutoBuffer>::make(wgi.get_vulkan_device_(), uniform_min_size, uniform_buffer_usages);
				s.descriptor_pool->update_descriptor(0, 0, s.uniform_buffer);
			}
			descriptor_pool->update_descriptor(0, 2, s.image_view, wgi.get_shadow_sampler_(), s.light_index);
		}

		
		if(!uniform_buffer or uniform_buffer->get_buffer().get_size() < uniform_min_size) {
			uniform_buffer = Ref<Vulkan::AutoBuffer>::make(wgi.get_vulkan_device_(), uniform_min_size, uniform_buffer_usages);
			descriptor_pool->update_descriptor(0, 0, uniform_buffer);
		}

	}

	CmdBuffer& FrameObject::create_cmd_buf_(optional<CmdBuffer> & o, RenderStage stage, 
		optional<reference_wrapper<const Vulkan::RenderPass>> rp, 
		optional<reference_wrapper<const Vulkan::Framebuffer>> framebuffer, 
		optional<reference_wrapper<const Vulkan::DescriptorPool>> descriptor_pool_,
		unsigned int viewport_width, unsigned int viewport_height)
	{
		if(o.has_value()) {
			return o.value();
		}
		o.emplace(rp, take_secondary_command_pool_(), stage, viewport_width, viewport_height, descriptor_pool_, framebuffer);
		return o.value();
	}

	CmdBuffer& FrameObject::get_upload_cmd_buf()
	{
		return create_cmd_buf_(cmd_buf__upload, RenderStage::Upload, nullopt, nullopt, nullopt, 0, 0);
	}

	CmdBuffer& FrameObject::get_depth_cmd_buf()
	{
		auto& sc = *wgi.get_swapchain_();
		return create_cmd_buf_(cmd_buf__depth, RenderStage::Depth, wgi.get_depth_renderpass_(), get_depth_framebuffer(), *descriptor_pool, sc.get_width(), sc.get_height());
	}

	CmdBuffer& FrameObject::get_render_cmd_buf()
	{
		auto& sc = *wgi.get_swapchain_();
		return create_cmd_buf_(cmd_buf__render, RenderStage::Render, wgi.get_main_renderpass_(), get_hdr_framebuffer(), *descriptor_pool, sc.get_width(), sc.get_height());
	}

	CmdBuffer& FrameObject::get_shadow_cmd_buf(unsigned int i)
	{
		assert__(i <= wgi.edit_lights().size(), "No such light");
		for(auto & so : shadow_objects) {
			if(so.light_index == i) {
				return so.cmd_buf.value();
			}
		}
		assert_(false);
		return (*shadow_objects.begin()).cmd_buf.value();
	}

	

	void CmdBuffer::mesh_upload(Ref<Mesh> mesh)
	{
		assert_(render_stage == RenderStage::Upload);
		mesh->upload_(*command_pool);
		meshes.insert(move(mesh));
	}

	CmdBuffer::~CmdBuffer()
	{
	}

	CmdBuffer::CmdBuffer(optional<reference_wrapper<const Vulkan::RenderPass>> rp,
		unique_ptr<Vulkan::CommandPool> command_pool_,
		RenderStage render_stage_, unsigned int viewport_width, unsigned int viewport_height, 
		optional<reference_wrapper<const Vulkan::DescriptorPool>> descriptor_pool_,
		optional<reference_wrapper<const Vulkan::Framebuffer>> framebuffer_)

		: command_pool(move(command_pool_)), descriptor_pool(descriptor_pool_),
			render_stage(render_stage_), render_pass(rp), framebuffer(framebuffer_)
	{
		assert_(render_stage != RenderStage::PostProcessComposite);
		if(render_pass) assert_(framebuffer);
		if(render_stage != RenderStage::Upload) {
			assert_(descriptor_pool.has_value());
		}
		command_pool->start_submit(rp, framebuffer);

		command_pool->set_viewport_size(viewport_width, viewport_height);
	}

	void CmdBuffer::draw_object(Ref<Mesh> mesh, Ref<PipelineInstance> pipeline, unsigned int uniform_index)
	{
		assert_(render_stage != RenderStage::Upload);
		assert_(pipeline);

		PipelineType p_type = PipelineType::Normal;
		if(render_stage == RenderStage::Shadow) {
			p_type = PipelineType::Shadow;
		}
		else if(render_stage == RenderStage::Depth) {
			p_type = PipelineType::DepthOnly;
		}

		command_pool->bind_pipeline(pipeline->get_pipeline_(p_type));	
		command_pool->bind_descriptor_set(descriptor_pool.value(), 0);


		if(mesh) {
			if(p_type != PipelineType::Normal) {
				// Find vertex positions and bind them

				uint64_t positions_offset = UINT64_MAX;
				auto const& attributes = mesh->get_meta().vertex_attributes;
				for(unsigned int i = 0; i < attributes.size(); i++) {
					if(attributes[i] == VertexAttributeType::Position2D or
						attributes[i] == VertexAttributeType::PositionNormalised or
						attributes[i] == VertexAttributeType::Position)
					{
						positions_offset = mesh->get_meta().vertex_offsets[i];
						break;
					}
				}
				command_pool->bind_vertex_buffers(mesh->get_buffer_(), 
					ArrayPointer<const uint64_t> (&positions_offset, 1)
				);
			}
			else {
				auto const& v_off = mesh->get_meta().vertex_offsets;
				command_pool->bind_vertex_buffers(mesh->get_buffer_(), 
					ArrayPointer<const uint64_t> (v_off.data(), v_off.size())
				);
		}
		}

		array<uint32_t, 1> pushc = {uniform_index};

		if(mesh) {
			if(mesh->get_meta().index_count) {
				command_pool->bind_index_buffers(mesh->get_buffer_(), mesh->get_meta().indices_offset, mesh->get_meta().big_indices);
				command_pool->draw_indexed(
					mesh->get_meta().index_count, 0,
					ArrayPointer<uint8_t>(reinterpret_cast<uint8_t*>(pushc.data()), pushc.size()*4)
				);
			}
			else {
				command_pool->draw(mesh->get_meta().vertex_count,
					ArrayPointer<uint8_t>(reinterpret_cast<uint8_t*>(pushc.data()), pushc.size()*4)
				);
			}
			meshes.insert(move(mesh));
		}
		else {
			command_pool->draw(3,
				ArrayPointer<uint8_t>(reinterpret_cast<uint8_t*>(pushc.data()), pushc.size()*4)
			);
		}


		pipelines.insert(move(pipeline));
	}

	FrameTimeStats FrameObject::present()
	{
		FrameTimeStats stats;

		if(first_usage_of_object) {
			first_usage_of_object = false;
		}
		else {
			// Fetch statistics from 2 (or more) frames ago
			auto times = query_pool->get_time_values();

			stats.total_time = times[3] - times[0];
			stats.pre_render_time = times[1] - times[0];
			stats.render_time = times[2] - times[1];
			stats.post_render_time = times[3] - times[2];
		}



		bool timestamps_supported = wgi.get_vulkan_device_().get_general_queue().get_timestamp_bits_valid();

		auto const& swapchain = *wgi.get_swapchain_();
		struct {
			float viewport_width, viewport_height;
			float one_pixel_x, one_pixel_y;
			float ssao_parameter;
		} pushc;
		pushc.viewport_width = static_cast<float>(swapchain.get_width());
		pushc.viewport_height = static_cast<float>(swapchain.get_height());
		pushc.one_pixel_x = 1.0f / pushc.viewport_width;
		pushc.one_pixel_y = 1.0f / pushc.viewport_height;
		pushc.ssao_parameter = wgi.ssao_parameter;


		Vulkan::CommandPool & cmd_pool =  *primary_command_pool;
		cmd_pool.start_submit(nullopt, nullopt);
		cmd_pool.set_viewport_size(swapchain.get_width(), swapchain.get_height());

		if(timestamps_supported) {
			cmd_pool.reset_query_pool(*query_pool);
			cmd_pool.set_timer(*query_pool, 0);
		}

		const auto use_cmd_buf = [this, &cmd_pool](optional<CmdBuffer>& r_cmd_buf__) {
			if(r_cmd_buf__.has_value()) {
				auto& secondary_cmd_buf = r_cmd_buf__.value();

				add_meshes_(secondary_cmd_buf.take_mesh_refs_());
				add_pipelines_(secondary_cmd_buf.take_pipeline_refs_());

				auto& p = secondary_cmd_buf.get_command_pool_();

				if(secondary_cmd_buf.get_render_pass().has_value()) {
					cmd_pool.start_render_pass(secondary_cmd_buf.get_render_pass().value(), secondary_cmd_buf.get_framebuffer().value(), true);
				}

				p.end_secondary();				
				cmd_pool.execute_secondary(p);
				
				if(secondary_cmd_buf.get_render_pass().has_value()) {
					cmd_pool.end_render_pass();
				}

			}
		};

		use_cmd_buf(cmd_buf__upload);
		use_cmd_buf(cmd_buf__depth);

		for(auto& s : shadow_objects) {
			use_cmd_buf(s.cmd_buf);
		}

		{ // SSAO
			cmd_pool.wait_for_depth_write(*depth_buffer_image.get());
			cmd_pool.start_render_pass(wgi.get_ssao_renderpass_(), *framebuffer_ssao, false);
			cmd_pool.bind_pipeline(wgi.get_ssao_pipeline_()->get_pipeline_(PipelineType::Normal));
			cmd_pool.bind_descriptor_set(*ssao_descriptor_pool, 0);
			cmd_pool.draw(3, ArrayPointer<uint8_t>(reinterpret_cast<uint8_t*>(&pushc), sizeof pushc));
			cmd_pool.end_render_pass();
		}

		{
			cmd_pool.wait_for_colour_write(*ssao_image.get());
			cmd_pool.start_render_pass(wgi.get_ssao_blur_renderpass_(), *framebuffer_ssao_blur, false);
			cmd_pool.bind_pipeline(wgi.get_ssao_blur_pipeline_()->get_pipeline_(PipelineType::Normal));
			cmd_pool.bind_descriptor_set(*ssao_blur_descriptor_pool, 0);
			cmd_pool.draw(3, ArrayPointer<uint8_t>(reinterpret_cast<uint8_t*>(&pushc), 4*4));
			cmd_pool.end_render_pass();			
		}

		if(timestamps_supported) {
			cmd_pool.set_timer(*query_pool, 1);
		}



		cmd_pool.wait_for_colour_write(*ssao_blur_image.get());
		use_cmd_buf(cmd_buf__render);

		if(timestamps_supported) {
			cmd_pool.set_timer(*query_pool, 2);
		}

		// use_cmd_buf(cmd_buf__hud);
		
		

		{ // Post-processing
			cmd_pool.wait_for_colour_write(*hdr_colour_image.get());
			cmd_pool.start_render_pass(wgi.get_post_renderpass_(), *framebuffer_post, false);
			cmd_pool.bind_pipeline(wgi.get_post_pipeline_()->get_pipeline_(PipelineType::Normal));
			cmd_pool.bind_descriptor_set(*post_process_descriptor_pool, 0);
			cmd_pool.draw(3, ArrayPointer<uint8_t>(reinterpret_cast<uint8_t*>(&pushc), 4*4));
			cmd_pool.end_render_pass();
		}

		if(timestamps_supported) {
			cmd_pool.set_timer(*query_pool, 3);
		}


		cmd_pool.submit_and_present(swapchain, swapchain.get_render_complete_fence());

		
		try {
			wgi.get_swapchain_()->present(); // blocks
		}
		catch (Vulkan::RecreateSwapchainException&) {
			wgi.create_swapchain_();
		}

		return stats;
		
	}

	void FrameObject::clear_secondary_command_buffers() {
		const auto del = [this](optional<CmdBuffer>& r_cmd_buf__) {
			if(r_cmd_buf__.has_value()) {
				return_secondary_command_pool_(r_cmd_buf__.value().take_command_pool_());
			}
			r_cmd_buf__ = nullopt;
		};

		del(cmd_buf__upload);
		del(cmd_buf__depth);
		del(cmd_buf__render);

		for(auto& s : shadow_objects) {
			del(s.cmd_buf);
		}
	}

	FrameObject::~FrameObject() {
		clear_secondary_command_buffers();
	}


	void FrameObject::add_meshes_(unordered_set<Ref<Mesh>> && m)
	{
		merge(meshes.begin(), meshes.end(),
            m.begin(), m.end(),
            inserter(meshes, meshes.begin()));
	}

	void FrameObject::add_pipelines_(unordered_set<Ref<PipelineInstance>> && i)
	{
		merge(pipelines.begin(), pipelines.end(),
            i.begin(), i.end(),
            inserter(pipelines, pipelines.begin()));
	}

	void WGI::wait_idle()
	{
		vulkan_device->vulkan_wait_idle();
	}

	FrameObject& WGI::start_frame(unsigned int num_objects)
	{
		next_frame_();
		auto& f = *frames[this_frame_swapchain_index];
		f.start_(num_objects);
		return f;
	}

	FrameTimeStats WGI::present()
	{
		return frames[this_frame_swapchain_index]->present();
	}

	glm::mat4 get_coord_system_switch_matrix()
	{
		return glm::scale(glm::vec3(1.0f, -1.0f, 1.0f));
	}


	Vulkan::RenderPass const& WGI::get_renderpass_(RenderStage stage) const
	{
		if(stage == RenderStage::Render) {
			return *main_render_pass;
		}
		else if(stage == RenderStage::PostProcessComposite) {
			return *post_render_pass;
		}
		else {
			assert_(false);
		}
	}
}