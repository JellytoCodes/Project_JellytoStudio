
#include "Framework.h"
#include "Actor.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"

bool Actor::Spawn(std::shared_ptr<Scene> scene)
{
	if (!scene) return false;

	_entity = std::make_shared<Entity>();
	_entity->AddComponent(std::make_shared<Transform>());

	BuildEntity();

	scene->Add(_entity);
	return true;
}

void Actor::Despawn(std::shared_ptr<Scene> scene)
{
	if (!_entity || !scene) return;
	scene->Remove(_entity);
	_entity.reset();
}