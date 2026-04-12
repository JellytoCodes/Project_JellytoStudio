#include "pch.h"
#include "OneBlockScript.h"

#include "UI/InventoryData.h"
#include "UI/PaletteWidget.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Resource/Managers/ResourceManager.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"

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
    if (_isBroken)
    {
        _respawnTimer -= GET_SINGLE(TimeManager)->GetDeltaTime();
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
    _isBroken     = true;
    _respawnTimer = kRespawnDelay;
    _totalBreaks++;

    if (Entity* entity = GetEntity())
        entity->GetComponent<Transform>()->SetLocalScale(Vec3(0.001f));

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

    entity->GetComponent<Transform>()->SetLocalScale(Vec3(1.f));

    const auto& phaseTable   = GET_SINGLE(BlockDataTable)->GetPhaseSequence();
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
    const auto& phaseTable   = GET_SINGLE(BlockDataTable)->GetPhaseSequence();
    const int32 clampedPhase = std::min(_currentPhase,
                                        static_cast<int32>(phaseTable.size()) - 1);
    return phaseTable[clampedPhase].dropSlot;
}

// ── IsCharacterNearby ─────────────────────────────────────────────────────────
bool OneBlockScript::IsCharacterNearby()
{
    if (!_character) return false;

    Entity* myEntity = GetEntity();
    if (!myEntity) return false;

    const Vec3 myPos   = myEntity->GetComponent<Transform>()->GetPosition();
    const Vec3 charPos = _character->GetComponent<Transform>()->GetPosition();

    Vec3 diff = charPos - myPos;
    diff.y = 0.f;
    return diff.Length() <= kMineRange;
}