#pragma once
#include "UIComponent.h"

// UIButton과 동일하게 UIManager::AddText() 경유
class UIText : public UIComponent
{
    using Super = UIComponent;
public:
    UIText();
    virtual ~UIText() = default;

    virtual void Update()  override;
    virtual void DrawUI(float ox, float oy) override;

    void SetText(const std::wstring& t)                      { _text = t; }
    void SetTextGetter(std::function<std::wstring()> getter) { _getter = getter; }

    void SetTextColor(Color c)                 { _textColor = c; }
    void SetBgColor(Color c, bool use = true)  { _bgColor = c; _useBg = use; }
    void SetFontSize(int s)                    { _fontSize = s; }
    void SetFontName(const std::wstring& name) { _fontName = name; }

private:
    std::wstring                  _text;
    std::function<std::wstring()> _getter;

    Color        _textColor = Color(1.f, 1.f, 0.4f, 1.f);
    Color        _bgColor   = Color(0.08f, 0.08f, 0.08f, 0.85f);
    bool         _useBg     = true;
    int          _fontSize  = 18;
    std::wstring _fontName  = L"Arial";
};