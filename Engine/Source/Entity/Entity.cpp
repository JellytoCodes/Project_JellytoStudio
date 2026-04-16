
#include "Framework.h"
#include "Entity/Entity.h"
#include "Components/Component.h"
#include "Entity/Components/MonoBehaviour.h"

Entity::Entity(const std::wstring& name)
    : _entityName(name)
{

}

Entity::~Entity()
{
	
}

void Entity::Awake()
{
    for (auto& component : _components)
        if (component) component->Awake();

    for (auto& script : _scripts)
        if (script) script->Awake();
}

void Entity::Start()
{
    for (auto& component : _components)
        if (component) component->Start();

    for (auto& script : _scripts)
        if (script) script->Start();
}

void Entity::Update()
{
    for (auto& component : _components)
        if (component) component->Update();

    for (auto& script : _scripts)
        if (script) script->Update();
}

void Entity::LateUpdate()
{
    for (auto& component : _components)
        if (component) component->LateUpdate();

    for (auto& script : _scripts)
        if (script) script->LateUpdate();
}

void Entity::OnDestroy()
{
    for (auto& component : _components)
        if (component) component->OnDestroy();

    for (auto& script : _scripts)
        if (script) script->OnDestroy();
}

void Entity::OnCollision(Entity* other)
{
    for (auto& script : _scripts)
        if (script) script->OnCollision(other);
}

void Entity::AddComponent(std::unique_ptr<Component> component)
{
    component->SetEntity(this);
    const int8 index = static_cast<int8>(component->GetType());

    if (index < FIXED_COMPONENT_COUNT)
    {
        _components[index] = std::move(component);
    }
    else if (index == FIXED_COMPONENT_COUNT)
    {
        std::unique_ptr<MonoBehaviour> script(static_cast<MonoBehaviour*>(component.release()));
        _scripts.push_back(std::move(script));
    }
}
