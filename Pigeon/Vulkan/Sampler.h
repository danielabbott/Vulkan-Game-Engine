#pragma once

struct VkSampler_T;

namespace Vulkan {

    class Device;

    class Sampler {
        Device& device;
        VkSampler_T* sampler_handle;

    public:
        enum class Type {
            Nearest, Smooth, 
            PostProcess, // Nearest filtering for FXAA (linear filter done in GLSL)
            Depth = PostProcess, // Depth texture lookups
            Shadow,
            SSAO, // Linear filtering on SSAO framebuffer (for blur)
            Blur = SSAO
        };

        Sampler(Device&, Type);
        ~Sampler();

        VkSampler_T* get_handle_() const { return sampler_handle; }

		Sampler(const Sampler&) = delete;
		Sampler& operator=(const Sampler&) = delete;
		Sampler(Sampler&&) = delete;
		Sampler& operator=(Sampler&&) = delete;
    };

}