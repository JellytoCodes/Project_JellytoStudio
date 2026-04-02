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

    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return;

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();

    // walkChannel 채널로 블록 피킹 (Priming 블록의 pickableMask에 Character가 포함되어야 찾힘)
    std::shared_ptr<Entity> hitEntity;
    Vec3  hitNormal;
    float hitDist;

    if (!scene->PickBlock((int32)mp.x, (int32)mp.y,
                          _walkChannel, hitEntity, hitNormal, hitDist))
        return;

    // 상면만 허용 (hitNormal.y > 0.7) — 측면/하면 클릭은 이동 무시
    if (hitNormal.y < 0.7f) return;

    auto aabb = hitEntity->GetComponent<AABBCollider>();
    if (!aabb) return;

    const BoundingBox& box = aabb->GetBoundingBox();
    float blockTopY = box.Center.y + box.Extents.y;

    // 목적지 = 블록 상단 XZ 중심, Y = 블록 상단 (캐릭터 발 위치)
    Vec3 dest(box.Center.x, blockTopY, box.Center.z);
    MoveTo(dest);
}

// ── 이동 ──────────────────────────────────────────────────────────────────

void PointClickController::MoveTo(const Vec3& worldPos)
{
    _destination = worldPos; // XYZ 모두 포함 (블록 높이 반영)
    _isMoving    = true;
    UpdateAnimState(true);
}

void PointClickController::MoveToDestination(float dt)
{
    auto transform = GetTransform();
    if (!transform) return;

    Vec3  pos      = transform->GetLocalPosition();
    Vec3  toTarget = _destination - pos;
    float dist     = toTarget.Length();

    // 도착 판정
    if (dist <= _stopThreshold)
    {
        transform->SetLocalPosition(_destination);
        _isMoving = false;
        UpdateAnimState(false);
        return;
    }

    Vec3  dir      = toTarget / dist;
    float moveStep = std::min(_moveSpeed * dt, dist);

    // ── 블록 측면 충돌 체크 ──────────────────────────────────────
    // XZ 이동만 체크 (Y는 목적지로 부드럽게 이동하므로 현재 Y 기준)
    Vec3 nextPosXZ(pos.x + dir.x * moveStep,
                   pos.y,
                   pos.z + dir.z * moveStep);

    if (IsMovementBlocked(nextPosXZ))
    {
        _isMoving = false;
        UpdateAnimState(false);
        return;
    }

    // ── Y축 회전 ─────────────────────────────────────────────────
    Vec3  dirXZ  = Vec3(dir.x, 0.f, dir.z);
    float xzLen  = dirXZ.Length();
    if (xzLen > 0.001f)
    {
        dirXZ /= xzLen;
        float targetYaw = atan2f(-dirXZ.x, -dirXZ.z);
        Vec3  curRot    = transform->GetLocalRotation();
        float diff      = targetYaw - curRot.y;
        while (diff >  XM_PI) diff -= XM_2PI;
        while (diff < -XM_PI) diff += XM_2PI;
        float step  = XMConvertToRadians(_rotateSpeed) * dt;
        curRot.y   += (fabsf(diff) <= step) ? diff : (diff > 0.f ? step : -step);
        transform->SetLocalRotation(curRot);
    }

    // ── 이동 적용 (X, Y, Z 동시) ─────────────────────────────────
    transform->SetLocalPosition(pos + dir * moveStep);
}

// ── 블록 측면 충돌 체크 ───────────────────────────────────────────────────
//
// nextEntityPos: 캐릭터 Entity의 다음 위치 (콜라이더 하단 기준)
// 체크 대상: Priming 채널 블록 중 blockTopY > charFeetY 인 것 (벽 역할)
// 바닥 블록(blockTopY ≤ charFeetY)은 벽이 아니므로 제외

bool PointClickController::IsMovementBlocked(const Vec3& nextEntityPos) const
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return false;

    auto selfEntity = _entity.lock();
    if (!selfEntity) return false;

    auto charAabb = selfEntity->GetComponent<AABBCollider>();
    if (!charAabb) return false;

    // 현재 BoundingBox에서 월드 스케일 extents와 Y 오프셋 계산
    const BoundingBox& curBox  = charAabb->GetBoundingBox();
    Vec3  charExtents           = curBox.Extents;
    float curEntityY            = selfEntity->GetTransform()->GetLocalPosition().y;
    float colOffsetY            = curBox.Center.y - curEntityY; // Entity Y → 콜라이더 중심 Y 오프셋

    // 다음 위치에서의 캐릭터 AABB
    BoundingBox nextCharBox;
    nextCharBox.Center  = Vec3(nextEntityPos.x,
                               nextEntityPos.y + colOffsetY,
                               nextEntityPos.z);
    nextCharBox.Extents = charExtents;

    float charFeetY = nextEntityPos.y; // 발 위치 = Entity Y

    for (auto& entity : scene->GetEntities())
    {
        if (entity == selfEntity) continue;

        auto blockAabb = entity->GetComponent<AABBCollider>();
        if (!blockAabb) continue;

        // Priming 채널 블록만 벽 역할 (Mushroom은 관통 가능 — 장식용)
        if (blockAabb->GetOwnChannel() != CollisionChannel::Priming) continue;

        const BoundingBox& blockBox = blockAabb->GetBoundingBox();
        float blockTopY = blockBox.Center.y + blockBox.Extents.y;

        // 블록 상단이 캐릭터 발보다 높을 때만 벽 취급
        // (발 아래 블록 = 바닥이므로 제외, 같은 높이 블록도 제외)
        if (blockTopY <= charFeetY + 0.05f) continue;

        if (nextCharBox.Intersects(blockBox))
            return true;
    }

    return false;
}

// ── 애니메이션 ────────────────────────────────────────────────────────────

void PointClickController::UpdateAnimState(bool moving)
{
    auto entity = _entity.lock();
    if (!entity) return;

    auto asm_ = entity->GetComponent<AnimStateMachine>();
    if (!asm_) return;

    if (asm_->IsState(AnimState::Die) || asm_->IsState(AnimState::Attack)) return;
    asm_->SetState(moving ? AnimState::Walk : AnimState::Idle);
}