#include "pch.h"
#include "Actors.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/TileMap.h"
#include "Entity/Components/AnimStateMachine.h"
#include "Scripts/PointClickController.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Components/Collider/CollisionChannel.h"
#include "Entity/Components/Collider/SphereCollider.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Resource/Managers/ResourceManager.h"
#include "Resource/Texture.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"
#include "Entity/Components/Light.h"

void SkySphereActor::BuildEntity()
{
	std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/SkySphere.hlsl");
	std::shared_ptr<Material> material = std::make_shared<Material>();
	material->SetShader(shader);
	std::shared_ptr<Texture> texture = GET_SINGLE(ResourceManager)->Load<Texture>(L"Sky", L"../Resources/Textures/clear_sky.png");
	material->SetDiffuseMap(texture);
	MaterialDesc& desc = material->GetMaterialDesc();
	desc.ambient = Vec4(1.f);
	desc.diffuse = Vec4(1.f);
	desc.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"Sky", material);

	std::unique_ptr<MeshRenderer> meshRenderer = std::make_unique<MeshRenderer>();
	meshRenderer->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Sphere"));
	meshRenderer->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"Sky"));
	_entity->AddComponent(std::move(meshRenderer));
}

void FloorActor::BuildEntity()
{
	std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
	std::shared_ptr<Material> material = std::make_shared<Material>();
	material->SetShader(shader);
	std::shared_ptr<Texture> texture = GET_SINGLE(ResourceManager)->Load<Texture>(L"FloorMat", L"../Resources/Textures/GridTile.jpg");
	material->SetDiffuseMap(texture);
	MaterialDesc& desc = material->GetMaterialDesc();
	desc.ambient = Vec4(1.f);
	desc.diffuse = Vec4(1.f);
	desc.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"FloorMat", material);

	std::unique_ptr<TileMap> tileMap = std::make_unique<TileMap>();
	_entity->AddComponent(std::move(tileMap));
	tileMap->Create(20, 20, 1.f, GET_SINGLE(ResourceManager)->Get<Material>(L"FloorMat"));

	_entity->GetComponent<Transform>()->SetLocalPosition(Vec3(-10.f, 0.f, -10.f));
}

void CubeActor::BuildEntity()
{
	std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
	std::shared_ptr<Material> material = std::make_shared<Material>();
	material->SetShader(shader);
	std::shared_ptr<Texture> texture = GET_SINGLE(ResourceManager)->Load<Texture>(L"CubeTex", L"../Resources/Textures/GridTile.jpg");
	material->SetDiffuseMap(texture);
	MaterialDesc& desc = material->GetMaterialDesc();
	desc.ambient = Vec4(1.f);
	desc.diffuse = Vec4(1.f);
	desc.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"CubeMat", material);
	_entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.5f));

	std::unique_ptr<MeshRenderer> meshRenderer = std::make_unique<MeshRenderer>();
	meshRenderer->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube"));
	meshRenderer->SetPass(0);
	meshRenderer->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"CubeMat"));
	_entity->AddComponent(std::move(meshRenderer));

	auto col = std::make_unique<AABBCollider>();
	col->SetBoxExtents(Vec3(0.5f));
	_entity->AddComponent(std::move(col));
}

void SphereActor::BuildEntity()
{
	std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
	std::shared_ptr<Material> material = std::make_shared<Material>();
	material->SetShader(shader);
	std::shared_ptr<Texture> texture = GET_SINGLE(ResourceManager)->Load<Texture>(L"CubeTex", L"../Resources/Textures/GridTile.jpg");
	material->SetDiffuseMap(texture);
	MaterialDesc& desc = material->GetMaterialDesc();
	desc.ambient = Vec4(1.f);
	desc.diffuse = Vec4(1.f);
	desc.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"CubeMat", material);
	_entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.5f));

	std::unique_ptr<MeshRenderer> meshRenderer = std::make_unique<MeshRenderer>();
	meshRenderer->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Sphere"));
	meshRenderer->SetPass(0);
	meshRenderer->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"CubeMat"));
	_entity->AddComponent(std::move(meshRenderer));

	std::unique_ptr<SphereCollider> col = std::make_unique<SphereCollider>();
	col->SetRadius(0.5f);
	_entity->AddComponent(std::move(col));
}

void CharacterActor::BuildEntity()
{
	std::shared_ptr<Shader> shader = std::make_shared<Shader>(L"../Engine/Shaders/ModelShader.hlsl");
	std::shared_ptr<Model> model = std::make_shared<Model>();
	model->ReadModel(L"Kaya/Kaya");
	model->ReadMaterial(L"Kaya/Kaya");
	model->ReadAnimation(L"Kaya/Idle");
	model->ReadAnimation(L"Kaya/Walking");

	_entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.01f));
	_entity->GetComponent<Transform>()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));

	std::unique_ptr<ModelAnimator> animator = std::make_unique<ModelAnimator>(shader);
	animator->SetModel(model);
	_entity->AddComponent(std::move(animator));

	std::unique_ptr<AABBCollider> col = std::make_unique<AABBCollider>();
	col->SetBoxExtents(Vec3(20.f, 88.f, 20.f));
	col->SetOffsetPosition(Vec3(0.f, 88.f, 0.f));
	col->SetShowDebug(false);
	col->SetOwnChannel(CollisionChannel::Character);
	col->SetPickableMask(0);
	_entity->AddComponent(std::move(col));

	std::unique_ptr<AnimStateMachine> animStateMachine = std::make_unique<AnimStateMachine>();
	animStateMachine->RegisterClip(AnimState::Idle, 0);
	animStateMachine->RegisterClip(AnimState::Walk, 1);
	animStateMachine->SetTweenDuration(0.2f);
	_entity->AddComponent(std::move(animStateMachine));

	std::unique_ptr<PointClickController> pcc = std::make_unique<PointClickController>();
	pcc->SetMoveSpeed(3.f);
	pcc->SetWalkChannel(CollisionChannel::Character);
	_entity->AddComponent(std::move(pcc));
}

void LightActor::BuildEntity()
{
	std::unique_ptr<Light> light = std::make_unique<Light>();

	LightDesc desc;
	desc.ambient = Color(0.35f, 0.38f, 0.42f, 1.f);
	desc.diffuse = Color(1.0f, 0.95f, 0.85f, 1.f);
	desc.specular = Color(0.6f, 0.6f, 0.6f, 1.f);
	desc.emissive = Color(0.0f, 0.0f, 0.0f, 1.f);

	Vec3 dir = Vec3(1.f, -3.f, 1.f);
	dir.Normalize();
	desc.direction = dir;
	light->SetLightDesc(desc);

	_entity->AddComponent(std::move(light));

	auto check = _entity->GetComponent<Light>();
	assert(check != nullptr && "[LightActor] GetComponent<Light>() returned null after AddComponent");
}