#pragma once

#include "Ref.h"
#include <unordered_set>
#include "WGI/GraphicsSettings.h"
#include "WGI/Window.h"
#include "MeshRenderer.h"

void enable_debug_logging();

class Transform;

namespace WGI {
	class WGI;
}

class EngineState {
	Ref<WGI::WGI> wgi;
	Ref<Transform> root;
	Ref<Transform> camera_transform;

public:
	EngineState(WGI::WindowTitle window_title, WGI::GraphicsSettings);
	~EngineState();

	Ref<WGI::WGI> const& get_wgi() const {
		return wgi;
	};

	void set_root(Ref<Transform>);
	void set_camera(Ref<Transform>);

	bool should_terminate();

	void render(std::unordered_set<Ref<MeshRenderer>> const&);
};
