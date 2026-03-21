#pragma once
#include "Entity/Components/MonoBehaviour.h"

// ── PointClickController ──────────────────────────────────────────────────
// 우클릭 → 바닥 클릭 위치로 캐릭터 이동 (Point & Click)
// AnimStateMachine 연동: 이동 중 Walk, 정지 시 Idle 자동 전환
// 배치: Client/Source/Scripts/
class PointClickController : public MonoBehaviour
{
public:
    PointClickController();
    virtual ~PointClickController() = default;

    virtual void Awake()      override;
    virtual void Start()      override {}
    virtual void Update()     override;
    virtual void LateUpdate() override {}
    virtual void OnDestroy()  override {}

    void SetMoveSpeed(float s)     { _moveSpeed     = s; }
    void SetRotateSpeed(float s)   { _rotateSpeed   = s; }
    void SetStopThreshold(float t) { _stopThreshold = t; }
    void SetGroundY(float y)       { _groundY       = y; }

    void MoveTo(const Vec3& worldPos); // 외부(AI 등)에서 직접 목표 지정
    bool IsMoving()       const { return _isMoving;    }
    Vec3 GetDestination() const { return _destination; }

private:
    void HandleInput();
    void MoveToDestination(float dt);
    void UpdateAnimState(bool moving);

    Vec3  _destination   = Vec3::Zero;
    bool  _isMoving      = false;

    float _moveSpeed     = 5.f;
    float _rotateSpeed   = 720.f;  // deg/sec
    float _stopThreshold = 0.1f;
    float _groundY       = 0.f;
};