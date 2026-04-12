
#include "pch.h"
#include "InventoryData.h"

using SlotType = PaletteWidget::SlotType;

// ── 생성자 ────────────────────────────────────────────────────
InventoryData::InventoryData()
{
    _typeCounts.fill(0);
}

// ── 추가 ─────────────────────────────────────────────────────
void InventoryData::AddItem(SlotType type, int32 amount)
{
    const int32 idx = static_cast<int32>(type);
    if (idx < 0 || idx >= kTypeCount) return;
    if (type == SlotType::Eraser)     return;

    _typeCounts[idx] = max(0, _typeCounts[idx] + amount);
}

// ── 소비 ─────────────────────────────────────────────────────
bool InventoryData::ConsumeItem(SlotType type, int32 amount)
{
    const int32 idx = static_cast<int32>(type);
    if (idx < 0 || idx >= kTypeCount)  return false;
    if (type == SlotType::Eraser)      return true;
    if (_typeCounts[idx] < amount)     return false;

    _typeCounts[idx] -= amount;
    return true;
}

// ── 지급 ─────────────────────────────────────────────────────
void InventoryData::GiveItem(SlotType type, int32 amount)
{
    AddItem(type, amount);
}

void InventoryData::GiveAll(int32 amount)
{
    for (int32 i = 0; i < kTypeCount; ++i)
    {
        const auto t = static_cast<SlotType>(i);
        if (t != SlotType::Eraser)
            _typeCounts[i] = amount;
    }
}

// ── 조회 ─────────────────────────────────────────────────────
int32 InventoryData::GetCount(SlotType type) const
{
    const int32 idx = static_cast<int32>(type);
    if (idx < 0 || idx >= kTypeCount) return 0;
    if (type == SlotType::Eraser)     return 999;
    return _typeCounts[idx];
}

bool InventoryData::HasItem(SlotType type, int32 amount) const
{
    if (type == SlotType::Eraser) return true;
    return GetCount(type) >= amount;
}

InventorySlotData InventoryData::GetGridSlot(int32 index) const
{
    if (index < 0 || index >= kTotalSlots)
        return {};

    if (index < kTypeCount)
    {
        const auto t = static_cast<SlotType>(index);
        return { t, GetCount(t) };
    }

    return { SlotType::Count, 0 };
}

InventorySlotData InventoryData::GetHotbarSlot(int32 index) const
{
    if (index < 0 || index >= kHotbarSlots)
        return {};

    const auto t = static_cast<SlotType>(index);
    return { t, GetCount(t) };
}