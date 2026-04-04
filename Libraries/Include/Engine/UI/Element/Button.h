#pragma once
#include "UIElement.h"

// ── Button ────────────────────────────────────────────────────────────────
// 클릭 이벤트 UIElement
// Widget::Update()가 마우스 상태를 SetHovered/SetPressed로 전달
class Button : public UIElement
{
public:
    Button()  = default;
    ~Button() = default;

    virtual void Update(float dt) override {}
    virtual void Draw(UIDrawList& dl, float px, float py) override;
    virtual void OnClick() override { if (_onClick) _onClick(); }

    void SetText(const std::wstring& t)        { _text = t; }
    void SetOnClick(std::function<void()> cb)  { _onClick = cb; }

    void SetNormalColor(Color c)   { _normalColor  = c; }
    void SetHoverColor(Color c)    { _hoverColor   = c; }
    void SetPressedColor(Color c)  { _pressedColor = c; }
    void SetTextColor(Color c)     { _textColor    = c; }
    void SetFontSize(int s)        { _fontSize     = s; }

    // Widget이 매 프레임 마우스 상태 전달
    void SetHovered(bool h) { _hovered = h; }
    void SetPressed(bool p) { _pressed = p; }

private:
    std::wstring           _text;
    std::function<void()>  _onClick;

    Color _normalColor  = Color(0.20f, 0.20f, 0.22f, 0.90f);
    Color _hoverColor   = Color(0.30f, 0.30f, 0.35f, 0.95f);
    Color _pressedColor = Color(0.10f, 0.10f, 0.12f, 1.00f);
    Color _textColor    = Color(1.f, 1.f, 1.f, 1.f);
    int   _fontSize     = 16;

    bool _hovered = false;
    bool _pressed = false;
};