
#include "pch.h"
#include "Actors.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Terrain.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Components/Collider/SphereCollider.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Resource/Managers/ResourceManager.h"
#include "Resource/Texture.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"
#include "Scripts/CubeScript.h"
#include "Scripts/CharacterController.h"

void SkySphereActor::BuildEntity()
{
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/SkySphere.hlsl");
	auto mat = std::make_shared<Material>();
	mat->SetShader(shader);
	auto tex = GET_SINGLE(ResourceManager)->Load<Texture>(L"Sky", L"../Resources/Textures/clear_sky.png");
	mat->SetDiffuseMap(tex);
	MaterialDesc& d = mat->GetMaterialDesc();
	d.ambient = d.diffuse = d.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"Sky", mat);

	auto mr = std::make_shared<MeshRenderer>();
	mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Sphere"));
	mr->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"Sky"));
	_entity->AddComponent(mr);
}

void FloorActor::BuildEntity()
{
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl");
	auto mat = std::make_shared<Material>();
	mat->SetShader(shader);
	auto tex = GET_SINGLE(ResourceManager)->Load<Texture>(L"FloorMat", L"../Resources/Textures/GridTile.jpg");
	mat->SetDiffuseMap(tex);
	MaterialDesc& d = mat->GetMaterialDesc();
	d.ambient = d.diffuse = d.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"FloorMat", mat);

	auto terrain = std::make_shared<Terrain>();
	_entity->AddComponent(terrain);
	terrain->Create(5.f, 5.f, GET_SINGLE(ResourceManager)->Get<Material>(L"FloorMat"));
}

void CubeActor::BuildEntity()
{
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl");
	auto mat = std::make_shared<Material>();
	mat->SetShader(shader);
	auto tex = GET_SINGLE(ResourceManager)->Load<Texture>(L"CubeTex", L"../Resources/Textures/GridTile.jpg");
	mat->SetDiffuseMap(tex);
	MaterialDesc& d = mat->GetMaterialDesc();
	d.ambient = d.diffuse = d.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"CubeMat", mat);
	_entity->GetTransform()->SetLocalScale(Vec3(0.5f));

	auto mr = std::make_shared<MeshRenderer>();
	mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube"));
	mr->SetPass(0);
	mr->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"CubeMat"));
	_entity->AddComponent(mr);

	auto col = std::make_shared<AABBCollider>();
	col->SetBoxExtents(Vec3(0.5f));
	_entity->AddComponent(col);

	_entity->AddComponent(std::make_shared<CubeScript>());
}

void SphereActor::BuildEntity()
{
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl");
	auto mat = std::make_shared<Material>();
	mat->SetShader(shader);
	auto tex = GET_SINGLE(ResourceManager)->Load<Texture>(L"CubeTex", L"../Resources/Textures/GridTile.jpg");
	mat->SetDiffuseMap(tex);
	MaterialDesc& d = mat->GetMaterialDesc();
	d.ambient = d.diffuse = d.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"CubeMat", mat);
	_entity->GetTransform()->SetLocalScale(Vec3(0.5f));

	auto mr = std::make_shared<MeshRenderer>();
	mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Sphere"));
	mr->SetPass(0);
	mr->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"CubeMat"));
	_entity->AddComponent(mr);

	auto col = std::make_shared<SphereCollider>();
	col->SetRadius(0.5f);
	_entity->AddComponent(col);

	_entity->AddComponent(std::make_shared<CubeScript>());
}

void CharacterActor::BuildEntity()
{
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/ModelShader.hlsl");
	auto model = std::make_shared<Model>();
	model->ReadModel(L"Character/Ch03");
	model->ReadMaterial(L"Character/Ch03");
	model->ReadAnimation(L"Character/Idle");

	_entity->GetTransform()->SetLocalScale(Vec3(0.01f));
	_entity->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));

	//_entity->AddComponent(std::make_shared<CharacterController>());

	auto animator = std::make_shared<ModelAnimator>(shader);
	animator->SetModel(model);
	_entity->AddComponent(animator);

	auto col = std::make_shared<AABBCollider>();
	col->SetBoxExtents(Vec3(20.f, 88.f, 20.f));
	col->SetOffsetPosition(Vec3(0.f, 88.f, 0.f));
	_entity->AddComponent(col);
}
