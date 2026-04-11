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

	// ── 추적 타깃 ───────────────────────────────────────────────────────
	void SetTarget(Entity* target)			{ _target = target; }
	void ClearTarget()						{ _target = nullptr; }

	void  SetPanSpeed(float speed)			{ _panSpeed   = speed; }
	void  SetZoomSpeed(float speed)			{ _zoomSpeed  = speed; }
	void  SetOrbitSpeed(float speed)		{ _orbitSpeed = speed; }
	void  SetMinDistance(float distance)	{ _minDist    = distance; }
	void  SetMaxDistance(float distance)	{ _maxDist    = distance; }
	void  SetDistance(float distance)		{ _distance   = distance; }
	void  SetPivot(Vec3 pivot)				{ _pivot      = pivot; }

	float GetDistance()  const				{ return _distance; }
	float GetYawDegree() const				{ return _yawDeg;   }
	Vec3  GetPivot()     const				{ return _pivot;    }

private:
	void HandlePan(float dt);
	void HandleZoom(float dt);
	void HandleOrbit(float dt);
	void ApplyTransform();

	Entity*		_target			= nullptr;

	Vec3		_pivot			= Vec3::Zero;
	float		_distance		= 10.f;
	float		_pitchDeg		= 35.264f;
	float		_yawDeg			= 45.f;
	float		_panSpeed		= 10.f;
	float		_zoomSpeed		= 10.f;
	float		_orbitSpeed		= 90.f;
	float		_minDist		= 5.f;
	float		_maxDist		= 40.f;
};
