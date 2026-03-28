
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
    // 슬롯 데이터 초기화
    _slots[0] = { L"Block",   L"1x1x1",  Color(0.25f, 0.25f, 0.30f, 0.90f), SlotType::BlockNormal };
    _slots[1] = { L"Flat",    L"1x0.5",  Color(0.20f, 0.28f, 0.35f, 0.90f), SlotType::BlockFlat };
    _slots[2] = { L"Large",   L"2x1x2",  Color(0.22f, 0.30f, 0.25f, 0.90f), SlotType::BlockLarge };
    _slots[3] = { L"Sphere",  L"round",  Color(0.30f, 0.22f, 0.28f, 0.90f), SlotType::Sphere };
    _slots[4] = { L"Eraser",  L"remove", Color(0.40f, 0.18f, 0.18f, 0.90f), SlotType::Eraser };

    // 팔레트 위치 설정: 화면 하단 중앙
    float totalW = SLOT_COUNT * SLOT_W + (SLOT_COUNT - 1) * SLOT_GAP + BAR_PADDING * 2;
    SetScreenPos((SCR_W - totalW) * 0.5f, SCR_H - BAR_H - 8.f);
}

// ── 모드 ─────────────────────────────────────────────────────────────────

void PaletteWidget::SetPlacingMode(bool on)
{
    _placingMode = on;
}

// ── Update ────────────────────────────────────────────────────────────────

void PaletteWidget::Update()
{
    HandleInput();
}

void PaletteWidget::HandleInput()
{
    auto input = GET_SINGLE(InputManager);

    // 숫자키 1~5 → 슬롯 직접 선택
    static const KEY_TYPE numKeys[] = {
        KEY_TYPE::KEY_1, KEY_TYPE::KEY_2, KEY_TYPE::KEY_3,
        KEY_TYPE::KEY_4, KEY_TYPE::KEY_5
    };
    for (int32 i = 0; i < SLOT_COUNT; i++)
    {
        if (input->GetButtonDown(numKeys[i]))
        {
            _selectedSlot = i;
            wchar_t dbg[64];
            swprintf_s(dbg, L"[Palette] 슬롯 %d 선택: %s\n", i + 1, _slots[i].label.c_str());
            ::OutputDebugStringW(dbg);
            return;
        }
    }
}

// ── DrawUI ────────────────────────────────────────────────────────────────

void PaletteWidget::DrawUI()
{
    DrawBar();
}

void PaletteWidget::DrawBar()
{
    auto ui = GET_SINGLE(UIManager);

    float bx = GetScreenX();
    float by = GetScreenY();
    float totalW = SLOT_COUNT * SLOT_W + (SLOT_COUNT - 1) * SLOT_GAP + BAR_PADDING * 2;

    // 팔레트 바 배경
    Color bgColor = _placingMode
        ? Color(0.12f, 0.15f, 0.20f, 0.92f)   // 배치 모드: 약간 밝게
        : Color(0.08f, 0.08f, 0.10f, 0.85f);   // 일반: 어둡게
    ui->AddRect(bx, by, totalW, BAR_H, bgColor);

    // 배치 모드 표시 라벨
    if (_placingMode)
    {
        ui->AddText(L"[ PLACE MODE ]", bx, by - 22.f, totalW, 18.f,
            Color(0.3f, 0.9f, 0.5f, 1.f), 13);
    }
    else
    {
        ui->AddText(L"Tab: 배치 모드", bx, by - 22.f, totalW, 18.f,
            Color(0.5f, 0.5f, 0.5f, 0.8f), 12);
    }

    // 슬롯 렌더
    float slotX = bx + BAR_PADDING;
    float slotY = by + BAR_PADDING;

    for (int32 i = 0; i < SLOT_COUNT; i++)
    {
        bool selected = (i == _selectedSlot);
        const SlotInfo& slot = _slots[i];

        // 슬롯 배경
        Color bg = slot.color;
        if (selected)
        {
            // 선택 슬롯: 밝게 + 테두리 강조
            bg.R(std::min(1.f, bg.R() + 0.25f));
            bg.G(std::min(1.f, bg.G() + 0.25f));
            bg.B(std::min(1.f, bg.B() + 0.25f));
        }
        ui->AddRect(slotX, slotY, SLOT_W, SLOT_H, bg);

        // 선택 테두리
        if (selected)
            ui->AddRectBorder(slotX, slotY, SLOT_W, SLOT_H,
                Color(0.9f, 0.85f, 0.3f, 1.f), 2.f);
        else
            ui->AddRectBorder(slotX, slotY, SLOT_W, SLOT_H,
                Color(0.35f, 0.35f, 0.40f, 0.8f), 1.f);

        // 숫자 키 힌트 (좌상단 작게)
        wchar_t numBuf[4];
        swprintf_s(numBuf, L"%d", i + 1);
        ui->AddText(numBuf, slotX + 4.f, slotY + 3.f, 20.f, 14.f,
            Color(0.6f, 0.6f, 0.6f, 0.9f), 11);

        // 슬롯 이름
        Color labelColor = selected
            ? Color(1.f, 0.95f, 0.5f, 1.f)
            : Color(0.85f, 0.85f, 0.85f, 1.f);
        ui->AddText(slot.label, slotX, slotY + SLOT_H * 0.30f, SLOT_W, 20.f,
            labelColor, 14);

        // 서브텍스트 (크기 정보)
        ui->AddText(slot.subtext, slotX, slotY + SLOT_H * 0.58f, SLOT_W, 16.f,
            Color(0.55f, 0.55f, 0.60f, 1.f), 11);

        // 지우개 슬롯 특수 표시
        if (slot.type == SlotType::Eraser && selected)
        {
            ui->AddText(L"X", slotX, slotY + SLOT_H * 0.10f, SLOT_W, 24.f,
                Color(1.f, 0.35f, 0.35f, 1.f), 18);
        }

        slotX += SLOT_W + SLOT_GAP;
    }
}