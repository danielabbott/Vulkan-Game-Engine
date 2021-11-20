#pragma once

#include <pigeon/util.h>

typedef enum {
	PIGEON_WGI_RENDER_STAGE_UPLOAD, // Required (resets timer queries)

	// Shadow stage must be recorded if corresponding shadow is enabled in PigeonWGIShadowParameters
	PIGEON_WGI_RENDER_STAGE_SHADOW0,
	PIGEON_WGI_RENDER_STAGE_SHADOW1,
	PIGEON_WGI_RENDER_STAGE_SHADOW2,
	PIGEON_WGI_RENDER_STAGE_SHADOW3,

	PIGEON_WGI_RENDER_STAGE_DEPTH,
	PIGEON_WGI_RENDER_STAGE_SSAO, // optional, cacheable (vulkan)
	PIGEON_WGI_RENDER_STAGE_RENDER,
	PIGEON_WGI_RENDER_STAGE_BLOOM, // optional, cacheable (vulkan)
	PIGEON_WGI_RENDER_STAGE_POST_AND_UI,

	PIGEON_WGI_RENDER_STAGE__FIRST = PIGEON_WGI_RENDER_STAGE_UPLOAD,
	PIGEON_WGI_RENDER_STAGE__LAST = PIGEON_WGI_RENDER_STAGE_POST_AND_UI,
	PIGEON_WGI_RENDER_STAGE__COUNT = PIGEON_WGI_RENDER_STAGE__LAST - PIGEON_WGI_RENDER_STAGE__FIRST + 1
} PigeonWGIRenderStage;

// Scene renderer setup

typedef struct PigeonWGIRenderConfig {
	bool ssao;
	bool bloom;
	// unsigned int ssao_blur_passes;
} PigeonWGIRenderConfig;

// Call after pigeon_wgi_next_frame_* and before pigeon_wgi_start_frame
void pigeon_wgi_set_active_render_config(PigeonWGIRenderConfig);

// Command buffer recording

// Check these to avoid creating unnecessary jobs
bool pigeon_wgi_ssao_record_needed(void);
bool pigeon_wgi_bloom_record_needed(void);

// Record functions must be called in order if pigeon_wgi_multithreading_supported() == false
// Otherwise, record in parallel then submit the frame. Don't record to the same stage on multiple threads

PIGEON_ERR_RET pigeon_wgi_record_ssao(void); // check pigeon_wgi_ssao_record_needed
PIGEON_ERR_RET pigeon_wgi_record_bloom(void); // check pigeon_wgi_bloom_record_needed

PIGEON_ERR_RET pigeon_wgi_start_record(PigeonWGIRenderStage);
PIGEON_ERR_RET pigeon_wgi_end_record(PigeonWGIRenderStage);
