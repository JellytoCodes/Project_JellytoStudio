
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

using SlotType = PaletteWidget::SlotType;

const std::vector<OneBlockScript::PhaseData>& OneBlockScript::GetPhaseTable()
{
    static const std::vector<PhaseData> s_table =
    {
        { L"Priming_01",  SlotType::Priming1,  L"숲 지대",        5 },
        { L"Priming_02",  SlotType::Priming2,  L"동굴",           5 },
        { L"Priming_03",  SlotType::Priming3,  L"지하 깊은 곳",   5 },
        { L"Bridge",      SlotType::Bridge,    L"절벽 지대",      5 },
        { L"Mushroom_01", SlotType::Mushroom1, L"버섯 숲",        5 },
        { L"Mushroom_02", SlotType::Mushroom2, L"심층부",         5 },
        { L"Mushroom_03", SlotType::Mushroom3, L"마지막 차원",   99 },
    };
    return s_table;
}

OneBlockScript::OneBlockScript() {}

void OneBlockScript::Start()
{
    const auto& table = GetPhaseTable();
    _phaseModels.resize(table.size());

    auto loadModel = [](const std::wstring& name) -> std::shared_ptr<Model>
    {
        auto m = std::make_shared<Model>();
        m->SetModelPath(L"../Resources/Models/MapModel/");
        m->SetTexturePath(L"../Resources/Textures/MapModel/");
        m->ReadModel(name);
        m->ReadMaterial(name);
        return m;
    };

    for (int32 i = 0; i < static_cast<int32>(table.size()); ++i)
        _phaseModels[i] = loadModel(table[i].modelName);
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

void OneBlockScript::Respawn()
{
    _isBroken = false;

    Entity* entity = GetEntity();
    if (!entity) return;

    entity->GetComponent<Transform>()->SetLocalScale(Vec3(1.f));

    const auto& table = GetPhaseTable();
    const int32 clampedPhase = std::min(_currentPhase, static_cast<int32>(table.size()) - 1);
    ApplyPhaseModel(table[clampedPhase].modelName);
}

void OneBlockScript::UpdatePhase()
{
    const auto& table = GetPhaseTable();
    int32 accumulated = 0;
    for (int32 i = 0; i < static_cast<int32>(table.size()); ++i)
    {
        accumulated += table[i].breaksToNext;
        if (_totalBreaks <= accumulated) { _currentPhase = i; return; }
    }
    _currentPhase = static_cast<int32>(table.size()) - 1;
}

void OneBlockScript::ApplyPhaseModel(const std::wstring& modelName)
{
    Entity* entity = GetEntity();
    if (!entity) return;

    ModelRenderer* mr = entity->GetComponent<ModelRenderer>();
    if (!mr) return;

    const auto& table = GetPhaseTable();
    for (int32 i = 0; i < static_cast<int32>(table.size()); ++i)
    {
        if (table[i].modelName == modelName && i < static_cast<int32>(_phaseModels.size()))
        {
            mr->SetModel(_phaseModels[i]);
            return;
        }
    }
}

PaletteWidget::SlotType OneBlockScript::GetCurrentDropSlotType() const
{
    const auto& table = GetPhaseTable();
    const int32 clampedPhase = std::min(_currentPhase, static_cast<int32>(table.size()) - 1);
    return table[clampedPhase].dropSlot;
}

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