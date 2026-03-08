
#include "Core/Framework.h"
#include "MainApp.h"

#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Transform.h"
#include "Entity/Entity.h"
#include "Entity/Components/Camera.h"
#include "Scene/SceneManager.h"
#include "Scripts/CameraController.h"

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

	GET_SINGLE(SceneManager)->ChangeScene(scene);
}

void MainApp::Update()
{
	
}

void MainApp::Render()
{

}
