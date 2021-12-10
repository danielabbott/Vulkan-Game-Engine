#include "Pigeon.h"
#include "IncludeSpdlog.h"
#include "WGI/WGI.h"

using namespace std;

void enable_debug_logging()
{
#ifndef NDEBUG
	spdlog::set_level(spdlog::level::trace);
#endif
}

EngineState::EngineState(std::string window_title, WGI::GraphicsSettings graphics)
	: wgi(Ref<WGI::WGI>::make(window_title, graphics))
{
}
EngineState::~EngineState(){}


void EngineState::set_root(Ref<Transform> t)
{
	root = move(t);
}

void EngineState::set_camera(Ref<Transform> t)
{
	camera_transform = move(t);
}

bool EngineState::should_terminate()
{
	return wgi->should_terminate();
}

void EngineState::render(std::unordered_set<Ref<MeshRenderer>> const&)
{
	// TODO
}

