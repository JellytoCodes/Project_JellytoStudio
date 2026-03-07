
#include "Framework.h"
#include "Entity/Entity.h"

Entity::Entity()
{
	_transform = make_shared<Transform>();
}

Entity::~Entity()
{

}

void Entity::Awake()
{
	for (std::shared_ptr<Component>& component : _components)
	{
		if (component)
			component->Awake();
	}
}

void Entity::Start()
{
	for (std::shared_ptr<Component>& component : _components)
	{
		if (component)
			component->Start();
	}
}

void Entity::Update()
{
	for (std::shared_ptr<Component>& component : _components)
	{
		if (component)
			component->Update();
	}
}

void Entity::LateUpdate()
{
	for (std::shared_ptr<Component>& component : _components)
	{
		if (component)
			component->LateUpdate();
	}
}

void Entity::OnDestroy()
{
	for (std::shared_ptr<Component>& component : _components)
	{
		if (component)
			component->OnDestroy();
	}
}

void Entity::Render()
{
	for (std::shared_ptr<Component>& component : _components)
	{
		if (component)
			component->Render();
	}
}

shared_ptr<Transform> Entity::GetTransform()
{
	return _transform;
}

void Entity::AddComponent(const std::shared_ptr<Component>& component)
{
	component->SetEntity(shared_from_this());

	int8 index = static_cast<int8>(component->GetType());
	if (index < FIXED_COMPONENT_COUNT)
		_components[index] = component;
}