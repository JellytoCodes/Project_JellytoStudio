#pragma once

#include "Types/KeyEnums.h"

class InputManager
{
	DECLARE_SINGLE(InputManager);

public:
	void Init(HWND hwnd);
	void Update();

	void AddAllowedWindow(HWND hwnd)
	{
		if (hwnd) _allowedWindows.push_back(hwnd);
	}

	bool IsMainWindowActive() const { return ::GetActiveWindow() == _hwnd; }

	bool GetButton(KEY_TYPE key) { return GetState(key) == KEY_STATE::PRESS; }

	bool GetButtonDown(KEY_TYPE key) { return GetState(key) == KEY_STATE::DOWN; }

	bool GetButtonUp(KEY_TYPE key) { return GetState(key) == KEY_STATE::UP; }

	const POINT& GetMousePos() const { return _mousePos; }
	const POINT& GetMouseDelta() const { return _mouseDelta; }

private:
	inline KEY_STATE GetState(KEY_TYPE key) { return _states[static_cast<uint8>(key)]; }

	HWND _hwnd = nullptr;
	std::vector<HWND> _allowedWindows;
	std::vector<KEY_STATE> _states;
	POINT _mousePos = {};
	POINT _mouseDelta = {};
};