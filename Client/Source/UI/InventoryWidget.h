
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

    virtual void Update() override;
    virtual void DrawUI() override;

    bool IsOpen()  const { return _isOpen; }
    void SetOpen(bool open);

private:
    void DrawBackground(float scrW, float scrH);
    void DrawInventoryGrid();
    void DrawHotbarMirror();
    void DrawSingleSlot(float x, float y, float w, float h, const InventorySlotData& slot, bool selected, bool isHotbar);
    void DrawSlotLabel(float x, float y, float w, float h, const InventorySlotData& slot, bool isHotbar);

    void HandleInput();

    static const wchar_t* GetSlotLabel(PaletteWidget::SlotType type);
    static Color          GetSlotColor(PaletteWidget::SlotType type);

    static constexpr float kGridSlotW     = 64.f;
    static constexpr float kGridSlotH     = 64.f;
    static constexpr float kGridSlotGap   = 6.f; 
    static constexpr float kGridPanelPad  = 8.f; 
    static constexpr float kGridPanelW    = kGridPanelPad * 2 + InventoryData::kGridCols * kGridSlotW + (InventoryData::kGridCols - 1) * kGridSlotGap;
    static constexpr float kGridPanelH    = kGridPanelPad * 2 + 28.f + InventoryData::kGridRows * kGridSlotH + (InventoryData::kGridRows - 1) * kGridSlotGap;

    static constexpr float kHotbarSlotW   = 64.f;
    static constexpr float kHotbarSlotH   = 64.f;
    static constexpr float kHotbarSlotGap = 6.f;
    static constexpr float kHotbarPad     = 8.f;
    static constexpr float kHotbarPanelW  = kHotbarPad * 2 + InventoryData::kHotbarSlots * kHotbarSlotW + (InventoryData::kHotbarSlots - 1) * kHotbarSlotGap;
    static constexpr float kHotbarPanelH  = kHotbarSlotH + kHotbarPad * 2;

    static constexpr float kPanelGap      = 10.f;

    bool             _isOpen     = false;
    InventoryData*   _pInventory = nullptr;
    PaletteWidget*   _pPalette   = nullptr;
};