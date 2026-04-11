
#pragma once
#include "UI/PaletteWidget.h"

struct InventorySlotData
{
    PaletteWidget::SlotType type  = PaletteWidget::SlotType::Count; // Count = 비어있음
    int32                   count = 0;
};

class InventoryData
{
public:
    static constexpr int32 kHotbarSlots = 8;
    static constexpr int32 kGridCols    = 8;
    static constexpr int32 kGridRows    = 5;
    static constexpr int32 kTotalSlots  = kGridCols * kGridRows;
    static constexpr int32 kTypeCount   = static_cast<int32>(PaletteWidget::SlotType::Count);

    InventoryData();

    void AddItem(PaletteWidget::SlotType type, int32 amount = 1);
    bool ConsumeItem(PaletteWidget::SlotType type, int32 amount = 1);
    void GiveItem(PaletteWidget::SlotType type, int32 amount);
    void GiveAll(int32 amount = 99);

    int32 GetCount(PaletteWidget::SlotType type) const;
    bool  HasItem(PaletteWidget::SlotType type, int32 amount = 1) const;

    InventorySlotData GetGridSlot(int32 index) const;

    InventorySlotData GetHotbarSlot(int32 index) const;

private:
    std::array<int32, kTypeCount> _typeCounts{};
};