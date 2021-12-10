#include "Device.h"
#include "Shader.h"
#include "../BetterAssert.h"
#include "Include.h"
#include "../IncludeSpdlog.h"

namespace Vulkan {

	Shader::Shader(Device& device_, ArrayPointer<unsigned char> bytecode)
		: device(device_)
	{
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = bytecode.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

		assert__(vkCreateShaderModule(device.get_device_handle_(), &create_info, nullptr, &handle) == VK_SUCCESS, "vkCreateShaderModule error");
	}

	VkShaderModule Shader::get_handle() const
	{
		return handle;
	}

	Shader::~Shader()
	{
		vkDestroyShaderModule(device.get_device_handle_(), handle, nullptr);
	}
}