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
    {
        if (object->GetComponent<Camera>() == _mainCamera) continue;
        object->Update();
    }
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

bool Scene::BuildPickRay(int32 screenX, int32 screenY, Vec3& outOrigin, Vec3& outDir) const
{
    if (!_mainCamera) return false;

    const float width = GET_SINGLE(Graphics)->GetViewport().GetWidth();
    const float height = GET_SINGLE(Graphics)->GetViewport().GetHeight();

    const Matrix& proj = _mainCamera->GetProjectionMatrix();
    const Matrix  viewInv = _mainCamera->GetViewMatrix().Invert();

    const float viewX = (+2.f * screenX / width - 1.f) / proj(0, 0);
    const float viewY = (-2.f * screenY / height + 1.f) / proj(1, 1);

    outOrigin = XMVector3TransformCoord(Vec4(0.f, 0.f, 0.f, 1.f), viewInv);
    outDir = XMVector3TransformNormal(Vec4(viewX, viewY, 1.f, 0.f), viewInv);
    outDir.Normalize();

    return outDir.LengthSquared() > 1e-6f;
}

Entity* Scene::Pick(int32 screenX, int32 screenY)
{
    Vec3 rayOrigin, rayDir;
    if (!BuildPickRay(screenX, screenY, rayOrigin, rayDir)) return nullptr;

    Ray     ray(rayOrigin, rayDir);
    Entity* picked = nullptr;
    float   minDist = FLT_MAX;

    for (auto& entity : _objects)
    {
        if (_mainCamera->IsCulled(entity->GetLayerIndex())) continue;
        auto collider = entity->GetComponent<BaseCollider>();
        if (!collider) continue;

        float dist = 0.f;
        Ray   r = ray;
        if (collider->Intersects(r, dist) && dist < minDist)
        {
            minDist = dist;
            picked = entity.get();
        }
    }
    return picked;
}

bool Scene::PickGroundPoint(int32 screenX, int32 screenY, Vec3& outWorldPos, float groundY)
{
    Vec3 rayOrigin, rayDir;
    if (!BuildPickRay(screenX, screenY, rayOrigin, rayDir)) return false;

    if (fabsf(rayDir.y) < 1e-6f) return false;

    const float t = (groundY - rayOrigin.y) / rayDir.y;
    if (t < 0.f) return false;

    outWorldPos = rayOrigin + rayDir * t;
    return true;
}

bool Scene::PickBlock(int32 screenX, int32 screenY, CollisionChannel queryChan, Entity*& outEntity, Vec3& outHitNormal, float& outDist)
{
    Vec3 rayOrigin, rayDir;
    if (!BuildPickRay(screenX, screenY, rayOrigin, rayDir)) return false;

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
            outEntity = entity.get();
            outHitNormal = normal;
        }
    }

    return outEntity != nullptr;
}