#include "Framework.h"
#include "ChunkDebugWindow.h"

#include "Scene/ChunkManager.h"
#include "Pipeline/DynamicInstancePool.h"
#include "Entity/Entity.h"

namespace
{
    std::wstring FmtInt(int32 v)
    {
        wchar_t raw[32]; swprintf_s(raw, L"%d", v);
        std::wstring s = raw;
        int pos = (int)s.size() - 3;
        while (pos > 0) { s.insert(pos, L","); pos -= 3; }
        return s;
    }

    std::wstring ProgressBar(float ratio, int width = 16)
    {
        ratio = std::clamp(ratio, 0.f, 1.f);
        const int filled = static_cast<int>(ratio * width);
        std::wstring bar(filled, L'█');
        bar += std::wstring(width - filled, L'░');
        wchar_t pct[16]; swprintf_s(pct, L"  %.1f%%", ratio * 100.f);
        return bar + pct;
    }

}

bool ChunkDebugWindow::Create(HINSTANCE hInstance, HWND hMainWnd)
{
    if (_created) return true;
    _hInstance = hInstance;

    RegisterWindowClass(hInstance);

    RECT mainRect = {};
    if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
    const int x = hMainWnd ? mainRect.right + 376 : CW_USEDEFAULT;
    const int y = hMainWnd ? mainRect.top          : CW_USEDEFAULT;

    RECT wr = { 0, 0, 420, 680 };
    ::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    _hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio — Chunk / Instancing Debug",
        WS_OVERLAPPEDWINDOW,
        x, y, wr.right - wr.left, wr.bottom - wr.top,
        hMainWnd, nullptr, hInstance, this);

    if (!_hWnd) return false;

    BuildUI();
    _created = true;
    return true;
}

void ChunkDebugWindow::Show()
{
    if (!_hWnd) return;
    ::ShowWindow(_hWnd, SW_SHOW);
    ::SetForegroundWindow(_hWnd);
    _visible = true;
}

void ChunkDebugWindow::Hide()
{
    if (!_hWnd) return;
    ::ShowWindow(_hWnd, SW_HIDE);
    _visible = false;
}

void ChunkDebugWindow::Toggle() { _visible ? Hide() : Show(); }

void ChunkDebugWindow::BuildUI()
{
    constexpr int W  = 390;
    constexpr int LX = 10;
    constexpr int VX = 200;
    constexpr int LW = 185;
    constexpr int VW = 200;
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

    MkSep(L"▶  Frustum Culling  [갱신: 0.25초마다]", y); y += RH + 6;

    MkL(L"Total Chunks",          y); MkV(_hTotalChunks,   L"—", y); y += RS;
    MkL(L"Visible Chunks",        y); MkV(_hVisibleChunks, L"—", y); y += RS;
    MkL(L"Culled Chunks",         y); MkV(_hCulledChunks,  L"—", y); y += RS;
    MkL(L"Cull Rate (Bar)",       y); MkV(_hCullRate,      L"—", y); y += RS + 10;

    MkSep(L"▶  Instancing  (DynamicInstancePool)", y); y += RH + 6;

    MkL(L"사용 인스턴스",         y); MkV(_hUsedInstances, L"—", y); y += RS;
    MkL(L"Ring Buffer 슬롯",      y); MkV(_hRingSlot,      L"—", y); y += RS;
    MkL(L"Pool 사용률",           y); MkV(_hPoolPct,       L"—", y); y += RS + 10;

    MkSep(L"▶  선택된 Entity → 청크  (Inspector와 연동)", y); y += RH + 6;

    _hSelNone = MkL(L"(Inspector에서 Entity를 선택하세요)", y); y += RS + 2;

    MkL(L"청크 좌표  (CX, CZ)",   y); MkV(_hSelCoord,      L"—", y); y += RS;
    MkL(L"청크 내 Entity 수",      y); MkV(_hSelEntCount,   L"—", y); y += RS;
    MkL(L"가시 여부",              y); MkV(_hSelVisible,    L"—", y); y += RS;
    MkL(L"AABB Center (X,Y,Z)",   y); MkV(_hSelAABBCenter, L"—", y); y += RS;
    MkL(L"AABB Extents (X,Y,Z)",  y); MkV(_hSelAABBExt,    L"—", y); y += RS + 10;

    MkSep(L"▶  청크 전체 목록  (CX,CZ | 엔티티 | 가시 | AABB.Center)", y);
    y += RH + 6;

    _hChunkList = ::CreateWindowW(L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        LBS_NOINTEGRALHEIGHT | LBS_NOSEL,
        LX, y, W, 180, _hWnd, nullptr, _hInstance, nullptr);

    HFONT hMono = ::CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Consolas");
    if (hMono)
        ::SendMessage(_hChunkList, WM_SETFONT, (WPARAM)hMono, TRUE);
}

void ChunkDebugWindow::Refresh(Entity* selectedEntity)
{
    if (!_visible || !_hWnd) return;

    RefreshCullingSection();
    RefreshInstancingSection();
    RefreshSelectedSection(selectedEntity);
    RefreshChunkList();
}

void ChunkDebugWindow::RefreshCullingSection()
{
    ChunkManager* cm = GET_SINGLE(ChunkManager);

    const int32 total   = cm->GetChunkCount();
    const int32 visible = cm->GetVisibleChunkCount();
    const int32 culled  = total - visible;
    const float cullPct = (total > 0)
        ? static_cast<float>(culled) / static_cast<float>(total)
        : 0.f;

    auto Set = [](HWND h, const std::wstring& s) { ::SetWindowTextW(h, s.c_str()); };

    Set(_hTotalChunks,   FmtInt(total));
    Set(_hVisibleChunks, FmtInt(visible) + L"개");
    Set(_hCulledChunks,  FmtInt(culled)  + L"개  (" +
        [&]{ wchar_t b[16]; swprintf_s(b, L"%.1f%%", cullPct * 100.f); return std::wstring(b); }() + L")");
    Set(_hCullRate,      ProgressBar(cullPct, 14));
}

void ChunkDebugWindow::RefreshInstancingSection()
{
    DynamicInstancePool* pool = GET_SINGLE(DynamicInstancePool);
    if (!pool->IsReady()) return;

    const uint32 used    = pool->GetUsedInstances();
    const uint32 maxInst = DynamicInstancePool::kMaxInstances;
    const uint32 slot    = pool->GetCurrentSlot();
    const uint32 ringN   = DynamicInstancePool::kRingCount;
    const float  pct     = static_cast<float>(used) / static_cast<float>(maxInst);

    auto Set = [](HWND h, const std::wstring& s) { ::SetWindowTextW(h, s.c_str()); };

    Set(_hUsedInstances,
        FmtInt(static_cast<int32>(used)) + L" / " + FmtInt(static_cast<int32>(maxInst)));

    wchar_t slotBuf[32];
    swprintf_s(slotBuf, L"슬롯 %u / %u  (3-way Ring Buffer)", slot, ringN);
    Set(_hRingSlot, slotBuf);

    wchar_t pctBuf[32];
    swprintf_s(pctBuf, L"%.2f %%", pct * 100.f);
    Set(_hPoolPct, pctBuf);
}

void ChunkDebugWindow::RefreshSelectedSection(Entity* selectedEntity)
{
    auto Set = [](HWND h, const std::wstring& s) { if (h) ::SetWindowTextW(h, s.c_str()); };

    if (!selectedEntity)
    {
        ::SetWindowTextW(_hSelNone, L"(Inspector에서 Entity를 선택하세요)");
        Set(_hSelCoord,      L"—");
        Set(_hSelEntCount,   L"—");
        Set(_hSelVisible,    L"—");
        Set(_hSelAABBCenter, L"—");
        Set(_hSelAABBExt,    L"—");
        return;
    }

    ChunkSnapshot snap;
    if (!GET_SINGLE(ChunkManager)->TryGetChunkSnapshot(selectedEntity, snap))
    {
        ::SetWindowTextW(_hSelNone,
            (L"'" + selectedEntity->GetEntityName() + L"' — ChunkManager 미등록").c_str());
        Set(_hSelCoord,      L"—");
        Set(_hSelEntCount,   L"—");
        Set(_hSelVisible,    L"—");
        Set(_hSelAABBCenter, L"—");
        Set(_hSelAABBExt,    L"—");
        return;
    }

    wchar_t buf[128];

    ::SetWindowTextW(_hSelNone,
        (L"선택됨: '" + selectedEntity->GetEntityName() + L"'").c_str());

    swprintf_s(buf, L"(%d,  %d)  — 청크 크기 %.0f유닛",
               snap.cx, snap.cz, ChunkManager::kChunkSize);
    Set(_hSelCoord, buf);

    swprintf_s(buf, L"%d 개", snap.entityCount);
    Set(_hSelEntCount, buf);

    Set(_hSelVisible, snap.wasVisible
        ? L"✔  예  (가시 — 렌더링 대상)"
        : L"✕  아니요  (컬링됨 — Draw 제외)");

    swprintf_s(buf, L"(%.1f,  %.1f,  %.1f)",
               snap.aabb.Center.x, snap.aabb.Center.y, snap.aabb.Center.z);
    Set(_hSelAABBCenter, buf);

    swprintf_s(buf, L"(%.1f,  %.1f,  %.1f)",
               snap.aabb.Extents.x, snap.aabb.Extents.y, snap.aabb.Extents.z);
    Set(_hSelAABBExt, buf);
}

void ChunkDebugWindow::RefreshChunkList()
{
    if (!_hChunkList) return;

    const int prevTop = (int)::SendMessage(_hChunkList, LB_GETTOPINDEX, 0, 0);
    ::SendMessage(_hChunkList, LB_RESETCONTENT, 0, 0);

    const auto snapshots = GET_SINGLE(ChunkManager)->GetChunkSnapshots();

    for (const ChunkSnapshot& s : snapshots)
    {
        wchar_t line[128];
        swprintf_s(line,
            L"(%3d,%3d) | %4d엔티티 | %s | C=(%.0f,%.0f,%.0f)",
            s.cx, s.cz,
            s.entityCount,
            s.wasVisible ? L"✔ 가시" : L"✕ 컬링",
            s.aabb.Center.x, s.aabb.Center.y, s.aabb.Center.z);
        ::SendMessage(_hChunkList, LB_ADDSTRING, 0, (LPARAM)line);
    }

    if (prevTop > 0)
        ::SendMessage(_hChunkList, LB_SETTOPINDEX, prevTop, 0);
}

void ChunkDebugWindow::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex   = {};
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = ChunkDebugWindow::WndProc;
    wcex.hInstance     = hInstance;
    wcex.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = CLASS_NAME;
    ::RegisterClassExW(&wcex);
}

LRESULT CALLBACK ChunkDebugWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA,
                           reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

    ChunkDebugWindow* self = reinterpret_cast<ChunkDebugWindow*>(
        ::GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (self && msg == WM_CLOSE)
    {
        self->Hide();
        return 0;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}