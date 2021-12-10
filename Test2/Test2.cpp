auto engine_ptr = Ref<EngineState>::make("Test1", WGI::GraphicsSettings { 
	WGI::WindowDimensions(800, 600), WGI::Window::FullscreenMode::Windowed, WGI::GPUPreference::Integrated 
});
auto & engine = *engine_ptr;


// engine.render(all_mesh_renderers);
// auto mesh_renderer = Ref<MeshRenderer>::make(mesh, render_config);
// unordered_set<Ref<MeshRenderer>> all_mesh_renderers = {mesh_renderer};

// auto transform = Ref<Transform>();
// add_component_to_transform(transform, mesh_renderer);
// engine.set_root(transform);


// auto camera_transform = Ref<Transform>();
// // Objects face +Z
// camera_transform->edit_local_transform() = PositionRotationScale {
// 	.position = glm::vec3(0.0f, 0.0f, 2.0f),
// 	.rotation = glm::quat(glm::vec3(0.0f, glm::radians(180.0f), 0.0f)),
// 	.scale = glm::vec3(1.0f)
// };
// engine.set_camera(camera_transform);