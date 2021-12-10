#include "Sampler.h"
#include "../BetterAssert.h"
#include "Include.h"
#include "Device.h"

namespace Vulkan {

    Sampler::Sampler(Device& d_, Type type)
        :device(d_)
    {
        VkSamplerCreateInfo sampler_create {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

        if(type == Type::Nearest or type == Type::PostProcess) {
            sampler_create.magFilter = VK_FILTER_NEAREST;
            sampler_create.minFilter = VK_FILTER_NEAREST;
        }
        else {
            sampler_create.magFilter = VK_FILTER_LINEAR;
            sampler_create.minFilter = VK_FILTER_LINEAR;
        }

        if(device.supports_anisotropic_filtering() and type == Type::Smooth) {
            sampler_create.anisotropyEnable = VK_TRUE;
            sampler_create.maxAnisotropy = 16;
        }
        sampler_create.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

        if(type == Type::Shadow) {
            sampler_create.compareEnable = true;
            sampler_create.compareOp = VK_COMPARE_OP_LESS;
        }

        if(type == Type::PostProcess or type == Type::SSAO) {
            // sampler_create.unnormalizedCoordinates = VK_TRUE;
            sampler_create.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_create.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        }

        assert__(vkCreateSampler(device.get_device_handle_(), &sampler_create, nullptr, &sampler_handle) == VK_SUCCESS, "vkCreateSampler error");
    }

    Sampler::~Sampler() 
    {
        vkDestroySampler(device.get_device_handle_(), sampler_handle, nullptr);
    }

}