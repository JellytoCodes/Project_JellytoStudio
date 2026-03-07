
#include "Framework.h"
#include "Component.h"

#include "Entity/Entity.h"

Component::Component(ComponentType type)
	: _type(type)
{

}

Component::~Component()
{

}

void Component::Awake()
{

}

void Component::Start()
{

}

void Component::Update()
{

}

void Component::LateUpdate()
{

}

void Component::OnDestroy()
{

}

shared_ptr<Transform> Component::GetTransform()
{
	return _transform;
}

std::shared_ptr<Entity> Component::GetEntity()
{
	return _entity.lock();
}

void Component::SetEntity(const std::shared_ptr<Entity>& entity)
{
	_entity = entity;

	if (entity)
		_transform = entity->GetTransform();
}