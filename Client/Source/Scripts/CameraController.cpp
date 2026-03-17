#include "pch.h"
#include "CameraController.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Entity/Components/Transform.h"

CameraController::CameraController()
{
}

CameraController::~CameraController()
{
}

void CameraController::Awake()
{
}

void CameraController::Start()
{
}

void CameraController::Update()
{
	float dt = GET_SINGLE(TimeManager)->GetDeltaTime();
	auto input = GET_SINGLE(InputManager);
	auto transform = GetTransform();

	if (input->GetButton(KEY_TYPE::RBUTTON))
	{
		POINT mouseDelta = input->GetMouseDelta();
		Vec3 rotation = transform->GetLocalRotation();

		rotation.y += mouseDelta.x * _rotationSpeed * dt;
		rotation.x += mouseDelta.y * _rotationSpeed * dt;

		const float limit = XMConvertToRadians(89.f);
		if (rotation.x > limit) rotation.x = limit;
		if (rotation.x < -limit) rotation.x = -limit;

		transform->SetLocalRotation(rotation);
	}
}

void CameraController::LateUpdate()
{
}

void CameraController::OnDestroy()
{
}