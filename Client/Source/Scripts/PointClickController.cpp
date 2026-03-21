#include "pch.h"
#include "PointClickController.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/AnimStateMachine.h"
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

void PointClickController::HandleInput()
{
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::RBUTTON)) return;

    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return;

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();
    Vec3 groundPos;
    if (!scene->PickGroundPoint((int32)mp.x, (int32)mp.y, groundPos, _groundY)) return;

    MoveTo(groundPos);

    wchar_t dbg[128];
    swprintf_s(dbg, L"[PointClick] 목표: (%.2f, %.2f, %.2f)\n",
        groundPos.x, groundPos.y, groundPos.z);
    ::OutputDebugStringW(dbg);
}

void PointClickController::MoveTo(const Vec3& worldPos)
{
    _destination   = worldPos;
    _destination.y = _groundY;
    _isMoving      = true;
    UpdateAnimState(true);
}

void PointClickController::MoveToDestination(float dt)
{
    auto transform = GetTransform();
    if (!transform) return;

    Vec3  pos      = transform->GetLocalPosition();
    Vec3  toTarget = _destination - pos;
    toTarget.y     = 0.f;
    float dist     = toTarget.Length();

    // 도착
    if (dist <= _stopThreshold)
    {
        transform->SetLocalPosition(Vec3(_destination.x, pos.y, _destination.z));
        _isMoving = false;
        UpdateAnimState(false);
        ::OutputDebugStringW(L"[PointClick] 도착\n");
        return;
    }

    // Y축 회전
    // 캐릭터 모델이 FBX 기본 임포트 시 -Z를 바라봄
    // GetLook() = Backward() = +Z이므로 → 180° 보정
    Vec3  dir       = toTarget / dist; // Normalize
    float targetYaw = atan2f(-dir.x, -dir.z); // -Z 정면 모델 기준
    Vec3  curRot   = transform->GetLocalRotation();
    float diff     = targetYaw - curRot.y;
    while (diff >  XM_PI) diff -= XM_2PI;
    while (diff < -XM_PI) diff += XM_2PI;
    float step = XMConvertToRadians(_rotateSpeed) * dt;
    curRot.y  += (fabsf(diff) <= step) ? diff : (diff > 0.f ? step : -step);
    transform->SetLocalRotation(curRot);

    // 이동
    float moveStep = std::min(_moveSpeed * dt, dist);
    pos.x += dir.x * moveStep;
    pos.z += dir.z * moveStep;
    transform->SetLocalPosition(pos);
}

void PointClickController::UpdateAnimState(bool moving)
{
    auto entity = _entity.lock();
    if (!entity) return;

    auto asm_ = entity->GetComponent<AnimStateMachine>();
    if (!asm_) return;

    // Die / Attack 상태는 PointClick이 건드리지 않음
    if (asm_->IsState(AnimState::Die) || asm_->IsState(AnimState::Attack)) return;

    asm_->SetState(moving ? AnimState::Walk : AnimState::Idle);
}