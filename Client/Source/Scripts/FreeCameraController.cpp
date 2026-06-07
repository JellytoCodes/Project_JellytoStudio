#include "pch.h"
#include "FreeCameraController.h"

#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Transform.h"
#include "Entity/Entity.h"

void FreeCameraController::Awake()
{
	ApplyTransform();
}

void FreeCameraController::SetBenchmarkView(const Vec3& pivot, float distance, float yawDeg)
{
	const float pitchRad = XMConvertToRadians(_pitchDeg);
	const float yawRad = XMConvertToRadians(yawDeg);

	const float cp = cosf(pitchRad);
	const float sp = sinf(pitchRad);
	const float cy = cosf(yawRad);
	const float sy = sinf(yawRad);

	Vec3 lookDir(cp * sy, -sp, cp * cy);
	lookDir.Normalize();

	if (Transform* transform = GetTransform())
		transform->SetLocalPosition(pivot - lookDir * distance);

	_yawDeg = yawDeg;
	ApplyTransform();
}

void FreeCameraController::SetView(const Vec3& position, float pitchDeg, float yawDeg)
{
	if (Transform* transform = GetTransform())
		transform->SetLocalPosition(position);

	_pitchDeg = std::clamp(pitchDeg, kMinPitch, kMaxPitch);
	_yawDeg = yawDeg;
	ApplyTransform();
}

void FreeCameraController::Update()
{
	if (!GET_SINGLE(InputManager)->IsMainWindowActive()) return;

	const float dt = GET_SINGLE(TimeManager)->GetDeltaTime();
	HandleLook(dt);
	HandleMove(dt);
	ApplyTransform();
}

void FreeCameraController::HandleLook(float)
{
	auto input = GET_SINGLE(InputManager);
	if (!input->GetButton(KEY_TYPE::RBUTTON)) return;

	const POINT& delta = input->GetMouseDelta();
	_yawDeg += static_cast<float>(delta.x) * _lookSpeed;
	_pitchDeg += static_cast<float>(delta.y) * _lookSpeed;
	_pitchDeg = std::clamp(_pitchDeg, kMinPitch, kMaxPitch);

	if (_yawDeg >= 360.f) _yawDeg -= 360.f;
	if (_yawDeg < 0.f) _yawDeg += 360.f;
}

void FreeCameraController::HandleMove(float dt)
{
	auto input = GET_SINGLE(InputManager);
	Transform* transform = GetTransform();
	if (!transform) return;

	const float pitchRad = XMConvertToRadians(_pitchDeg);
	const float yawRad = XMConvertToRadians(_yawDeg);

	const float cp = cosf(pitchRad);
	const float sp = sinf(pitchRad);
	const float cy = cosf(yawRad);
	const float sy = sinf(yawRad);

	Vec3 forward(cp * sy, -sp, cp * cy);
	forward.Normalize();

	Vec3 right(cy, 0.f, -sy);
	right.Normalize();

	Vec3 up(0.f, 1.f, 0.f);
	Vec3 move = Vec3::Zero;
	Vec3 wheelMove = Vec3::Zero;

	if (input->GetButton(KEY_TYPE::W) || input->GetButton(KEY_TYPE::UP)) move += forward;
	if (input->GetButton(KEY_TYPE::S) || input->GetButton(KEY_TYPE::DOWN)) move -= forward;
	if (input->GetButton(KEY_TYPE::D) || input->GetButton(KEY_TYPE::RIGHT)) move += right;
	if (input->GetButton(KEY_TYPE::A) || input->GetButton(KEY_TYPE::LEFT)) move -= right;
	if (input->GetButton(KEY_TYPE::E)) move += up;
	if (input->GetButton(KEY_TYPE::Q)) move -= up;

	const int wheel = input->GetMouseWheelDelta();
	if (wheel != 0)
		wheelMove = forward * (static_cast<float>(wheel) * _wheelSpeed);

	const float speed = input->GetButton(KEY_TYPE::SHIFT) ? _moveSpeed * _fastMultiplier : _moveSpeed;
	const Vec3 prevPos = transform->GetLocalPosition();
	Vec3 nextPos = prevPos + wheelMove;
	if (move.LengthSquared() > 0.f)
	{
		move.Normalize();
		nextPos += move * speed * dt;
	}

	if ((nextPos - prevPos).LengthSquared() <= 0.000001f) return;
	transform->SetLocalPosition(nextPos);

	if (GetEntity())
	{
		if (Camera* camera = GetEntity()->GetComponent<Camera>())
			camera->SetSortDirty();
	}
}

void FreeCameraController::ApplyTransform()
{
	Transform* transform = GetTransform();
	if (!transform) return;

	const Vec3 prevRot = transform->GetLocalRotation();
	const Vec3 rot(XMConvertToRadians(_pitchDeg), XMConvertToRadians(_yawDeg), 0.f);
	transform->SetLocalRotation(rot);

	if ((prevRot - rot).LengthSquared() > 0.000001f && GetEntity())
	{
		if (Camera* camera = GetEntity()->GetComponent<Camera>())
			camera->SetSortDirty();
	}
}
