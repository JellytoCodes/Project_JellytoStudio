#pragma once
#include "UI/Widget.h"

class PaletteWidget : public Widget
{
    using Super = Widget;
public:
    // 슬롯 타입
    enum class SlotType : uint8
    {
        BlockNormal = 0,  // 1×1×1
        BlockFlat = 1,  // 1×0.5×1
        BlockLarge = 2,  // 2×1×2
        Sphere = 3,  // 구체
        Eraser = 4,  // 제거 모드
        Count
    };

    struct SlotInfo
    {
        std::wstring label;   // 표시 이름
        std::wstring subtext; // 크기 등 부가 정보
        Color        color;   // 슬롯 색상
        SlotType     type;
    };

    explicit PaletteWidget(const std::wstring& name);
    virtual ~PaletteWidget() = default;

    virtual void Update()  override;
    virtual void DrawUI() override;  // Widget::DrawUI 오버라이드

    int32    GetSelectedSlot()    const { return _selectedSlot; }
    SlotType GetSelectedSlotType()const { return _slots[_selectedSlot].type; }

    bool IsEraserMode()  const { return _slots[_selectedSlot].type == SlotType::Eraser; }
    bool IsPlacingMode() const { return _placingMode; }
    void SetPlacingMode(bool on);

private:
    void HandleInput();
    void DrawBar();     // 팔레트 바 전체 렌더

    static constexpr int32 SLOT_COUNT = static_cast<int32>(SlotType::Count);
    static constexpr float SLOT_W = 80.f;
    static constexpr float SLOT_H = 70.f;
    static constexpr float SLOT_GAP = 6.f;
    static constexpr float BAR_PADDING = 10.f;
    static constexpr float BAR_H = SLOT_H + BAR_PADDING * 2;

    std::array<SlotInfo, SLOT_COUNT> _slots;
    int32 _selectedSlot = 0;
    bool  _placingMode = false;
};