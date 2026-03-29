#pragma once
#include "Entity/Components/MonoBehaviour.h"

class IsometricCameraController : public MonoBehaviour
{
public:
    IsometricCameraController() = default;
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

    float _pitchDeg   = 35.264f;
    float _yawDeg     = 45.f;

    float _panSpeed   = 10.f;
    float _zoomSpeed  = 20.f;
    float _orbitSpeed = 90.f;
    float _minDist    = 5.f;
    float _maxDist    = 80.f;
};