#pragma once

#include <stdbool.h>
#include <stdint.h>

ERROR_RETURN_TYPE pigeon_create_vulkan_context(bool prefer_dedicated_gpu);
void pigeon_vulkan_wait_idle(void);
void pigeon_destroy_vulkan_context(void);

unsigned int pigeon_vulkan_get_uniform_buffer_min_alignment(void);


typedef struct PigeonVulkanDrawIndexedIndirectCommand {
    uint32_t    indexCount;
    uint32_t    instanceCount;
    uint32_t    firstIndex;
    int32_t     vertexOffset;
    uint32_t    firstInstance;
} PigeonVulkanDrawIndexedIndirectCommand;
