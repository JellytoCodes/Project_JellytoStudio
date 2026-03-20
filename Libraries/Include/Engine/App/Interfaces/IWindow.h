#pragma once

class IWindow
{
public:
	virtual bool Create(HINSTANCE hInstance, HWND hMainWnd) = 0;
	virtual void Show() = 0;
	virtual void Hide() = 0;
	virtual void Toggle() = 0;

	virtual bool IsVisible() const = 0;
	virtual HWND GetHWnd() const = 0;
};
