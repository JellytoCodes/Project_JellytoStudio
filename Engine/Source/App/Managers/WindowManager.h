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
		_hMainWnd = hMainWnd;
	}

	void ToggleWindow(const std::wstring& name)
	{
		auto it = _windows.find(name);
		if (it != _windows.end())
			it->second->Toggle();
	}

	template<typename T>
	std::shared_ptr<T> RegisterWindow(const std::wstring& name);

	template<typename T>
	std::shared_ptr<T> GetWindow(const std::wstring& name);

private :
	HINSTANCE _hInstance = nullptr;
	HWND _hMainWnd = nullptr;

	std::unordered_map<std::wstring, std::shared_ptr<IWindow>> _windows;
};

template <typename T>
std::shared_ptr<T> WindowManager::RegisterWindow(const std::wstring& name)
{
	auto window = std::make_shared<T>();
	if (window->Create(_hInstance, _hMainWnd))
	{
		_windows[name] = window;
	}
	return window;
}

template <typename T>
std::shared_ptr<T> WindowManager::GetWindow(const std::wstring& name)
{
	auto it = _windows.find(name);
	if (it != _windows.end())
		return std::static_pointer_cast<T>(it->second);
	return nullptr;
}
