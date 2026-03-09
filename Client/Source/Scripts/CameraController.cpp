
#include "Core/Framework.h"
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

    Vec3 pos = transform->GetPosition();

    if (input->GetButton(KEY_TYPE::W)) 
		pos += transform->GetLook() * _speed * dt;

    if (input->GetButton(KEY_TYPE::S)) 
		pos -= transform->GetLook() * _speed * dt;

    if (input->GetButton(KEY_TYPE::A)) 
		pos -= transform->GetRight() * _speed * dt;

    if (input->GetButton(KEY_TYPE::D)) 
		pos += transform->GetRight() * _speed * dt;

    transform->SetPosition(pos);

	if (input->GetButton(KEY_TYPE::RBUTTON))
    {
        POINT mouseDelta = input->GetMouseDelta();
        Vec3 rotation = transform->GetRotation();

        rotation.y += mouseDelta.x * _rotationSpeed * dt;
        rotation.x += mouseDelta.y * _rotationSpeed * dt;

        // Áü¹ú¶ô ¹æÁö
        if (rotation.x > 89.f) rotation.x = 89.f;
        if (rotation.x < -89.f) rotation.x = -89.f;

        transform->SetRotation(rotation);
    }

    if (input->GetButton(KEY_TYPE::R))
    {
	    transform->SetPosition(Vec3(0.f, 0.f, -5.f));
        transform->SetRotation(Vec3(0.f, 0.f, 0.f));
    }
}

void CameraController::LateUpdate()
{
	
}

void CameraController::OnDestroy()
{
	
}
