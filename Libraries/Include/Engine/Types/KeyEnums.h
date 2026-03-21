#pragma once

// ============================================================
//  KeyEnums.h
//  위치: Engine/Source/Core/Managers/
//  InputManager.h에서 #include "KeyEnums.h" 로 사용
//  Windows VK 코드 기반 전체 키 등록
// ============================================================

enum class KEY_TYPE
{
    // ── 마우스 ───────────────────────────────────────────────
    LBUTTON  = VK_LBUTTON,   // 마우스 좌클릭
    RBUTTON  = VK_RBUTTON,   // 마우스 우클릭
    MBUTTON  = VK_MBUTTON,   // 마우스 휠 클릭

    // ── 기능키 ───────────────────────────────────────────────
    BACK     = VK_BACK,      // Backspace
    TAB      = VK_TAB,
    ENTER    = VK_RETURN,
    SHIFT    = VK_SHIFT,
    CTRL     = VK_CONTROL,
    ALT      = VK_MENU,
    ESCAPE   = VK_ESCAPE,
    SPACE    = VK_SPACE,

    // ── 방향키 ───────────────────────────────────────────────
    LEFT     = VK_LEFT,
    RIGHT    = VK_RIGHT,
    UP       = VK_UP,
    DOWN     = VK_DOWN,

    // ── 편집키 ───────────────────────────────────────────────
    INS      = VK_INSERT,
    DEL      = VK_DELETE,
    HOME     = VK_HOME,
    END      = VK_END,
    PAGEUP   = VK_PRIOR,
    PAGEDOWN = VK_NEXT,

    // ── F 키 ─────────────────────────────────────────────────
    F1  = VK_F1,
    F2  = VK_F2,
    F3  = VK_F3,
    F4  = VK_F4,
    F5  = VK_F5,
    F6  = VK_F6,
    F7  = VK_F7,
    F8  = VK_F8,
    F9  = VK_F9,
    F10 = VK_F10,
    F11 = VK_F11,
    F12 = VK_F12,

    // ── 숫자 (상단) ──────────────────────────────────────────
    KEY_0 = '0',
    KEY_1 = '1',
    KEY_2 = '2',
    KEY_3 = '3',
    KEY_4 = '4',
    KEY_5 = '5',
    KEY_6 = '6',
    KEY_7 = '7',
    KEY_8 = '8',
    KEY_9 = '9',

    // ── 알파벳 ───────────────────────────────────────────────
    A = 'A',
    B = 'B',
    C = 'C',
    D = 'D',
    E = 'E',
    F = 'F',
    G = 'G',
    H = 'H',
    I = 'I',
    J = 'J',
    K = 'K',
    L = 'L',
    M = 'M',
    N = 'N',
    O = 'O',
    P = 'P',
    Q = 'Q',
    R = 'R',
    S = 'S',
    T = 'T',
    U = 'U',
    V = 'V',
    W = 'W',
    X = 'X',
    Y = 'Y',
    Z = 'Z',

    // ── 숫자패드 ─────────────────────────────────────────────
    NUM0 = VK_NUMPAD0,
    NUM1 = VK_NUMPAD1,
    NUM2 = VK_NUMPAD2,
    NUM3 = VK_NUMPAD3,
    NUM4 = VK_NUMPAD4,
    NUM5 = VK_NUMPAD5,
    NUM6 = VK_NUMPAD6,
    NUM7 = VK_NUMPAD7,
    NUM8 = VK_NUMPAD8,
    NUM9 = VK_NUMPAD9,
};

enum class KEY_STATE
{
	NONE,
	PRESS,
	DOWN,
	UP,
	END
};

enum
{
	KEY_TYPE_COUNT = static_cast<int32>(UINT8_MAX + 1),
	KEY_STATE_COUNT = static_cast<int32>(KEY_STATE::END),
};