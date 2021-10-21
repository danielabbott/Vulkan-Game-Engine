#pragma once

#include <pigeon/wgi/textures.h>
#include <pigeon/wgi/vulkan/command.h>
#include <pigeon/wgi/opengl/texture.h>
#include <pigeon/util.h>
#include "singleton.h"

typedef struct PigeonWGIArrayTextureData
{
    union {
        struct {
            PigeonVulkanMemoryAllocation image_memory;
            PigeonVulkanImage image;
            PigeonVulkanImageView image_view;

            PigeonVulkanMemoryAllocation staging_buffer_memory;
            PigeonVulkanBuffer staging_buffer;
        };
        struct
        {
            PigeonOpenGLTexture texture;
        } gl;        
    };
} PigeonWGIArrayTextureData;
