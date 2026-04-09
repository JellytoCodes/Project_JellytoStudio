#include "Framework.h"
#include "Widget.h"

#include "UI/Components/UIButton.h"
#include "UI/Components/UIText.h"
#include "UI/UIManager.h"
#include "Core/Managers/InputManager.h"

Widget::Widget(const std::wstring& name) : Super(name) {}

void Widget::AddUIComponent(std::unique_ptr<UIComponent> comp)
{
	_uiComponents.push_back(std::move(comp));
}

void Widget::Update()
{
	POINT mp       = GET_SINGLE(InputManager)->GetMousePos();
	float mx       = static_cast<float>(mp.x);
	float my       = static_cast<float>(mp.y);
	bool  lDown    = GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON);
	bool  lPressed = GET_SINGLE(InputManager)->GetButton(KEY_TYPE::LBUTTON);

	for (auto& comp : _uiComponents)
	{
		comp->Update();

		if (UIButton* btn = dynamic_cast<UIButton*>(comp.get()))
		{
			bool over = btn->HitTest(mx, my, _sx, _sy);
			btn->SetHovered(over && !lPressed);
			btn->SetPressed(over &&  lPressed);
			if (over && lDown)
				btn->OnClick();
		}
	}
}

void Widget::DrawUI()
{
	for (auto& comp : _uiComponents)
		comp->DrawUI(_sx, _sy);
}
