#pragma once

#include <string>
#include <memory>
#include "../Ref.h"

namespace Vulkan {
	class Shader;
};

namespace WGI {

	class WGI;

	class Shader {
	public:

		enum class Type {
			Vertex, Fragment
		};

		WGI& wgi;

		Type type;

		std::unique_ptr<Vulkan::Shader> shader;
	public:
		Shader(WGI&, std::string const& file_path, Type);
		~Shader();

		WGI& get_wgi() const {
			return wgi;
		};

		Vulkan::Shader const& get_shader_() const {
			return *shader;
		}
	};
}