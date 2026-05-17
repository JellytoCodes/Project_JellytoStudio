#include "pch.h"
#include "BlockTestPanel.h"

#include "Data/BlockTable.h"

namespace
{
    std::wstring ColliderStr(ColliderSize s)
    {
        switch (s)
        {
        case ColliderSize::Small: return L"Small";
        case ColliderSize::Tall:  return L"Tall";
        case ColliderSize::Wide:  return L"Wide";
        default:                  return L"Unit";
        }
    }

    std::wstring ChannelStr(CollisionChannel ch)
    {
        switch (ch)
        {
        case CollisionChannel::Character: return L"Character";
        case CollisionChannel::Priming:   return L"Priming";
        case CollisionChannel::Mushroom:  return L"Mushroom";
        case CollisionChannel::Floor:     return L"Floor";
        default:                          return L"Default";
        }
    }

    std::wstring MaskStr(uint8 mask)
    {
        if (mask == 0)    return L"None";
        if (mask == 0xFF) return L"All";

        static const std::pair<CollisionChannel, const wchar_t*> kMap[] = {
            { CollisionChannel::Default,   L"Default"   },
            { CollisionChannel::Character, L"Character" },
            { CollisionChannel::Priming,   L"Priming"   },
            { CollisionChannel::Mushroom,  L"Mushroom"  },
            { CollisionChannel::Floor,     L"Floor"     },
        };
        std::wstring r;
        for (auto& [ch, name] : kMap)
            if (ChannelInMask(ch, mask)) { if (!r.empty()) r += L" | "; r += name; }
        return r.empty() ? L"None" : r;
    }

    std::wstring FaceStr(uint8 mask)
    {
        if (mask == 0)    return L"None";
        if (mask == 0xFF) return L"All";

        std::wstring r;
        if (mask & static_cast<uint8>(PlaceFace::Top))    r += L"Top";
        if (mask & static_cast<uint8>(PlaceFace::Side))   { if (!r.empty()) r += L" | "; r += L"Side";   }
        if (mask & static_cast<uint8>(PlaceFace::Bottom)) { if (!r.empty()) r += L" | "; r += L"Bottom"; }
        return r.empty() ? L"None" : r;
    }

    std::wstring RenderTypeStr(BlockRenderType t)
    {
        return t == BlockRenderType::Model ? L"Model" : L"Mesh";
    }
}

bool BlockTestPanel::Create(HINSTANCE hInstance, HWND hMainWnd)
{
    if (_created) return true;
    _hInstance = hInstance;

    RegisterWindowClass(hInstance);

    RECT mainRect = {};
    if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
    const int x = hMainWnd ? mainRect.right + 8  : CW_USEDEFAULT;
    const int y = hMainWnd ? mainRect.top  + 400 : CW_USEDEFAULT;

    RECT wr = { 0, 0, 400, 600 };
    ::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    _hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio — Block Test Panel",
        WS_OVERLAPPEDWINDOW,
        x, y, wr.right - wr.left, wr.bottom - wr.top,
        hMainWnd, nullptr, hInstance, this);

    if (!_hWnd) return false;

    BuildUI();
    _created = true;
    return true;
}

void BlockTestPanel::Show()
{
    if (!_hWnd) return;
    ::ShowWindow(_hWnd, SW_SHOW);
    ::SetForegroundWindow(_hWnd);
    _visible = true;
}

void BlockTestPanel::Hide()
{
    if (!_hWnd) return;
    ::ShowWindow(_hWnd, SW_HIDE);
    _visible = false;
}

void BlockTestPanel::Toggle() { _visible ? Hide() : Show(); }

void BlockTestPanel::BuildUI()
{
    constexpr int W  = 370;
    constexpr int LX = 10;
    constexpr int VX = 140;
    constexpr int LW = 125;
    constexpr int VW = 230;
    constexpr int RH = 17;
    constexpr int RS = 21;

    int y = 8;

    auto MkL = [&](const wchar_t* txt, int yy) -> HWND
    {
        return ::CreateWindowW(L"STATIC", txt,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            LX, yy, LW, RH, _hWnd, nullptr, _hInstance, nullptr);
    };
    auto MkV = [&](HWND& out, const wchar_t* def, int yy)
    {
        out = ::CreateWindowW(L"STATIC", def,
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN,
            VX, yy, VW, RH, _hWnd, nullptr, _hInstance, nullptr);
    };
    auto MkSep = [&](const wchar_t* title, int yy)
    {
        ::CreateWindowW(L"STATIC", title,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            LX, yy, W, RH, _hWnd, nullptr, _hInstance, nullptr);
        ::CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            LX, yy + RH + 1, W, 2, _hWnd, nullptr, _hInstance, nullptr);
    };

    MkSep(L"▶  Block List  (F3 토글)", y); y += RH + 6;

    _hCountLabel = MkL(L"전체 0개", y); y += RS;

    _hList = ::CreateWindowW(L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        LX, y, W, 150, _hWnd,
        (HMENU)(INT_PTR)ID_LIST_BLOCK, _hInstance, nullptr);
    y += 158;

    MkSep(L"▶  선택된 블록 상세", y); y += RH + 6;

    MkL(L"ID",           y); MkV(_hValId,         L"—", y); y += RS;
    MkL(L"Key",          y); MkV(_hValKey,         L"—", y); y += RS;
    MkL(L"Label",        y); MkV(_hValLabel,       L"—", y); y += RS;
    MkL(L"Eraser",       y); MkV(_hValEraser,      L"—", y); y += RS;
    MkL(L"RenderType",   y); MkV(_hValRenderType,  L"—", y); y += RS;
    MkL(L"ModelName",    y); MkV(_hValModelName,   L"—", y); y += RS;
    MkL(L"ModelScale",   y); MkV(_hValModelScale,  L"—", y); y += RS;
    MkL(L"Collider",     y); MkV(_hValCollider,    L"—", y); y += RS;
    MkL(L"OwnChannel",   y); MkV(_hValOwnChannel,  L"—", y); y += RS;
    MkL(L"Pickable",     y); MkV(_hValPickable,    L"—", y); y += RS;
    MkL(L"Faces",        y); MkV(_hValFaces,       L"—", y); y += RS;
    MkL(L"Color (RGBA)", y); MkV(_hValColor,       L"—", y);
}

void BlockTestPanel::Load()
{
    if (!GET_SINGLE(BlockTable)->IsLoaded()) return;
    PopulateList();
}

void BlockTestPanel::PopulateList()
{
    if (!_hList) return;
    ::SendMessage(_hList, LB_RESETCONTENT, 0, 0);

    const auto& records = GET_SINGLE(BlockTable)->GetAllRecords();

    for (const BlockRecord& rec : records)
    {
        if (rec.key.empty()) continue;
        std::wstring line = L"[" + std::to_wstring(rec.typeId) + L"]  "
            + rec.key + L"  —  "
            + RenderTypeStr(rec.renderType)
            + L" / " + ChannelStr(rec.ownChannel);
        ::SendMessage(_hList, LB_ADDSTRING, 0, (LPARAM)line.c_str());
    }

    wchar_t buf[32];
    swprintf_s(buf, L"전체 %d개", (int)records.size());
    ::SetWindowTextW(_hCountLabel, buf);
}

void BlockTestPanel::ShowRecord(int listIdx)
{
    const auto& records = GET_SINGLE(BlockTable)->GetAllRecords();

    int realIdx = -1;
    int count   = 0;
    for (int i = 0; i < (int)records.size(); ++i)
    {
        if (records[i].key.empty()) continue;
        if (count == listIdx) { realIdx = i; break; }
        ++count;
    }

    if (realIdx < 0 || realIdx >= (int)records.size()) return;
    const BlockRecord& rec = records[realIdx];

    auto Set = [](HWND h, const std::wstring& s) { ::SetWindowTextW(h, s.c_str()); };

    Set(_hValId,        std::to_wstring(rec.typeId));
    Set(_hValKey,       rec.key);
    Set(_hValLabel,     rec.label);
    Set(_hValEraser,    rec.isEraser ? L"예" : L"아니요");
    Set(_hValRenderType, RenderTypeStr(rec.renderType));
    Set(_hValModelName,  rec.modelName.empty() ? L"—" : rec.modelName);

    wchar_t scaleBuf[32];
    swprintf_s(scaleBuf, L"%.4f", rec.modelScale);
    Set(_hValModelScale, scaleBuf);

    Set(_hValCollider,   ColliderStr(rec.collider));
    Set(_hValOwnChannel, ChannelStr(rec.ownChannel));
    Set(_hValPickable,   MaskStr(rec.pickableMask));
    Set(_hValFaces,      FaceStr(rec.faceMask));

    wchar_t colorBuf[64];
    swprintf_s(colorBuf, L"(%.2f, %.2f, %.2f, %.2f)",
        rec.color.x, rec.color.y, rec.color.z, rec.color.w);
    Set(_hValColor, colorBuf);
}

void BlockTestPanel::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex   = {};
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = BlockTestPanel::WndProc;
    wcex.hInstance     = hInstance;
    wcex.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = CLASS_NAME;
    ::RegisterClassExW(&wcex);
}

LRESULT CALLBACK BlockTestPanel::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA,
                           reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

    BlockTestPanel* self = reinterpret_cast<BlockTestPanel*>(
        ::GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (self)
    {
        switch (msg)
        {
        case WM_CLOSE:
            self->Hide();
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_LIST_BLOCK && HIWORD(wParam) == LBN_SELCHANGE)
            {
                const int sel = (int)::SendMessage(self->_hList, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) self->ShowRecord(sel);
                return 0;
            }
            break;
        }
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}