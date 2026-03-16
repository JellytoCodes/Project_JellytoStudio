
#include "pch.h"
#include "MainApp.h"

#include "Resource/Managers/ResourceManager.h"
#include "Scene/Scene.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Terrain.h"
#include "Entity/Components/Camera.h"
#include "Entity/Entity.h"
#include "Scene/SceneManager.h"
#include "Scripts/CameraController.h"
#include "Resource/Texture.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"

void MainApp::Init()
{
    GET_SINGLE(ResourceManager)->Init();
    
    _scene = std::make_shared<Scene>();
    CreateCamera();
    CreateModelAnimator();
    CreateSkySphere();
    CreateFloor();

    GET_SINGLE(SceneManager)->ChangeScene(_scene);
}

void MainApp::Update()
{
	
}

void MainApp::Render()
{

}

void MainApp::CreateCamera()
{
    auto cameraEntity = std::make_shared<Entity>();
    cameraEntity->AddComponent(std::make_shared<Transform>());
    cameraEntity->AddComponent(std::make_shared<Camera>());
    cameraEntity->AddComponent(std::make_shared<CameraController>());
    cameraEntity->GetTransform()->SetPosition(Vec3(0, 1, -3.f));

    _scene->Add(cameraEntity);
    _scene->SetMainCamera(cameraEntity->GetComponent<Camera>());
}

void MainApp::CreateModelAnimator()
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/ModelShader.hlsl");

    std::shared_ptr<Model> m1 = std::make_shared<Model>();
    m1->ReadModel(L"Character/Ch03");
    m1->ReadMaterial(L"Character/Ch03");
    m1->ReadAnimation(L"Character/Dance");

    auto model = std::make_shared<Entity>();
    model->AddComponent(std::make_shared<Transform>());

    auto animator = std::make_shared<ModelAnimator>(shader);
    animator->SetModel(m1);
    model->AddComponent(animator);

    model->GetTransform()->SetScale(Vec3(0.01f));

    _scene->Add(model);
}

void MainApp::CreateSkySphere()
{
	std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/SkySphere.hlsl");

	std::shared_ptr<Material> material = std::make_shared<Material>();
	material->SetShader(shader);
	auto texture = GET_SINGLE(ResourceManager)->Load<Texture>(L"Sky", L"../Resources/Textures/clear_sky.png");
	material->SetDiffuseMap(texture);
	MaterialDesc& desc = material->GetMaterialDesc();
	desc.ambient = Vec4(1.f);
	desc.diffuse = Vec4(1.f);
	desc.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"Sky", material);

    // Object
	auto skySphere = std::make_shared<Entity>();
    skySphere->AddComponent(std::make_shared<Transform>());
	skySphere->AddComponent(std::make_shared<MeshRenderer>());
	{
		auto mesh = GET_SINGLE(ResourceManager)->Get<Mesh>(L"Sphere");
		skySphere->GetComponent<MeshRenderer>()->SetMesh(mesh);
	}
	{
		auto skyMaterial = GET_SINGLE(ResourceManager)->Get<Material>(L"Sky");
		skySphere->GetComponent<MeshRenderer>()->SetMaterial(skyMaterial);
	}

    _scene->Add(skySphere);
}

void MainApp::CreateLightSphere()
{
	// TODO : 
}

void MainApp::CreateFloor()
{
	std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl");

	std::shared_ptr<Material> material = std::make_shared<Material>();
	material->SetShader(shader);
	auto texture = GET_SINGLE(ResourceManager)->Load<Texture>(L"FloorMat", L"../Resources/Textures/grass.jpg");
	material->SetDiffuseMap(texture);
	MaterialDesc& desc = material->GetMaterialDesc();
	desc.ambient = Vec4(1.f);
	desc.diffuse = Vec4(1.f);
	desc.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"FloorMat", material);

    auto floorMat = GET_SINGLE(ResourceManager)->Get<Material>(L"FloorMat");

    auto terrainEntity = std::make_shared<Entity>();

    auto terrain = std::make_shared<Terrain>();
    terrainEntity->AddComponent(terrain);
    terrainEntity->AddComponent(std::make_shared<Transform>());
    terrainEntity->GetTransform()->SetPosition(Vec3(-2.f, 0.f, -2.f));

    terrain->Create(5.f, 5.f, floorMat);

    _scene->Add(terrainEntity);
}
