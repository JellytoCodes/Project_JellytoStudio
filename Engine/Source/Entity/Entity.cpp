
#include "Framework.h"
#include "Entity/Entity.h"
#include "Entity/Components/MonoBehaviour.h"

Entity::Entity()
{
	_transform = std::make_shared<Transform>();
}

Entity::~Entity()
{

}

void Entity::Awake()
{
	for (auto& component : _components)
	{
		if (component)
			component->Awake();
	}

	for (auto& script : _scripts)
	{
		if (script)
			script->Awake();
	}
}

void Entity::Start()
{
	for (auto& component : _components)
	{
		if (component)
			component->Start();
	}

	for (auto& script : _scripts)
	{
		if (script)
			script->Start();
	}
}

void Entity::Update()
{
	for (auto& component : _components)
	{
		if (component)
			component->Update();
	}

	for (auto& script : _scripts)
	{
		if (script)
			script->Update();
	}
}

void Entity::LateUpdate()
{
	for (auto& component : _components)
	{
		if (component)
			component->LateUpdate();
	}

	for (auto& script : _scripts)
	{
		if (script)
			script->LateUpdate();
	}
}

void Entity::OnDestroy()
{
	for (auto& component : _components)
	{
		if (component)
			component->OnDestroy();
	}

	for (auto& script : _scripts)
	{
		if (script)
			script->OnDestroy();
	}
}

void Entity::Render()
{
	for (auto& component : _components)
	{
		if (component)
			component->Render();
	}

	for (auto& script : _scripts)
	{
		if (script)
			script->Render();
	}
}

std::shared_ptr<Transform> Entity::GetTransform()
{
	return _transform;
}

void Entity::AddComponent(const std::shared_ptr<Component>& component)
{
	component->SetEntity(shared_from_this());
	int8 index = static_cast<int8>(component->GetType());

	if (index < FIXED_COMPONENT_COUNT)
	{
		_components[index] = component;
	}
	else if (index == FIXED_COMPONENT_COUNT)
	{
		auto script = std::static_pointer_cast<MonoBehaviour>(component);
		_scripts.push_back(script);
	}
}