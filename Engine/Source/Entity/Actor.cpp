#include "Framework.h"
#include "Actor.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"

bool Actor::Spawn(Scene* scene)
{
	if (!scene) return false;

	auto entity = std::make_unique<Entity>(GetActorName());
	entity->AddComponent(std::make_unique<Transform>());

	_entity = entity.get();

	BuildEntity();

	scene->Add(std::move(entity));
	return true;
}

void Actor::Despawn(Scene* scene)
{
	if (!_entity || !scene) return;
	scene->Remove(_entity);
	_entity = nullptr;
}
