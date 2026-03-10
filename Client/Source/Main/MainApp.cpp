
#include "Core/Framework.h"
#include "MainApp.h"

#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Entity/Components/Transform.h"
#include "Entity/Entity.h"
#include "Entity/Components/Camera.h"
#include "Scene/SceneManager.h"
#include "Scripts/CameraController.h"
#include "Pipeline/Shader.h"

void MainApp::Init()
{
	std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/ModelShader.hlsl");

	auto scene = std::make_shared<Scene>();

	auto cameraEntity = std::make_shared<Entity>();
	cameraEntity->AddComponent(std::make_shared<Transform>());

	auto camera = std::make_shared<Camera>();
	cameraEntity->AddComponent(camera);
	cameraEntity->AddComponent(std::make_shared<CameraController>());

	cameraEntity->GetTransform()->SetPosition(Vec3(0, 0, -10.f));

	scene->Add(cameraEntity);
	scene->SetMainCamera(camera);

	std::shared_ptr<Model> chickenModel = std::make_shared <Model>();
	chickenModel->ReadModel(L"Separate/Pinguin_001");

	std::shared_ptr<ModelRenderer> chickenRenderer = std::make_shared<ModelRenderer>(shader);
	chickenRenderer->SetModel(chickenModel);
	chickenRenderer->SetPass(0);

	auto chicken = std::make_shared<Entity>();

	chicken->AddComponent(std::make_shared<Transform>());
	chicken->AddComponent(chickenRenderer);

	chicken->GetTransform()->SetPosition(Vec3(1.f, 0.f, 1.2f));

	scene->Add(chicken);

	GET_SINGLE(SceneManager)->ChangeScene(scene);
}

void MainApp::Update()
{
	
}

void MainApp::Render()
{

}
