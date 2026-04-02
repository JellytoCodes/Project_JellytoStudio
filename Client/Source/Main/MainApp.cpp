#include "pch.h"
#include "MainApp.h"

#include "Actors.h"

#include "Resource/Managers/ResourceManager.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Actor.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Entity/Managers/CollisionManager.h"
#include "Scripts/IsometricCameraController.h"
#include "Scripts/BlockPlacer.h"
#include "UI/PaletteWidget.h"
#include "Entity/Components/Light.h"
#include "Scene/SceneSerializer.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Pipeline/Shader.h"

void MainApp::Init()
{
    GET_SINGLE(ResourceManager)->Init();

    SceneSerializer::RegisterActor(L"SkySphereActor", [] { return std::make_shared<SkySphereActor>(); });
    SceneSerializer::RegisterActor(L"CubeActor", [] { return std::make_shared<CubeActor>(); });
    SceneSerializer::RegisterActor(L"SphereActor", [] { return std::make_shared<SphereActor>(); });
    SceneSerializer::RegisterActor(L"CharacterActor", [] { return std::make_shared<CharacterActor>(); });
    SceneSerializer::RegisterActor(L"LightActor", [] { return std::make_shared<LightActor>(); });

    _scene = std::make_shared<Scene>();
    _scene->SetName(L"Main Scene");

    InitScene();
    CreateCamera();
    CreatePlacementSystem();

    GET_SINGLE(SceneManager)->ChangeScene(_scene);
}

void MainApp::InitScene()
{
    auto spawn = 
    [&](std::shared_ptr<Actor> actor)
    {
        actor->Spawn(_scene);
        _actors.push_back(actor);
    };

    spawn(std::make_shared<SkySphereActor>());

    // 라이트
    auto lightActor = std::make_shared<LightActor>();
    lightActor->Spawn(_scene);
    _actors.push_back(lightActor);
    if (auto lightComp = lightActor->GetEntity()->GetComponent<Light>())
        _scene->SetMainLight(lightComp);

    {
        auto shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
        auto model  = std::make_shared<Model>();
        model->SetModelPath(L"../Resources/Models/MapModel/");
        model->SetTexturePath(L"../Resources/Textures/MapModel/");
        model->ReadModel(L"Priming_01");
        model->ReadMaterial(L"Priming_01");

        auto startBlock = std::make_shared<Entity>(L"StartBlock");
        startBlock->AddComponent(std::make_shared<Transform>());
        startBlock->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));

        auto mr = std::make_shared<ModelRenderer>(shader);
        mr->SetModel(model);
        mr->SetModelScale(Vec3(0.01f));
        startBlock->AddComponent(mr);

        auto col = std::make_shared<AABBCollider>();
        col->SetBoxExtents(Vec3(0.5f, 0.5f, 0.5f));
        col->SetOffsetPosition(Vec3(0.f, 0.5f, 0.f));       // 하단 기준
        col->SetOwnChannel(CollisionChannel::Priming);
        // BlockPlacer(Priming) + 캐릭터 이동(Character) 모두 피킹 가능
        col->SetPickableMask(
            static_cast<uint8>(CollisionChannel::Priming) |
            static_cast<uint8>(CollisionChannel::Character));
        startBlock->AddComponent(col);

        _scene->Add(startBlock);
        _startBlock = startBlock;
    }

    // 캐릭터 — 시작 블록 상단에 배치 (Y=1.0)
    auto charActor = std::make_shared<CharacterActor>();
    charActor->Spawn(_scene);
    _actors.push_back(charActor);
    _characterEntity = charActor->GetEntity();
    _characterEntity->GetTransform()->SetLocalPosition(Vec3(0.f, 1.0f, 0.f));
}

void MainApp::CreateCamera()
{
    auto cam = std::make_shared<Entity>(L"Camera");
    cam->AddComponent(std::make_shared<Transform>());
    cam->AddComponent(std::make_shared<Camera>());

    auto isoCtrl = std::make_shared<IsometricCameraController>();
    isoCtrl->SetDistance(10.f);
    isoCtrl->SetPanSpeed(10.f);
    isoCtrl->SetZoomSpeed(10.f);
    isoCtrl->SetMinDistance(5.f);
    isoCtrl->SetMaxDistance(40.f);
    cam->AddComponent(isoCtrl);
    _isoCamCtrl = isoCtrl;

    _scene->Add(cam);
    _scene->SetMainCamera(cam->GetComponent<Camera>());

    if (_characterEntity)
        isoCtrl->SetTarget(_characterEntity);
}

void MainApp::CreatePlacementSystem()
{
    _palette = std::make_shared<PaletteWidget>(L"Palette");
    _scene->Add(_palette);

    auto placerEntity = std::make_shared<Entity>(L"BlockPlacer");
    placerEntity->AddComponent(std::make_shared<Transform>());

    auto placer = std::make_shared<BlockPlacer>();
    placer->SetPalette(_palette);
    placer->SetSavePath(L"../Saved/scene.xml");

    placerEntity->AddComponent(placer);
    _blockPlacer = placer;
    _scene->Add(placerEntity);

    if (_characterEntity)
        placer->SetCharacterEntity(_characterEntity);
}

void MainApp::Update()
{
    CollisionManager::CheckCollision(_scene);
}

void MainApp::Render() {}