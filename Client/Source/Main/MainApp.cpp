#include "pch.h"
#include "MainApp.h"

#include "Actors.h"

#include "Resource/Managers/ResourceManager.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Actor.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Entity/Managers/CollisionManager.h"
#include "Scripts/IsometricCameraController.h"
#include "Entity/Components/Light.h"

void MainApp::Init()
{
    GET_SINGLE(ResourceManager)->Init();

    _scene = std::make_shared<Scene>();
    _scene->SetName(L"Main Scene");

    InitScene();
    CreateCamera();

    GET_SINGLE(SceneManager)->ChangeScene(_scene);
}

void MainApp::InitScene()
{
    auto spawn = [&](std::shared_ptr<Actor> actor)
    {
        actor->Spawn(_scene);
        _actors.push_back(actor);
    };

    spawn(std::make_shared<SkySphereActor>());
    spawn(std::make_shared<FloorActor>());

    // 캐릭터
    auto charActor = std::make_shared<CharacterActor>();
    charActor->Spawn(_scene);
    _actors.push_back(charActor);
    _characterEntity = charActor->GetEntity();

    // 라이트
    auto lightActor = std::make_shared<LightActor>();
    lightActor->Spawn(_scene);
    _actors.push_back(lightActor);
    if (auto lightComp = lightActor->GetEntity()->GetComponent<Light>())
        _scene->SetMainLight(lightComp);
}

void MainApp::CreateCamera()
{
    auto cam = std::make_shared<Entity>(L"Camera");
    cam->AddComponent(std::make_shared<Transform>());
    cam->AddComponent(std::make_shared<Camera>());

    auto isoCtrl = std::make_shared<IsometricCameraController>();
    isoCtrl->SetDistance(20.f);
    isoCtrl->SetPanSpeed(10.f);
    isoCtrl->SetZoomSpeed(15.f);
    isoCtrl->SetMinDistance(5.f);
    isoCtrl->SetMaxDistance(60.f);
    cam->AddComponent(isoCtrl);
    _isoCamCtrl = isoCtrl;

    _scene->Add(cam);
    _scene->SetMainCamera(cam->GetComponent<Camera>());

    if (_characterEntity)
        isoCtrl->SetTarget(_characterEntity);
}

void MainApp::Update()
{
    CollisionManager::CheckCollision(_scene);
}

void MainApp::Render()
{
}