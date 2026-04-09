#pragma once
#include "Entity/Entity.h"
#include "UI/Components/UIComponent.h"

class Widget : public Entity
{
	using Super = Entity;
public:
	explicit Widget(const std::wstring& name);
	virtual ~Widget() = default;

	void AddUIComponent(std::unique_ptr<UIComponent> comp);

	void  SetScreenPos(float x, float y) { _sx = x; _sy = y; }
	float GetScreenX() const { return _sx; }
	float GetScreenY() const { return _sy; }

	virtual void Update() override;
	virtual void DrawUI();

private:
	std::vector<std::unique_ptr<UIComponent>> _uiComponents;
	float _sx = 0.f;
	float _sy = 0.f;
};
