#pragma once
#include "App/Interfaces/IWindow.h"

class IWindow;

class WindowManager
{
	DECLARE_SINGLE(WindowManager)

public:
	void Init(HINSTANCE hInstance, HWND hMainWnd)
	{
		_hInstance = hInstance;
		_hMainWnd  = hMainWnd;
	}

	void ToggleWindow(const std::wstring& name)
	{
		auto it = _windows.find(name);
		if (it != _windows.end())
		{
			it->second->Toggle();
			if (it->second->UsesInternalUI() && it->second->IsVisible())
			{
				auto orderIt = std::find(_windowOrder.begin(), _windowOrder.end(), name);
				if (orderIt != _windowOrder.end())
					_windowOrder.erase(orderIt);
				_windowOrder.push_back(name);
				_activeHudName = name;
			}
			else if (_activeHudName == name)
				ActivateNextVisibleHud();
		}
	}

	void DrawUI();
	void UpdateUI();
	bool IsMouseOverUI(float x, float y) const;

	template<typename T>
	T* RegisterWindow(const std::wstring& name);

	template<typename T>
	T* GetWindow(const std::wstring& name);

private:
	void ActivateNextVisibleHud();

	HINSTANCE _hInstance = nullptr;
	HWND      _hMainWnd  = nullptr;

	std::vector<std::wstring> _windowOrder;
	std::wstring _activeHudName;
	std::unordered_map<std::wstring, std::unique_ptr<IWindow>> _windows;
};

template<typename T>
T* WindowManager::RegisterWindow(const std::wstring& name)
{
	if (auto* registered = GetWindow<T>(name))
		return registered;

	auto window = std::make_unique<T>();
	if (!window->Create(_hInstance, _hMainWnd))
		return nullptr;

	T* raw = window.get();
	_windows[name] = std::move(window);
	_windowOrder.push_back(name);
	return raw;
}

template<typename T>
T* WindowManager::GetWindow(const std::wstring& name)
{
	auto it = _windows.find(name);
	if (it != _windows.end())
		return static_cast<T*>(it->second.get());
	return nullptr;
}
