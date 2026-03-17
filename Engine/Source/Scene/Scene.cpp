#include "Framework.h"
#include "Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Collider/BaseCollider.h"
#include "Entity/Managers/CollisionManager.h"

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::Awake()
{
	for (auto& object : _objects)
		object->Awake();
}

void Scene::Start()
{
	for (auto& object : _objects)
		object->Start();
}

void Scene::Update()
{
	if (_mainCamera)
		_mainCamera->Update();

	for (auto& object : _objects)
		object->Update();
}

void Scene::LateUpdate()
{
	for (auto& object : _objects)
		object->LateUpdate();
}

void Scene::OnDestroy()
{
	for (auto& object : _objects)
		object->OnDestroy();
}

void Scene::Render()
{
	if (_mainCamera == nullptr) return;

	_mainCamera->SortEntities();
	_mainCamera->RenderForward();

	for (auto& object : _objects)
	{
		auto collider = object->GetComponent<BaseCollider>();
		if (collider == nullptr) continue;
		collider->RenderDebug();
	}
}

void Scene::Add(const std::shared_ptr<Entity>& object)
{
	_objects.insert(object);
}

void Scene::Remove(const std::shared_ptr<Entity>& object)
{
	_objects.erase(object);
}