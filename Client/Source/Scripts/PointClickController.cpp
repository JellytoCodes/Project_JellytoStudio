#include "pch.h"
#include "PointClickController.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/AnimStateMachine.h"
#include "Entity/Components/TileMap.h"
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

std::shared_ptr<TileMap> PointClickController::FindTileMap() const
{
    auto tm = _tileMap.lock();
    if (tm) return tm;

    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return nullptr;

    for (auto& entity : scene->GetEntities())
        if (auto tileMap = entity->GetComponent<TileMap>()) return tileMap;
    return nullptr;
}

// ── 입력 처리 ─────────────────────────────────────────────────────────────

void PointClickController::HandleInput()
{
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::RBUTTON)) return;

    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return;

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();
    Vec3 groundPos;
    if (!scene->PickGroundPoint((int32)mp.x, (int32)mp.y, groundPos, _groundY)) return;

    auto tileMap = FindTileMap();
    if (tileMap)
    {
        if (!tileMap->IsInsideBounds(groundPos))
        {
            ::OutputDebugStringW(L"[PointClick] 타일맵 범위 밖 — 이동 무시\n");
            return;
        }
        if (!tileMap->IsWalkableWorld(groundPos))
        {
            ::OutputDebugStringW(L"[PointClick] walkable=false 타일 — 이동 무시\n");
            return;
        }

        Vec3 snapped;
        if (tileMap->SnapToGrid(groundPos, snapped))
        {
            MoveTo(snapped);
            wchar_t dbg[128];
            swprintf_s(dbg, L"[PointClick] 목표 (스냅): (%.2f, %.2f)\n", snapped.x, snapped.z);
            ::OutputDebugStringW(dbg);
            return;
        }
    }

    MoveTo(groundPos);
}

// ── 이동 ──────────────────────────────────────────────────────────────────

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

    Vec3  dir      = toTarget / dist;
    float moveStep = std::min(_moveSpeed * dt, dist);

    auto tileMap = FindTileMap();
    if (tileMap)
    {
        Vec3 nextPos = pos + dir * moveStep;
        nextPos.y    = 0.f;

        int32 curCol, curRow, nextCol, nextRow;
        bool curValid  = tileMap->WorldToGrid(pos,     curCol,  curRow);
        bool nextValid = tileMap->WorldToGrid(nextPos, nextCol, nextRow);

        if (nextValid && (nextCol != curCol || nextRow != curRow))
        {
            if (!tileMap->IsWalkable(nextCol, nextRow))
            {
                _isMoving = false;
                UpdateAnimState(false);
                ::OutputDebugStringW(L"[PointClick] 블록 타일 진입 차단 — 멈춤\n");
                return;
            }
        }
    }

    float targetYaw = atan2f(-dir.x, -dir.z);
    Vec3  curRot    = transform->GetLocalRotation();
    float diff      = targetYaw - curRot.y;
    while (diff >  XM_PI) diff -= XM_2PI;
    while (diff < -XM_PI) diff += XM_2PI;
    float step  = XMConvertToRadians(_rotateSpeed) * dt;
    curRot.y   += (fabsf(diff) <= step) ? diff : (diff > 0.f ? step : -step);
    transform->SetLocalRotation(curRot);

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

    if (asm_->IsState(AnimState::Die) || asm_->IsState(AnimState::Attack)) return;
    asm_->SetState(moving ? AnimState::Walk : AnimState::Idle);
}