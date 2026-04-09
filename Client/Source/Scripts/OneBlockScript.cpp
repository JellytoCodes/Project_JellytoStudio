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
	Vec3( 0.f, 0.f, +1.f),
	Vec3( 0.f, 0.f, -1.f),
};

const std::vector<OneBlockScript::PhaseData>& OneBlockScript::GetPhaseTable()
{
	static const std::vector<PhaseData> s_table = {
		{ L"Priming_01",  L"Priming_01", L"숲 지대",      5 },
		{ L"Priming_02",  L"Priming_02", L"동굴",          5 },
		{ L"Priming_03",  L"Priming_03", L"지하 깊은 곳",  5 },
		{ L"Bridge",      L"Bridge",     L"절벽 지대",     5 },
		{ L"Mushroom_01", L"Mushroom_01",L"버섯 숲",       5 },
		{ L"Mushroom_02", L"Mushroom_02",L"심층부",        5 },
		{ L"Mushroom_03", L"Mushroom_03",L"마지막 차원",   99 },
	};
	return s_table;
}

OneBlockScript::OneBlockScript() {}

void OneBlockScript::Start()
{
	const auto& table = GetPhaseTable();
	_phaseModels.resize(table.size());
	_dropModelPtrs.resize(table.size(), nullptr);

	// 별도 dropModel unique_ptr 보관용 (modelName != dropModel인 경우)
	static std::vector<std::unique_ptr<Model>> s_extraDropModels;

	auto loadModel = [](const std::wstring& name) -> std::unique_ptr<Model>
	{
		auto m = std::make_unique<Model>();
		m->SetModelPath(L"../Resources/Models/MapModel/");
		m->SetTexturePath(L"../Resources/Textures/MapModel/");
		m->ReadModel(name);
		m->ReadMaterial(name);
		return m;
	};

	for (int32 i = 0; i < (int32)table.size(); i++)
	{
		// 이전: shared_ptr<Model> — 제어블록 별도 heap 할당 ×N
		// 변경: unique_ptr<Model> — 단순 소유
		_phaseModels[i] = loadModel(table[i].modelName);

		if (table[i].dropModel == table[i].modelName)
		{
			// 같은 모델 → raw ptr로 참조
			_dropModelPtrs[i] = _phaseModels[i].get();
		}
		else
		{
			// 다른 모델 → 별도 unique_ptr 보관
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
	_isBroken     = true;
	_respawnTimer = RESPAWN_DELAY;
	_totalBreaks++;

	Entity* entity = GetEntity();
	if (entity)
		entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.001f));

	UpdatePhase();

	const auto& table = GetPhaseTable();
	int32 clampedPhase = std::min(_currentPhase, (int32)table.size() - 1);
	SpawnDropBlock(table[clampedPhase].dropModel);
}

void OneBlockScript::Respawn()
{
	_isBroken = false;

	Entity* entity = GetEntity();
	if (!entity) return;

	entity->GetComponent<Transform>()->SetLocalScale(Vec3(1.f));

	const auto& table = GetPhaseTable();
	int32 clampedPhase = std::min(_currentPhase, (int32)table.size() - 1);
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
			mr->SetModel(_phaseModels[i].get());
			return;
		}
	}
}

void OneBlockScript::SpawnDropBlock(const std::wstring& modelName)
{
	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (!scene) return;

	Entity* myEntity = GetEntity();
	Vec3 origin = myEntity ? myEntity->GetComponent<Transform>()->GetLocalPosition() : Vec3::Zero;

	int32 dir    = (_totalBreaks - 1) % 4;
	Vec3  dropPos = origin + s_dropOffsets[dir];

	// 드랍 모델 선택 (raw ptr 캐시)
	Model* dropModel = nullptr;
	const auto& table = GetPhaseTable();
	for (int32 i = 0; i < (int32)table.size(); i++)
	{
		if (table[i].dropModel == modelName && i < (int32)_dropModelPtrs.size())
		{
			dropModel = _dropModelPtrs[i];
			break;
		}
	}
	if (!dropModel) return;

	// Shader: ResourceManager에 등록하여 수명 관리
	static Shader* s_dropShader = nullptr;
	if (!s_dropShader)
	{
		auto shaderPtr = std::make_unique<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
		s_dropShader = shaderPtr.get();
		GET_SINGLE(ResourceManager)->Add(L"DropBlockShader", std::move(shaderPtr));
	}

	// Entity 생성 — 모두 make_unique
	auto dropEntity = std::make_unique<Entity>(L"DropBlock");
	dropEntity->AddComponent(std::make_unique<Transform>());
	dropEntity->GetComponent<Transform>()->SetLocalPosition(dropPos);
	dropEntity->GetComponent<Transform>()->SetLocalScale(Vec3(1.f));

	auto mr = std::make_unique<ModelRenderer>(s_dropShader, false);
	mr->SetModel(dropModel);
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
	// 이전: auto charEntity = _character.lock() → shared_ptr 임시 생성
	// 변경: Entity* 직접 사용 → atomic 0
	if (!_character) return false;

	Entity* myEntity = GetEntity();
	if (!myEntity) return false;

	Vec3 myPos   = myEntity->GetComponent<Transform>()->GetPosition();
	Vec3 charPos = _character->GetComponent<Transform>()->GetPosition();

	Vec3 diff = charPos - myPos;
	diff.y    = 0.f;
	return diff.Length() <= MINE_RANGE;
}
