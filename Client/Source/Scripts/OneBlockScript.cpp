#include "pch.h"
#include "OneBlockScript.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Components/Collider/CollisionChannel.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Pipeline/Shader.h"
#include "Resource/Managers/ResourceManager.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

const Vec3 OneBlockScript::s_dropOffsets[4] = {
	Vec3(+1.f, 0.f,  0.f),
	Vec3(-1.f, 0.f,  0.f),
	Vec3(0.f, 0.f, +1.f),
	Vec3(0.f, 0.f, -1.f),
};

const std::vector<OneBlockScript::PhaseData>& OneBlockScript::GetPhaseTable()
{
	static const std::vector<PhaseData> s_table = {
		{ L"Priming_01",  L"Priming_01",  L"숲 지대",       5 },
		{ L"Priming_02",  L"Priming_02",  L"동굴",           5 },
		{ L"Priming_03",  L"Priming_03",  L"지하 깊은 곳",   5 },
		{ L"Bridge",      L"Bridge",      L"절벽 지대",      5 },
		{ L"Mushroom_01", L"Mushroom_01", L"버섯 숲",        5 },
		{ L"Mushroom_02", L"Mushroom_02", L"심층부",         5 },
		{ L"Mushroom_03", L"Mushroom_03", L"마지막 차원",    99 },
	};
	return s_table;
}

OneBlockScript::OneBlockScript() {}

void OneBlockScript::Start()
{
	const auto& table = GetPhaseTable();
	_phaseModels.resize(table.size());
	_dropModelPtrs.resize(table.size(), nullptr);

	static std::vector<std::shared_ptr<Model>> s_extraDropModels;

	// ModelRenderer::SetModel(shared_ptr<Model>) 요구 →
	// loadModel 반환형을 shared_ptr<Model>로 통일.
	// (이전 코드는 unique_ptr<Model>을 반환해 _phaseModels[i](shared_ptr)에
	//  암시적 변환 대입은 가능하나, SetModel 호출 시 .get() 로 raw ptr를
	//  전달하면 컴파일 오류 발생)
	auto loadModel = [](const std::wstring& name) -> std::shared_ptr<Model>
		{
			auto m = std::make_shared<Model>();
			m->SetModelPath(L"../Resources/Models/MapModel/");
			m->SetTexturePath(L"../Resources/Textures/MapModel/");
			m->ReadModel(name);
			m->ReadMaterial(name);
			return m;
		};

	for (int32 i = 0; i < (int32)table.size(); i++)
	{
		_phaseModels[i] = loadModel(table[i].modelName);

		if (table[i].dropModel == table[i].modelName)
		{
			_dropModelPtrs[i] = _phaseModels[i].get();
		}
		else
		{
			auto extra = loadModel(table[i].dropModel);
			_dropModelPtrs[i] = extra.get();
			s_extraDropModels.push_back(std::move(extra));
		}
	}
}

void OneBlockScript::Update()
{
	if (_isBroken)
	{
		_respawnTimer -= GET_SINGLE(TimeManager)->GetDeltaTime();
		if (_respawnTimer <= 0.f) Respawn();
		return;
	}
	TryMine();
}

void OneBlockScript::TryMine()
{
	if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::F)) return;
	if (!IsCharacterNearby()) return;
	Mine();
}

void OneBlockScript::Mine()
{
	_isBroken = true;
	_respawnTimer = RESPAWN_DELAY;
	_totalBreaks++;

	Entity* entity = GetEntity();
	if (entity)
		entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.001f));

	UpdatePhase();

	const auto& table = GetPhaseTable();
	const int32 clampedPhase = std::min(_currentPhase, (int32)table.size() - 1);
	SpawnDropBlock(table[clampedPhase].dropModel);
}

void OneBlockScript::Respawn()
{
	_isBroken = false;

	Entity* entity = GetEntity();
	if (!entity) return;

	entity->GetComponent<Transform>()->SetLocalScale(Vec3(1.f));

	const auto& table = GetPhaseTable();
	const int32 clampedPhase = std::min(_currentPhase, (int32)table.size() - 1);
	ApplyPhaseModel(table[clampedPhase].modelName);
}

void OneBlockScript::UpdatePhase()
{
	const auto& table = GetPhaseTable();
	int32 accumulated = 0;
	for (int32 i = 0; i < (int32)table.size(); i++)
	{
		accumulated += table[i].breaksToNext;
		if (_totalBreaks <= accumulated) { _currentPhase = i; return; }
	}
	_currentPhase = (int32)table.size() - 1;
}

void OneBlockScript::ApplyPhaseModel(const std::wstring& modelName)
{
	Entity* entity = GetEntity();
	if (!entity) return;

	ModelRenderer* mr = entity->GetComponent<ModelRenderer>();
	if (!mr) return;

	const auto& table = GetPhaseTable();
	for (int32 i = 0; i < (int32)table.size(); i++)
	{
		if (table[i].modelName == modelName && i < (int32)_phaseModels.size())
		{
			// ModelRenderer::SetModel(shared_ptr<Model>) 요구 →
			// 이전 코드의 .get() 제거: shared_ptr 직접 전달
			mr->SetModel(_phaseModels[i]);
			return;
		}
	}
}

void OneBlockScript::SpawnDropBlock(const std::wstring& modelName)
{
	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (!scene) return;

	Entity* myEntity = GetEntity();
	const Vec3 origin = myEntity
		? myEntity->GetComponent<Transform>()->GetLocalPosition()
		: Vec3::Zero;

	const int32 dir = (_totalBreaks - 1) % 4;
	const Vec3  dropPos = origin + s_dropOffsets[dir];

	// ── 드랍 모델 shared_ptr 획득 ────────────────────────────────────────
	// _dropModelPtrs 는 raw ptr — ModelRenderer::SetModel은 shared_ptr 요구.
	// _phaseModels 에서 직접 shared_ptr 탐색.
	std::shared_ptr<Model> dropModel;
	const auto& table = GetPhaseTable();
	for (int32 i = 0; i < (int32)table.size(); i++)
	{
		if (table[i].dropModel == modelName && i < (int32)_phaseModels.size())
		{
			dropModel = _phaseModels[i];
			break;
		}
	}
	if (!dropModel) return;

	static std::shared_ptr<Shader> s_dropShader;
	if (!s_dropShader)
		s_dropShader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");

	auto dropEntity = std::make_unique<Entity>(L"DropBlock");
	dropEntity->AddComponent(std::make_unique<Transform>());
	dropEntity->GetComponent<Transform>()->SetLocalPosition(dropPos);
	dropEntity->GetComponent<Transform>()->SetLocalScale(Vec3(1.f));

	auto mr = std::make_unique<ModelRenderer>(s_dropShader, false);
	mr->SetModel(dropModel);   // shared_ptr<Model> ✓
	mr->SetModelScale(Vec3(0.01f));
	dropEntity->AddComponent(std::move(mr));

	auto col = std::make_unique<AABBCollider>();
	col->SetBoxExtents(Vec3(0.5f, 0.5f, 0.5f));
	col->SetOffsetPosition(Vec3(0.f, 0.5f, 0.f));
	col->SetOwnChannel(CollisionChannel::Priming);
	col->SetPickableMask(
		static_cast<uint8>(CollisionChannel::Priming) |
		static_cast<uint8>(CollisionChannel::Character));
	dropEntity->AddComponent(std::move(col));

	scene->Add(std::move(dropEntity));
}

bool OneBlockScript::IsCharacterNearby()
{
	if (!_character) return false;

	Entity* myEntity = GetEntity();
	if (!myEntity) return false;

	const Vec3 myPos = myEntity->GetComponent<Transform>()->GetPosition();
	const Vec3 charPos = _character->GetComponent<Transform>()->GetPosition();

	Vec3 diff = charPos - myPos;
	diff.y = 0.f;
	return diff.Length() <= MINE_RANGE;
}