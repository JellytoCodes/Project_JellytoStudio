
#include "Framework.h"
#include "Component.h"

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

std::shared_ptr<Entity> Component::GetEntity()
{
	return _entity.lock();
}
