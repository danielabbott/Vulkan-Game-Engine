#pragma once

#include "Component.h"


class Mesh;
class MeshRenderer : public Component {
	Ref<Mesh> mesh;
public:
	MeshRenderer(Ref<Mesh>);
};

