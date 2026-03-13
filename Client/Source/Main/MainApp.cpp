
#include "Core/Framework.h"
#include "MainApp.h"

#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
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
    cameraEntity->AddComponent(std::make_shared<Camera>());
    cameraEntity->AddComponent(std::make_shared<CameraController>());
    cameraEntity->GetTransform()->SetPosition(Vec3(0, 5, -20.f));

    scene->Add(cameraEntity);
    scene->SetMainCamera(cameraEntity->GetComponent<Camera>());

    std::shared_ptr<Model> m1 = std::make_shared<Model>();
    m1->ReadModel(L"Character/Ch03");
    m1->ReadMaterial(L"Character/Ch03");
    m1->ReadAnimation(L"Character/Twist_Dance");

    auto e1 = std::make_shared<Entity>();
    e1->AddComponent(std::make_shared<Transform>());

    auto animator = std::make_shared<ModelAnimator>(shader);
    animator->SetModel(m1);
    e1->AddComponent(animator);

    e1->GetTransform()->SetScale(Vec3(0.1f));

    scene->Add(e1);

    GET_SINGLE(SceneManager)->ChangeScene(scene);
}

void MainApp::Update()
{
	
}

void MainApp::Render()
{

}
