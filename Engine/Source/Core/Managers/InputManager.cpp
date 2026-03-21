#include "Framework.h"
#include "InputManager.h"

void InputManager::Init(HWND hwnd)
{
	_hwnd = hwnd;
	_states.resize(KEY_TYPE_COUNT, KEY_STATE::NONE);
}

void InputManager::Update()
{
	HWND activeHwnd = ::GetActiveWindow();

	// 메인 윈도우 또는 허용된 서브 윈도우가 활성화된 경우에만 입력 처리
	bool isAllowed = (activeHwnd == _hwnd);
	if (!isAllowed)
	{
		for (HWND allowed : _allowedWindows)
			if (activeHwnd == allowed) { isAllowed = true; break; }
	}

	if (!isAllowed)
	{
		for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
			_states[key] = KEY_STATE::NONE;
		return;
	}

	BYTE asciiKeys[KEY_TYPE_COUNT] = {};
	if (::GetKeyboardState(asciiKeys) == false)
		return;

	for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
	{
		if (asciiKeys[key] & 0x80)
		{
			KEY_STATE& state = _states[key];

			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::PRESS;
			else
				state = KEY_STATE::DOWN;
		}
		else
		{
			KEY_STATE& state = _states[key];

			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::UP;
			else
				state = KEY_STATE::NONE;
		}
	}

	POINT curPos;
	::GetCursorPos(&curPos);
	::ScreenToClient(_hwnd, &curPos);

	_mouseDelta.x = curPos.x - _mousePos.x;
	_mouseDelta.y = curPos.y - _mousePos.y;
	_mousePos = curPos;
}