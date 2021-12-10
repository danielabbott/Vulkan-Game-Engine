#pragma once

#include "../IncludeGLM.h"

namespace WGI {
    struct ObjectUniformData {
		glm::vec4 position_min;
		glm::vec3 position_range;
		float ssao_intensity;
		glm::mat4 model;
		glm::mat4 view_model;
		glm::mat4 proj_view_model;
		glm::vec3 colour = glm::vec3(0.7f, 0.7f, 0.7f);
		float rsvd;
		// unsigned char custom[64];
	};

	struct SceneUniformData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 proj_view;
		glm::vec2 viewport_size;
		float one_pixel_x;
		float one_pixel_y;
		float time;
    	uint32_t number_of_lights;
		float rsvd2;
		float rsvd3;

		struct Light {
			glm::vec3 neg_direction;
			float is_shadow_caster; // 0 -> no shadows
   			glm::vec3 intensity;
			float shadow_pixel_offset;
   			glm::mat4 shadow_proj_view;
		};
    	Light lights[4];
    	glm::vec3 ambient;
		float rsvd4;
	};
}