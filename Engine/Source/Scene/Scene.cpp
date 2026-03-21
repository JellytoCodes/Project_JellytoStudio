#include "Framework.h"
#include "Graphics/Graphics.h"
#include "Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Collider/BaseCollider.h"

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

std::shared_ptr<Entity> Scene::Pick(int32 screenX, int32 screenY)
{
	if (!_mainCamera) return nullptr;

	// Viewport 크기
	float width  = Graphics::Get()->GetViewport().GetWidth();
	float height = Graphics::Get()->GetViewport().GetHeight();

	Matrix projMatrix = _mainCamera->GetProjectionMatrix();
	Matrix viewMatrix = _mainCamera->GetViewMatrix();
	Matrix viewMatrixInv = viewMatrix.Invert();

	// View Space 좌표 (참고 코드 방식)
	float viewX = (+2.f * screenX / width  - 1.f) / projMatrix(0, 0);
	float viewY = (-2.f * screenY / height + 1.f) / projMatrix(1, 1);

	// View Space Ray → World Space 변환
	Vec4 rayOrigin4 = Vec4(0.f, 0.f, 0.f, 1.f);
	Vec4 rayDir4    = Vec4(viewX, viewY, 1.f, 0.f);

	Vec3 worldRayOrigin = XMVector3TransformCoord(rayOrigin4, viewMatrixInv);
	Vec3 worldRayDir    = XMVector3TransformNormal(rayDir4, viewMatrixInv);
	worldRayDir.Normalize();

	// [DEBUG]
	wchar_t dbg[256];
	swprintf_s(dbg, L"[Scene::Pick] origin=(%.2f,%.2f,%.2f) dir=(%.3f,%.3f,%.3f)\n",
		worldRayOrigin.x, worldRayOrigin.y, worldRayOrigin.z,
		worldRayDir.x, worldRayDir.y, worldRayDir.z);
	::OutputDebugStringW(dbg);

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
			picked  = entity;

			wchar_t dbgHit[256];
			swprintf_s(dbgHit, L"[Scene::Pick] 후보 히트: %s dist=%.2f\n",
				entity->GetEntityName().c_str(), dist);
			::OutputDebugStringW(dbgHit);
		}
	}

	return picked;
}
// ── Ray → Y=groundY 평면 교차 → 월드 좌표 반환 ──────────────────────────
// 아이소메트릭 클릭 이동에서 "바닥을 클릭한 위치" 를 구할 때 사용
bool Scene::PickGroundPoint(int32 screenX, int32 screenY, Vec3& outWorldPos, float groundY)
{
	if (!_mainCamera) return false;

	float width  = Graphics::Get()->GetViewport().GetWidth();
	float height = Graphics::Get()->GetViewport().GetHeight();

	Matrix projMatrix    = _mainCamera->GetProjectionMatrix();
	Matrix viewMatrixInv = _mainCamera->GetViewMatrix().Invert();

	float viewX = (+2.f * screenX / width  - 1.f) / projMatrix(0, 0);
	float viewY = (-2.f * screenY / height + 1.f) / projMatrix(1, 1);

	Vec4 rayOrigin4 = Vec4(0.f, 0.f, 0.f, 1.f);
	Vec4 rayDir4    = Vec4(viewX, viewY, 1.f, 0.f);

	Vec3 rayOrigin = XMVector3TransformCoord(rayOrigin4, viewMatrixInv);
	Vec3 rayDir    = XMVector3TransformNormal(rayDir4, viewMatrixInv);
	rayDir.Normalize();

	// Ray-Plane 교차: Y = groundY 인 수평 평면
	// t = (groundY - rayOrigin.y) / rayDir.y
	if (fabsf(rayDir.y) < 1e-6f) return false; // 평행 (교차 없음)

	float t = (groundY - rayOrigin.y) / rayDir.y;
	if (t < 0.f) return false; // 카메라 뒤쪽

	outWorldPos = rayOrigin + rayDir * t;
	return true;
}