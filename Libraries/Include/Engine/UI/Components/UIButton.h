#pragma once
#include "UIComponent.h"

class UIButton : public UIComponent
{
    using Super = UIComponent;
public:
    UIButton();
    virtual ~UIButton() = default;

    virtual void Update()  override;
    virtual void DrawUI(float ox, float oy) override;

    void SetText(const std::wstring& t)        { _text = t; }
    void SetOnClick(std::function<void()> cb)  { _onClick = cb; }

    void SetNormalColor (Color c) { _normalColor  = c; }
    void SetHoverColor  (Color c) { _hoverColor   = c; }
    void SetPressedColor(Color c) { _pressedColor = c; }
    void SetTextColor   (Color c) { _textColor    = c; }
    void SetFontSize    (int   s) { _fontSize     = s; }

    // Widget이 마우스 상태 전달
    void SetHovered(bool h) { _hovered = h; }
    void SetPressed(bool p) { _pressed = p; }
    void OnClick()          { if (_onClick) _onClick(); }

private:
    std::wstring          _text;
    std::function<void()> _onClick;

    Color _normalColor  = Color(0.20f, 0.20f, 0.22f, 0.90f);
    Color _hoverColor   = Color(0.30f, 0.30f, 0.35f, 0.95f);
    Color _pressedColor = Color(0.10f, 0.10f, 0.12f, 1.00f);
    Color _textColor    = Color(1.f,   1.f,   1.f,   1.f);
    int   _fontSize     = 16;

    bool _hovered = false;
    bool _pressed = false;
};