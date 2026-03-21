#pragma once
#include "Entity/Components/MonoBehaviour.h"

// ── IsometricCameraController ─────────────────────────────────────────────
// DirectX LH 좌표계 + SimpleMath 기준 아이소메트릭 카메라
//
// SimpleMath 행렬 규칙 (row-major):
//   Right()    = (_11,_12,_13) = +X 축
//   Up()       = (_21,_22,_23) = +Y 축
//   Backward() = (_31,_32,_33) = +Z 축  ← GetLook()이 이걸 사용
//   Forward()  = -Backward()            = -Z 축
//
// Transform::UpdateTransform() 회전 순서: Rx * Ry * Rz (로컬 오일러)
// Camera::UpdateMatrix() : focusPos = eyePos + GetLook() (= eyePos + Backward())
//
// 아이소메트릭 설정:
//   Pitch(X축 양수 회전) = 앞으로 고개 숙임 → 위에서 내려다봄
//   기본값 35.264° (arctan(1/√2))
//   Yaw(Y축)   = 45° 시작, Q/E로 궤도 회전
//
// 조작:
//   W/S/A/D 또는 방향키 → 카메라 패닝
//   Q/E                 → Y축 궤도 회전 (45° 단위 스냅: F키)
//   Z/C                 → 줌 아웃/인
//   SetTarget(entity)   → Entity 자동 추적 (패닝 비활성)
class IsometricCameraController : public MonoBehaviour
{
public:
    IsometricCameraController();
    virtual ~IsometricCameraController() = default;

    virtual void Awake()      override;
    virtual void Start()      override;
    virtual void Update()     override;
    virtual void LateUpdate() override;
    virtual void OnDestroy()  override;

    // 추적 타깃 (nullptr이면 자유 패닝)
    void SetTarget(std::shared_ptr<Entity> target) { _target = target; }
    void ClearTarget()                             { _target.reset();  }

    // 파라미터 설정
    void  SetPanSpeed(float s)   { _panSpeed   = s; }
    void  SetZoomSpeed(float s)  { _zoomSpeed  = s; }
    void  SetOrbitSpeed(float s) { _orbitSpeed = s; }
    void  SetMinDistance(float d){ _minDist    = d; }
    void  SetMaxDistance(float d){ _maxDist    = d; }
    void  SetDistance(float d)   { _distance   = d; }
    void  SetPivot(Vec3 p)       { _pivot      = p; }

    float GetDistance()    const { return _distance; }
    float GetYawDegree()   const { return _yawDeg;   }
    Vec3  GetPivot()       const { return _pivot;    }

private:
    void HandlePan(float dt);
    void HandleZoom(float dt);
    void HandleOrbit(float dt);
    void ApplyTransform();

    std::weak_ptr<Entity> _target;

    Vec3  _pivot      = Vec3::Zero;
    float _distance   = 20.f;

    // DirectX LH + Backward()=Look 기준:
    // Pitch 양수 = X축 회전 = 앞으로 숙임 = 위에서 내려다봄
    float _pitchDeg   = 35.264f;   // 양수! (LH에서 내려다보기)
    float _yawDeg     = 45.f;

    float _panSpeed   = 10.f;
    float _zoomSpeed  = 20.f;
    float _orbitSpeed = 90.f;
    float _minDist    = 5.f;
    float _maxDist    = 80.f;
};