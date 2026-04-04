#pragma once
#include "Entity/Components/Component.h"

class UIManager;

class UIComponent : public Component
{
    using Super = Component;

public:
    explicit UIComponent(ComponentType type = ComponentType::UIComponent);
    virtual ~UIComponent() = default;

    // Draw: UIManager DrawList에 커맨드 누적
    // ox, oy = Widget(부모 Entity) 의 스크린 기준점
    virtual void DrawUI(float ox, float oy) {}

    // 레이아웃 (Widget 내 오프셋 + 크기, 픽셀)
    void SetRect(float x, float y, float w, float h) { _x=x; _y=y; _w=w; _h=h; }
    float GetX() const { return _x; }
    float GetY() const { return _y; }
    float GetW() const { return _w; }
    float GetH() const { return _h; }

    // 히트테스트: ox/oy = Widget 스크린 기준점
    bool HitTest(float mx, float my, float ox, float oy) const
    {
        return mx >= ox+_x && mx <= ox+_x+_w
            && my >= oy+_y && my <= oy+_y+_h;
    }

protected:
    float _x = 0.f, _y = 0.f, _w = 100.f, _h = 30.f;
};