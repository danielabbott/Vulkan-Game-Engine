#include "Device.h"
#include "Buffer.h"
#include "Include.h"
#include <limits>

using namespace std;

namespace Vulkan {

    Buffer::Buffer(Device& device_, uint32_t bytes, WGI::MemoryCriteria& criteria, BufferUsages usages_)
        : device(device_), size(bytes), usages(usages_)
    {
        auto d = device.get_device_handle_();

        VkBufferCreateInfo create{};
        create.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create.size = bytes;
        create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (usages.vertices) {
            create.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (usages.indices) {
            create.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if (usages.transfer_dst) {
            create.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        if (usages.transfer_src) {
            create.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if (usages.uniforms) {
            create.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }

        assert__(vkCreateBuffer(d, &create, nullptr, &buffer) == VK_SUCCESS, "vkCreateBuffer error");

        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(d, buffer, &memory_requirements);

        criteria.alignment = static_cast<uint32_t>(memory_requirements.alignment);
        criteria.memory_type_bits = memory_requirements.memoryTypeBits;

        assert_(memory_requirements.size <= numeric_limits<uint32_t>::max());
        criteria.size = static_cast<uint32_t>(memory_requirements.size);
    }

    void Buffer::bind_memory(Ref<MemoryAllocation> memory_to_use, WGI::MemoryRegion region)
    {
        assert_(region.size >= size);
        assert__(vkBindBufferMemory(device.get_device_handle_(), buffer, memory_to_use->get_handle(), region.start) == VK_SUCCESS, "vkBindBufferMemory error");
        memory = memory_to_use;
        memory_properties = memory.value()->get_properties();
        memory_region = region;
    }

    Buffer::~Buffer()
    {
        if (buffer) {
            vkDestroyBuffer(device.get_device_handle_(), buffer, nullptr);
            buffer = nullptr;
        }
    }
}