#include "Framework.h"
#include "CollisionManager.h"
#include "Entity/Entity.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Components/Collider/BaseCollider.h"
#include "Scene/ChunkManager.h"

std::vector<BaseCollider*> CollisionManager::s_DynamicColliders;
std::vector<BaseCollider*> CollisionManager::s_StaticColliders;

namespace
{
    void NotifyCollision(BaseCollider* a, BaseCollider* b)
    {
        Entity* entityA = a ? a->GetEntity() : nullptr;
        Entity* entityB = b ? b->GetEntity() : nullptr;
        if (entityA == nullptr || entityB == nullptr) return;

        entityA->OnCollision(entityB);
        entityB->OnCollision(entityA);
    }
}

void CollisionManager::RegisterCollider(BaseCollider* collider, bool isStatic)
{
    if (collider == nullptr) return;
    auto& list = isStatic ? s_StaticColliders : s_DynamicColliders;
    if (std::find(list.begin(), list.end(), collider) != list.end()) return;
    list.push_back(collider);
}

void CollisionManager::UnregisterCollider(BaseCollider* collider)
{
    if (collider == nullptr) return;

    auto eraseFrom = [&](std::vector<BaseCollider*>& list)
    {
        auto it = std::find(list.begin(), list.end(), collider);
        if (it != list.end())
        {
            *it = list.back();
            list.pop_back();
        }
    };

    eraseFrom(s_DynamicColliders);
    eraseFrom(s_StaticColliders);
}

void CollisionManager::CheckCollision()
{
    const int32 dynCount = static_cast<int32>(s_DynamicColliders.size());
    for (int32 i = 0; i < dynCount; ++i)
    {
        BaseCollider* a = s_DynamicColliders[i];
        for (int32 j = i + 1; j < dynCount; ++j)
        {
            BaseCollider* b = s_DynamicColliders[j];
            if (Intersects(a, b))
                NotifyCollision(a, b);
        }
    }

    auto* chunkMgr = GET_SINGLE(ChunkManager);
    std::vector<BaseCollider*> chunkCandidates;

    for (int32 i = 0; i < dynCount; ++i)
    {
        BaseCollider* a = s_DynamicColliders[i];

        chunkCandidates.clear();
        bool usedChunkBroadphase = false;
        if (auto* aabb = dynamic_cast<AABBCollider*>(a))
        {
            chunkCandidates.reserve(32);
            chunkMgr->CollectStaticColliders(aabb->GetBoundingBox(), chunkCandidates);
            usedChunkBroadphase = true;
        }

        for (BaseCollider* b : chunkCandidates)
        {
            if (Intersects(a, b))
                NotifyCollision(a, b);
        }

        const int32 statCount = static_cast<int32>(s_StaticColliders.size());
        for (int32 j = 0; j < statCount; ++j)
        {
            BaseCollider* b = s_StaticColliders[j];
            if (usedChunkBroadphase && chunkMgr->IsManaged(b->GetEntity())) continue;

            if (Intersects(a, b))
                NotifyCollision(a, b);
        }
    }
}

bool CollisionManager::Intersects(BaseCollider* a, BaseCollider* b)
{
    if (a == nullptr || b == nullptr) return false;

    return a->Intersects(b);
}
