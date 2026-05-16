#include "pch.h"
#include "MainApp.h"

#include "Actors.h"
#include "Core/Managers/InputManager.h"
#include "Data/BlockTable.h"
#include "Entity/Actor.h"
#include "Entity/Entity.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Light.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Managers/CollisionManager.h"
#include "Graphics/Managers/InstancingManager.h"
#include "Resource/Managers/ResourceManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneSerializer.h"
#include "Scripts/IsometricCameraController.h"
#include "Scripts/BlockPlacer.h"
#include "Scripts/OneBlockScript.h"
#include "UI/PaletteWidget.h"
#include "UI/InventoryWidget.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Pipeline/Shader.h"

MainApp::MainApp()  {}
MainApp::~MainApp() {}

void MainApp::Init()
{
    GET_SINGLE(ResourceManager)->Init();
    GET_SINGLE(BlockTable)->Load(L"../Resources/Data/BlockMaster.json");

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

    Camera* mainCam = _scene->GetMainCamera();
    GET_SINGLE(SceneManager)->ChangeScene(std::move(_scene));

    _debugHUD.Init(mainCam);
}

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

    // ── StartBlock (원블록) ─────────────────────────────────────────────────
    const BlockRecord* startRec = GET_SINGLE(BlockTable)->GetRecordByKey(L"Priming1");
    assert(startRec && "BlockMaster.json 에 key='Priming1' 항목이 없습니다.");

    std::shared_ptr<Shader> meshShader = std::make_shared<Shader>(L"../Engine/Shaders/StaticMeshShader.hlsl");

    std::shared_ptr<Model> startModel = std::make_shared<Model>();
    startModel->SetModelPath  (L"../Resources/Models/MapModel/");
    startModel->SetTexturePath(L"../Resources/Textures/MapModel/");
    startModel->ReadModel   (startRec->modelName);
    startModel->ReadMaterial(startRec->modelName);

    auto mr = std::make_unique<ModelRenderer>(meshShader, false);
    mr->SetModel(startModel);
    mr->SetModelScale(Vec3(startRec->modelScale));

    const Vec3 startHalf = BlockPlacer::GetHalfExtents(startRec->collider);
    auto col = std::make_unique<AABBCollider>();
    col->SetBoxExtents(startHalf);
    col->SetOffsetPosition(Vec3(0.f, startHalf.y, 0.f));
    col->SetOwnChannel(startRec->ownChannel);
    col->SetPickableMask(startRec->pickableMask);
    col->SetStatic(true);

    auto startBlock = std::make_unique<Entity>(L"StartBlock");
    startBlock->AddComponent(std::make_unique<Transform>());
    startBlock->GetComponent<Transform>()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
    startBlock->AddComponent(std::move(mr));
    startBlock->AddComponent(std::move(col));

    _startBlock = startBlock.get();
    scene->Add(std::move(startBlock));

    // ── 캐릭터 ─────────────────────────────────────────────────────────────
    Actor* charActor = spawn(std::make_unique<CharacterActor>());
    _characterEntity = charActor->GetEntity();
    _characterEntity->GetComponent<Transform>()->SetLocalPosition(Vec3(0.f, 1.0f, 0.f));

    // ── OneBlockScript ──────────────────────────────────────────────────────
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

    auto camera = std::make_unique<Entity>(L"Camera");
    camera->AddComponent(std::make_unique<Transform>());
    camera->AddComponent(std::make_unique<Camera>());

    auto isoCtrl = std::make_unique<IsometricCameraController>();
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

    auto palette = std::make_unique<PaletteWidget>(L"Palette");
    _palette = palette.get();
    scene->Add(std::move(palette));

    auto placerEntity = std::make_unique<Entity>(L"BlockPlacer");
    placerEntity->AddComponent(std::make_unique<Transform>());

    auto placer = std::make_unique<BlockPlacer>();
    placer->SetPalette(_palette);
    placer->SetSavePath(L"../Saved/scene.json");

    _blockPlacer = placer.get();
    placerEntity->AddComponent(std::move(placer));
    scene->Add(std::move(placerEntity));

    if (_characterEntity)
        _blockPlacer->SetCharacterEntity(_characterEntity);
}

void MainApp::CreateInventorySystem()
{
    _inventoryData.GiveAll(10);

    if (_blockPlacer)    _blockPlacer->SetInventoryData(&_inventoryData);
    if (_oneBlockScript) _oneBlockScript->SetInventoryData(&_inventoryData);

    auto invWidget = std::make_unique<InventoryWidget>(L"Inventory");
    invWidget->SetInventoryData(&_inventoryData);
    invWidget->SetPalette(_palette);

    _inventoryWidget = invWidget.get();
    _scene->Add(std::move(invWidget));
}

void MainApp::Update()
{
    CollisionManager::CheckCollision();
    _debugHUD.Update();

    if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::F1))
        GET_SINGLE(InstancingManager)->DumpInstancingStats();
}

void MainApp::Render()
{
    _debugHUD.Render();
}