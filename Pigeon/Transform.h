#pragma once

#include "Ref.h"
#include <optional>
#include <unordered_set>
#include <variant>
#include "IncludeGLM.h"
#include "BetterAssert.h"

struct PositionRotationScale {
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
};

class Component;

class Transform {
	using LocalTransformType = std::variant<std::monostate, PositionRotationScale, glm::mat4>;
	LocalTransformType local_transform = std::monostate();

	std::optional<glm::mat4> world_transform;

	Ref<Transform> parent; // can be nullptr
	// std::unordered_set<Ref<Transform>> children;
	std::unordered_set <Ref<Component>> components;


public:
	// LocalTransformType const& get_local_transform() const { return local_transform; }
	LocalTransformType & edit_local_transform();

	glm::mat4 get_local_transform_as_mat4() const;

	// Value is cached
	glm::mat4 calculate_world_transform();

	void add_child(Ref<Transform>);

	void add_component_ptr_(Ref<Component> c) {
		assert_(c);
		components.insert(c);
	}

	void remove_component_ptr_(Ref<Component> const& c) {
		assert_(c);
		components.erase(c);
	}
};

void add_component_to_transform(Ref<Transform>, Ref<Component>);
void remove_component_from_transform(Ref<Transform> const&, Ref<Component> const&);