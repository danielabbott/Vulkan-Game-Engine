#include "Shader.h"
#include "WGI.h"
#include "../FileIO.h"
#include "../Vulkan/Shader.h"

using namespace std;

namespace WGI {

	Shader::Shader(WGI& wgi_, std::string const& file_path, Type t)
	: wgi(wgi_), type(t)
	{
		auto bytecode = load_file_blocking(file_path);
		shader = make_unique<Vulkan::Shader>(wgi.get_vulkan_device_(), bytecode.get_array_ptr<uint32_t>());
	}

	Shader::~Shader() {}
}