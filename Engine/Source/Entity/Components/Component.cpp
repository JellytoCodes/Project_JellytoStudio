
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

Transform* Component::GetTransform()
{
	return _transform;
}

Entity* Component::GetEntity()
{
	return _entity;
}

void Component::SetEntity(Entity* entity)
{
	_entity = entity;

	if (_entity)
		_transform = entity->GetComponent<Transform>();
}