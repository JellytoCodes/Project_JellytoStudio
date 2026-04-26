#include "pch.h"
#include "PaletteWidget.h"

#include "UI/UIManager.h"
#include "Core/Managers/InputManager.h"
#include "Types/GlobalTypes.h"

#include "Data/BlockDataTable.h"

static constexpr float SCR_W = static_cast<float>(kWindowWidth);
static constexpr float SCR_H = static_cast<float>(kWindowHeight);

// ── 생성자 ────────────────────────────────────────────────────────────────────
PaletteWidget::PaletteWidget(const std::wstring& name)
    : Super(name)
{
    assert(GET_SINGLE(BlockDataTable)->IsLoaded() && "PaletteWidget 생성 전에 BlockDataTable::Load() 가 선행되어야 합니다.");

    const auto& allRecords = GET_SINGLE(BlockDataTable)->GetAllSlotRecords();
    assert(static_cast<int32>(allRecords.size()) == SLOT_COUNT && "BlockData.xml 의 Slot 항목 수가 SlotType::Count 와 다릅니다.");

    for (int32 i = 0; i < SLOT_COUNT; ++i)
    {
        const BlockSlotRecord& rec = allRecords[static_cast<size_t>(i)];

        _slots[i].label     = rec.paletteLabel;
        _slots[i].modelName = rec.modelName;
        _slots[i].color     = rec.color;
        _slots[i].type      = rec.slotType;
    }

    float totalW = SLOT_COUNT * SLOT_W + (SLOT_COUNT - 1) * SLOT_GAP + BAR_PADDING * 2;
    SetScreenPos((SCR_W - totalW) * 0.5f, SCR_H - BAR_H - 8.f);
}

// ── Update ────────────────────────────────────────────────────────────────────
void PaletteWidget::Update()
{
    if (_placingMode)
        HandleInput();
}

// ── HandleInput ───────────────────────────────────────────────────────────────
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

// ── DrawUI ────────────────────────────────────────────────────────────────────
void PaletteWidget::DrawUI()
{
    if (!_placingMode) return;
    DrawBar();
}

// ── DrawBar ───────────────────────────────────────────────────────────────────
void PaletteWidget::DrawBar()
{
    auto ui = GET_SINGLE(UIManager);
    float bx = GetScreenX();
    float by = GetScreenY();
    float totalW = SLOT_COUNT * SLOT_W + (SLOT_COUNT - 1) * SLOT_GAP + BAR_PADDING * 2;

    // 바 배경
    ui->AddRect(bx, by, totalW, BAR_H, Color(0.10f, 0.12f, 0.16f, 0.93f));

    // 상단 라벨
    ui->AddText(L"[ PLACE MODE ]  1~8: 슬롯 선택  Tab: 종료",
        bx, by - 22.f, totalW, 18.f, Color(0.3f, 0.9f, 0.5f, 1.f), 12);

    float slotX = bx + BAR_PADDING;
    float slotY = by + BAR_PADDING;

    for (int32 i = 0; i < SLOT_COUNT; i++)
    {
        bool selected = (i == _selectedSlot);
        const SlotInfo& slot = _slots[i];

        Color bg = slot.color;
        if (selected)
        {
            bg.R(std::min(1.f, bg.R() + 0.30f));
            bg.G(std::min(1.f, bg.G() + 0.30f));
            bg.B(std::min(1.f, bg.B() + 0.30f));
        }
        ui->AddRect(slotX, slotY, SLOT_W, SLOT_H, bg);

        const Color borderCol = selected
            ? Color(0.98f, 0.85f, 0.25f, 1.f)
            : Color(0.30f, 0.30f, 0.38f, 0.70f);
        ui->AddRectBorder(slotX, slotY, SLOT_W, SLOT_H, borderCol, selected ? 2.5f : 1.f);

        // 슬롯 레이블 (paletteLabel)
        ui->AddText(slot.label.c_str(),
            slotX + 4.f, slotY + SLOT_H * 0.35f,
            SLOT_W - 8.f, 14.f,
            Color(0.9f, 0.9f, 0.95f, 1.f), 11);

        // 단축키 번호
        wchar_t keyBuf[4];
        ::swprintf_s(keyBuf, L"%d", i + 1);
        ui->AddText(keyBuf,
            slotX + SLOT_W - 16.f, slotY + 4.f,
            12.f, 14.f,
            Color(0.6f, 0.6f, 0.7f, 0.85f), 10);

        slotX += SLOT_W + SLOT_GAP;
    }
}