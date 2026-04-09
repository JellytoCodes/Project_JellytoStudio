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
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/SkySphere.hlsl");
	auto mat = std::make_shared<Material>();
	mat->SetShader(shader);
	auto tex = GET_SINGLE(ResourceManager)->Load<Texture>(L"Sky", L"../Resources/Textures/clear_sky.png");
	mat->SetDiffuseMap(tex);
	MaterialDesc& d = mat->GetMaterialDesc();
	d.ambient = d.diffuse = d.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"Sky", mat);

	auto mr = std::make_unique<MeshRenderer>();
	mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Sphere"));
	mr->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"Sky"));
	_entity->AddComponent(std::move(mr));
}

void FloorActor::BuildEntity()
{
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
	auto mat = std::make_shared<Material>();
	mat->SetShader(shader);
	auto tex = GET_SINGLE(ResourceManager)->Load<Texture>(L"FloorMat", L"../Resources/Textures/GridTile.jpg");
	mat->SetDiffuseMap(tex);
	MaterialDesc& d = mat->GetMaterialDesc();
	d.ambient = d.diffuse = d.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"FloorMat", mat);

	auto tileMap = std::make_unique<TileMap>();
	_entity->AddComponent(std::move(tileMap));
	tileMap->Create(20, 20, 1.f, GET_SINGLE(ResourceManager)->Get<Material>(L"FloorMat"));
	
	_entity->GetComponent<Transform>()->SetLocalPosition(Vec3(-10.f, 0.f, -10.f));
}

void CubeActor::BuildEntity()
{
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
	auto mat = std::make_shared<Material>();
	mat->SetShader(shader);
	auto tex = GET_SINGLE(ResourceManager)->Load<Texture>(L"CubeTex", L"../Resources/Textures/GridTile.jpg");
	mat->SetDiffuseMap(tex);
	MaterialDesc& d = mat->GetMaterialDesc();
	d.ambient = d.diffuse = d.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"CubeMat", mat);
	_entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.5f));

	auto mr = std::make_unique<MeshRenderer>();
	mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube"));
	mr->SetPass(0);
	mr->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"CubeMat"));
	_entity->AddComponent(std::move(mr));

	auto col = std::make_unique<AABBCollider>();
	col->SetBoxExtents(Vec3(0.5f));
	_entity->AddComponent(std::move(col));
}

void SphereActor::BuildEntity()
{
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
	auto mat = std::make_shared<Material>();
	mat->SetShader(shader);
	auto tex = GET_SINGLE(ResourceManager)->Load<Texture>(L"CubeTex", L"../Resources/Textures/GridTile.jpg");
	mat->SetDiffuseMap(tex);
	MaterialDesc& d = mat->GetMaterialDesc();
	d.ambient = d.diffuse = d.specular = Vec4(1.f);
	GET_SINGLE(ResourceManager)->Add(L"CubeMat", mat);
	_entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.5f));

	auto mr = std::make_unique<MeshRenderer>();
	mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Sphere"));
	mr->SetPass(0);
	mr->SetMaterial(GET_SINGLE(ResourceManager)->Get<Material>(L"CubeMat"));
	_entity->AddComponent(std::move(mr));

	auto col = std::make_unique<SphereCollider>();
	col->SetRadius(0.5f);
	_entity->AddComponent(std::move(col));
}

void CharacterActor::BuildEntity()
{
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/ModelShader.hlsl");
	auto model = std::make_shared<Model>();
	model->ReadModel(L"Kaya/Kaya");
	model->ReadMaterial(L"Kaya/Kaya");
	model->ReadAnimation(L"Kaya/Idle"); // index 0
	model->ReadAnimation(L"Kaya/Walking"); // index 1

	_entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.01f));
	_entity->GetComponent<Transform>()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));

	auto animator = std::make_unique<ModelAnimator>(shader);
	animator->SetModel(model);
	_entity->AddComponent(std::move(animator));

	auto col = std::make_unique<AABBCollider>();
	col->SetBoxExtents(Vec3(20.f, 88.f, 20.f));
	col->SetOffsetPosition(Vec3(0.f, 88.f, 0.f));
	col->SetShowDebug(false);
	// 캐릭터 채널 — 블록 배치 대상이 아님(pickableMask=0)
	col->SetOwnChannel(CollisionChannel::Character);
	col->SetPickableMask(0);
	_entity->AddComponent(std::move(col));

	// 애니메이션 상태머신
	auto animStateMachine = std::make_unique<AnimStateMachine>();
	animStateMachine->RegisterClip(AnimState::Idle, 0);
	animStateMachine->RegisterClip(AnimState::Walk, 1);
	animStateMachine->SetTweenDuration(0.2f);
	_entity->AddComponent(std::move(animStateMachine));

	// 클릭 이동 컨트롤러
	auto pcc = std::make_unique<PointClickController>();
	pcc->SetMoveSpeed(3.f);
	pcc->SetWalkChannel(CollisionChannel::Character);
	_entity->AddComponent(std::move(pcc));
}

void LightActor::BuildEntity()
{
	assert(_entity != nullptr && "[LightActor] _entity is null before AddComponent");
	assert(_entity->GetComponent<Transform>() != nullptr && "[LightActor] Transform not set");

	auto light = std::make_unique<Light>();

	LightDesc desc;
	desc.ambient  = Color(3.f, 3.f, 3.f, 1.f);
	desc.diffuse  = Color(1.0f, 0.98f, 0.9f, 1.f);
	desc.specular = Color(1.f, 1.f, 1.f, 1.f);
	desc.emissive = Color(0.0f, 0.0f, 0.0f, 1.f);

	Vec3 dir = Vec3(5.f, -2.f, 5.f);
	dir.Normalize();
	desc.direction = dir;
	light->SetLightDesc(desc);

	_entity->AddComponent(std::move(light));

	// 컴포넌트가 정상 등록됐는지 즉시 검증
	auto check = _entity->GetComponent<Light>();
	assert(check != nullptr && "[LightActor] GetComponent<Light>() returned null after AddComponent");
}