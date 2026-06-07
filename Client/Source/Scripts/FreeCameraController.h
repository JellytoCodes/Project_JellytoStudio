#pragma once
#include "Entity/Components/MonoBehaviour.h"

class FreeCameraController : public MonoBehaviour
{
public:
	FreeCameraController() = default;
	virtual ~FreeCameraController() = default;

	virtual void Awake() override;
	virtual void Update() override;

	void SetMoveSpeed(float speed) { _moveSpeed = speed; }
	void SetFastMultiplier(float multiplier) { _fastMultiplier = multiplier; }
	void SetLookSpeed(float speed) { _lookSpeed = speed; }
	void SetWheelSpeed(float speed) { _wheelSpeed = speed; }
	void SetBenchmarkView(const Vec3& pivot, float distance, float yawDeg);
	void SetView(const Vec3& position, float pitchDeg, float yawDeg);

private:
	void HandleLook(float dt);
	void HandleMove(float dt);
	void ApplyTransform();

	float _pitchDeg = 28.f;
	float _yawDeg = 45.f;
	float _moveSpeed = 12.f;
	float _fastMultiplier = 3.f;
	float _lookSpeed = 0.16f;
	float _wheelSpeed = 4.f;

	static constexpr float kMinPitch = -85.f;
	static constexpr float kMaxPitch = 85.f;
};
