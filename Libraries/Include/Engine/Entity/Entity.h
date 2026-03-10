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

	std::shared_ptr<Transform> GetTransform();

	void AddComponent(const std::shared_ptr<Component>& component);

	template<typename T>
	std::shared_ptr<T> GetComponent();

	void SetLayerIndex(const uint8 layer)	{ _layerIndex = layer; }
	uint8 GetLayerIndex() const				{ return _layerIndex; }

protected :
	std::shared_ptr<Transform> _transform;
	std::array<std::shared_ptr<Component>, FIXED_COMPONENT_COUNT> _components;

	std::vector<std::shared_ptr<MonoBehaviour>> _scripts;

	uint8 _layerIndex = 0;
};

template <typename T>
std::shared_ptr<T> Entity::GetComponent()
{
	for (auto& component : _components)
	{
		if (component == nullptr) continue;

		std::shared_ptr<T> target = std::dynamic_pointer_cast<T>(component);
		if (target != nullptr) 
			return target;
	}
	return nullptr;
}
