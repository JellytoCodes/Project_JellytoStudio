#pragma once

#include "Types/KeyEnums.h"

class InputManager
{
	DECLARE_SINGLE(InputManager);

public:
	void Init(HWND hwnd);
	void Update();

	// 메인 윈도우 외에도 입력을 허용할 서브 윈도우 등록
	// (e.g. DetailWindow가 포커스를 가져도 키 입력 유지)
	void AddAllowedWindow(HWND hwnd)
	{
		if (hwnd) _allowedWindows.push_back(hwnd);
	}

	// 메인 윈도우가 현재 활성(포커스) 상태인지 반환
	// Picking 등 메인 창 전용 입력 처리 시 사용
	bool IsMainWindowActive() const { return ::GetActiveWindow() == _hwnd; }

	// 누르고 있을 때
	bool GetButton(KEY_TYPE key) { return GetState(key) == KEY_STATE::PRESS; }

	// 맨 처음 눌렀을 때
	bool GetButtonDown(KEY_TYPE key) { return GetState(key) == KEY_STATE::DOWN; }

	// 맨 처음 눌렀다 뗐을 때
	bool GetButtonUp(KEY_TYPE key) { return GetState(key) == KEY_STATE::UP; }

	const POINT& GetMousePos() const { return _mousePos; }
	const POINT& GetMouseDelta() const { return _mouseDelta; }

private:
	inline KEY_STATE GetState(KEY_TYPE key) { return _states[static_cast<uint8>(key)]; }

	HWND _hwnd;
	std::vector<HWND> _allowedWindows; // 메인 외 입력 허용 윈도우 목록
	std::vector<KEY_STATE> _states;
	POINT _mousePos = {};
	POINT _mouseDelta = {};
};