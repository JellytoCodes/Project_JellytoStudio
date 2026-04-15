
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
	_shadowPass = std::make_unique<ShadowPass>();
    _shadowPass->Init();
}
Scene::~Scene() {}

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
        if (_mainLight)
    {
        const Vec3 lightDir = _mainLight->GetLightDesc().direction;
        if (lightDir.LengthSquared() > 1e-6f)
        {
            _shadowPass->Render(_mainCamera->GetVisibleEntities(), lightDir);
        }
    }

    _mainCamera->RenderForward();

    for (Entity* object : _collidableObjects)
    {
        auto collider = object->GetComponent<BaseCollider>();
        if (collider == nullptr) continue;
        collider->RenderDebug();
    }

    for (Widget* widget : _widgetObjects)
        widget->DrawUI();

    GET_SINGLE(UIManager)->Render();
}

void Scene::Add(std::unique_ptr<Entity> object)
{
    if (BaseCollider* collider = object->GetComponent<BaseCollider>())
    {
        _collidableObjects.insert(object.get());
        CollisionManager::RegisterCollider(collider, collider->IsStatic());
    }

    if (Widget* w = dynamic_cast<Widget*>(object.get()))
        _widgetObjects.push_back(w);

    _objects.insert(std::move(object));

    GET_SINGLE(InstancingManager)->SetDirty();
    if (_mainCamera) _mainCamera->SetSortDirty();
}

void Scene::Remove(Entity* object)
{
    auto it = std::find_if(_objects.begin(), _objects.end(),
        [object](const std::unique_ptr<Entity>& p) { return p.get() == object; });

    if (it == _objects.end()) return;

    if (auto collider = object->GetComponent<BaseCollider>())
        CollisionManager::UnregisterCollider(collider);

    _collidableObjects.erase(object);

    _widgetObjects.erase(
        std::remove_if(_widgetObjects.begin(), _widgetObjects.end(),
            [object](Widget* w) { return static_cast<Entity*>(w) == object; }),
        _widgetObjects.end());

    _objects.erase(it);

    GET_SINGLE(InstancingManager)->SetDirty();
    if (_mainCamera) _mainCamera->SetSortDirty();
}


// ── 피킹 (원본 동일 유지) ─────────────────────────────────────────────────

Entity* Scene::Pick(int32 screenX, int32 screenY)
{
    if (!_mainCamera) return nullptr;

    float width  = GET_SINGLE(Graphics)->GetViewport().GetWidth();
    float height = GET_SINGLE(Graphics)->GetViewport().GetHeight();

    Matrix projMatrix    = _mainCamera->GetProjectionMatrix();
    Matrix viewMatrixInv = _mainCamera->GetViewMatrix().Invert();

    float viewX = (+2.f * screenX / width  - 1.f) / projMatrix(0, 0);
    float viewY = (-2.f * screenY / height + 1.f) / projMatrix(1, 1);

    Vec4 rayOrigin4 = Vec4(0.f, 0.f, 0.f, 1.f);
    Vec4 rayDir4    = Vec4(viewX, viewY, 1.f, 0.f);

    Vec3 worldRayOrigin = XMVector3TransformCoord(rayOrigin4, viewMatrixInv);
    Vec3 worldRayDir    = XMVector3TransformNormal(rayDir4, viewMatrixInv);
    worldRayDir.Normalize();

    if (worldRayDir.LengthSquared() < 1e-6f) return nullptr;

    Ray ray = Ray(worldRayOrigin, worldRayDir);

    Entity* picked  = nullptr;
    float   minDist = FLT_MAX;

    for (auto& entity : _objects)
    {
        if (_mainCamera->IsCulled(entity->GetLayerIndex())) continue;

        auto collider = entity->GetComponent<BaseCollider>();
        if (!collider) continue;

        float dist = 0.f;
        Ray   r    = ray;
        if (collider->Intersects(r, dist) && dist < minDist)
        {
            minDist = dist;
            picked  = entity.get();
        }
    }

    return picked;
}

bool Scene::PickGroundPoint(int32 screenX, int32 screenY, Vec3& outWorldPos, float groundY)
{
    if (!_mainCamera) return false;

    float width  = GET_SINGLE(Graphics)->GetViewport().GetWidth();
    float height = GET_SINGLE(Graphics)->GetViewport().GetHeight();

    Matrix projMatrix    = _mainCamera->GetProjectionMatrix();
    Matrix viewMatrixInv = _mainCamera->GetViewMatrix().Invert();

    float viewX = (+2.f * screenX / width  - 1.f) / projMatrix(0, 0);
    float viewY = (-2.f * screenY / height + 1.f) / projMatrix(1, 1);

    Vec3 rayOrigin = XMVector3TransformCoord(Vec4(0, 0, 0, 1), viewMatrixInv);
    Vec3 rayDir    = XMVector3TransformNormal(Vec4(viewX, viewY, 1, 0), viewMatrixInv);
    rayDir.Normalize();

    if (fabsf(rayDir.y) < 1e-6f) return false;

    float t = (groundY - rayOrigin.y) / rayDir.y;
    if (t < 0.f) return false;

    outWorldPos = rayOrigin + rayDir * t;
    return true;
}

bool Scene::PickBlock(int32 screenX, int32 screenY,
    CollisionChannel queryChan,
    Entity*& outEntity,
    Vec3&    outHitNormal,
    float&   outDist)
{
    if (!_mainCamera) return false;

    float width  = GET_SINGLE(Graphics)->GetViewport().GetWidth();
    float height = GET_SINGLE(Graphics)->GetViewport().GetHeight();

    Matrix projMatrix    = _mainCamera->GetProjectionMatrix();
    Matrix viewMatrixInv = _mainCamera->GetViewMatrix().Invert();

    float viewX = (+2.f * screenX / width  - 1.f) / projMatrix(0, 0);
    float viewY = (-2.f * screenY / height + 1.f) / projMatrix(1, 1);

    Vec3 rayOrigin = XMVector3TransformCoord(Vec4(0, 0, 0, 1), viewMatrixInv);
    Vec3 rayDir    = XMVector3TransformNormal(Vec4(viewX, viewY, 1, 0), viewMatrixInv);
    rayDir.Normalize();

    if (rayDir.LengthSquared() < 1e-6f) return false;

    Ray ray(rayOrigin, rayDir);

    outEntity    = nullptr;
    outDist      = FLT_MAX;
    outHitNormal = Vec3(0, 1, 0);

    for (auto& entity : _objects)
    {
        if (_mainCamera->IsCulled(entity->GetLayerIndex())) continue;

        // [Fix A] GetComponent<AABBCollider>(): O(1)
        auto aabb = entity->GetComponent<AABBCollider>();
        if (!aabb) continue;
        if (!aabb->CanBePickedBy(queryChan)) continue;

        float dist = 0.f;
        Vec3  normal;
        Ray   r = ray;
        if (aabb->IntersectsWithNormal(r, dist, normal) && dist < outDist)
        {
            outDist      = dist;
            outEntity    = entity.get();
            outHitNormal = normal;
        }
    }

    return outEntity != nullptr;
}
