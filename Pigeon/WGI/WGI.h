#pragma once

#include <memory>
#include "../Ref.h"
#include "GraphicsSettings.h"
#include "../ArrayPointer.h"
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <unordered_set>
#include <tuple>
#include "../IncludeGLM.h"
#include "Uniforms.h"
#include <list>

namespace Vulkan {
	class Device;
	class RenderPass;
	class Swapchain;
	class CommandPool;
	class DescriptorPool;
	class DescriptorLayout;
	class AutoBuffer;
	class Image;
	class ImageView;
	class Framebuffer;
	class Pipeline;
	class Sampler;
	class QueryPool;
}

namespace WGI {
	
	class Mesh;
	class PipelineInstance;

	enum class RenderStage {
		Upload, // Uploading meshes, textures, etc.
		Depth, // Depth prepass
		Shadow, // One depth pass pass per shadow-casting light source
		Render, // HDR render of scene. One render pass for each active camera
		PostProcessComposite // Colour correction etc. and overlays the UI
	};


	

	struct FrameTimeStats {
		// GPU time to render scene
		float total_time = 0;

		// Depth texture, shadow maps, SSAO
		float pre_render_time = 0;

		// HDR render
		float render_time = 0;

		// Tonemapping, FXAA
		float post_render_time = 0;
	};

	// Lights are currently all directional
	struct Light {
		// Identity matrix = light at (0,0,0) facing +Z
		// Position is used for shadows
		glm::mat4 matrix;
		
		glm::vec3 intensity = glm::vec3(0.1f,0.1f,0.1f);
		bool casts_shadows = false;

		unsigned int shadow_map_resolution = 2048;
		float shadow_map_near_plane = 1;
		float shadow_map_far_plane = 100;

		Light(){}

		Light(glm::mat4 matrix_, glm::vec3 intensity_, bool casts_shadows_ = false)
		: matrix(matrix_), intensity(intensity_), casts_shadows(casts_shadows_)
		{}

		glm::mat4 get_projection_matrix() const;
	};

	class FrameObject;
	class WGI {
		Ref<Window> window;
		std::unique_ptr<Vulkan::Device> vulkan_device;
		std::unique_ptr<Vulkan::RenderPass> depth_render_pass;
		std::unique_ptr<Vulkan::RenderPass> ssao_render_pass;
		std::unique_ptr<Vulkan::RenderPass> ssao_blur_render_pass;
		std::unique_ptr<Vulkan::RenderPass> main_render_pass;
		std::unique_ptr<Vulkan::RenderPass> post_render_pass;
		Ref<Vulkan::Swapchain> swapchain;


		WindowDimensions window_dimensions;

		WindowVisibilityState poll_or_wait_for_input(std::function<void()> f);

		unsigned int frame_number = 0;

		// Index of active swapchain image (same index used for frames vector)
		unsigned int this_frame_swapchain_index = -1;

		std::vector<std::unique_ptr<FrameObject>> frames;
		std::unique_ptr<Vulkan::DescriptorLayout> descriptor_layout;
		std::unique_ptr<Vulkan::DescriptorLayout> post_and_ssao_descriptor_layout;
		std::unique_ptr<Vulkan::DescriptorLayout> shadow_descriptor_layout;
		
		Ref<Vulkan::Sampler> ssao_image_sampler;
		Ref<Vulkan::Sampler> hdr_and_depth_sampler;
		Ref<Vulkan::Sampler> shadow_sampler;

		Ref<PipelineInstance> ssao_pipeline;
		Ref<PipelineInstance> ssao_blur_pipeline;
		Ref<PipelineInstance> post_pipeline;


		std::vector<Light> lights;

		Ref<Vulkan::Image> white_1px_image;
		Ref<Vulkan::ImageView> white_1px_image_view;


	public:
		float ssao_parameter = 0.0019f;

		Ref<Window> const& get_window() {
			return window;
		};
		 

		Vulkan::RenderPass const& get_renderpass_(RenderStage) const;

		WGI(WindowTitle window_title, GraphicsSettings graphics);

		auto & edit_lights() { return lights; }

		bool should_terminate() const;
		WindowVisibilityState process_input();
		WindowVisibilityState wait_for_input();

		bool supports_multithreaded_rendering() const { return true; }

		unsigned int image_count() const;

		unsigned int get_frame_number() const
		{
			return frame_number;
		}

		unsigned int get_frame_number_modulo() const
		{
			return frame_number % image_count();
		}

		void wait_idle(); // Avoid calling this

		// waits for previous frame to finish
		// num_objects is the number of uniform indices needed (= number of game objects with mesh renderer components)
		FrameObject& start_frame(unsigned int num_objects);

		// Submits command buffers then swaps buffers asynchronously
		// ** Statistics lag 2* frames behind (* number of swapchain images)
		FrameTimeStats present();

		void destroy();



		~WGI();
		WGI(const WGI&) = delete;
		WGI& operator=(const WGI&) = delete;
		WGI(WGI&&) = delete;
		WGI& operator=(WGI&&) = delete;

		Vulkan::Device & get_vulkan_device_() const {
			return *vulkan_device;
		}
		Vulkan::RenderPass const& get_depth_renderpass_() const {
			return *depth_render_pass;
		}
		Vulkan::RenderPass const& get_shadow_renderpass_() const {
			return *depth_render_pass;
		}
		Vulkan::RenderPass const& get_main_renderpass_() const {
			return *main_render_pass;
		}
		Vulkan::RenderPass const& get_post_renderpass_() const {
			return *post_render_pass;
		}
		Vulkan::RenderPass const& get_ssao_renderpass_() const {
			return *ssao_render_pass;
		}
		Vulkan::RenderPass const& get_ssao_blur_renderpass_() const {
			return *ssao_blur_render_pass;
		}
		Ref<Vulkan::Swapchain> const& get_swapchain_() const {
			return swapchain;
		}
		Ref<PipelineInstance> const& get_post_pipeline_() const {
			return post_pipeline;
		}
		Ref<PipelineInstance> const& get_ssao_pipeline_() const {
			return ssao_pipeline;
		}
		Ref<PipelineInstance> const& get_ssao_blur_pipeline_() const {
			return ssao_blur_pipeline;
		}
		Ref<Vulkan::Sampler> const& get_hdr_image_sampler_() const {
			return hdr_and_depth_sampler;
		}
		Ref<Vulkan::Sampler> const& get_ssao_image_sampler_() const {
			return ssao_image_sampler;
		}
		Ref<Vulkan::Sampler> const& get_depth_sampler_() const {
			return hdr_and_depth_sampler;
		}
		Ref<Vulkan::Sampler> const& get_shadow_sampler_() const {
			return shadow_sampler;
		}

		Ref<Vulkan::ImageView> get_white_1px_image_view_() {
			return white_1px_image_view;
		}
		

		void next_frame_();


		void create_swapchain_();

		Vulkan::DescriptorLayout const& get_descriptor_layout_() const
		{
			return *descriptor_layout;
		}
		Vulkan::DescriptorLayout const& get_hdr_image_descriptor_layout_() const
		{
			return *post_and_ssao_descriptor_layout;
		}
		Vulkan::DescriptorLayout const& get_ssao_descriptor_layout_() const
		{
			return *post_and_ssao_descriptor_layout;
		}
		Vulkan::DescriptorLayout const& get_shadow_descriptor_layout_() const
		{
			return *shadow_descriptor_layout;
		}
		

		Vulkan::DescriptorLayout const& get_descriptor_layout_(RenderStage stage) const
		{
			return stage == RenderStage::PostProcessComposite ? *post_and_ssao_descriptor_layout : *descriptor_layout;
		}
		
	};



	// One (secondary) command buffer per render stage per frame - except for post processing 
	class CmdBuffer {
		std::unique_ptr<Vulkan::CommandPool> command_pool;
		std::unordered_set<Ref<Mesh>> meshes;
		std::unordered_set<Ref<PipelineInstance>> pipelines;
		std::optional<std::reference_wrapper<const Vulkan::DescriptorPool>> descriptor_pool;
		RenderStage render_stage;

		std::optional<std::reference_wrapper<const Vulkan::RenderPass>> render_pass;
		std::optional<std::reference_wrapper<const Vulkan::Framebuffer>> framebuffer;

	public:
		CmdBuffer(std::optional<std::reference_wrapper<const Vulkan::RenderPass>>,
			std::unique_ptr<Vulkan::CommandPool>, RenderStage,
			unsigned int viewport_width, unsigned int viewport_height,
			std::optional<std::reference_wrapper<const Vulkan::DescriptorPool>>, 
			std::optional<std::reference_wrapper<const Vulkan::Framebuffer>>);

		~CmdBuffer();
		
		void draw_object(Ref<Mesh> mesh, Ref<PipelineInstance> pipeline, unsigned int uniform_index);
		void mesh_upload(Ref<Mesh>);

		std::optional<std::reference_wrapper<const Vulkan::RenderPass>> get_render_pass() const { return render_pass; }
		std::optional<std::reference_wrapper<const Vulkan::Framebuffer>> get_framebuffer() const { return framebuffer; }

		Vulkan::CommandPool & get_command_pool_() {
			return *command_pool;
		}

		std::unique_ptr<Vulkan::CommandPool> take_command_pool_() {
			return std::move(command_pool);
		}

		std::unordered_set<Ref<Mesh>> take_mesh_refs_() {
			return std::move(meshes);
		}

		std::unordered_set<Ref<PipelineInstance>> take_pipeline_refs_() {
			return std::move(pipelines);
		}

		RenderStage get_stage() const { return render_stage; }

	};

	// Create this object each frame
	class FrameObject {
		WGI& wgi;
		bool first_usage_of_object = true;
		unsigned int frame_number_modulo = 0;

		std::unordered_set<Ref<Mesh>> meshes;
		std::unordered_set<Ref<PipelineInstance>> pipelines;

		// TODO move all but light_index into struct RenderObjects{...}; 
		struct ShadowObjects {
			unsigned int light_index;
			unsigned int resolution;
			std::optional<CmdBuffer> cmd_buf;
			Ref<Vulkan::Image> image;
			Ref<Vulkan::ImageView> image_view;
			std::unique_ptr<Vulkan::Framebuffer> framebuffer;
			Ref<Vulkan::AutoBuffer> uniform_buffer;
			std::unique_ptr<Vulkan::DescriptorPool> descriptor_pool;
		};

		std::list<ShadowObjects> shadow_objects;

		std::optional<CmdBuffer> cmd_buf__upload;
		
		std::optional<CmdBuffer> cmd_buf__depth;
		std::optional<CmdBuffer> cmd_buf__render;

		std::unique_ptr<Vulkan::CommandPool> primary_command_pool;

		// One per thread
		std::vector<std::unique_ptr<Vulkan::CommandPool>> secondary_command_pools;

		std::unique_ptr<Vulkan::DescriptorPool> ssao_descriptor_pool;
		std::unique_ptr<Vulkan::DescriptorPool> ssao_blur_descriptor_pool;
		std::unique_ptr<Vulkan::DescriptorPool> descriptor_pool;
		std::unique_ptr<Vulkan::DescriptorPool> post_process_descriptor_pool;

		Ref<Vulkan::AutoBuffer> uniform_buffer;


		Ref<Vulkan::Image> depth_buffer_image;
		Ref<Vulkan::ImageView> depth_buffer_image_view;

		Ref<Vulkan::Image> ssao_image;
		Ref<Vulkan::ImageView> ssao_image_view;

		Ref<Vulkan::Image> ssao_blur_image;
		Ref<Vulkan::ImageView> ssao_blur_image_view;

		Ref<Vulkan::Image> hdr_colour_image;
		Ref<Vulkan::ImageView> hdr_colour_image_view;


		std::unique_ptr<Vulkan::Framebuffer> framebuffer_depth;
		std::unique_ptr<Vulkan::Framebuffer> framebuffer_ssao;
		std::unique_ptr<Vulkan::Framebuffer> framebuffer_ssao_blur;
		std::unique_ptr<Vulkan::Framebuffer> framebuffer_hdr_render;
		std::unique_ptr<Vulkan::Framebuffer> framebuffer_post;

		std::unique_ptr<Vulkan::QueryPool> query_pool;


		void add_meshes_(std::unordered_set<Ref<Mesh>> && m);
		void add_pipelines_(std::unordered_set<Ref<PipelineInstance>> && i);
		CmdBuffer& create_cmd_buf_(std::optional<CmdBuffer> & o, RenderStage,
			std::optional<std::reference_wrapper<const Vulkan::RenderPass>>,
			std::optional<std::reference_wrapper<const Vulkan::Framebuffer>>,
			std::optional<std::reference_wrapper<const Vulkan::DescriptorPool>>,
			unsigned int viewport_width, unsigned int viewport_height);

		void clear_secondary_command_buffers();

		std::unique_ptr<Vulkan::CommandPool> take_secondary_command_pool_();
		void return_secondary_command_pool_(std::unique_ptr<Vulkan::CommandPool>);


		Vulkan::Framebuffer const& get_depth_framebuffer() const {
			return *framebuffer_depth;
		}
		Vulkan::Framebuffer const& get_hdr_framebuffer() const {
			return *framebuffer_hdr_render;
		}
		Vulkan::Framebuffer const& get_swapchain_framebuffer() {
			return *framebuffer_post;
		}

		void set_scene_uniform_data(SceneUniformData &, unsigned int viewport_width, unsigned int viewport_height);
	public:
		FrameObject(WGI&, Ref<Vulkan::ImageView> swapchain_image_view);
		void start_(unsigned int num_objects);
		~FrameObject();

		CmdBuffer& get_upload_cmd_buf();
		CmdBuffer& get_depth_cmd_buf();
		CmdBuffer& get_render_cmd_buf();

		// light_index: index into wgi.lights
		CmdBuffer& get_shadow_cmd_buf(unsigned int light_index);

		SceneUniformData & edit_scene_uniform_data();
		ArrayPointer<ObjectUniformData> edit_object_uniform_data();

		// scene data, projection matrix, view matrix
		std::tuple<SceneUniformData &, glm::mat4, glm::mat4> edit_shadow_scene_uniform_data(unsigned int light_index);
		ArrayPointer<ObjectUniformData> edit_shadow_object_uniform_data(unsigned int light_index);

		FrameTimeStats present();
	};

	// Flips Y-axis for Vulkan
	glm::mat4 get_coord_system_switch_matrix();

	uint64_t get_time_micro();
	uint32_t get_time_millis();
	float get_time_seconds();
	double get_time_seconds_double();
}