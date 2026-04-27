#include "pch.h"
#include "PaletteWidget.h"
#include "UI/UIManager.h"
#include "Core/Managers/InputManager.h"
#include "Core/DisplayContext.h"
#include "Types/GlobalTypes.h"
#include "Data/BlockDataTable.h"

PaletteWidget::PaletteWidget(const std::wstring& name)
    : Super(name)
{
    assert(GET_SINGLE(BlockDataTable)->IsLoaded() && "PaletteWidget 생성 전 BlockDataTable::Load() 필요.");

    const auto& allRecords = GET_SINGLE(BlockDataTable)->GetAllSlotRecords();
    assert(static_cast<int32>(allRecords.size()) == SLOT_COUNT && "BlockData.xml의 Slot 수와 SlotType::Count가 다름.");

    for (int32 i = 0; i < SLOT_COUNT; ++i)
    {
        const BlockSlotRecord& rec = allRecords[static_cast<size_t>(i)];
        _slots[i].label     = rec.paletteLabel;
        _slots[i].modelName = rec.modelName;
        _slots[i].color     = rec.color;
        _slots[i].type      = rec.slotType;
    }
}

void PaletteWidget::Update()
{
    if (_placingMode)
        HandleInput();
}

void PaletteWidget::HandleInput()
{
    auto input = GET_SINGLE(InputManager);

    static const KEY_TYPE numKeys[] = {
        KEY_TYPE::KEY_1, KEY_TYPE::KEY_2, KEY_TYPE::KEY_3, KEY_TYPE::KEY_4,
        KEY_TYPE::KEY_5, KEY_TYPE::KEY_6, KEY_TYPE::KEY_7, KEY_TYPE::KEY_8
    };

    for (int32 i = 0; i < SLOT_COUNT; i++)
    {
        if (input->GetButtonDown(numKeys[i]))
        {
            _selectedSlot = i;
            return;
        }
    }
}

void PaletteWidget::DrawUI()
{
    if (!_placingMode) return;

    const float scrW  = GET_SINGLE(DisplayContext)->GetWidthF();
    const float scrH  = GET_SINGLE(DisplayContext)->GetHeightF();
    const float totalW = SLOT_COUNT * SLOT_W + (SLOT_COUNT - 1) * SLOT_GAP + BAR_PADDING * 2;

    SetScreenPos((scrW - totalW) * 0.5f, scrH - BAR_H - 8.f);

    DrawBar();
}

void PaletteWidget::DrawBar()
{
    auto  ui     = GET_SINGLE(UIManager);
    float bx     = GetScreenX();
    float by     = GetScreenY();
    float totalW = SLOT_COUNT * SLOT_W + (SLOT_COUNT - 1) * SLOT_GAP + BAR_PADDING * 2;

    ui->AddRect(bx, by, totalW, BAR_H, Color(0.10f, 0.12f, 0.16f, 0.93f));

    for (int32 i = 0; i < SLOT_COUNT; ++i)
    {
        const float sx = bx + BAR_PADDING + i * (SLOT_W + SLOT_GAP);
        const float sy = by + (BAR_H - SLOT_H) * 0.5f;

        const bool selected = (i == _selectedSlot);

        Color bg = _slots[i].color;
        if (selected)
        {
            bg.R(std::min(1.f, bg.R() + 0.25f));
            bg.G(std::min(1.f, bg.G() + 0.25f));
            bg.B(std::min(1.f, bg.B() + 0.25f));
        }

        ui->AddRect(sx, sy, SLOT_W, SLOT_H, bg);

        const Color borderCol = selected
            ? Color(0.98f, 0.85f, 0.20f, 1.f)
            : Color(0.30f, 0.30f, 0.40f, 0.75f);
        ui->AddRectBorder(sx, sy, SLOT_W, SLOT_H, borderCol, selected ? 2.5f : 1.f);

        if (!_slots[i].label.empty())
        {
            ui->AddText(_slots[i].label,
                sx + 4.f, sy + 4.f,
                SLOT_W - 8.f, SLOT_H - 8.f,
                Color(0.9f, 0.9f, 0.95f, 1.f), 12);
        }

        wchar_t keyBuf[4];
        ::swprintf_s(keyBuf, L"%d", i + 1);
        ui->AddText(keyBuf,
            sx + SLOT_W - 14.f, sy + 4.f,
            10.f, 12.f,
            Color(0.55f, 0.55f, 0.65f, 0.85f), 10);
    }
}
