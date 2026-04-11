
#include "pch.h"
#include "InventoryWidget.h"

#include "UI/PaletteWidget.h"
#include "UI/UIManager.h"
#include "Core/Managers/InputManager.h"

using SlotType = PaletteWidget::SlotType;

const wchar_t* InventoryWidget::GetSlotLabel(SlotType type)
{
    switch (type)
    {
    case SlotType::Mushroom1: return L"버섯1";
    case SlotType::Mushroom2: return L"버섯2";
    case SlotType::Mushroom3: return L"버섯3";
    case SlotType::Priming1:  return L"블록1";
    case SlotType::Priming2:  return L"블록2";
    case SlotType::Priming3:  return L"블록3";
    case SlotType::Bridge:    return L"브릿지";
    case SlotType::Eraser:    return L"지우개";
    default:                  return L"";
    }
}

Color InventoryWidget::GetSlotColor(SlotType type)
{
    switch (type)
    {
    case SlotType::Mushroom1: return Color(0.20f, 0.30f, 0.22f, 0.90f);
    case SlotType::Mushroom2: return Color(0.22f, 0.32f, 0.20f, 0.90f);
    case SlotType::Mushroom3: return Color(0.18f, 0.28f, 0.24f, 0.90f);
    case SlotType::Priming1:  return Color(0.28f, 0.22f, 0.18f, 0.90f);
    case SlotType::Priming2:  return Color(0.30f, 0.24f, 0.18f, 0.90f);
    case SlotType::Priming3:  return Color(0.26f, 0.20f, 0.20f, 0.90f);
    case SlotType::Bridge:    return Color(0.22f, 0.22f, 0.30f, 0.90f);
    case SlotType::Eraser:    return Color(0.40f, 0.15f, 0.15f, 0.90f);
    default:                  return Color(0.12f, 0.12f, 0.14f, 0.85f);
    }
}

InventoryWidget::InventoryWidget(const std::wstring& name)
    : Super(name)
{
    const float totalH   = kGridPanelH + kPanelGap + kHotbarPanelH;
    const float startX   = (static_cast<float>(MAIN_WINDOW_WIDTH)  - kGridPanelW)  * 0.5f;
    const float startY   = (static_cast<float>(MAIN_WINDOW_HEIGHT) - totalH)       * 0.5f;
    SetScreenPos(startX, startY);
}

void InventoryWidget::SetOpen(bool open)
{
    _isOpen = open;
}

void InventoryWidget::Update()
{
    HandleInput();
}

void InventoryWidget::HandleInput()
{
    auto* input = GET_SINGLE(InputManager);

    if (input->GetButtonDown(KEY_TYPE::I))
        SetOpen(!_isOpen);
}

void InventoryWidget::DrawUI()
{
    if (!_isOpen) return;
    if (!_pInventory) return;

    DrawBackground();
    DrawInventoryGrid();
    DrawHotbarMirror();
}

void InventoryWidget::DrawBackground()
{
    auto* ui = GET_SINGLE(UIManager);
    const float sx = GetScreenX();
    const float sy = GetScreenY();
    const float totalH = kGridPanelH + kPanelGap + kHotbarPanelH;

    ui->AddRect(0.f, 0.f, static_cast<float>(MAIN_WINDOW_WIDTH), static_cast<float>(MAIN_WINDOW_HEIGHT), Color(0.f, 0.f, 0.f, 0.55f));
    ui->AddRect(sx, sy, kGridPanelW, kGridPanelH, Color(0.08f, 0.09f, 0.11f, 0.96f));
    ui->AddRect(sx, sy + kGridPanelH + kPanelGap, kHotbarPanelW, kHotbarPanelH, Color(0.06f, 0.07f, 0.09f, 0.96f));

	ui->AddRectBorder(sx, sy, kGridPanelW, kGridPanelH, Color(0.35f, 0.35f, 0.45f, 0.80f), 2.f);
    ui->AddRectBorder(sx, sy + kGridPanelH + kPanelGap, kHotbarPanelW, kHotbarPanelH, Color(0.35f, 0.35f, 0.45f, 0.80f), 2.f);

    ui->AddText(L"[ 인벤토리 ]", sx + kGridPanelPad, sy + kGridPanelPad * 0.5f, kGridPanelW - kGridPanelPad * 2, 24.f, Color(0.85f, 0.85f, 1.f, 1.f), 16);
    ui->AddText(L"I : 닫기", sx + kGridPanelW - 90.f, sy + kGridPanelPad * 0.5f, 80.f, 20.f, Color(0.5f, 0.5f, 0.6f, 0.9f), 13);
}

void InventoryWidget::DrawInventoryGrid()
{
    const float sx      = GetScreenX() + kGridPanelPad;
    const float sy      = GetScreenY() + kGridPanelPad + 28.f;

    const int32 selSlot = _pPalette ? _pPalette->GetSelectedSlot() : -1;

    for (int32 i = 0; i < InventoryData::kTotalSlots; ++i)
    {
        const int32 col = i % InventoryData::kGridCols;
        const int32 row = i / InventoryData::kGridCols;

        const float slotX = sx + col * (kGridSlotW + kGridSlotGap);
        const float slotY = sy + row * (kGridSlotH + kGridSlotGap);

        const InventorySlotData slot = _pInventory->GetGridSlot(i);

        const bool selected = (i < InventoryData::kHotbarSlots && i == selSlot);

        DrawSingleSlot(slotX, slotY, kGridSlotW, kGridSlotH, slot, selected, /*isHotbar=*/false);
    }
}

void InventoryWidget::DrawHotbarMirror()
{
    const float panelY  = GetScreenY() + kGridPanelH + kPanelGap;
    const float sx      = GetScreenX() + kHotbarPad;
    const float sy      = panelY       + kHotbarPad;

    const int32 selSlot = _pPalette ? _pPalette->GetSelectedSlot() : -1;

    for (int32 i = 0; i < InventoryData::kHotbarSlots; ++i)
    {
        const float slotX = sx + i * (kHotbarSlotW + kHotbarSlotGap);

        const InventorySlotData slot = _pInventory->GetHotbarSlot(i);
        const bool selected = (i == selSlot);

        DrawSingleSlot(slotX, sy, kHotbarSlotW, kHotbarSlotH,
                       slot, selected, /*isHotbar=*/true);
    }
}

void InventoryWidget::DrawSingleSlot(float x, float y, float w, float h, const InventorySlotData& slot, bool selected, bool isHotbar)
{
    auto* ui = GET_SINGLE(UIManager);

    const bool isEmpty = (slot.type == SlotType::Count);

    Color bg = isEmpty ? Color(0.10f, 0.10f, 0.12f, 0.80f) : GetSlotColor(slot.type);

    if (selected)
    {
        bg.R(std::min(1.f, bg.R() + 0.30f));
        bg.G(std::min(1.f, bg.G() + 0.30f));
        bg.B(std::min(1.f, bg.B() + 0.30f));
    }
    ui->AddRect(x, y, w, h, bg);

    const Color borderCol = selected
        ? Color(0.98f, 0.85f, 0.25f, 1.f)
        : Color(0.30f, 0.30f, 0.38f, 0.70f);
    ui->AddRectBorder(x, y, w, h, borderCol, selected ? 2.5f : 1.f);

    if (isEmpty) return;

    DrawSlotLabel(x, y, w, h, slot, isHotbar);
}

void InventoryWidget::DrawSlotLabel(float x, float y, float w, float h, const InventorySlotData& slot, bool isHotbar)
{
    auto* ui = GET_SINGLE(UIManager);

    const wchar_t* name = GetSlotLabel(slot.type);

    // 아이템 이름 (슬롯 상단)
    const Color nameCol = Color(0.90f, 0.90f, 0.95f, 1.f);
    ui->AddText(name, x + 4.f, y + 4.f, w - 8.f, 18.f, nameCol, 18);

    wchar_t countBuf[16];
    if (slot.type == SlotType::Eraser)
        ::wcscpy_s(countBuf, L"∞");
    else
        ::swprintf_s(countBuf, L"%d", slot.count);

    const Color countCol = (slot.count == 0 && slot.type != SlotType::Eraser)
        ? Color(0.8f, 0.2f, 0.2f, 1.f) : Color(0.6f, 0.95f, 0.6f, 1.f);

    ui->AddText(countBuf, x + 4.f, y + h - 22.f, w - 8.f, 18.f, countCol, 14);

    if (isHotbar)
    {
        const int32 keyNum = static_cast<int32>(slot.type) + 1;
        if (keyNum >= 1 && keyNum <= InventoryData::kHotbarSlots)
        {
            wchar_t keyBuf[4];
            ::swprintf_s(keyBuf, L"%d", keyNum);
            ui->AddText(keyBuf, x + w - 16.f, y + 4.f, 12.f, 14.f, Color(0.6f, 0.6f, 0.7f, 0.85f), 11);
        }
    }
}