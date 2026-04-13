#include "pch.h"
#include "MainApp.h"

#include <Core/Managers/InputManager.h>

#include "Actors.h"
#include "../../../Engine/Source/Graphics/Managers/InstancingManager.h"
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
#include "Scripts/OneBlockScript.h"
#include "UI/PaletteWidget.h"
#include "UI/InventoryWidget.h"
#include "Entity/Components/Light.h"
#include "Scene/SceneSerializer.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Pipeline/Shader.h"

#include "Data/BlockDataTable.h"

MainApp::MainApp()  {}
MainApp::~MainApp() {}

void MainApp::Init()
{
    GET_SINGLE(ResourceManager)->Init();
    GET_SINGLE(BlockDataTable)->Load(L"../Resources/Data/BlockData.xml");

    SceneSerializer::RegisterActor(L"SkySphereActor",  [] { return std::make_unique<SkySphereActor>(); });
    SceneSerializer::RegisterActor(L"CubeActor",       [] { return std::make_unique<CubeActor>(); });
    SceneSerializer::RegisterActor(L"SphereActor",     [] { return std::make_unique<SphereActor>(); });
    SceneSerializer::RegisterActor(L"CharacterActor",  [] { return std::make_unique<CharacterActor>(); });
    SceneSerializer::RegisterActor(L"LightActor",      [] { return std::make_unique<LightActor>(); });

    _scene = std::make_unique<Scene>();
    _scene->SetName(L"Main Scene");

    InitScene();
    CreateCamera();
    CreatePlacementSystem();
    CreateInventorySystem();

    GET_SINGLE(SceneManager)->ChangeScene(std::move(_scene));
}

// ── 씬 초기화 ─────────────────────────────────────────────────────────────────

void MainApp::InitScene()
{
    Scene* scene = _scene.get();

    auto spawn = [&](std::unique_ptr<Actor> actor) -> Actor*
    {
        actor->Spawn(scene);
        Actor* raw = actor.get();
        _actors.push_back(std::move(actor));
        return raw;
    };

    spawn(std::make_unique<SkySphereActor>());

    Actor* lightActor = spawn(std::make_unique<LightActor>());
    if (Light* lightComp = lightActor->GetEntity()->GetComponent<Light>())
        scene->SetMainLight(lightComp);

    std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");

    std::shared_ptr<Model> model = std::make_shared<Model>();
    model->SetModelPath(L"../Resources/Models/MapModel/");
    model->SetTexturePath(L"../Resources/Textures/MapModel/");
    model->ReadModel(L"Priming_01");
    model->ReadMaterial(L"Priming_01");

    auto mr = std::make_unique<ModelRenderer>(shader, false);
    mr->SetModel(model);
    mr->SetModelScale(Vec3(0.01f));

    auto col = std::make_unique<AABBCollider>();
    col->SetBoxExtents(Vec3(0.5f, 0.5f, 0.5f));
    col->SetOffsetPosition(Vec3(0.f, 0.5f, 0.f));
    col->SetOwnChannel(CollisionChannel::Priming);
    col->SetPickableMask(
        static_cast<uint8>(CollisionChannel::Priming) |
        static_cast<uint8>(CollisionChannel::Character));
    col->SetStatic(true);

    auto startBlock = std::make_unique<Entity>(L"StartBlock");
    startBlock->AddComponent(std::make_unique<Transform>());
    startBlock->GetComponent<Transform>()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
    startBlock->AddComponent(std::move(mr));
    startBlock->AddComponent(std::move(col));

    _startBlock = startBlock.get();
    scene->Add(std::move(startBlock));

    Actor* charActor = spawn(std::make_unique<CharacterActor>());
    _characterEntity = charActor->GetEntity();
    _characterEntity->GetComponent<Transform>()->SetLocalPosition(Vec3(0.f, 1.0f, 0.f));

    if (_startBlock)
    {
        auto oneBlock = std::make_unique<OneBlockScript>();
        oneBlock->SetCharacterEntity(_characterEntity);
        _oneBlockScript = oneBlock.get();
        _startBlock->AddComponent(std::move(oneBlock));
    }
}

void MainApp::CreateCamera()
{
    Scene* scene = _scene.get();

    std::unique_ptr<Entity> camera = std::make_unique<Entity>(L"Camera");
    camera->AddComponent(std::make_unique<Transform>());
    camera->AddComponent(std::make_unique<Camera>());

    std::unique_ptr<IsometricCameraController> isoCtrl = std::make_unique<IsometricCameraController>();
    isoCtrl->SetDistance(10.f);
    isoCtrl->SetPanSpeed(10.f);
    isoCtrl->SetZoomSpeed(10.f);
    isoCtrl->SetMinDistance(5.f);
    isoCtrl->SetMaxDistance(40.f);

    _isoCamCtrl = isoCtrl.get();
    Camera* camComp = camera->GetComponent<Camera>();
    camera->AddComponent(std::move(isoCtrl));

    scene->SetMainCamera(camComp);
    scene->Add(std::move(camera));

    if (_characterEntity)
        _isoCamCtrl->SetTarget(_characterEntity);
}

void MainApp::CreatePlacementSystem()
{
    Scene* scene = _scene.get();

    std::unique_ptr<PaletteWidget> palette = std::make_unique<PaletteWidget>(L"Palette");
    _palette = palette.get();
    scene->Add(std::move(palette));

    std::unique_ptr<Entity> placerEntity = std::make_unique<Entity>(L"BlockPlacer");
    placerEntity->AddComponent(std::make_unique<Transform>());

    std::unique_ptr<BlockPlacer> placer = std::make_unique<BlockPlacer>();
    placer->SetPalette(_palette);
    placer->SetSavePath(L"../Saved/scene.xml");

    _blockPlacer = placer.get();
    placerEntity->AddComponent(std::move(placer));
    scene->Add(std::move(placerEntity));

    if (_characterEntity)
        _blockPlacer->SetCharacterEntity(_characterEntity);
}

void MainApp::CreateInventorySystem()
{
    //  _inventoryData.GiveAll(10);

    if (_blockPlacer)
        _blockPlacer->SetInventoryData(&_inventoryData);

    if (_oneBlockScript)
        _oneBlockScript->SetInventoryData(&_inventoryData);

    auto invWidget = std::make_unique<InventoryWidget>(L"Inventory");
    invWidget->SetInventoryData(&_inventoryData);
    invWidget->SetPalette(_palette);

    _inventoryWidget = invWidget.get();
    _scene->Add(std::move(invWidget));
}

void MainApp::Update()
{
    CollisionManager::CheckCollision();

    if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::F1))
        GET_SINGLE(InstancingManager)->DumpInstancingStats();
}

void MainApp::Render() {}