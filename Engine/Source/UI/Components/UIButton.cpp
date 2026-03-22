#include "Framework.h"
#include "UIButton.h"
#include "UI/UIManager.h"
#include "Core/Managers/InputManager.h"

UIButton::UIButton()
    : Super(ComponentType::UIComponent)
{
}

void UIButton::Update()
{
    POINT mp      = GET_SINGLE(InputManager)->GetMousePos();
    float mx      = static_cast<float>(mp.x);
    float my      = static_cast<float>(mp.y);
}

void UIButton::DrawUI(float ox, float oy)
{
    float ax = ox + _x;
    float ay = oy + _y;

    // 배경
    Color bg = _pressed ? _pressedColor
             : _hovered ? _hoverColor
             :            _normalColor;
    GET_SINGLE(UIManager)->AddRect(ax, ay, _w, _h, bg);

    // 테두리
    Color border = _hovered
        ? Color(0.6f, 0.6f, 0.7f, 1.f)
        : Color(0.35f, 0.35f, 0.4f, 1.f);
    GET_SINGLE(UIManager)->AddRectBorder(ax, ay, _w, _h, border, 1.f);

    // 텍스트
    if (!_text.empty())
        GET_SINGLE(UIManager)->AddText(_text, ax, ay, _w, _h, _textColor, _fontSize);
}