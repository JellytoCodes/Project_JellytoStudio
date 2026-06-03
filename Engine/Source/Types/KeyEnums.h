#pragma once

enum class KEY_TYPE
{
    LBUTTON  = VK_LBUTTON,
    RBUTTON  = VK_RBUTTON,
    MBUTTON  = VK_MBUTTON,

    BACK     = VK_BACK,
    TAB      = VK_TAB,
    ENTER    = VK_RETURN,
    SHIFT    = VK_SHIFT,
    CTRL     = VK_CONTROL,
    ALT      = VK_MENU,
    ESCAPE   = VK_ESCAPE,
    SPACE    = VK_SPACE,

    LEFT     = VK_LEFT,
    RIGHT    = VK_RIGHT,
    UP       = VK_UP,
    DOWN     = VK_DOWN,

    INS      = VK_INSERT,
    DEL      = VK_DELETE,
    HOME     = VK_HOME,
    END      = VK_END,
    PAGEUP   = VK_PRIOR,
    PAGEDOWN = VK_NEXT,

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
    KEY_TYPE_COUNT  = static_cast<int32>(UINT8_MAX + 1),
    KEY_STATE_COUNT = static_cast<int32>(KEY_STATE::END),
};
