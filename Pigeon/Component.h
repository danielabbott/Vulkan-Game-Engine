#pragma once

#include "Ref.h"
#include <unordered_set>
#include "BetterAssert.h"

class Transform;
class Component {
	std::unordered_set <Ref<Transform>> transforms;

public:
	void add_transform_ptr_(Ref<Transform> t) {
		assert_(t);
		transforms.insert(std::move(t));
	}
	void remove_transform_ptr_(Ref<Transform> const& t) {
		assert_(t);
		transforms.erase(t);
	}
};
