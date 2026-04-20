#include "pch.h"
#include "IsometricCameraController.h"

#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"

void IsometricCameraController::Awake()
{
	_targetPivot    = _pivot;
	_targetDistance = _distance;
	ApplyTransform();
}

void IsometricCameraController::Start()
{
	
}

void IsometricCameraController::LateUpdate()
{
	
}

void IsometricCameraController::OnDestroy()
{
	
}

void IsometricCameraController::Update()
{
	const float dt = GET_SINGLE(TimeManager)->GetDeltaTime();

	if (_target && _target->GetComponent<Transform>())
		_targetPivot = _target->GetComponent<Transform>()->GetPosition();

	HandleOrbit(dt);
	HandleZoom(dt);
	HandlePan(dt);

	const float pivotAlpha = 1.f - expf(-kPivotEase * dt);
	const float zoomAlpha  = 1.f - expf(-kZoomEase  * dt);

	_pivot    = Vec3::Lerp(_pivot,    _targetPivot,    pivotAlpha);
	_distance = _distance + (_targetDistance - _distance) * zoomAlpha;

	ApplyTransform();
}

void IsometricCameraController::HandlePan(float dt)
{
	if (_target != nullptr) return;

	auto input = GET_SINGLE(InputManager);

	float yawRad = XMConvertToRadians(_yawDeg);
	Vec3 forward( sinf(yawRad), 0.f,  cosf(yawRad));
	Vec3 right  ( cosf(yawRad), 0.f, -sinf(yawRad));

	Vec3 move = Vec3::Zero;
	if (input->GetButton(KEY_TYPE::W) || input->GetButton(KEY_TYPE::UP))    move += forward;
	if (input->GetButton(KEY_TYPE::S) || input->GetButton(KEY_TYPE::DOWN))  move -= forward;
	if (input->GetButton(KEY_TYPE::D) || input->GetButton(KEY_TYPE::RIGHT)) move += right;
	if (input->GetButton(KEY_TYPE::A) || input->GetButton(KEY_TYPE::LEFT))  move -= right;

	if (move.LengthSquared() > 0.f)
	{
		move.Normalize();
		_targetPivot += move * _panSpeed * dt;
	}
}

void IsometricCameraController::HandleZoom(float dt)
{
	auto input = GET_SINGLE(InputManager);
	if (input->GetButton(KEY_TYPE::Z)) _targetDistance += _zoomSpeed * dt;
	if (input->GetButton(KEY_TYPE::C)) _targetDistance -= _zoomSpeed * dt;
	_targetDistance = max(_minDist, std::min(_maxDist, _targetDistance));
}

void IsometricCameraController::HandleOrbit(float dt)
{
	auto input = GET_SINGLE(InputManager);
	if (input->GetButton(KEY_TYPE::Q)) _yawDeg -= _orbitSpeed * dt;
	if (input->GetButton(KEY_TYPE::E)) _yawDeg += _orbitSpeed * dt;
	if (_yawDeg >= 360.f) _yawDeg -= 360.f;
	if (_yawDeg <    0.f) _yawDeg += 360.f;
}

void IsometricCameraController::ApplyTransform()
{
	Transform* transform = GetTransform();
	if (!transform) return;

	float pitchRad = XMConvertToRadians(_pitchDeg);
	float yawRad   = XMConvertToRadians(_yawDeg);

	float cp = cosf(pitchRad), sp = sinf(pitchRad);
	float cy = cosf(yawRad),   sy = sinf(yawRad);

	Vec3 lookDir(cp * sy, -sp, cp * cy);
	lookDir.Normalize();

	Vec3 camPos = _pivot - lookDir * _distance;
	transform->SetLocalPosition(camPos);
	transform->SetLocalRotation(Vec3(pitchRad, yawRad, 0.f));
}