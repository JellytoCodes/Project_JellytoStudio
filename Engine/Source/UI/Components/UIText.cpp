#include "Framework.h"
#include "UIText.h"
#include "UI/UIManager.h"

UIText::UIText()
    : Super(ComponentType::UIComponent)
{
}

void UIText::Update()
{
    if (_getter)
        _text = _getter();
}

void UIText::DrawUI(float ox, float oy)
{
    float ax = ox + _x;
    float ay = oy + _y;

    if (_useBg)
        GET_SINGLE(UIManager)->AddRect(ax, ay, _w, _h, _bgColor);

    if (!_text.empty())
        GET_SINGLE(UIManager)->AddText(_text, ax, ay, _w, _h, _textColor, _fontSize);
}