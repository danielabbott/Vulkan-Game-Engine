#include <pigeon/scene/component.h>
#include <pigeon/util.h>
#include <stdbool.h>
#include <stdint.h>

struct PigeonAsset;
struct PigeonWGIMultiMesh;
struct PigeonWGIPipeline;
struct PigeonAsset;

typedef struct PigeonRenderState {
	struct PigeonWGIMultiMesh* mesh;
	struct PigeonWGIPipeline* pipeline;

	struct PigeonArrayList* models; // array of PigeonModelMaterial*
	struct PigeonArrayList* mr; // array of PigeonMaterialRenderer*

	unsigned int _index;
	unsigned int _draws, _multidraws;
	unsigned int _start_draw_index;
	unsigned int _start_multidraw_index;

	//  Valid when multidraw is supported by _multidraws is 1

	uint32_t start_vertex;
	uint32_t instances;
	uint32_t first;
	uint32_t count;
} PigeonRenderState;

typedef struct PigeonModelMaterial {
	struct PigeonAsset* model_asset;
	unsigned int material_index;

	struct PigeonArrayList* rs; // array of PigeonRenderState*

	struct PigeonArrayList* mr; // array of PigeonMaterialRenderer*

} PigeonModelMaterial;

typedef struct PigeonAnimationState {
	struct PigeonAsset* model_asset;
	struct PigeonArrayList* mr; // array of PigeonMaterialRenderer*

	double animation_start_time;
	int animation_index; // < 0 for T pose

	unsigned int _first_bone_index;
} PigeonAnimationState;

// PIGEON_COMPONENT_TYPE_MATERIAL_RENDERER
typedef struct PigeonMaterialRenderer {
	PigeonComponent c;

	PigeonModelMaterial* model;
	PigeonRenderState* render_state;

	uint32_t diffuse_bind_point; // index into sampler2DArray[] or UINT32_MAX
	uint32_t diffuse_layer; // array texture layer
	uint32_t nmap_bind_point; // index into sampler2DArray[] or UINT32_MAX
	uint32_t nmap_layer; // array texture layer
	float colour[3];
	float luminosity, specular_intensity;
	float under_colour[3];
	bool use_transparency;
	bool use_under_colour;

	PigeonAnimationState* animation_state;

	// unsigned int _draw_index;
} PigeonMaterialRenderer;

// These all use object pools

PigeonRenderState* pigeon_create_render_state(struct PigeonWGIMultiMesh*, struct PigeonWGIPipeline*);
void pigeon_destroy_render_state(PigeonRenderState*);

PigeonModelMaterial* pigeon_create_model_renderer(struct PigeonAsset* model_asset, unsigned int material_index);
void pigeon_destroy_model_renderer(PigeonModelMaterial*);

PigeonMaterialRenderer* pigeon_create_material_renderer(PigeonModelMaterial*);
void pigeon_destroy_material_renderer(PigeonMaterialRenderer*);

PigeonAnimationState* pigeon_create_animation_state(struct PigeonAsset* model_asset);
void pigeon_destroy_animation_state(PigeonAnimationState*);

PIGEON_ERR_RET pigeon_join_rs_model(PigeonRenderState*, PigeonModelMaterial*);
void pigeon_unjoin_rs_model(PigeonRenderState*, PigeonModelMaterial*);

PIGEON_ERR_RET pigeon_join_mr_anim(PigeonMaterialRenderer*, PigeonAnimationState*);
void pigeon_unjoin_mr_anim(PigeonMaterialRenderer*, PigeonAnimationState*);
