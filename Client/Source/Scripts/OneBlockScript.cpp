
#include "pch.h"
#include "OneBlockScript.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Components/Collider/CollisionChannel.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Pipeline/Shader.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

// 4방향 드랍 위치 (원점 블록 기준 XZ 인접)
const Vec3 OneBlockScript::s_dropOffsets[4] = {
    Vec3(+1.f, 0.f,  0.f),
    Vec3(-1.f, 0.f,  0.f),
    Vec3( 0.f, 0.f, +1.f),
    Vec3( 0.f, 0.f, -1.f),
};

// ── 단계 테이블 ───────────────────────────────────────────────────────────

const std::vector<OneBlockScript::PhaseData>& OneBlockScript::GetPhaseTable()
{
    static const std::vector<PhaseData> s_table = {
        // modelName      dropModel      phaseName     breaksToNext
        { L"Priming_01",  L"Priming_01", L"숲 지대",      5 },
        { L"Priming_02",  L"Priming_02", L"동굴",          5 },
        { L"Priming_03",  L"Priming_03", L"지하 깊은 곳",  5 },
        { L"Bridge",      L"Bridge",     L"절벽 지대",     5 },
        { L"Mushroom_01", L"Mushroom_01",L"버섯 숲",       5 },
        { L"Mushroom_02", L"Mushroom_02",L"심층부",        5 },
        { L"Mushroom_03", L"Mushroom_03",L"마지막 차원",   99 }, // 최종 단계
    };
    return s_table;
}

// ── 생명주기 ─────────────────────────────────────────────────────────────

OneBlockScript::OneBlockScript()
{
}

void OneBlockScript::Start()
{
    const auto& table = GetPhaseTable();
    _phaseModels.resize(table.size());
    _dropModels.resize(table.size());

    for (int32 i = 0; i < (int32)table.size(); i++)
    {
        auto loadModel = [](const std::wstring& name) -> std::shared_ptr<Model>
        {
            auto m = std::make_shared<Model>();
            m->SetModelPath(L"../Resources/Models/MapModel/");
            m->SetTexturePath(L"../Resources/Textures/MapModel/");
            m->ReadModel(name);
            m->ReadMaterial(name);
            return m;
        };

        _phaseModels[i] = loadModel(table[i].modelName);

        if (table[i].dropModel == table[i].modelName)
            _dropModels[i] = _phaseModels[i];
        else
            _dropModels[i] = loadModel(table[i].dropModel);
    }
}

// ── Update ────────────────────────────────────────────────────────────────

void OneBlockScript::Update()
{
    if (_isBroken)
    {
        // 재생성 카운트다운
        _respawnTimer -= GET_SINGLE(TimeManager)->GetDeltaTime();
        if (_respawnTimer <= 0.f)
            Respawn();
        return;
    }

    TryMine();
}

// ── 채굴 입력 ─────────────────────────────────────────────────────────────

void OneBlockScript::TryMine()
{
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::F)) return;
    if (!IsCharacterNearby()) return;
    Mine();
}

// ── 채굴 실행 ─────────────────────────────────────────────────────────────

void OneBlockScript::Mine()
{
    _isBroken     = true;
    _respawnTimer = RESPAWN_DELAY;
    _totalBreaks++;

    // 블록 숨김: Entity 스케일 0에 가깝게 → 렌더/피킹 비활성화
    auto entity = GetEntity();
    if (entity)
        entity->GetTransform()->SetLocalScale(Vec3(0.001f));

    // 단계 갱신
    UpdatePhase();

    // 드랍 블록 스폰
    const auto& table = GetPhaseTable();
    int32 clampedPhase = std::min(_currentPhase, (int32)table.size() - 1);
    SpawnDropBlock(table[clampedPhase].dropModel);

    // 디버그 로그
    wchar_t dbg[256];
    swprintf_s(dbg,
        L"[OneBlock] 채굴 %d회 | Phase %d (%s) | 재생성까지 %.1f초\n",
        _totalBreaks, _currentPhase,
        table[clampedPhase].phaseName.c_str(),
        RESPAWN_DELAY);
    ::OutputDebugStringW(dbg);
}

// ── 재생성 ────────────────────────────────────────────────────────────────

void OneBlockScript::Respawn()
{
    _isBroken = false;

    auto entity = GetEntity();
    if (!entity) return;

    // 스케일 복구
    entity->GetTransform()->SetLocalScale(Vec3(1.f));

    // 현재 단계 모델로 교체
    const auto& table = GetPhaseTable();
    int32 clampedPhase = std::min(_currentPhase, (int32)table.size() - 1);
    ApplyPhaseModel(table[clampedPhase].modelName);

    wchar_t dbg[128];
    swprintf_s(dbg, L"[OneBlock] 재생성 완료 — %s\n",
        table[clampedPhase].phaseName.c_str());
    ::OutputDebugStringW(dbg);
}

// ── 단계 갱신 ─────────────────────────────────────────────────────────────

void OneBlockScript::UpdatePhase()
{
    const auto& table = GetPhaseTable();
    int32 accumulated = 0;
    for (int32 i = 0; i < (int32)table.size(); i++)
    {
        accumulated += table[i].breaksToNext;
        if (_totalBreaks <= accumulated)
        {
            _currentPhase = i;
            return;
        }
    }
    // 최종 단계 유지
    _currentPhase = (int32)table.size() - 1;
}

// ── 모델 교체 ─────────────────────────────────────────────────────────────

void OneBlockScript::ApplyPhaseModel(const std::wstring& modelName)
{
    auto entity = GetEntity();
    if (!entity) return;

    auto mr = entity->GetComponent<ModelRenderer>();
    if (!mr) return;

    // 캐시에서 모델 찾기
    const auto& table = GetPhaseTable();
    for (int32 i = 0; i < (int32)table.size(); i++)
    {
        if (table[i].modelName == modelName && i < (int32)_phaseModels.size())
        {
            mr->SetModel(_phaseModels[i]);
            return;
        }
    }
}

// ── 드랍 블록 생성 ────────────────────────────────────────────────────────

void OneBlockScript::SpawnDropBlock(const std::wstring& modelName)
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return;

    auto myEntity = GetEntity();
    Vec3 origin = myEntity ? myEntity->GetTransform()->GetLocalPosition() : Vec3::Zero;

    // 4방향 순환 배치
    int32 dir = (_totalBreaks - 1) % 4;
    Vec3 dropPos = origin + s_dropOffsets[dir];

    // 드랍 모델 선택
    std::shared_ptr<Model> dropModel;
    const auto& table = GetPhaseTable();
    for (int32 i = 0; i < (int32)table.size(); i++)
    {
        if (table[i].dropModel == modelName && i < (int32)_dropModels.size())
        {
            dropModel = _dropModels[i];
            break;
        }
    }
    if (!dropModel) return;

    // Entity 생성
    auto dropEntity = std::make_shared<Entity>(L"DropBlock");
    dropEntity->AddComponent(std::make_shared<Transform>());
    dropEntity->GetTransform()->SetLocalPosition(dropPos);
    dropEntity->GetTransform()->SetLocalScale(Vec3(1.f));

    // ModelRenderer (Static Mesh)
    auto shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
    auto mr = std::make_shared<ModelRenderer>(shader, false);
    mr->SetModel(dropModel);
    mr->SetModelScale(Vec3(0.01f));
    dropEntity->AddComponent(mr);

    // AABBCollider — Priming 채널, 캐릭터 이동 가능
    auto col = std::make_shared<AABBCollider>();
    col->SetBoxExtents(Vec3(0.5f, 0.5f, 0.5f));
    col->SetOffsetPosition(Vec3(0.f, 0.5f, 0.f)); // 하단 기준 → 중심 보정
    col->SetOwnChannel(CollisionChannel::Priming);
    col->SetPickableMask(
        static_cast<uint8>(CollisionChannel::Priming) |
        static_cast<uint8>(CollisionChannel::Character));
    dropEntity->AddComponent(col);

    scene->Add(dropEntity);
}

// ── 근접 판정 ─────────────────────────────────────────────────────────────

bool OneBlockScript::IsCharacterNearby()
{
    auto charEntity = _character.lock();
    if (!charEntity) return false;

    auto myEntity = GetEntity();
    if (!myEntity) return false;
    
    Vec3 myPos   = myEntity->GetTransform()->GetPosition();
    Vec3 charPos = charEntity->GetTransform()->GetPosition();
    
    Vec3 diff = charPos - myPos;
    diff.y    = 0.f; // Y축 무시 (높이 차이는 무관)
    return diff.Length() <= MINE_RANGE;

    return false;
}