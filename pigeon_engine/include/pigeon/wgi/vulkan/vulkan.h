#pragma once

#include <stdbool.h>
#include <stdint.h>

PIGEON_ERR_RET pigeon_create_vulkan_context(bool prefer_dedicated_gpu);
void pigeon_vulkan_wait_idle(void);
void pigeon_destroy_vulkan_context(void);

unsigned int pigeon_vulkan_get_buffer_min_alignment(void);


typedef struct PigeonVulkanDrawIndexedIndirectCommand {
    uint32_t    indexCount;
    uint32_t    instanceCount;
    uint32_t    firstIndex;
    int32_t     vertexOffset;
    uint32_t    firstInstance;
} PigeonVulkanDrawIndexedIndirectCommand;

// PIGEON_WGI_IMAGE_FORMAT_B10G11R11_UF_LINEAR
// If false then use PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR for HDR framebuffers
bool pigeon_vulkan_compact_hdr_framebuffer_available(void);

bool pigeon_vulkan_bc1_optimal_available(void);
bool pigeon_vulkan_bc3_optimal_available(void);
bool pigeon_vulkan_bc5_optimal_available(void);
bool pigeon_vulkan_bc7_optimal_available(void);
bool pigeon_vulkan_etc1_optimal_available(void);
bool pigeon_vulkan_etc2_optimal_available(void);
bool pigeon_vulkan_etc2_rgba_optimal_available(void);
