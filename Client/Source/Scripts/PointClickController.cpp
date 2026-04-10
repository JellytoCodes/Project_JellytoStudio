#include "pch.h"
#include "PointClickController.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/AnimStateMachine.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

PointClickController::PointClickController() {}

void PointClickController::Awake()
{
    UpdateAnimState(false);
}

void PointClickController::Update()
{
    HandleInput();
    if (_isMoving)
        MoveToDestination(GET_SINGLE(TimeManager)->GetDeltaTime());
}

// ── 입력: 블록 상면 피킹 → 이동 목표 설정 ───────────────────────────────

void PointClickController::HandleInput()
{
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::RBUTTON)) return;

    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return;

    const POINT mp = GET_SINGLE(InputManager)->GetMousePos();

    // Scene::PickBlock 출력 파라미터는 Entity*& (raw pointer)
    // 이전 코드의 shared_ptr<Entity> hitEntity는 컴파일 오류
    Entity* hitEntity = nullptr;
    Vec3    hitNormal;
    float   hitDist;

    if (!scene->PickBlock((int32)mp.x, (int32)mp.y,
        _walkChannel, hitEntity, hitNormal, hitDist))
        return;

    // 상면만 허용 (hitNormal.y > 0.7) — 측면/하면 클릭은 이동 무시
    if (hitNormal.y < 0.7f) return;

    AABBCollider* aabb = hitEntity->GetComponent<AABBCollider>();
    if (!aabb) return;

    const BoundingBox& box = aabb->GetBoundingBox();
    const float        blockTopY = box.Center.y + box.Extents.y;

    Vec3 dest(box.Center.x, blockTopY, box.Center.z);
    MoveTo(dest);
}

// ── 이동 ──────────────────────────────────────────────────────────────────

void PointClickController::MoveTo(const Vec3& worldPos)
{
    _destination = worldPos;
    _isMoving = true;
    UpdateAnimState(true);
}

void PointClickController::MoveToDestination(float dt)
{
    Transform* transform = GetTransform();   // MonoBehaviour → Component::GetTransform()
    if (!transform) return;

    Vec3  pos = transform->GetLocalPosition();
    Vec3  toTarget = _destination - pos;
    float dist = toTarget.Length();

    if (dist <= _stopThreshold)
    {
        transform->SetLocalPosition(_destination);
        _isMoving = false;
        UpdateAnimState(false);
        return;
    }

    Vec3  dir = toTarget / dist;
    float moveStep = std::min(_moveSpeed * dt, dist);

    Vec3 nextPosXZ(pos.x + dir.x * moveStep, pos.y, pos.z + dir.z * moveStep);
    if (IsMovementBlocked(nextPosXZ))
    {
        _isMoving = false;
        UpdateAnimState(false);
        return;
    }

    Vec3  dirXZ = Vec3(dir.x, 0.f, dir.z);
    float xzLen = dirXZ.Length();
    if (xzLen > 0.001f)
    {
        dirXZ /= xzLen;
        float targetYaw = atan2f(-dirXZ.x, -dirXZ.z);
        Vec3  curRot = transform->GetLocalRotation();
        float diff = targetYaw - curRot.y;
        while (diff > XM_PI) diff -= XM_2PI;
        while (diff < -XM_PI) diff += XM_2PI;
        float step = XMConvertToRadians(_rotateSpeed) * dt;
        curRot.y += (fabsf(diff) <= step) ? diff : (diff > 0.f ? step : -step);
        transform->SetLocalRotation(curRot);
    }

    transform->SetLocalPosition(pos + dir * moveStep);
}

// ── 블록 측면 충돌 체크 ───────────────────────────────────────────────────

bool PointClickController::IsMovementBlocked(const Vec3& nextEntityPos) const
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return false;

    // Component::_entity 는 Entity* (raw pointer) — .lock() 없음
    // 이전 코드의 _entity.lock() 는 컴파일 오류
    Entity* selfEntity = _entity;
    if (!selfEntity) return false;

    AABBCollider* charAabb = selfEntity->GetComponent<AABBCollider>();
    if (!charAabb) return false;

    const BoundingBox& curBox = charAabb->GetBoundingBox();
    const Vec3         charExtents = curBox.Extents;

    // Entity에는 GetTransform()이 없음 → GetComponent<Transform>() 사용
    Transform* tf = selfEntity->GetComponent<Transform>();
    if (!tf) return false;
    const float curEntityY = tf->GetLocalPosition().y;
    const float colOffsetY = curBox.Center.y - curEntityY;

    BoundingBox nextCharBox;
    nextCharBox.Center = Vec3(nextEntityPos.x,
        nextEntityPos.y + colOffsetY,
        nextEntityPos.z);
    nextCharBox.Extents = charExtents;

    const float charFeetY = nextEntityPos.y;

    for (const auto& entity : scene->GetEntities())
    {
        // entity 는 const unique_ptr<Entity>& → .get() 로 raw ptr 비교
        if (entity.get() == selfEntity) continue;

        AABBCollider* blockAabb = entity->GetComponent<AABBCollider>();
        if (!blockAabb) continue;

        if (blockAabb->GetOwnChannel() != CollisionChannel::Priming) continue;

        const BoundingBox& blockBox = blockAabb->GetBoundingBox();

        // XZ 사전 컬링
        {
            const float dx = blockBox.Center.x - nextEntityPos.x;
            const float dz = blockBox.Center.z - nextEntityPos.z;
            if (dx * dx + dz * dz > 4.0f) continue;
        }

        const float blockTopY = blockBox.Center.y + blockBox.Extents.y;
        if (blockTopY <= charFeetY + 0.05f) continue;

        if (nextCharBox.Intersects(blockBox))
            return true;
    }

    return false;
}

// ── 애니메이션 ────────────────────────────────────────────────────────────

void PointClickController::UpdateAnimState(bool moving)
{
    // Component::_entity 는 Entity* (raw pointer) — .lock() 없음
    Entity* entity = _entity;
    if (!entity) return;

    AnimStateMachine* asm_ = entity->GetComponent<AnimStateMachine>();
    if (!asm_) return;

    if (asm_->IsState(AnimState::Die) || asm_->IsState(AnimState::Attack)) return;
    asm_->SetState(moving ? AnimState::Walk : AnimState::Idle);
}