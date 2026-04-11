#include "pch.h"
#include "PaletteWidget.h"

#include "UI/UIManager.h"
#include "Core/Managers/InputManager.h"
#include "Types/GlobalTypes.h"

static constexpr float SCR_W = static_cast<float>(MAIN_WINDOW_WIDTH);
static constexpr float SCR_H = static_cast<float>(MAIN_WINDOW_HEIGHT);

PaletteWidget::PaletteWidget(const std::wstring& name)
    : Super(name)
{
    _slots[0] = { L"Mushroom1", L"Mushroom_01", Color(0.20f,0.30f,0.22f,0.90f), SlotType::Mushroom1 };
    _slots[1] = { L"Mushroom2", L"Mushroom_02", Color(0.22f,0.32f,0.20f,0.90f), SlotType::Mushroom2 };
    _slots[2] = { L"Mushroom3", L"Mushroom_03", Color(0.18f,0.28f,0.24f,0.90f), SlotType::Mushroom3 };
    _slots[3] = { L"Priming1",  L"Priming_01",  Color(0.28f,0.22f,0.18f,0.90f), SlotType::Priming1  };
    _slots[4] = { L"Priming2",  L"Priming_02",  Color(0.30f,0.24f,0.18f,0.90f), SlotType::Priming2  };
    _slots[5] = { L"Priming3",  L"Priming_03",  Color(0.26f,0.20f,0.20f,0.90f), SlotType::Priming3  };
    _slots[6] = { L"Bridge",    L"Bridge",       Color(0.22f,0.22f,0.30f,0.90f), SlotType::Bridge    };
    _slots[7] = { L"Eraser",    L"",             Color(0.40f,0.15f,0.15f,0.90f), SlotType::Eraser    };

    float totalW = SLOT_COUNT * SLOT_W + (SLOT_COUNT - 1) * SLOT_GAP + BAR_PADDING * 2;
    SetScreenPos((SCR_W - totalW) * 0.5f, SCR_H - BAR_H - 8.f);
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
    DrawBar();
}

void PaletteWidget::DrawBar()
{
    auto ui = GET_SINGLE(UIManager);
    float bx = GetScreenX();
    float by = GetScreenY();
    float totalW = SLOT_COUNT * SLOT_W + (SLOT_COUNT - 1) * SLOT_GAP + BAR_PADDING * 2;

    // 바 배경
    ui->AddRect(bx, by, totalW, BAR_H, Color(0.10f,0.12f,0.16f,0.93f));

    // 상단 라벨
    ui->AddText(L"[ PLACE MODE ]  1~8: 슬롯 선택  Tab: 종료",
        bx, by - 22.f, totalW, 18.f, Color(0.3f,0.9f,0.5f,1.f), 12);

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

        // 선택 테두리
        ui->AddRectBorder(slotX, slotY, SLOT_W, SLOT_H,
            selected ? Color(0.9f,0.85f,0.3f,1.f) : Color(0.35f,0.35f,0.40f,0.6f),
            selected ? 2.f : 1.f);

        // 숫자 힌트
        wchar_t numBuf[4];
        swprintf_s(numBuf, L"%d", i+1);
        ui->AddText(numBuf, slotX+4.f, slotY+3.f, 20.f, 14.f, Color(0.6f,0.6f,0.6f,0.9f), 11);

        // 슬롯 이름
        Color labelClr = selected ? Color(1.f,0.95f,0.5f,1.f) : Color(0.85f,0.85f,0.85f,1.f);
        ui->AddText(slot.label, slotX, slotY + SLOT_H*0.35f, SLOT_W, 20.f, labelClr, 13);

        // 지우개 X
        if (slot.type == SlotType::Eraser)
            ui->AddText(L"X", slotX, slotY + SLOT_H*0.10f, SLOT_W, 24.f,
                Color(1.f,0.35f,0.35f,1.f), 18);

        slotX += SLOT_W + SLOT_GAP;
    }
}