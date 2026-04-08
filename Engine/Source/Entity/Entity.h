#pragma once
#include "Entity/Components/Transform.h"

class MonoBehaviour;

class Entity
{

public:
	Entity(const std::wstring& name);
	~Entity();

	virtual void Awake();
	virtual void Start();
	virtual void Update();
	virtual void LateUpdate();
	virtual void OnDestroy();

	void OnCollision(Entity* other);

	void AddComponent(std::unique_ptr<Component> component);

	template<typename T>
	T* GetComponent();

	void SetLayerIndex(const uint8 layer) { _layerIndex = layer; }
	uint8 GetLayerIndex() const { return _layerIndex; }

	std::wstring GetEntityName() const { return _entityName; }

protected :
	std::array<std::unique_ptr<Component>, FIXED_COMPONENT_COUNT> _components;
	std::vector<std::unique_ptr<MonoBehaviour>> _scripts;

	uint8 _layerIndex = 0;

	std::wstring _entityName = L"";
};

template <typename T>
T* Entity::GetComponent()
{
	for (auto& component : _components)
	{
		if (component == nullptr) continue;

		T* target = dynamic_cast<T>(component);
		if (target != nullptr)
			return target;
	}
	return nullptr;
}