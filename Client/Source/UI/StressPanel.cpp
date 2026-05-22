#include "pch.h"
#include "StressPanel.h"

#include "Scene/ChunkManager.h"
#include "Pipeline/DynamicInstancePool.h"
#include "Graphics/Managers/InstancingManager.h"
#include "Core/Managers/TimeManager.h"
#include "Data/BlockTable.h"

#include <filesystem>
#include <fstream>

bool StressPanel::Create(HINSTANCE hInstance, HWND hMainWnd)
{
    if (_created) return true;
    _hInstance = hInstance;

    RegisterWindowClass(hInstance);

    RECT mainRect = {};
    if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
    const int x = hMainWnd ? mainRect.right + 8   : CW_USEDEFAULT;
    const int y = hMainWnd ? mainRect.top  + 700  : CW_USEDEFAULT;

    RECT wr = { 0, 0, 380, 520 };
    ::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    _hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio — Stress Test",
        WS_OVERLAPPEDWINDOW,
        x, y, wr.right - wr.left, wr.bottom - wr.top,
        hMainWnd, nullptr, hInstance, this);

    if (!_hWnd) return false;

    BuildUI();
    _created = true;
    return true;
}

void StressPanel::Show()
{
    if (!_hWnd) return;
    ::ShowWindow(_hWnd, SW_SHOW);
    ::SetForegroundWindow(_hWnd);
    _visible = true;
}

void StressPanel::Hide()
{
    if (!_hWnd) return;
    ::ShowWindow(_hWnd, SW_HIDE);
    _visible = false;
}

void StressPanel::Toggle() { _visible ? Hide() : Show(); }

void StressPanel::BuildUI()
{
    constexpr int W  = 350;
    constexpr int BW = 108;
    constexpr int BH = 28;
    constexpr int LX = 10;
    constexpr int VX = 180;
    constexpr int LW = 165;
    constexpr int VW = 175;
    constexpr int RH = 17;
    constexpr int RS = 21;

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
    auto MkBtn = [&](const wchar_t* txt, int x, int yy, int w, int id)
    {
        ::CreateWindowW(L"BUTTON", txt,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            x, yy, w, BH, _hWnd,
            (HMENU)(INT_PTR)id, _hInstance, nullptr);
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

    MkSep(L"▶  추가 생성  (현재 씬에 누적)", y); y += RH + 6;
    MkBtn(L"+100",    LX,           y, BW, ID_BTN_SPAWN100);
    MkBtn(L"+1,000",  LX + BW + 4,  y, BW, ID_BTN_SPAWN1K);
    MkBtn(L"+10,000", LX + (BW+4)*2, y, BW, ID_BTN_SPAWN10K);
    y += BH + 10;

    MkSep(L"▶  재현 프리셋  (Clear 후 생성)", y); y += RH + 6;
    MkBtn(L"Grid 10K",        LX,           y, BW, ID_BTN_GRID10K);
    MkBtn(L"Seed Random 10K", LX + BW + 4,  y, BW + 20, ID_BTN_RANDOM10K);
    y += BH + 10;

    MkSep(L"▶  삭제", y); y += RH + 6;
    MkBtn(L"Clear All",       LX,            y, BW + 20, ID_BTN_CLEAR);
    MkBtn(L"Random 10% 삭제", LX + BW + 24,  y, BW + 20, ID_BTN_DEL10);
    y += BH + 10;

    MkSep(L"▶  현재 통계  (0.25초 갱신)", y); y += RH + 6;

    MkL(L"배치 블록 수",      y); MkV(_hValBlockCount,  L"—", y); y += RS;
    MkL(L"추정 Draw Calls",   y); MkV(_hValDrawCalls,   L"—", y); y += RS;
    MkL(L"사용 인스턴스",     y); MkV(_hValInstances,   L"—", y); y += RS;
    MkL(L"Ring Buffer 슬롯",  y); MkV(_hValRingSlot,    L"—", y); y += RS;
    MkL(L"총 청크 수",        y); MkV(_hValTotalChunks, L"—", y); y += RS;
    MkL(L"가시 청크 수",      y); MkV(_hValVisChunks,   L"—", y); y += RS;
    MkL(L"Frame Time (CPU)",  y); MkV(_hValFrameMs,     L"—", y); y += RS + 10;
    MkL(L"마지막 시나리오",    y); MkV(_hValScenario,    L"Idle", y); y += RS + 10;

    MkSep(L"▶  덤프", y); y += RH + 6;
    MkBtn(L"통계 덤프", LX, y, BW + 20, ID_BTN_DUMP);
    MkBtn(L"CSV 기록",  LX + BW + 24, y, BW + 20, ID_BTN_EXPORT);
}

std::vector<int32> StressPanel::CollectValidTypes() const
{
    std::vector<int32> validTypes;
    if (!GET_SINGLE(BlockTable)->IsLoaded()) return validTypes;

    const auto& records = GET_SINGLE(BlockTable)->GetAllRecords();
    validTypes.reserve(records.size());
    for (const BlockRecord& rec : records)
        if (!rec.key.empty() && !rec.isEraser)
            validTypes.push_back(rec.typeId);

    return validTypes;
}

void StressPanel::SpawnBlocks(int count)
{
    if (!_placer) return;
    const auto validTypes = CollectValidTypes();
    if (validTypes.empty()) return;

    std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> distXZ(-kSpawnRange, kSpawnRange);
    std::uniform_int_distribution<int>    distType(0, (int)validTypes.size() - 1);

    for (int i = 0; i < count; ++i)
    {
        const float x    = distXZ(rng);
        const float z    = distXZ(rng);
        const int32 type = validTypes[distType(rng)];
        _placer->PlaceBlock(x, 0.f, z, type);
    }

    _lastScenario = L"Append Random +" + std::to_wstring(count);
    RefreshStats();
}

void StressPanel::SpawnGridPreset(int count)
{
    if (!_placer) return;
    const auto validTypes = CollectValidTypes();
    if (validTypes.empty()) return;

    _placer->ClearAllBlocks();

    const int side = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));
    const float origin = -static_cast<float>(side - 1) * kGridSpacing * 0.5f;

    for (int i = 0; i < count; ++i)
    {
        const int ix = i % side;
        const int iz = i / side;
        const float x = origin + static_cast<float>(ix) * kGridSpacing;
        const float z = origin + static_cast<float>(iz) * kGridSpacing;
        const int32 type = validTypes[static_cast<size_t>(i) % validTypes.size()];
        _placer->PlaceBlock(x, 0.f, z, type);
    }

    _lastScenario = L"Grid " + std::to_wstring(count);
    RefreshStats();
}

void StressPanel::SpawnRandomPreset(int count)
{
    if (!_placer) return;
    const auto validTypes = CollectValidTypes();
    if (validTypes.empty()) return;

    _placer->ClearAllBlocks();

    std::mt19937 rng{ kRandomSeed };
    std::uniform_real_distribution<float> distXZ(-kSpawnRange, kSpawnRange);
    std::uniform_int_distribution<int>    distType(0, static_cast<int>(validTypes.size()) - 1);

    for (int i = 0; i < count; ++i)
    {
        const float x = distXZ(rng);
        const float z = distXZ(rng);
        const int32 type = validTypes[distType(rng)];
        _placer->PlaceBlock(x, 0.f, z, type);
    }

    _lastScenario = L"Seed Random " + std::to_wstring(count);
    RefreshStats();
}

void StressPanel::DeleteRandom10Pct()
{
    if (!_placer) return;

    std::vector<PlacedBlockRecord> snapshot = _placer->GetPlacedBlocks();
    if (snapshot.empty()) return;

    std::mt19937 rng{ std::random_device{}() };
    std::shuffle(snapshot.begin(), snapshot.end(), rng);

    const int keepCount = static_cast<int>(snapshot.size() * 0.9f);
    snapshot.resize(static_cast<size_t>(keepCount));

    _placer->ClearAllBlocks();
    for (const PlacedBlockRecord& rec : snapshot)
        _placer->PlaceBlock(rec.x, rec.y, rec.z, rec.type);

    _lastScenario = L"Delete Random 10%";
    RefreshStats();
}

void StressPanel::DumpToLog()
{
    if (!_placer) return;

    const auto& placed = _placer->GetPlacedBlocks();

    std::unordered_set<int32> typeSet;
    for (const auto& rec : placed) typeSet.insert(rec.type);

    ChunkManager*       cm   = GET_SINGLE(ChunkManager);
    DynamicInstancePool* pool = GET_SINGLE(DynamicInstancePool);
    const RenderStats&   rs   = GET_SINGLE(InstancingManager)->GetStats();

    const int32 total   = cm->GetChunkCount();
    const int32 visible = cm->GetVisibleChunkCount();
    const float cullPct = total > 0
        ? (1.f - static_cast<float>(visible) / static_cast<float>(total)) * 100.f
        : 0.f;

    wchar_t buf[1024];
    swprintf_s(buf,
        L"[StressPanel Dump]\n"
        L"  마지막 시나리오  : %s\n"
        L"  배치 블록 수     : %d\n"
        L"  실제 Draw Calls  : %u  (Mesh %u / Model %u)\n"
        L"  렌더 인스턴스    : %u\n"
        L"  고유 블록 타입   : %d\n"
        L"  사용 인스턴스    : %u / %u\n"
        L"  Ring Buffer 슬롯 : %u / %u\n"
        L"  총 청크 수       : %d\n"
        L"  가시 청크 수     : %d  (Cull %.1f%%)\n"
        L"  Frame Time (CPU) : %.2f ms\n",
        _lastScenario.c_str(),
        static_cast<int>(placed.size()),
        rs.totalDrawCalls, rs.meshDrawCalls, rs.modelDrawCalls,
        rs.totalInstances,
        (int)typeSet.size(),
        pool->GetUsedInstances(), DynamicInstancePool::kMaxInstances,
        pool->GetCurrentSlot(),   DynamicInstancePool::kRingCount,
        total, visible, cullPct,
        GET_SINGLE(TimeManager)->GetDeltaTime() * 1000.f);

    ::OutputDebugStringW(buf);
}

void StressPanel::ExportCsv()
{
    if (!_placer) return;

    const auto& placed = _placer->GetPlacedBlocks();
    const RenderStats& rs = GET_SINGLE(InstancingManager)->GetStats();
    ChunkManager* cm = GET_SINGLE(ChunkManager);
    DynamicInstancePool* pool = GET_SINGLE(DynamicInstancePool);

    const int32 totalChunks = cm->GetChunkCount();
    const int32 visibleChunks = cm->GetVisibleChunkCount();
    const float cullPct = totalChunks > 0
        ? (1.f - static_cast<float>(visibleChunks) / static_cast<float>(totalChunks)) * 100.f
        : 0.f;

    const std::filesystem::path dir = L"../StressReports";
    std::filesystem::create_directories(dir);
    const std::filesystem::path file = dir / L"stress_metrics.csv";
    const bool writeHeader = !std::filesystem::exists(file);

    std::ofstream out(file, std::ios::app);
    if (!out.is_open()) return;

    if (writeHeader)
    {
        out << "scenario,blocks,draw_calls,mesh_draw_calls,model_draw_calls,"
            << "render_instances,pool_instances,pool_max,ring_slot,total_chunks,"
            << "visible_chunks,cull_pct,cpu_frame_ms\n";
    }

    std::string scenario;
    scenario.reserve(_lastScenario.size());
    for (wchar_t ch : _lastScenario)
        scenario.push_back(ch < 128 ? static_cast<char>(ch) : '?');
    out << scenario << ','
        << placed.size() << ','
        << rs.totalDrawCalls << ','
        << rs.meshDrawCalls << ','
        << rs.modelDrawCalls << ','
        << rs.totalInstances << ','
        << pool->GetUsedInstances() << ','
        << DynamicInstancePool::kMaxInstances << ','
        << pool->GetCurrentSlot() << ','
        << totalChunks << ','
        << visibleChunks << ','
        << cullPct << ','
        << GET_SINGLE(TimeManager)->GetDeltaTime() * 1000.f
        << '\n';

    ::OutputDebugStringW(L"[StressPanel] CSV exported: ../StressReports/stress_metrics.csv\n");
}

void StressPanel::Refresh()
{
    if (!_visible || !_hWnd) return;
    RefreshStats();
}

void StressPanel::RefreshStats()
{
    auto SetW = [](HWND h, const std::wstring& s) { if (h) ::SetWindowTextW(h, s.c_str()); };

    auto FmtInt = [](int32 v) -> std::wstring
    {
        wchar_t raw[32]; swprintf_s(raw, L"%d", v);
        std::wstring s = raw;
        int pos = (int)s.size() - 3;
        while (pos > 0) { s.insert(pos, L","); pos -= 3; }
        return s;
    };

    int32 blockCount  = 0;
    int32 drawCallEst = 0;

    if (_placer)
    {
        const auto& placed = _placer->GetPlacedBlocks();
        blockCount = static_cast<int32>(placed.size());

        std::unordered_set<int32> typeSet;
        for (const auto& rec : placed) typeSet.insert(rec.type);
        drawCallEst = static_cast<int32>(typeSet.size());
    }

    SetW(_hValBlockCount, FmtInt(blockCount) + L" 개");
    const RenderStats& rs = GET_SINGLE(InstancingManager)->GetStats();
    SetW(_hValDrawCalls,
        FmtInt(static_cast<int32>(rs.totalDrawCalls))
        + L" 회  (Mesh " + FmtInt(static_cast<int32>(rs.meshDrawCalls))
        + L" / Model " + FmtInt(static_cast<int32>(rs.modelDrawCalls)) + L")");

    DynamicInstancePool* pool = GET_SINGLE(DynamicInstancePool);
    if (pool->IsReady())
    {
        SetW(_hValInstances,
            FmtInt(static_cast<int32>(pool->GetUsedInstances()))
            + L" / " + FmtInt(DynamicInstancePool::kMaxInstances));

        wchar_t slotBuf[32];
        swprintf_s(slotBuf, L"슬롯 %u / %u", pool->GetCurrentSlot(), DynamicInstancePool::kRingCount);
        SetW(_hValRingSlot, slotBuf);
    }

    ChunkManager* cm      = GET_SINGLE(ChunkManager);
    const int32   total   = cm->GetChunkCount();
    const int32   visible = cm->GetVisibleChunkCount();
    const float   cullPct = total > 0
        ? (1.f - static_cast<float>(visible) / static_cast<float>(total)) * 100.f
        : 0.f;

    SetW(_hValTotalChunks, FmtInt(total));

    wchar_t visBuf[48];
    swprintf_s(visBuf, L"%s  (Cull %.1f%%)", FmtInt(visible).c_str(), cullPct);
    SetW(_hValVisChunks, visBuf);

    wchar_t msBuf[24];
    swprintf_s(msBuf, L"%.2f ms", GET_SINGLE(TimeManager)->GetDeltaTime() * 1000.f);
    SetW(_hValFrameMs, msBuf);

    SetW(_hValScenario, _lastScenario);
}

void StressPanel::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex   = {};
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = StressPanel::WndProc;
    wcex.hInstance     = hInstance;
    wcex.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = CLASS_NAME;
    ::RegisterClassExW(&wcex);
}

LRESULT CALLBACK StressPanel::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA,
                           reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

    StressPanel* self = reinterpret_cast<StressPanel*>(
        ::GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (self)
    {
        switch (msg)
        {
        case WM_CLOSE:
            self->Hide();
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case ID_BTN_SPAWN100:  self->SpawnBlocks(100);     return 0;
            case ID_BTN_SPAWN1K:   self->SpawnBlocks(1000);    return 0;
            case ID_BTN_SPAWN10K:  self->SpawnBlocks(10000);   return 0;
            case ID_BTN_GRID10K:   self->SpawnGridPreset(10000); return 0;
            case ID_BTN_RANDOM10K: self->SpawnRandomPreset(10000); return 0;
            case ID_BTN_CLEAR:
                if (self->_placer)
                {
                    self->_placer->ClearAllBlocks();
                    self->_lastScenario = L"Clear All";
                    self->RefreshStats();
                }
                return 0;
            case ID_BTN_DEL10:     self->DeleteRandom10Pct();  return 0;
            case ID_BTN_DUMP:      self->DumpToLog();          return 0;
            case ID_BTN_EXPORT:    self->ExportCsv();          return 0;
            }
            break;
        }
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
