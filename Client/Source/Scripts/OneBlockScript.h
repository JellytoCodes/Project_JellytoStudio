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

    // ── 외부 의존성 주입 ──────────────────────────────────────────────────────
    void SetCharacterEntity(Entity* ch)       { _character  = ch; }
    void SetInventoryData(InventoryData* inv) { _pInventory = inv; }

    // ── 상태 조회 ─────────────────────────────────────────────────────────────
    int32 GetTotalBreaks()  const { return _totalBreaks; }
    int32 GetCurrentPhase() const { return _currentPhase; }
    bool  IsBroken()        const { return _isBroken; }

private:

    // ── 채굴 로직 ─────────────────────────────────────────────────────────────
    void TryMine();
    void Mine();
    void Respawn();
    void UpdatePhase();
    void ApplyPhaseModel(const std::wstring& modelName);

    // ── 헬퍼: 현재 페이즈의 드랍 SlotType 반환 ──────────────────────────────
    PaletteWidget::SlotType GetCurrentDropSlotType() const;

    bool IsCharacterNearby();

    // ── 멤버 변수 ─────────────────────────────────────────────────────────────
    Entity*        _character  = nullptr;
    InventoryData* _pInventory = nullptr;   // 소유하지 않음 (observer)

    int32 _totalBreaks  = 0;
    int32 _currentPhase = 0;
    bool  _isBroken     = false;
    float _respawnTimer = 0.f;

    static constexpr float kRespawnDelay = 2.5f;
    static constexpr float kMineRange    = 3.0f;

    std::vector<std::shared_ptr<Model>> _phaseModels;
};