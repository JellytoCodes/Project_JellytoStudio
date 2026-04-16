#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "Entity/Components/Collider/CollisionChannel.h"

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

    void SetMoveSpeed(float s)     { _moveSpeed = s; }
    void SetRotateSpeed(float s)   { _rotateSpeed = s; }
    void SetStopThreshold(float t) { _stopThreshold = t; }

    void SetWalkChannel(CollisionChannel ch) { _walkChannel = ch; }

    void SetGroundY(float) {}

    void MoveTo(const Vec3& worldPos);
    bool IsMoving()       const { return _isMoving; }
    Vec3 GetDestination() const { return _destination; }

private:
    void HandleInput();
    void MoveToDestination(float dt);
    void UpdateAnimState(bool moving);

    bool IsMovementBlocked(const Vec3& nextEntityPos) const;

    Vec3                _destination  = Vec3::Zero;
    bool                _isMoving     = false;

    float               _moveSpeed     = 5.f;
    float               _rotateSpeed   = 720.f;
    float               _stopThreshold = 0.1f;

    CollisionChannel    _walkChannel = CollisionChannel::Character;
};