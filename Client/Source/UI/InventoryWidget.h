
#pragma once
#include "UI/Widget.h"
#include "InventoryData.h"

class PaletteWidget;

class InventoryWidget : public Widget
{
    using Super = Widget;

public:
    explicit InventoryWidget(const std::wstring& name);
    virtual ~InventoryWidget() = default;

    void SetInventoryData(InventoryData* data)   { _pInventory = data; }
    void SetPalette(PaletteWidget* palette)      { _pPalette   = palette; }

    // ── IExecute 구현 ────────────────────────────────────────────
    virtual void Update() override;
    virtual void DrawUI() override;

    // ── 상태 ──────────────────────────────────────────────────────
    bool IsOpen()  const { return _isOpen; }
    void SetOpen(bool open);

private:
    // ── DrawUI 서브루틴 ───────────────────────────────────────────
    void DrawBackground();
    void DrawInventoryGrid();
    void DrawHotbarMirror();
    void DrawSingleSlot(float x, float y, float w, float h, const InventorySlotData& slot, bool selected, bool isHotbar);
    void DrawSlotLabel(float x, float y, float w, float h, const InventorySlotData& slot, bool isHotbar);

    // ── 입력 처리 ─────────────────────────────────────────────────
    void HandleInput();

    // ── 헬퍼: SlotType → 표시 이름 & 색상 ────────────────────────
    static const wchar_t* GetSlotLabel(PaletteWidget::SlotType type);
    static Color          GetSlotColor(PaletteWidget::SlotType type);

    //  [인벤토리 그리드]
    static constexpr float kGridSlotW     = 64.f;
    static constexpr float kGridSlotH     = 64.f;
    static constexpr float kGridSlotGap   = 6.f; 
    static constexpr float kGridPanelPad  = 8.f; 
    static constexpr float kGridPanelW    = kGridPanelPad * 2 + InventoryData::kGridCols * kGridSlotW + (InventoryData::kGridCols - 1) * kGridSlotGap;
    static constexpr float kGridPanelH    = kGridPanelPad * 2 + 28.f + InventoryData::kGridRows * kGridSlotH + (InventoryData::kGridRows - 1) * kGridSlotGap;

    //  [핫바 미러]
    static constexpr float kHotbarSlotW   = 64.f;
    static constexpr float kHotbarSlotH   = 64.f;
    static constexpr float kHotbarSlotGap = 6.f;
    static constexpr float kHotbarPad     = 8.f;
    static constexpr float kHotbarPanelW  = kHotbarPad * 2 + InventoryData::kHotbarSlots * kHotbarSlotW + (InventoryData::kHotbarSlots - 1) * kHotbarSlotGap;
    static constexpr float kHotbarPanelH  = kHotbarSlotH + kHotbarPad * 2;

    //  [패널 간격]
    static constexpr float kPanelGap      = 10.f;   // 그리드↔핫바 패널 사이 간격

    // ── 런타임 상태 ───────────────────────────────────────────────
    bool             _isOpen     = false;
    InventoryData*   _pInventory = nullptr;
    PaletteWidget*   _pPalette   = nullptr;
};