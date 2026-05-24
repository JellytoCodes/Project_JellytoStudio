#pragma once

class BaseCollider;

class CollisionManager
{
public:
    static void CheckCollision();

    static void RegisterCollider(BaseCollider* collider, bool isStatic);
    static void UnregisterCollider(BaseCollider* collider);

private:
    static bool Intersects(BaseCollider* a, BaseCollider* b);

    static std::vector<BaseCollider*> s_DynamicColliders;
    static std::vector<BaseCollider*> s_StaticColliders;
};
