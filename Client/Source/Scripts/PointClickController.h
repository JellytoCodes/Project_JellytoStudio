#pragma once
#include "Entity/Components/MonoBehaviour.h"

class TileMap;

// ── PointClickController ──────────────────────────────────────────────────
// 우클릭 → TileMap 기반 그리드 이동 (Point & Click)
// - 클릭 위치를 TileMap::SnapToGrid로 타일 중심으로 스냅
// - 타일맵 범위 밖이거나 walkable=false면 이동 무시
// - AnimStateMachine 연동: 이동 중 Walk, 정지 시 Idle
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

    void SetMoveSpeed(float s) { _moveSpeed = s; }
    void SetRotateSpeed(float s) { _rotateSpeed = s; }
    void SetStopThreshold(float t) { _stopThreshold = t; }
    void SetGroundY(float y) { _groundY = y; }

    // TileMap을 직접 설정 (캐릭터 이동 범위 제한에 사용)
    void SetTileMap(std::shared_ptr<TileMap> tileMap) { _tileMap = tileMap; }

    void MoveTo(const Vec3& worldPos); // 외부에서 직접 목표 지정 (그리드 스냅 적용)
    bool IsMoving()       const { return _isMoving; }
    Vec3 GetDestination() const { return _destination; }

private:
    void HandleInput();
    void MoveToDestination(float dt);
    void UpdateAnimState(bool moving);

    // 씬에서 TileMap 컴포넌트를 가진 Entity를 자동 탐색
    std::shared_ptr<TileMap> FindTileMap() const;

    Vec3  _destination = Vec3::Zero;
    bool  _isMoving = false;

    float _moveSpeed = 5.f;
    float _rotateSpeed = 720.f;  // deg/sec
    float _stopThreshold = 0.1f;
    float _groundY = 0.f;

    std::weak_ptr<TileMap> _tileMap;
};