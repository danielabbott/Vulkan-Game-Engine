#pragma once

#include "../ArrayPointer.h"
#include "../Ref.h"

struct VkShaderModule_T;

namespace Vulkan {

	class Device;

	class Shader {
		Device& device;
		VkShaderModule_T* handle;

	public:

		Shader(Device&, ArrayPointer<unsigned char> bytecode);
		~Shader();

		VkShaderModule_T* get_handle() const;

		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;

		Shader(Shader&&) = delete;
		Shader& operator=(Shader&&) = delete;
	};
}