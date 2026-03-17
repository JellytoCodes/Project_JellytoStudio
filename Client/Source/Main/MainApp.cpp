
#include "pch.h"
#include "MainApp.h"

#include "Core/Managers/TimeManager.h"
#include "Entity/Components/Collider/SphereCollider.h"
#include "Resource/Managers/ResourceManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Terrain.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Entity.h"
#include "Scripts/CameraController.h"
#include "Scripts/CharacterController.h"
#include "Scripts/CubeScript.h"
#include "Entity/Managers//CollisionManager.h"
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

    GET_SINGLE(SceneManager)->ChangeScene(_scene);
}

void MainApp::Update()
{
    CollisionManager::CheckCollision(_scene);
}

void MainApp::Render()
{

}

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
    charCollider->SetOffsetPosition(Vec3(0.f, 88.f, 0.f)); // 발 기준 중심
    _characterEntity->AddComponent(charCollider);

    _scene->Add(_characterEntity);

    // 카메라 (캐릭터 자식)
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
    desc.diffuse = Vec4(1.f, 1.f, 1.f, 1.f); // 붉은 색으로 구분
    desc.specular = Vec4(1.f);
    GET_SINGLE(ResourceManager)->Add(L"CubeMat", material);

    _cubeEntity = std::make_shared<Entity>();
    _cubeEntity->AddComponent(std::make_shared<Transform>());

    // 캐릭터 정면 3미터에 배치
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
    // TODO :
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
    auto terrain = std::make_shared<Terrain>();
    terrainEntity->AddComponent(terrain);
    terrainEntity->AddComponent(std::make_shared<Transform>());
    terrainEntity->GetTransform()->SetPosition(Vec3(-3.f, 0.f, -3.f));
    terrain->Create(5.f, 5.f, GET_SINGLE(ResourceManager)->Get<Material>(L"FloorMat"));
    
	_scene->Add(terrainEntity);
}