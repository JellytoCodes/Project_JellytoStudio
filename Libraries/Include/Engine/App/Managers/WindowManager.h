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
			it->second->Toggle();
	}

	template<typename T>
	T* RegisterWindow(const std::wstring& name);

	template<typename T>
	T* GetWindow(const std::wstring& name);

private:
	HINSTANCE _hInstance = nullptr;
	HWND      _hMainWnd  = nullptr;

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
