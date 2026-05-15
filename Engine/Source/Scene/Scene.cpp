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
#include "Scene/ChunkManager.h"
#include "Scene/PickUtils.h"

Scene::Scene()
{
    _shadowPass = std::make_unique<ShadowPass>();
    _shadowPass->Init();
}

Scene::~Scene() {}

void Scene::Awake()
{
    ++_iterationDepth;
    for (auto& object : _objects)
        object->Awake();
    --_iterationDepth;
    FlushPendingMutations();
}

void Scene::Start()
{
    ++_iterationDepth;
    for (auto& object : _objects)
        object->Start();
    --_iterationDepth;
    FlushPendingMutations();
}

void Scene::Update()
{
    ++_iterationDepth;
    if (_mainCamera)
    {
        if (Entity* camEntity = _mainCamera->GetEntity())
            camEntity->Update();
    }

    for (auto& object : _objects)
    {
        if (_mainCamera && object->GetComponent<Camera>() == _mainCamera) continue;
        object->Update();
    }
    --_iterationDepth;
    FlushPendingMutations();
}

void Scene::LateUpdate()
{
    ++_iterationDepth;
    for (auto& object : _objects)
        object->LateUpdate();
    --_iterationDepth;
    FlushPendingMutations();
}

void Scene::OnDestroy()
{
    ++_iterationDepth;
    for (auto& object : _objects)
        object->OnDestroy();
    --_iterationDepth;
    FlushPendingMutations();
}

void Scene::Render()
{
    if (_mainCamera == nullptr) return;

    ++_iterationDepth;
    _mainCamera->SortEntities();

    if (_mainLight)
    {
        const Vec3 lightDir = _mainLight->GetLightDesc().direction;
        if (lightDir.LengthSquared() > 1e-6f)
            _shadowPass->Render(_mainCamera->GetVisibleEntities(), lightDir);
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
    --_iterationDepth;
    FlushPendingMutations();
}

void Scene::Add(std::unique_ptr<Entity> object)
{
    if (!object) return;

    if (IsIterating())
    {
        _pendingAdds.push_back(std::move(object));
        return;
    }

    AddImmediate(std::move(object));
}

void Scene::Remove(Entity* object)
{
    if (!object) return;

    auto pendingAddIt = std::find_if(_pendingAdds.begin(), _pendingAdds.end(),
        [object](const std::unique_ptr<Entity>& p) { return p.get() == object; });

    if (pendingAddIt != _pendingAdds.end())
    {
        _pendingAdds.erase(pendingAddIt);
        return;
    }

    if (IsIterating())
    {
        if (std::find(_pendingRemoves.begin(), _pendingRemoves.end(), object) == _pendingRemoves.end())
            _pendingRemoves.push_back(object);
        return;
    }

    RemoveImmediate(object);
}

void Scene::FlushPendingMutations()
{
    if (IsIterating()) return;

    if (!_pendingRemoves.empty())
    {
        for (Entity* object : _pendingRemoves)
            RemoveImmediate(object);
        _pendingRemoves.clear();
    }

    if (!_pendingAdds.empty())
    {
        std::vector<std::unique_ptr<Entity>> pendingAdds = std::move(_pendingAdds);
        _pendingAdds.clear();

        for (auto& object : pendingAdds)
            AddImmediate(std::move(object));
    }
}

void Scene::AddImmediate(std::unique_ptr<Entity> object)
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

void Scene::RemoveImmediate(Entity* object)
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

    const float width  = GET_SINGLE(Graphics)->GetViewport().GetWidth();
    const float height = GET_SINGLE(Graphics)->GetViewport().GetHeight();

    const Matrix& proj    = _mainCamera->GetProjectionMatrix();
    const Matrix  viewInv = _mainCamera->GetViewMatrix().Invert();

    const float viewX = (+2.f * screenX / width  - 1.f) / proj(0, 0);
    const float viewY = (-2.f * screenY / height + 1.f) / proj(1, 1);

    outOrigin = XMVector3TransformCoord (Vec4(0.f,   0.f,   0.f, 1.f), viewInv);
    outDir    = XMVector3TransformNormal(Vec4(viewX, viewY, 1.f, 0.f), viewInv);
    outDir.Normalize();
    return outDir.LengthSquared() > 1e-6f;
}

Entity* Scene::Pick(int32 screenX, int32 screenY)
{
    Vec3 rayOrigin, rayDir;
    if (!BuildPickRay(screenX, screenY, rayOrigin, rayDir)) return nullptr;

    Ray     ray(rayOrigin, rayDir);
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
    Vec3 rayOrigin, rayDir;
    if (!BuildPickRay(screenX, screenY, rayOrigin, rayDir)) return false;
    if (fabsf(rayDir.y) < 1e-6f) return false;
    const float t = (groundY - rayOrigin.y) / rayDir.y;
    if (t < 0.f) return false;
    outWorldPos = rayOrigin + rayDir * t;
    return true;
}

bool Scene::PickBlock(int32 screenX, int32 screenY, CollisionChannel queryChan,
                      Entity*& outEntity, Vec3& outHitNormal, float& outDist)
{
    Vec3 rayOrigin, rayDir;
    if (!BuildPickRay(screenX, screenY, rayOrigin, rayDir)) return false;

    outEntity    = nullptr;
    outDist      = FLT_MAX;
    outHitNormal = Vec3(0, 1, 0);

    auto* chunkMgr = GET_SINGLE(ChunkManager);

    chunkMgr->PickBlock(rayOrigin, rayDir, queryChan, outEntity, outHitNormal, outDist);

    Ray ray(rayOrigin, rayDir);
    for (Entity* entity : _collidableObjects)
    {
        if (chunkMgr->IsManaged(entity)) continue;
        if (_mainCamera && _mainCamera->IsCulled(entity->GetLayerIndex())) continue;

        auto* aabb = entity->GetComponent<AABBCollider>();
        if (!aabb) continue;
        if (!aabb->CanBePickedBy(queryChan)) continue;

        float dist = 0.f;
        Vec3  normal;
        Ray   r = ray;
        if (aabb->IntersectsWithNormal(r, dist, normal) && dist < outDist)
        {
            outDist      = dist;
            outEntity    = entity;
            outHitNormal = normal;
        }
    }

    return outEntity != nullptr;
}

bool Scene::PickBlocks(int32 screenX, int32 screenY, uint8 queryMask, BlockPickHit& priming, BlockPickHit& floor, BlockPickHit& mushroom)
{
    priming.Reset();
    floor.Reset();
    mushroom.Reset();

    Vec3 rayOrigin, rayDir;
    if (!BuildPickRay(screenX, screenY, rayOrigin, rayDir)) return false;

    auto* chunkMgr = GET_SINGLE(ChunkManager);
    chunkMgr->PickBlocks(rayOrigin, rayDir, queryMask, priming, floor, mushroom);

    Ray ray(rayOrigin, rayDir);
    for (Entity* entity : _collidableObjects)
    {
        if (chunkMgr->IsManaged(entity)) continue;
        if (_mainCamera && _mainCamera->IsCulled(entity->GetLayerIndex())) continue;

        auto* aabb = entity->GetComponent<AABBCollider>();
        if (!aabb) continue;
        if ((aabb->GetPickableMask() & queryMask) == 0) continue;

        float dist = 0.f;
        Vec3  normal;
        Ray   r = ray;
        if (aabb->IntersectsWithNormal(r, dist, normal))
            UpdateMatchingPickHits(queryMask, aabb, entity, normal, dist, priming, floor, mushroom);
    }

    return priming.valid || floor.valid || mushroom.valid;
}