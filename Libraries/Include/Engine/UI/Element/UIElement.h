#pragma once

class UIDrawList;

// ── UIElement ─────────────────────────────────────────────────────────────
// TextBlock / Button 의 추상 베이스
// 좌표: Widget 기준 픽셀 오프셋 (x, y), 크기 (w, h)
class UIElement
{
public:
    UIElement()          = default;
    virtual ~UIElement() = default;

    virtual void Update(float dt) {}

    // Draw: UIDrawList에 커맨드 제출
    virtual void Draw(UIDrawList& dl, float px, float py) = 0;

    // 히트 테스트: px/py = Widget 스크린 기준점
    virtual bool HitTest(float mx, float my, float px, float py) const
    {
        return mx >= px + _x && mx <= px + _x + _w
            && my >= py + _y && my <= py + _y + _h;
    }

    virtual void OnClick() {}

    // x, y: Widget 내 오프셋 / w, h: 크기 (픽셀)
    void SetRect(float x, float y, float w, float h) { _x = x; _y = y; _w = w; _h = h; }

    void SetVisible(bool v) { _visible = v; }
    bool IsVisible()  const { return _visible; }

    float GetX() const { return _x; }
    float GetY() const { return _y; }
    float GetW() const { return _w; }
    float GetH() const { return _h; }

protected:
    float _x = 0.f, _y = 0.f, _w = 100.f, _h = 30.f;
    bool  _visible = true;
};