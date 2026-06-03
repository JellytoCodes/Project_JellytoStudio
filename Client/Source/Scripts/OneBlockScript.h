#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "UI/PaletteWidget.h"

class InventoryData;
class Model;

class OneBlockScript : public MonoBehaviour
{
public:
    OneBlockScript();
    virtual ~OneBlockScript() = default;

    virtual void Awake()      override {}
    virtual void Start()      override;
    virtual void Update()     override;
    virtual void LateUpdate() override {}
    virtual void OnDestroy()  override {}

    void SetCharacterEntity(Entity* ch) { _character = ch; }
    void SetInventoryData(InventoryData* inv) { _pInventory = inv; }

    int32 GetTotalBreaks()  const { return _totalBreaks; }
    int32 GetCurrentPhase() const { return _currentPhase; }
    bool  IsBroken()        const { return _isBroken; }

private:
    void TryMine();
    void Mine();
    void Respawn();
    void UpdatePhase();
    void ApplyPhaseModel(const std::wstring& modelName);

    PaletteWidget::SlotType GetCurrentDropSlotType() const;

    bool IsCharacterNearby();

    Entity* _character = nullptr;
    InventoryData* _pInventory = nullptr;

    int32                                   _totalBreaks = 0;
    int32                                   _currentPhase = 0;
    bool                                    _isBroken = false;
    float                                   _respawnTimer = 0.f;

    static constexpr float                  kRespawnDelay = 2.5f;
    static constexpr float                  kMineRange = 3.0f;
    static constexpr float                  kBreakDuration = 0.12f;
    static constexpr float                  kRespawnDuration = 0.22f;

    enum class TweenState { None, Breaking, Respawning };
    void        TickTween(float dt);
    TweenState  _tweenState = TweenState::None;
    float       _tweenElapsed = 0.f;

    std::vector<std::shared_ptr<Model>>     _phaseModels;
};
