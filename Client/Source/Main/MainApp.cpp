
#include "Core/Framework.h"
#include "MainApp.h"

#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Transform.h"
#include "Entity/Entity.h"
#include "Entity/Components/Camera.h"
#include "Scene/SceneManager.h"
#include "Scripts/CameraController.h"
#include "Pipeline/Shader.h"

void MainApp::Init()
{
	auto scene = std::make_shared<Scene>();

    auto box = std::make_shared<Entity>();
    
    box->AddComponent(std::make_shared<Transform>());
    box->AddComponent(std::make_shared<MeshRenderer>());
	
    scene->Add(box);

	auto cameraEntity = std::make_shared<Entity>();
	cameraEntity->AddComponent(std::make_shared<Transform>());

	auto camera = std::make_shared<Camera>();
	cameraEntity->AddComponent(camera);
	cameraEntity->AddComponent(std::make_shared<CameraController>());

	cameraEntity->GetTransform()->SetPosition(Vec3(0, 0, -5.f));

	scene->Add(cameraEntity);
	scene->SetMainCamera(camera);

	{
		std::shared_ptr<Shader> shader = std::make_shared<Shader>();

		std::shared_ptr<Model> chickenModel = std::make_shared <Model>();
		chickenModel->ReadModel(L"Separate/Chicken_001");

		auto chicken = std::make_shared<Entity>();

		chicken->GetTransform()->SetPosition(Vec3(0, 0, -25.f));

		std::shared_ptr<ModelRenderer> chickenRenderer = std::make_shared<ModelRenderer>(shader);
		chickenRenderer->SetModel(chickenModel);
		chickenRenderer->SetPass(1);

		chicken->AddComponent(std::make_shared<Transform>());
		chicken->AddComponent(chickenRenderer);

		scene->Add(chicken);

		int a = 0;
	}

	CreateChicken();

	GET_SINGLE(SceneManager)->ChangeScene(scene);
}

void MainApp::Update()
{
	
}

void MainApp::Render()
{

}

void MainApp::CreateChicken()
{

}