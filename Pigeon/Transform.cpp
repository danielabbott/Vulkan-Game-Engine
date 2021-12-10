#include "Transform.h"
#include "Component.h"
#include "BetterAssert.h"

using namespace std;

Transform::LocalTransformType & Transform::edit_local_transform() {
	world_transform = std::nullopt;
	// TODO invalidate world transforms of children down tree
	return local_transform;
}

glm::mat4 Transform::get_local_transform_as_mat4() const
{
	if (holds_alternative<glm::mat4>(local_transform)) {
		return get<glm::mat4>(local_transform);
	}
	else if (holds_alternative<PositionRotationScale>(local_transform)) {
		auto const& x = get<PositionRotationScale>(local_transform);

		glm::mat4 scale = glm::scale(x.scale);
		glm::mat4 rotate = glm::toMat4(x.rotation);
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), x.position);
		return translate * rotate * scale;
	}
	else {
		assert_(holds_alternative<monostate>(local_transform));
		return glm::mat4(1.0f);
	}
}

glm::mat4 Transform::calculate_world_transform()
{
	if(world_transform.has_value()) {
		return world_transform.value();
	}

	glm::mat4 m = get_local_transform_as_mat4();

	Transform * t = parent.get();
	if(!t) {
		return m;
	}

	return m * t->calculate_world_transform();
}

void add_component_to_transform(Ref<Transform> t, Ref<Component> c)
{
	assert_(t);
	assert_(c);
	t->add_component_ptr_(c);
	c->add_transform_ptr_(move(t));
}

void remove_component_from_transform(Ref<Transform> const& t, Ref<Component> const& c)
{
	assert_(t);
	assert_(c);
	t->remove_component_ptr_(c);
	c->remove_transform_ptr_(t);
}