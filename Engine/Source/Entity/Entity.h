#pragma once
#include "Entity/Components/Transform.h"

class Entity : public enable_shared_from_this<Entity>
{

public :
	Entity();
	~Entity();

	void Awake();
	void Start();
	void Update();
	void LateUpdate();
	void OnDestroy();

	void Render();

	shared_ptr<Transform> GetTransform();

	void AddComponent(const std::shared_ptr<Component>& component);

	void SetLayerIndex(const uint8 layer)	{ _layerIndex = layer; }
	uint8 GetLayerIndex() const				{ return _layerIndex; }

protected :
	shared_ptr<Transform> _transform;
	array<shared_ptr<Component>, FIXED_COMPONENT_COUNT> _components;

	uint8 _layerIndex = 0;
};

