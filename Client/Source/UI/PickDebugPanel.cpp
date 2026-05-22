#include "pch.h"
#include "PickDebugPanel.h"

#include "Scripts/BlockPlacer.h"

bool PickDebugPanel::Create(HINSTANCE hInstance, HWND hMainWnd)
{
    if (_created) return true;
    _hInstance = hInstance;

    RegisterWindowClass(hInstance);

    RECT mainRect = {};
    if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
    const int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
    const int y = hMainWnd ? mainRect.top + 260 : CW_USEDEFAULT;

    RECT wr = { 0, 0, 430, 380 };
    ::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    _hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio - Pick / Collision Debug",
        WS_OVERLAPPEDWINDOW,
        x, y, wr.right - wr.left, wr.bottom - wr.top,
        hMainWnd, nullptr, hInstance, this);

    if (!_hWnd) return false;

    BuildUI();
    _created = true;
    return true;
}

void PickDebugPanel::Show()
{
    if (!_hWnd) return;
    ::ShowWindow(_hWnd, SW_SHOW);
    ::SetForegroundWindow(_hWnd);
    _visible = true;
}

void PickDebugPanel::Hide()
{
    if (!_hWnd) return;
    ::ShowWindow(_hWnd, SW_HIDE);
    _visible = false;
}

void PickDebugPanel::Toggle()
{
    _visible ? Hide() : Show();
}

void PickDebugPanel::BuildUI()
{
    constexpr int W  = 400;
    constexpr int LX = 10;
    constexpr int VX = 150;
    constexpr int LW = 135;
    constexpr int VW = 255;
    constexpr int RH = 18;
    constexpr int RS = 23;

    int y = 8;

    auto MkSep = [&](const wchar_t* title, int yy)
    {
        ::CreateWindowW(L"STATIC", title,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            LX, yy, W, RH, _hWnd, nullptr, _hInstance, nullptr);
        ::CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            LX, yy + RH + 1, W, 2, _hWnd, nullptr, _hInstance, nullptr);
    };

    auto MkL = [&](const wchar_t* txt, int yy)
    {
        ::CreateWindowW(L"STATIC", txt,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            LX, yy, LW, RH, _hWnd, nullptr, _hInstance, nullptr);
    };

    auto MkV = [&](HWND& out, const wchar_t* def, int yy)
    {
        out = ::CreateWindowW(L"STATIC", def,
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN,
            VX, yy, VW, RH, _hWnd, nullptr, _hInstance, nullptr);
    };

    MkSep(L">  Placement Decision", y); y += RH + 6;
    MkL(L"Mode",       y); MkV(_hValMode,    L"-", y); y += RS;
    MkL(L"Mouse",      y); MkV(_hValMouse,   L"-", y); y += RS;
    MkL(L"Slot",       y); MkV(_hValSlot,    L"-", y); y += RS;
    MkL(L"Result",     y); MkV(_hValResult,  L"-", y); y += RS;
    MkL(L"Reason",     y); MkV(_hValReason,  L"-", y); y += RS;
    MkL(L"Channel",    y); MkV(_hValChannel, L"-", y); y += RS;
    MkL(L"Target Pos", y); MkV(_hValTarget,  L"-", y); y += RS;
    MkL(L"Inventory",  y); MkV(_hValStock,   L"-", y); y += RS + 8;

    MkSep(L">  Query Hits", y); y += RH + 6;
    MkL(L"Priming",  y); MkV(_hValPriming,  L"-", y); y += RS;
    MkL(L"Floor",    y); MkV(_hValFloor,    L"-", y); y += RS;
    MkL(L"Mushroom", y); MkV(_hValMushroom, L"-", y); y += RS;
}

void PickDebugPanel::Refresh()
{
    if (!_visible || !_hWnd) return;

    auto SetW = [](HWND h, const std::wstring& s)
    {
        if (h) ::SetWindowTextW(h, s.c_str());
    };

    auto VecStr = [](const Vec3& v) -> std::wstring
    {
        wchar_t buf[96];
        swprintf_s(buf, L"%.2f, %.2f, %.2f", v.x, v.y, v.z);
        return buf;
    };

    auto HitStr = [&](const BlockPlacer::PickDebugInfo::HitInfo& hit) -> std::wstring
    {
        if (!hit.valid) return L"No Hit";
        wchar_t buf[256];
        swprintf_s(buf, L"%s  d=%.2f  n=(%.1f,%.1f,%.1f)  %s",
            hit.entityName.c_str(), hit.dist,
            hit.normal.x, hit.normal.y, hit.normal.z,
            hit.face.c_str());
        return buf;
    };

    if (!_placer)
    {
        SetW(_hValMode, L"No BlockPlacer");
        return;
    }

    const auto& info = _placer->GetPickDebugInfo();

    wchar_t mouseBuf[64];
    swprintf_s(mouseBuf, L"%ld, %ld", info.mousePos.x, info.mousePos.y);

    SetW(_hValMode, info.placingMode ? L"Placing On" : L"Placing Off");
    SetW(_hValMouse, mouseBuf);
    SetW(_hValSlot, info.selectedSlot);
    SetW(_hValResult, info.result);
    SetW(_hValReason, info.rejectReason);
    SetW(_hValChannel, info.selectedChannel.empty() ? L"None" : info.selectedChannel);
    SetW(_hValTarget, info.canPlace ? VecStr(info.resolvedPos) : L"-");
    SetW(_hValStock, info.hasStock ? L"OK" : L"Empty");
    SetW(_hValPriming, HitStr(info.priming));
    SetW(_hValFloor, HitStr(info.floor));
    SetW(_hValMushroom, HitStr(info.mushroom));
}

void PickDebugPanel::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex   = {};
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = PickDebugPanel::WndProc;
    wcex.hInstance     = hInstance;
    wcex.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = CLASS_NAME;
    ::RegisterClassExW(&wcex);
}

LRESULT CALLBACK PickDebugPanel::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

    PickDebugPanel* self = reinterpret_cast<PickDebugPanel*>(
        ::GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (self && msg == WM_CLOSE)
    {
        self->Hide();
        return 0;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
