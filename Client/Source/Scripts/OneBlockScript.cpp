
#include "pch.h"
#include "OneBlockScript.h"

#include "UI/InventoryData.h"
#include "UI/PaletteWidget.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Resource/Managers/ResourceManager.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Graphics/Managers/InstancingManager.h"

#include "Data/BlockDataTable.h"

using SlotType = PaletteWidget::SlotType;

OneBlockScript::OneBlockScript()
{

}

// ── Start ─────────────────────────────────────────────────────────────────────
void OneBlockScript::Start()
{
    assert(GET_SINGLE(BlockDataTable)->IsLoaded() &&
        "OneBlockScript::Start() 전에 BlockDataTable::Load() 가 선행되어야 합니다.");

    const auto& phaseTable = GET_SINGLE(BlockDataTable)->GetPhaseSequence();
    _phaseModels.resize(phaseTable.size());

    auto loadModel = [](const std::wstring& name) -> std::shared_ptr<Model>
        {
            auto m = std::make_shared<Model>();
            m->SetModelPath(L"../Resources/Models/MapModel/");
            m->SetTexturePath(L"../Resources/Textures/MapModel/");
            m->ReadModel(name);
            m->ReadMaterial(name);
            return m;
        };

    for (int32 i = 0; i < static_cast<int32>(phaseTable.size()); ++i)
        _phaseModels[i] = loadModel(phaseTable[i].modelName);
}

// ── Update ────────────────────────────────────────────────────────────────────
void OneBlockScript::Update()
{
    const float dt = GET_SINGLE(TimeManager)->GetDeltaTime();

    TickTween(dt);

    if (_isBroken)
    {
        _respawnTimer -= dt;
        if (_respawnTimer <= 0.f) Respawn();
        return;
    }
    TryMine();
}

// ── TryMine ───────────────────────────────────────────────────────────────────
void OneBlockScript::TryMine()
{
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::F)) return;
    if (!IsCharacterNearby()) return;
    Mine();
}

// ── Mine ──────────────────────────────────────────────────────────────────────
void OneBlockScript::Mine()
{
    _isBroken = true;
    _respawnTimer = kRespawnDelay;
    _totalBreaks++;

    _tweenState = TweenState::Breaking;
    _tweenElapsed = 0.f;

    UpdatePhase();

    if (_pInventory)
    {
        const SlotType dropSlot = GetCurrentDropSlotType();
        _pInventory->AddItem(dropSlot, 1);
    }
}

// ── Respawn ───────────────────────────────────────────────────────────────────
void OneBlockScript::Respawn()
{
    _isBroken = false;

    Entity* entity = GetEntity();
    if (!entity) return;

    entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.001f));
    _tweenState = TweenState::Respawning;
    _tweenElapsed = 0.f;

    const auto& phaseTable = GET_SINGLE(BlockDataTable)->GetPhaseSequence();
    const int32 clampedPhase = std::min(_currentPhase,
        static_cast<int32>(phaseTable.size()) - 1);
    ApplyPhaseModel(phaseTable[clampedPhase].modelName);
}

// ── UpdatePhase ───────────────────────────────────────────────────────────────
void OneBlockScript::UpdatePhase()
{
    const auto& phaseTable = GET_SINGLE(BlockDataTable)->GetPhaseSequence();

    int32 accumulated = 0;
    for (int32 i = 0; i < static_cast<int32>(phaseTable.size()); ++i)
    {
        accumulated += phaseTable[i].breaksToNext;
        if (_totalBreaks <= accumulated)
        {
            _currentPhase = i;
            return;
        }
    }
    _currentPhase = static_cast<int32>(phaseTable.size()) - 1;
}

// ── ApplyPhaseModel ───────────────────────────────────────────────────────────
void OneBlockScript::ApplyPhaseModel(const std::wstring& modelName)
{
    Entity* entity = GetEntity();
    if (!entity) return;

    ModelRenderer* mr = entity->GetComponent<ModelRenderer>();
    if (!mr) return;

    const auto& phaseTable = GET_SINGLE(BlockDataTable)->GetPhaseSequence();
    for (int32 i = 0; i < static_cast<int32>(phaseTable.size()); ++i)
    {
        if (phaseTable[i].modelName == modelName &&
            i < static_cast<int32>(_phaseModels.size()))
        {
            mr->SetModel(_phaseModels[i]);
            return;
        }
    }
}

// ── GetCurrentDropSlotType ────────────────────────────────────────────────────
PaletteWidget::SlotType OneBlockScript::GetCurrentDropSlotType() const
{
    const auto& phaseTable = GET_SINGLE(BlockDataTable)->GetPhaseSequence();
    const int32 clampedPhase = std::min(_currentPhase,
        static_cast<int32>(phaseTable.size()) - 1);
    return phaseTable[clampedPhase].dropSlot;
}

// ── TickTween ─────────────────────────────────────────────────────────────────
void OneBlockScript::TickTween(float dt)
{
    if (_tweenState == TweenState::None) return;

    Entity* entity = GetEntity();
    if (!entity) { _tweenState = TweenState::None; return; }

    Transform* tf = entity->GetComponent<Transform>();
    if (!tf) { _tweenState = TweenState::None; return; }

    AABBCollider* col = entity->GetComponent<AABBCollider>();

    _tweenElapsed += dt;

    if (_tweenState == TweenState::Breaking)
    {
        const float t = std::min(_tweenElapsed / kBreakDuration, 1.f);
        const float ease = t * t;
        const float scaleY = 1.f - ease;
        const float scaleXZ = 1.f + ease * 0.6f;

        tf->SetLocalScale(Vec3(scaleXZ, max(scaleY, 0.001f), scaleXZ));
        if (col) col->InvalidateBounds();
        // ★ ModelRenderer world matrix 캐시 매 프레임 재빌드
        GET_SINGLE(InstancingManager)->SetDirty();

        if (t >= 1.f)
        {
            tf->SetLocalScale(Vec3(0.001f));
            if (col) col->InvalidateBounds();
            GET_SINGLE(InstancingManager)->SetDirty();
            _tweenState = TweenState::None;
        }
    }
    else if (_tweenState == TweenState::Respawning)
    {
        const float t = std::min(_tweenElapsed / kRespawnDuration, 1.f);

        float s;
        if (t < 0.5f)  s = t / 0.5f * 1.15f;
        else if (t < 0.75f) s = 1.15f - (t - 0.5f) / 0.25f * 0.25f;
        else                s = 0.9f + (t - 0.75f) / 0.25f * 0.1f;

        tf->SetLocalScale(Vec3(s));
        if (col) col->InvalidateBounds();
        GET_SINGLE(InstancingManager)->SetDirty();

        if (t >= 1.f)
        {
            tf->SetLocalScale(Vec3(1.f));
            if (col) col->InvalidateBounds();
            GET_SINGLE(InstancingManager)->SetDirty();
            _tweenState = TweenState::None;
        }
    }
}

// ── IsCharacterNearby ─────────────────────────────────────────────────────────
bool OneBlockScript::IsCharacterNearby()
{
    if (!_character) return false;

    Entity* myEntity = GetEntity();
    if (!myEntity) return false;

    const Vec3 myPos = myEntity->GetComponent<Transform>()->GetPosition();
    const Vec3 charPos = _character->GetComponent<Transform>()->GetPosition();

    Vec3 diff = charPos - myPos;
    diff.y = 0.f;
    return diff.Length() <= kMineRange;
}