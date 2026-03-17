#include "pch.h"
#include "CharacterController.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Entity/Components/Transform.h"

CharacterController::CharacterController()
{
}

CharacterController::~CharacterController()
{
}

void CharacterController::Awake()
{
}

void CharacterController::Start()
{
}

void CharacterController::Update()
{
	float dt = GET_SINGLE(TimeManager)->GetDeltaTime();
	auto input = GET_SINGLE(InputManager);
	auto transform = GetTransform();

	Vec3 rotation = transform->GetLocalRotation();

	if (input->GetButton(KEY_TYPE::A))
		rotation.y -= XMConvertToRadians(_rotationSpeed) * dt;

	if (input->GetButton(KEY_TYPE::D))
		rotation.y += XMConvertToRadians(_rotationSpeed) * dt;

	transform->SetLocalRotation(rotation);

	Vec3 pos = transform->GetLocalPosition();
	Vec3 look = transform->GetLook();
	look.y = 0.f;
	look.Normalize();

	if (input->GetButton(KEY_TYPE::W))
		pos -= look * _moveSpeed * dt;

	if (input->GetButton(KEY_TYPE::S))
		pos += look * _moveSpeed * dt;

	transform->SetLocalPosition(pos);
}

void CharacterController::LateUpdate()
{
}

void CharacterController::OnDestroy()
{
}