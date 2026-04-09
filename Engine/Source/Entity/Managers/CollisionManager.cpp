#include "Framework.h"
#include "CollisionManager.h"
#include "Entity/Entity.h"
#include "Entity/Components/Collider/BaseCollider.h"

std::vector<BaseCollider*> CollisionManager::s_DynamicColliders;
std::vector<BaseCollider*> CollisionManager::s_StaticColliders;

void CollisionManager::RegisterCollider(BaseCollider* collider, bool isStatic)
{
    if (collider == nullptr) return;
    auto& list = isStatic ? s_StaticColliders : s_DynamicColliders;
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
            {
                Entity* entityA = a->GetEntity();
                Entity* entityB = b->GetEntity();
                entityA->OnCollision(entityB);
                entityB->OnCollision(entityA);
            }
        }
    }

    const int32 statCount = static_cast<int32>(s_StaticColliders.size());
    for (int32 i = 0; i < dynCount; ++i)
    {
        BaseCollider* a = s_DynamicColliders[i];
        for (int32 j = 0; j < statCount; ++j)
        {
            BaseCollider* b = s_StaticColliders[j];
            if (Intersects(a, b))
            {
                Entity* entityA = a->GetEntity();
                Entity* entityB = b->GetEntity();
                entityA->OnCollision(entityB);
                entityB->OnCollision(entityA);
            }
        }
    }
}

bool CollisionManager::Intersects(BaseCollider* a, BaseCollider* b)
{
    if (a == nullptr || b == nullptr) return false;

    return a->Intersects(b);
}