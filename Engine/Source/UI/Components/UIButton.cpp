#include "Framework.h"
#include "UIButton.h"
#include "UI/UIManager.h"

UIButton::UIButton()
    : Super(ComponentType::UIComponent)
{
}

void UIButton::Update()
{
}

void UIButton::DrawUI(float ox, float oy)
{
    float ax = ox + _x;
    float ay = oy + _y;

    Color bg = _pressed ? _pressedColor
             : _hovered ? _hoverColor
             :            _normalColor;
    GET_SINGLE(UIManager)->AddRect(ax, ay, _w, _h, bg);

    Color border = _hovered
        ? Color(0.6f, 0.6f, 0.7f, 1.f)
        : Color(0.35f, 0.35f, 0.4f, 1.f);
    GET_SINGLE(UIManager)->AddRectBorder(ax, ay, _w, _h, border, 1.f);

    if (!_text.empty())
        GET_SINGLE(UIManager)->AddText(_text, ax, ay, _w, _h, _textColor, _fontSize);
}
