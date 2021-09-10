#pragma once

#include <pigeon/wgi/textures.h>
#include <pigeon/wgi/vulkan/command.h>
#include <pigeon/util.h>
#include "singleton.h"

typedef struct PigeonWGIArrayTextureData
{
    PigeonVulkanMemoryAllocation image_memory;
    PigeonVulkanImage image;
    PigeonVulkanImageView image_view;

    PigeonVulkanMemoryAllocation staging_buffer_memory;
    PigeonVulkanBuffer staging_buffer;
} PigeonWGIArrayTextureData;
