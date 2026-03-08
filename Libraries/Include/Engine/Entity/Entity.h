#pragma once
#include "Entity/Components/Transform.h"

class MonoBehaviour;

class Entity : public std::enable_shared_from_this<Entity>
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

	std::shared_ptr<Transform> GetTransform();

	void AddComponent(const std::shared_ptr<Component>& component);

	void SetLayerIndex(const uint8 layer)	{ _layerIndex = layer; }
	uint8 GetLayerIndex() const				{ return _layerIndex; }

protected :
	std::shared_ptr<Transform> _transform;
	std::array<std::shared_ptr<Component>, FIXED_COMPONENT_COUNT> _components;

	std::vector<std::shared_ptr<MonoBehaviour>> _scripts;

	uint8 _layerIndex = 0;
};

