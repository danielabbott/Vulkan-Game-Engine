#pragma once

#include "mesh.h"
#include "pipeline.h"
#include "rendergraph.h"
#include <pigeon/util.h>

void pigeon_wgi_draw_without_mesh(PigeonWGIRenderStage, PigeonWGIPipeline*, unsigned int vertices);
// pigeon_wgi_upload_multimesh

// first and count are either offsets into vertices or indices array depending on whether
//  the mesh has indices or not
// Requires 'instances' number of draw objects, starting at 'draw_index'
// diffuse_texture, nmap_texture, first_bone_index, bones_count are ignored if
//  multidraw is supported (vulkan renderer) and can be set to -1.
//  If multidraw is not supported (opengl) then the textures are the same values
//  passed to wgi_bind_array_texture
void pigeon_wgi_draw(PigeonWGIRenderStage, PigeonWGIPipeline*, PigeonWGIMultiMesh*, uint32_t start_vertex,
	uint32_t draw_index, uint32_t instances, uint32_t first, unsigned int count, int diffuse_texture, int nmap_texture,
	unsigned int first_bone_index, unsigned int bones_count);

void pigeon_wgi_multidraw_draw(unsigned int multidraw_draw_index, unsigned int start_vertex, uint32_t instances,
	uint32_t first, uint32_t count, uint32_t first_instance);

void pigeon_wgi_multidraw_submit(PigeonWGIRenderStage, PigeonWGIPipeline*, PigeonWGIMultiMesh*,
	uint32_t first_multidraw_index, uint32_t multidraw_count);
