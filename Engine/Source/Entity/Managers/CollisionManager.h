#pragma once

class BaseCollider;

class CollisionManager
{
public:
    // Scene 파라미터 제거 — 내부 캐시 리스트 직접 사용
    static void CheckCollision();

    // Scene::Add / Scene::Remove 에서 호출
    // isStatic: collider->IsStatic() 전달
    static void RegisterCollider(BaseCollider* collider, bool isStatic);
    static void UnregisterCollider(BaseCollider* collider);

private:
    static bool Intersects(BaseCollider* a, BaseCollider* b);

    // Dynamic: 매 프레임 이동 가능한 콜라이더 (캐릭터, 프로젝타일 등)
    // Static : 배치된 블록처럼 절대 이동하지 않는 콜라이더
    // Static vs Static 검사는 완전 생략 → O(D²/2 + D*S) 복잡도
    static std::vector<BaseCollider*> s_DynamicColliders;
    static std::vector<BaseCollider*> s_StaticColliders;
};