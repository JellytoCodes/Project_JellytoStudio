#pragma once
#include "Entity/Entity.h"
#include "UI/Components/UIComponent.h"

class Widget : public Entity
{
    using Super = Entity;
public:
    explicit Widget(const std::wstring& name);
    virtual ~Widget() = default;

    // UIComponent 추가 (Button, UIText 등)
    void AddUIComponent(const std::shared_ptr<UIComponent>& comp);

    // 스크린 기준점 (좌상단 픽셀)
    void  SetScreenPos(float x, float y) { _sx = x; _sy = y; }
    float GetScreenX() const { return _sx; }
    float GetScreenY() const { return _sy; }

    // Entity Update override: 입력 + UI 컴포넌트 Update
    virtual void Update() override;

    // DrawUI: UIManager DrawList에 커맨드 제출
    virtual void DrawUI();

private:
    std::vector<std::shared_ptr<UIComponent>> _uiComponents;
    float _sx = 0.f;
    float _sy = 0.f;
};