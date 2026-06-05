
#include "Framework.h"
#include "WindowManager.h"

#include "Core/DisplayContext.h"
#include "Core/Managers/InputManager.h"
#include "UI/UIManager.h"

namespace
{
	constexpr float kHudW = 462.f;
	constexpr float kHudH = 646.f;
	constexpr float kHudY = 42.f;
	constexpr float kHudPad = 14.f;
	constexpr float kInnerTabH = 26.f;
	constexpr float kInnerTabGap = 6.f;
	constexpr const wchar_t* kInternalHudNames[] =
	{
		L"ChunkDebugWindow",
		L"BlockTestPanel",
		L"StressPanel",
		L"PickDebugPanel",
		L"DetailWindow",
	};

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
	if (!_internalHudVisible)
	{
		for (const wchar_t* rawName : kInternalHudNames)
		{
			const std::wstring name(rawName);
			auto it = _windows.find(name);
			if (it != _windows.end() && it->second && it->second->UsesInternalUI())
				it->second->Hide();
		}
		return;
	}

	auto activeIt = _windows.find(_activeHudName);
	if (activeIt == _windows.end() || !activeIt->second || !activeIt->second->UsesInternalUI())
		_activeHudName.clear();

	for (const wchar_t* rawName : kInternalHudNames)
	{
		const std::wstring name(rawName);
		auto it = _windows.find(name);
		if (it == _windows.end()) continue;

		IWindow* window = it->second.get();
		if (!window || !window->UsesInternalUI()) continue;

		if (_activeHudName.empty())
			_activeHudName = name;

		if (name == _activeHudName)
			window->Show();
		else
			window->Hide();
	}
}

void WindowManager::UpdateUI()
{
	if (!_internalHudVisible) return;

	const POINT mp = GET_SINGLE(InputManager)->GetMousePos();
	const float mx = static_cast<float>(mp.x);
	const float my = static_cast<float>(mp.y);
	const bool click = GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON);

	const float panelX = GET_SINGLE(DisplayContext)->GetWidthF() - kHudW - kHudPad;
	const float panelY = kHudY;

	if (click && mx >= panelX + kHudW - 34.f && mx <= panelX + kHudW - 10.f && my >= panelY + 8.f && my <= panelY + 30.f)
	{
		_internalHudVisible = false;
		ActivateNextVisibleHud();
		return;
	}

	int visibleIndex = 0;
	for (const wchar_t* rawName : kInternalHudNames)
	{
		const std::wstring name(rawName);
		auto it = _windows.find(name);
		if (it == _windows.end()) continue;

		IWindow* window = it->second.get();
		if (!window || !window->UsesInternalUI()) continue;

		const float x = panelX + 14.f + visibleIndex * (82.f + kInnerTabGap);
		const float y = panelY + 36.f;
		if (click && mx >= x && mx <= x + 82.f && my >= y && my <= y + kInnerTabH)
		{
			_activeHudName = name;
			ActivateNextVisibleHud();
			return;
		}

		++visibleIndex;
	}

	ActivateNextVisibleHud();

	auto activeIt = _windows.find(_activeHudName);
	if (activeIt != _windows.end() && activeIt->second && activeIt->second->IsVisible() && activeIt->second->UsesInternalUI())
		activeIt->second->Update();
}

void WindowManager::DrawUI()
{
	if (!_internalHudVisible) return;

	auto* ui = GET_SINGLE(UIManager);

	ActivateNextVisibleHud();

	auto activeIt = _windows.find(_activeHudName);
	if (activeIt != _windows.end() && activeIt->second && activeIt->second->IsVisible() && activeIt->second->UsesInternalUI())
		activeIt->second->DrawUI();

	const float panelX = GET_SINGLE(DisplayContext)->GetWidthF() - kHudW - kHudPad;
	const float panelY = kHudY;

	ui->AddRect(panelX, panelY, kHudW, 66.f, Color(0.055f, 0.065f, 0.080f, 0.96f));
	ui->AddRectBorder(panelX, panelY, kHudW, kHudH, Color(0.30f, 0.36f, 0.44f, 0.95f), 1.5f);
	ui->AddText(L"Debug / Benchmark HUD", panelX + 14.f, panelY + 10.f, 220.f, 22.f,
		Color(0.90f, 0.94f, 1.f, 1.f), 17, L"Arial");
	ui->AddText(L"x", panelX + kHudW - 28.f, panelY + 8.f, 18.f, 22.f,
		Color(1.f, 0.82f, 0.82f, 1.f), 15, L"Arial");

	int visibleIndex = 0;
	for (const wchar_t* rawName : kInternalHudNames)
	{
		const std::wstring name(rawName);
		auto it = _windows.find(name);
		if (it == _windows.end()) continue;

		IWindow* window = it->second.get();
		if (!window || !window->UsesInternalUI()) continue;

		const float x = panelX + 14.f + visibleIndex * (82.f + kInnerTabGap);
		const float y = panelY + 36.f;
		const bool active = (_activeHudName == name);

		ui->AddRect(x, y, 82.f, kInnerTabH, active ? Color(0.13f, 0.16f, 0.20f, 0.98f) : Color(0.08f, 0.09f, 0.11f, 0.92f));
		ui->AddRectBorder(x, y, 82.f, kInnerTabH, active ? Color(0.95f, 0.74f, 0.18f, 1.f) : Color(0.28f, 0.34f, 0.42f, 1.f), active ? 2.f : 1.f);
		ui->AddText(TabLabel(name), x + 5.f, y + 4.f, 72.f, kInnerTabH - 8.f,
			active ? Color(1.f, 0.93f, 0.70f, 1.f) : Color(0.88f, 0.92f, 0.98f, 1.f), 13, L"Arial");

		++visibleIndex;
	}
}

bool WindowManager::IsMouseOverUI(float x, float y) const
{
	if (!_internalHudVisible) return false;

	const float panelX = GET_SINGLE(DisplayContext)->GetWidthF() - kHudW - kHudPad;
	const float panelY = kHudY;
	return x >= panelX && x <= panelX + kHudW && y >= panelY && y <= panelY + kHudH;
}
