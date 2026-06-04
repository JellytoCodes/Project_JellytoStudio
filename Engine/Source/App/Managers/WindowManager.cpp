
#include "Framework.h"
#include "WindowManager.h"

#include "Core/DisplayContext.h"
#include "Core/Managers/InputManager.h"
#include "UI/UIManager.h"

namespace
{
	constexpr float kTabX = 8.f;
	constexpr float kTabY = 5.f;
	constexpr float kTabW = 132.f;
	constexpr float kTabH = 28.f;
	constexpr float kTabGap = 4.f;
	constexpr float kCloseW = 22.f;

	std::wstring TabLabel(const std::wstring& name)
	{
		if (name == L"ChunkDebugWindow") return L"Chunk";
		if (name == L"BlockTestPanel") return L"Block";
		if (name == L"StressPanel") return L"Stress";
		if (name == L"PickDebugPanel") return L"Pick";
		if (name == L"DetailWindow") return L"Detail";
		if (name == L"ToolWindow") return L"Tool";
		if (name == L"ItemWindow") return L"Item";
		return name;
	}
}

void WindowManager::ActivateNextVisibleHud()
{
	_activeHudName.clear();

	for (const std::wstring& name : _windowOrder)
	{
		auto it = _windows.find(name);
		if (it == _windows.end()) continue;

		IWindow* window = it->second.get();
		if (!window || !window->IsVisible() || !window->UsesInternalUI()) continue;

		_activeHudName = name;
		return;
	}
}

void WindowManager::UpdateUI()
{
	const POINT mp = GET_SINGLE(InputManager)->GetMousePos();
	const float mx = static_cast<float>(mp.x);
	const float my = static_cast<float>(mp.y);
	const bool click = GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON);

	int visibleIndex = 0;
	for (const std::wstring& name : _windowOrder)
	{
		auto it = _windows.find(name);
		if (it == _windows.end()) continue;

		IWindow* window = it->second.get();
		if (!window || !window->IsVisible() || !window->UsesInternalUI()) continue;

		const float x = kTabX + visibleIndex * (kTabW + kTabGap);
		const float closeX = x + kTabW - kCloseW;
		if (click && mx >= closeX && mx <= x + kTabW && my >= kTabY && my <= kTabY + kTabH)
		{
			window->Hide();
			if (_activeHudName == name)
				ActivateNextVisibleHud();
			return;
		}

		if (click && mx >= x && mx <= x + kTabW && my >= kTabY && my <= kTabY + kTabH)
			_activeHudName = name;

		++visibleIndex;
	}

	auto activeIt = _windows.find(_activeHudName);
	if (activeIt == _windows.end() || !activeIt->second || !activeIt->second->IsVisible() || !activeIt->second->UsesInternalUI())
	{
		ActivateNextVisibleHud();
		activeIt = _windows.find(_activeHudName);
	}

	if (activeIt != _windows.end() && activeIt->second && activeIt->second->IsVisible() && activeIt->second->UsesInternalUI())
		activeIt->second->Update();
}

void WindowManager::DrawUI()
{
	auto* ui = GET_SINGLE(UIManager);

	int visibleIndex = 0;
	for (const std::wstring& name : _windowOrder)
	{
		auto it = _windows.find(name);
		if (it == _windows.end()) continue;

		IWindow* window = it->second.get();
		if (!window || !window->IsVisible() || !window->UsesInternalUI()) continue;

		const float x = kTabX + visibleIndex * (kTabW + kTabGap);
		const float y = kTabY;
		const bool active = (_activeHudName == name);

		ui->AddRect(x, y, kTabW, kTabH, active ? Color(0.13f, 0.16f, 0.20f, 0.98f) : Color(0.08f, 0.09f, 0.11f, 0.92f));
		ui->AddRectBorder(x, y, kTabW, kTabH, active ? Color(0.95f, 0.74f, 0.18f, 1.f) : Color(0.28f, 0.34f, 0.42f, 1.f), active ? 2.f : 1.f);
		ui->AddText(TabLabel(name), x + 10.f, y + 4.f, kTabW - kCloseW - 14.f, kTabH - 8.f,
			active ? Color(1.f, 0.93f, 0.70f, 1.f) : Color(0.88f, 0.92f, 0.98f, 1.f), 13, L"Arial");
		ui->AddText(L"x", x + kTabW - kCloseW, y + 4.f, kCloseW - 4.f, kTabH - 8.f,
			Color(0.95f, 0.85f, 0.85f, 1.f), 13, L"Arial");

		++visibleIndex;
	}

	auto activeIt = _windows.find(_activeHudName);
	if (activeIt == _windows.end() || !activeIt->second || !activeIt->second->IsVisible() || !activeIt->second->UsesInternalUI())
	{
		ActivateNextVisibleHud();
		activeIt = _windows.find(_activeHudName);
	}

	if (activeIt != _windows.end() && activeIt->second && activeIt->second->IsVisible() && activeIt->second->UsesInternalUI())
		activeIt->second->DrawUI();
}

bool WindowManager::IsMouseOverUI(float x, float y) const
{
	int visibleIndex = 0;
	for (const std::wstring& name : _windowOrder)
	{
		auto it = _windows.find(name);
		if (it == _windows.end()) continue;

		IWindow* window = it->second.get();
		if (!window || !window->IsVisible() || !window->UsesInternalUI()) continue;

		const float tabX = kTabX + visibleIndex * (kTabW + kTabGap);
		if (x >= tabX && x <= tabX + kTabW && y >= kTabY && y <= kTabY + kTabH)
			return true;

		++visibleIndex;
	}

	auto activeIt = _windows.find(_activeHudName);
	if (activeIt != _windows.end() && activeIt->second && activeIt->second->IsVisible() && activeIt->second->UsesInternalUI())
		return activeIt->second->HitTest(x, y);

	return false;
}
