
#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "UI/PaletteWidget.h"

// RHI 디커플링: InventoryData 는 순수 POD, DX11 헤더 없음
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

    // ── 외부 의존성 주입 ────────────────────────────────────────
    void SetCharacterEntity(Entity* ch)         { _character  = ch; }
    void SetInventoryData(InventoryData* inv)   { _pInventory = inv; } // ★ 추가

    // ── 상태 조회 ───────────────────────────────────────────────
    int32 GetTotalBreaks()  const { return _totalBreaks; }
    int32 GetCurrentPhase() const { return _currentPhase; }
    bool  IsBroken()        const { return _isBroken; }

private:
    // ── 페이즈 테이블 ────────────────────────────────────────────
    struct PhaseData
    {
        std::wstring             modelName;   
        PaletteWidget::SlotType  dropSlot;    
        std::wstring             phaseName;   
        int32                    breaksToNext;
    };
    static const std::vector<PhaseData>& GetPhaseTable();

    // ── 채굴 로직 ────────────────────────────────────────────────
    void TryMine();
    void Mine();
    void Respawn();
    void UpdatePhase();
    void ApplyPhaseModel(const std::wstring& modelName);

    // ── 헬퍼: 현재 페이즈의 드랍 SlotType 반환 ──────────────────
    PaletteWidget::SlotType GetCurrentDropSlotType() const;

    bool IsCharacterNearby();

    // ── 멤버 변수 ────────────────────────────────────────────────
    Entity*        _character  = nullptr;
    InventoryData* _pInventory = nullptr; // ★ 추가 (소유하지 않음, observer)

    int32 _totalBreaks  = 0;
    int32 _currentPhase = 0;
    bool  _isBroken     = false;
    float _respawnTimer = 0.f;

    static constexpr float kRespawnDelay = 2.5f;
    static constexpr float kMineRange    = 3.0f;

    // 페이즈별 모델 캐시 (사전 할당, 런타임 new 없음)
    std::vector<std::shared_ptr<Model>> _phaseModels;
};