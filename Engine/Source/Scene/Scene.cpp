#include "Framework.h"
#include "Graphics/Graphics.h"
#include "Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Collider/BaseCollider.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Components/Light.h"
#include "Entity/Managers/CollisionManager.h"
#include "UI/Widget.h"
#include "UI/UIManager.h"
#include "Graphics/Managers/InstancingManager.h"

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

	for (auto& object : _collidableObjects)
	{
		auto collider = object->GetComponent<BaseCollider>();
		if (collider == nullptr) continue;
		collider->RenderDebug();
	}

	for (auto& object : _widgetObjects)
	{
		if (auto widget = std::dynamic_pointer_cast<Widget>(object))
			widget->DrawUI();
	}
	GET_SINGLE(UIManager)->Render();
}

void Scene::Add(std::unique_ptr<Entity> object)
{
    // 콜라이더 캐시 + CollisionManager 등록

    if (BaseCollider* collider = object->GetComponent<BaseCollider>())
    {
        _collidableObjects.insert(object.get());
        CollisionManager::RegisterCollider(collider, collider->IsStatic());
    }

    if (std::dynamic_pointer_cast<Widget>(object))
        _widgetObjects.push_back(object);

	_objects.insert(std::move(object));

    GET_SINGLE(InstancingManager)->SetDirty();
    if (_mainCamera) _mainCamera->SetSortDirty();
}

void Scene::Remove(Entity* object)
{
    _objects.erase(object);

    if (auto collider = object->GetComponent<BaseCollider>())
        CollisionManager::UnregisterCollider(collider);

    _collidableObjects.erase(object);
    _widgetObjects.erase(
        std::remove_if(_widgetObjects.begin(), _widgetObjects.end(),
            [&object](const auto& w) { return w.get() == object.get(); }),
        _widgetObjects.end());

	GET_SINGLE(InstancingManager)->SetDirty();
	if (_mainCamera) _mainCamera->SetSortDirty();
}

std::shared_ptr<Entity> Scene::Pick(int32 screenX, int32 screenY)
{
	if (!_mainCamera) return nullptr;

	// Viewport 크기
	float width = Graphics::Get()->GetViewport().GetWidth();
	float height = Graphics::Get()->GetViewport().GetHeight();

	Matrix projMatrix = _mainCamera->GetProjectionMatrix();
	Matrix viewMatrix = _mainCamera->GetViewMatrix();
	Matrix viewMatrixInv = viewMatrix.Invert();

	// View Space 좌표 (참고 코드 방식)
	float viewX = (+2.f * screenX / width - 1.f) / projMatrix(0, 0);
	float viewY = (-2.f * screenY / height + 1.f) / projMatrix(1, 1);

	// View Space Ray → World Space 변환
	Vec4 rayOrigin4 = Vec4(0.f, 0.f, 0.f, 1.f);
	Vec4 rayDir4 = Vec4(viewX, viewY, 1.f, 0.f);

	Vec3 worldRayOrigin = XMVector3TransformCoord(rayOrigin4, viewMatrixInv);
	Vec3 worldRayDir = XMVector3TransformNormal(rayDir4, viewMatrixInv);
	worldRayDir.Normalize();

	Ray ray = Ray(worldRayOrigin, worldRayDir);

	std::shared_ptr<Entity> picked = nullptr;
	float minDist = FLT_MAX;

	for (auto& entity : _objects)
	{
		if (_mainCamera->IsCulled(entity->GetLayerIndex())) continue;

		auto collider = entity->GetComponent<BaseCollider>();
		if (!collider) continue;

		float dist = 0.f;
		Ray r = ray;
		if (collider->Intersects(r, dist) && dist < minDist)
		{
			minDist = dist;
			picked = entity;
		}
	}

	return picked;
}

bool Scene::PickGroundPoint(int32 screenX, int32 screenY, Vec3& outWorldPos, float groundY)
{
	if (!_mainCamera) return false;

	float width = Graphics::Get()->GetViewport().GetWidth();
	float height = Graphics::Get()->GetViewport().GetHeight();

	Matrix projMatrix = _mainCamera->GetProjectionMatrix();
	Matrix viewMatrixInv = _mainCamera->GetViewMatrix().Invert();

	float viewX = (+2.f * screenX / width - 1.f) / projMatrix(0, 0);
	float viewY = (-2.f * screenY / height + 1.f) / projMatrix(1, 1);

	Vec4 rayOrigin4 = Vec4(0.f, 0.f, 0.f, 1.f);
	Vec4 rayDir4 = Vec4(viewX, viewY, 1.f, 0.f);

	Vec3 rayOrigin = XMVector3TransformCoord(rayOrigin4, viewMatrixInv);
	Vec3 rayDir = XMVector3TransformNormal(rayDir4, viewMatrixInv);
	rayDir.Normalize();

	// Ray-Plane 교차: Y = groundY 인 수평 평면
	// t = (groundY - rayOrigin.y) / rayDir.y
	if (fabsf(rayDir.y) < 1e-6f) return false; // 평행 (교차 없음)

	float t = (groundY - rayOrigin.y) / rayDir.y;
	if (t < 0.f) return false; // 카메라 뒤쪽

	outWorldPos = rayOrigin + rayDir * t;
	return true;
}
bool Scene::PickBlock(int32 screenX, int32 screenY,
	CollisionChannel queryChan,
	std::shared_ptr<Entity>& outEntity,
	Vec3& outHitNormal,
	float& outDist)
{
	if (!_mainCamera) return false;

	float width = Graphics::Get()->GetViewport().GetWidth();
	float height = Graphics::Get()->GetViewport().GetHeight();

	Matrix projMatrix = _mainCamera->GetProjectionMatrix();
	Matrix viewMatrixInv = _mainCamera->GetViewMatrix().Invert();

	float viewX = (+2.f * screenX / width - 1.f) / projMatrix(0, 0);
	float viewY = (-2.f * screenY / height + 1.f) / projMatrix(1, 1);

	Vec3 rayOrigin = XMVector3TransformCoord(Vec4(0, 0, 0, 1), viewMatrixInv);
	Vec3 rayDir = XMVector3TransformNormal(Vec4(viewX, viewY, 1, 0), viewMatrixInv);
	rayDir.Normalize();

	Ray ray(rayOrigin, rayDir);

	outEntity = nullptr;
	outDist = FLT_MAX;
	outHitNormal = Vec3(0, 1, 0);

	for (auto& entity : _objects)
	{
		if (_mainCamera->IsCulled(entity->GetLayerIndex())) continue;

		auto aabb = entity->GetComponent<AABBCollider>();
		if (!aabb) continue;

		if (!aabb->CanBePickedBy(queryChan)) continue;

		float dist = 0.f;
		Vec3  normal;
		Ray   r = ray;
		if (aabb->IntersectsWithNormal(r, dist, normal) && dist < outDist)
		{
			outDist = dist;
			outEntity = entity;
			outHitNormal = normal;
		}
	}

	return outEntity != nullptr;
}