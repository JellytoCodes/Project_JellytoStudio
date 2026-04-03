#pragma once
#include "UI/Widget.h"

class PaletteWidget : public Widget
{
    using Super = Widget;
public:
    enum class SlotType : uint8
    {
        Mushroom1 = 0,
        Mushroom2,
        Mushroom3,
        Priming1,
        Priming2,
        Priming3,
        Bridge,
        Eraser,
        Count
    };

    struct SlotInfo
    {
        std::wstring label;     // 팔레트 표시 이름
        std::wstring modelName; // MapModel 폴더 내 파일명 (확장자 제외)
        Color        color;
        SlotType     type;
    };

    explicit PaletteWidget(const std::wstring& name);
    virtual ~PaletteWidget() = default;

    virtual void Update()         override;
    virtual void DrawUI()         override; // 배치 모드 ON일 때만 렌더

    int32    GetSelectedSlot()     const { return _selectedSlot; }
    SlotType GetSelectedSlotType() const { return _slots[_selectedSlot].type; }
    const SlotInfo& GetSelectedSlotInfo() const { return _slots[_selectedSlot]; }

    bool IsEraserMode()  const { return _slots[_selectedSlot].type == SlotType::Eraser; }
    bool IsPlacingMode() const { return _placingMode; }
    void SetPlacingMode(bool on) { _placingMode = on; }

private:
    void HandleInput();
    void DrawBar();

    static constexpr int32 SLOT_COUNT  = static_cast<int32>(SlotType::Count);
    static constexpr float SLOT_W      = 76.f;
    static constexpr float SLOT_H      = 68.f;
    static constexpr float SLOT_GAP    = 5.f;
    static constexpr float BAR_PADDING = 8.f;
    static constexpr float BAR_H       = SLOT_H + BAR_PADDING * 2;

    std::array<SlotInfo, SLOT_COUNT> _slots;
    int32 _selectedSlot = 0;
    bool  _placingMode  = false;
};