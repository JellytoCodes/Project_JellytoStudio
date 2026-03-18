#include "pch.h"
#include "MainApp.h"

#include "App/ItemWindow.h"
#include "App/DetailWindow.h"
#include "Core/Managers/TimeManager.h"
#include "Core/Managers/InputManager.h"
#include "Entity/Components/Collider/SphereCollider.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Resource/Managers/ResourceManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelAnimation.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Terrain.h"
#include "Entity/Components/Camera.h"
#include "Entity/Entity.h"
#include "Scripts/CameraController.h"
#include "Scripts/CharacterController.h"
#include "Scripts/CubeScript.h"
#include "Entity/Managers/CollisionManager.h"
#include "Resource/Texture.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"

void MainApp::Init()
{
    GET_SINGLE(ResourceManager)->Init();

    _scene = std::make_shared<Scene>();
    CreateSkySphere();
    CreateFloor();
    CreateCube();
    CreateCharacter();

    // 씬 이름 설정
    _scene->SetName(L"Main Scene");

    GET_SINGLE(SceneManager)->ChangeScene(_scene);

    // 창 주입 (이 시점에 창이 이미 생성된 경우에만 유효)
    if (_itemWindow)
        _itemWindow->SetScene(_scene);
    if (_detailWindow)
        _detailWindow->SetScene(_scene);
}

void MainApp::Update()
{
    CollisionManager::CheckCollision(_scene);
    UpdatePicking();

    // DetailWindow 씬 오브젝트 목록 0.5초마다 갱신
    if (_detailWindow && _detailWindow->IsVisible())
    {
        static float refreshTimer = 0.f;
        refreshTimer += GET_SINGLE(TimeManager)->GetDeltaTime();
        if (refreshTimer >= 0.5f)
        {
            refreshTimer = 0.f;
            _detailWindow->RefreshEntityList();
        }
    }
}

void MainApp::Render()
{
}

// ── 피킹 ────────────────────────────────────────────────────────────────

void MainApp::UpdatePicking()
{
    // 마우스 좌클릭 Down 시에만 처리
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON)) return;

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();
    Ray ray  = ScreenToRay(mp.x, mp.y);

    auto picked = _scene->Pick(ray);

    if (picked == _pickedEntity) return; // 같은 대상 재클릭 → 무시
    _pickedEntity = picked;

    if (!_detailWindow) return;

    if (!picked)
    {
        _detailWindow->ClearDetail();
        return;
    }

    // ── DetailInfo 채우기 ────────────────────────────────────────
    DetailInfo info;

    // Transform
    auto tf = picked->GetTransform();
    if (tf)
    {
        Vec3 pos = tf->GetPosition();
        Vec3 rot = tf->GetRotation();   // 라디안
        Vec3 scl = tf->GetScale();

        info.tx = pos.x; info.ty = pos.y; info.tz = pos.z;
        // 라디안 → 도(degree)
        info.rx = XMConvertToDegrees(rot.x);
        info.ry = XMConvertToDegrees(rot.y);
        info.rz = XMConvertToDegrees(rot.z);
        info.sx = scl.x; info.sy = scl.y; info.sz = scl.z;
    }

    // Model / Animation (ModelAnimator가 있는 경우)
    auto animator = picked->GetComponent<ModelAnimator>();
    if (animator)
    {
        auto model = animator->GetModel();
        if (model)
        {
            info.modelName = model->GetMeshCount() > 0
                ? model->GetMeshByIndex(0)->name : L"(알 수 없음)";
            info.boneCount = (int)model->GetBoneCount();
            info.meshCount = (int)model->GetMeshCount();

            // 첫 번째 애니메이션 정보
            if (model->GetAnimationCount() > 0)
            {
                auto anim     = model->GetAnimationByIndex(0);
                info.animName   = anim->name;
                info.frameCount = (int)anim->frameCount;
                info.frameRate  = anim->frameRate;
                info.duration   = anim->duration;
            }
        }
    }

    _detailWindow->UpdateDetail(info);

    // DetailWindow가 숨겨져 있으면 자동으로 보여줌
    if (!_detailWindow->IsVisible())
        _detailWindow->Show();
}

Ray MainApp::ScreenToRay(int screenX, int screenY)
{
    auto scene  = GET_SINGLE(SceneManager)->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;

    if (!camera)
        return Ray(Vec3::Zero, Vec3::Forward);

    float W = camera->GetWidth();
    float H = camera->GetHeight();

    // NDC 좌표 (-1 ~ 1)
    float ndcX = (2.f * screenX / W) - 1.f;
    float ndcY = 1.f - (2.f * screenY / H);

    Matrix invProj = camera->GetProjectionMatrix().Invert();
    Matrix invView = camera->GetViewMatrix().Invert();

    // View Space 방향 벡터
    Vec4 viewDir = Vec4::Transform(Vec4(ndcX, ndcY, 1.f, 0.f), invProj);
    viewDir.w = 0.f;

    // World Space 방향 벡터
    Vec4 worldDir = Vec4::Transform(viewDir, invView);
    Vec3 dir(worldDir.x, worldDir.y, worldDir.z);
    dir.Normalize();

    // 카메라 월드 위치 (View 역행렬의 이동 성분)
    Vec3 origin(invView._41, invView._42, invView._43);

    return Ray(origin, dir);
}

// ── 씬 생성 ──────────────────────────────────────────────────────────────

void MainApp::CreateCharacter()
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/ModelShader.hlsl");

    std::shared_ptr<Model> m1 = std::make_shared<Model>();
    m1->ReadModel(L"Character/Ch03");
    m1->ReadMaterial(L"Character/Ch03");
    m1->ReadAnimation(L"Character/Idle");

    _characterEntity = std::make_shared<Entity>();
    _characterEntity->AddComponent(std::make_shared<Transform>());
    _characterEntity->AddComponent(std::make_shared<CharacterController>());

    _characterEntity->GetTransform()->SetScale(Vec3(0.01f));
    _characterEntity->GetTransform()->SetPosition(Vec3(0.f, 0.f, 0.f));

    auto animator = std::make_shared<ModelAnimator>(shader);
    animator->SetModel(m1);
    _characterEntity->AddComponent(animator);

    auto charCollider = std::make_shared<AABBCollider>();
    charCollider->SetBoxExtents(Vec3(20.f, 88.f, 20.f));
    charCollider->SetOffsetPosition(Vec3(0.f, 88.f, 0.f));
    _characterEntity->AddComponent(charCollider);

    _scene->Add(_characterEntity);

    // 카메라
    auto cameraEntity = std::make_shared<Entity>();
    cameraEntity->AddComponent(std::make_shared<Transform>());
    cameraEntity->AddComponent(std::make_shared<Camera>());
    cameraEntity->AddComponent(std::make_shared<CameraController>());
    cameraEntity->GetTransform()->SetPosition(Vec3(0.f, 1.5f, -3.f));

    _scene->Add(cameraEntity);
    _scene->SetMainCamera(cameraEntity->GetComponent<Camera>());
}

void MainApp::CreateCube()
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl");

    auto material = std::make_shared<Material>();
    material->SetShader(shader);
    auto texture = GET_SINGLE(ResourceManager)->Load<Texture>(L"CubeTex", L"../Resources/Textures/GridTile.jpg");
    material->SetDiffuseMap(texture);
    MaterialDesc& desc = material->GetMaterialDesc();
    desc.ambient = Vec4(1.f);
    desc.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    desc.specular = Vec4(1.f);
    GET_SINGLE(ResourceManager)->Add(L"CubeMat", material);

    _cubeEntity = std::make_shared<Entity>();
    _cubeEntity->AddComponent(std::make_shared<Transform>());
    _cubeEntity->GetTransform()->SetPosition(Vec3(0.f, 1.f, 3.f));
    _cubeEntity->GetTransform()->SetScale(Vec3(1.f));

    auto meshRenderer = std::make_shared<MeshRenderer>();
    meshRenderer->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Sphere"));
    meshRenderer->SetPass(0);
    meshRenderer->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"CubeMat"));
    _cubeEntity->AddComponent(meshRenderer);

    auto cubeCollider = std::make_shared<SphereCollider>();
    cubeCollider->SetRadius(0.5f);
    _cubeEntity->AddComponent(cubeCollider);

    _cubeEntity->AddComponent(std::make_shared<CubeScript>());
    _scene->Add(_cubeEntity);
}

void MainApp::CreateSkySphere()
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/SkySphere.hlsl");

    auto material = std::make_shared<Material>();
    material->SetShader(shader);
    auto texture = GET_SINGLE(ResourceManager)->Load<Texture>(L"Sky", L"../Resources/Textures/clear_sky.png");
    material->SetDiffuseMap(texture);
    MaterialDesc& desc = material->GetMaterialDesc();
    desc.ambient = Vec4(1.f);
    desc.diffuse = Vec4(1.f);
    desc.specular = Vec4(1.f);
    GET_SINGLE(ResourceManager)->Add(L"Sky", material);

    auto skySphere = std::make_shared<Entity>();
    skySphere->AddComponent(std::make_shared<Transform>());
    skySphere->AddComponent(std::make_shared<MeshRenderer>());
    skySphere->GetComponent<MeshRenderer>()->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Sphere"));
    skySphere->GetComponent<MeshRenderer>()->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"Sky"));
    _scene->Add(skySphere);
}

void MainApp::CreateLightSphere()
{
    // TODO
}

void MainApp::CreateFloor()
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl");

    auto material = std::make_shared<Material>();
    material->SetShader(shader);
    auto texture = GET_SINGLE(ResourceManager)->Load<Texture>(L"FloorMat", L"../Resources/Textures/GridTile.jpg");
    material->SetDiffuseMap(texture);
    MaterialDesc& desc = material->GetMaterialDesc();
    desc.ambient = Vec4(1.f);
    desc.diffuse = Vec4(1.f);
    desc.specular = Vec4(1.f);
    GET_SINGLE(ResourceManager)->Add(L"FloorMat", material);

    auto terrainEntity = std::make_shared<Entity>();
    auto terrain       = std::make_shared<Terrain>();
    terrainEntity->AddComponent(terrain);
    terrainEntity->AddComponent(std::make_shared<Transform>());
    terrainEntity->GetTransform()->SetPosition(Vec3(-3.f, 0.f, -3.f));
    terrain->Create(5.f, 5.f, GET_SINGLE(ResourceManager)->Get<Material>(L"FloorMat"));
    _scene->Add(terrainEntity);
}