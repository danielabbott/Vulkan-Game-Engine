#pragma once

#include <stdbool.h>

ERROR_RETURN_TYPE pigeon_create_vulkan_context(bool prefer_dedicated_gpu);
void pigeon_vulkan_wait_idle(void);
void pigeon_destroy_vulkan_context(void);

unsigned int pigeon_vulkan_get_uniform_buffer_min_alignment(void);
unsigned int pigeon_vulkan_get_multidraw_struct_size(void);
