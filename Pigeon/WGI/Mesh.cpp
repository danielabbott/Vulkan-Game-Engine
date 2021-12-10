#include "Mesh.h"
#include "../Vulkan/AutoBuffer.h"
#include "RenderState.h"
#include "../Pigeon.h"
#include "WGI.h"

using namespace std;

namespace WGI {

	Mesh::Mesh(WGI& wgi_, MeshMeta meta_)
		: wgi(wgi_), meta(meta_)
	{
	}
	Mesh::~Mesh()
	{
	}


	ArrayPointer<uint8_t> Mesh::create_staging_buffer(uint32_t size)
	{
		Vulkan::BufferUsages buffer_usages{};
		buffer_usages.vertices = true;
		buffer_usages.indices = true;

		buffer = make_unique<Vulkan::AutoBuffer>(wgi.get_vulkan_device_(), size, buffer_usages);
		auto map = buffer->map_for_staging();

		data_size = size;

		return map;
	}

	void Mesh::upload_(Vulkan::CommandPool & cmd_pool)
	{
		assert_(buffer);
		buffer->staging_complete(cmd_pool, false);
	}

	Vulkan::Buffer const& Mesh::get_buffer_() const
	{
		assert_(buffer);
		return buffer->get_buffer();
	}
}