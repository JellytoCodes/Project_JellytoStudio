#include "pch.h"
#include "StressPanel.h"

#include "Core/DisplayContext.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Data/BlockTable.h"
#include "Entity/Components/Camera.h"
#include "Graphics/Managers/InstancingManager.h"
#include "Graphics/RenderDebugOptions.h"
#include "Pipeline/DynamicInstancePool.h"
#include "Scene/ChunkManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "UI/UIManager.h"

#include <filesystem>
#include <fstream>

bool StressPanel::Create(HINSTANCE, HWND)
{
    _created = true;
    return true;
}

void StressPanel::Show()
{
    _visible = true;
    RequestDeferredRefresh();
}

void StressPanel::Hide()
{
    _visible = false;
}

void StressPanel::Toggle()
{
    _visible ? Hide() : Show();
}

std::wstring StressPanel::FormatInt(int32 value) const
{
    wchar_t raw[32];
    swprintf_s(raw, L"%d", value);

    std::wstring s = raw;
    int pos = static_cast<int>(s.size()) - 3;
    while (pos > 0)
    {
        s.insert(pos, L",");
        pos -= 3;
    }
    return s;
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

std::vector<int32> StressPanel::CollectBenchmarkBlockTypes() const
{
    std::vector<int32> validTypes;
    if (!GET_SINGLE(BlockTable)->IsLoaded()) return validTypes;

    const auto& records = GET_SINGLE(BlockTable)->GetAllRecords();
    validTypes.reserve(records.size());
    for (const BlockRecord& rec : records)
    {
        if (!rec.key.empty() && !rec.isEraser && rec.collider == ColliderSize::Unit)
            validTypes.push_back(rec.typeId);
    }

    return validTypes;
}

void StressPanel::SpawnFrustumPreset(int count, float spacing, ActivePreset preset)
{
    if (!_placer) return;
    const auto validTypes = CollectBenchmarkBlockTypes();
    if (validTypes.empty()) return;

    _placer->ClearAllBlocks();

    const int side = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));
    const float origin = -static_cast<float>(side - 1) * spacing * 0.5f;

    for (int i = 0; i < count; ++i)
    {
        const int ix = i % side;
        const int iz = i / side;
        const float x = origin + static_cast<float>(ix) * spacing;
        const float z = origin + static_cast<float>(iz) * spacing;
        const int32 type = validTypes[static_cast<size_t>(i) % validTypes.size()];
        _placer->PlaceBlock(x, 0.f, z, type);
    }

    SetActivePreset(preset);
    _lastScenario = L"Frustum Field " + std::to_wstring(count);
    MarkBenchmarkDirty();
    RequestDeferredRefresh();
}

void StressPanel::SpawnFacePreset(int side, ActivePreset preset)
{
    if (!_placer) return;
    const auto validTypes = CollectBenchmarkBlockTypes();
    if (validTypes.empty()) return;

    _placer->ClearAllBlocks();

    const float origin = -static_cast<float>(side - 1) * 0.5f;
    int index = 0;

    for (int y = 0; y < side; ++y)
    {
        for (int z = 0; z < side; ++z)
        {
            for (int x = 0; x < side; ++x)
            {
                const int32 type = validTypes[static_cast<size_t>(index) % validTypes.size()];
                _placer->PlaceBlock(origin + static_cast<float>(x),
                                    static_cast<float>(y),
                                    origin + static_cast<float>(z),
                                    type);
                ++index;
            }
        }
    }

    SetActivePreset(preset);
    _lastScenario = L"Face Solid " + std::to_wstring(side) + L"^3";
    MarkBenchmarkDirty();
    RequestDeferredRefresh();
}

void StressPanel::SpawnSmartPreset(int count, ActivePreset preset)
{
    if (!_placer) return;
    const auto validTypes = CollectBenchmarkBlockTypes();
    if (validTypes.empty()) return;

    _placer->ClearAllBlocks();

    const int side = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));
    const float spacing = 1.25f;
    const float origin = -static_cast<float>(side - 1) * spacing * 0.5f;

    for (int i = 0; i < count; ++i)
    {
        const int ix = i % side;
        const int iz = i / side;
        const float x = origin + static_cast<float>(ix) * spacing;
        const float z = origin + static_cast<float>(iz) * spacing;
        const int32 type = validTypes[static_cast<size_t>(i) % validTypes.size()];
        _placer->PlaceBlock(x, 0.f, z, type);
    }

    SetActivePreset(preset);
    _lastNonZeroMeshRebuilt = 0;
    _lastNonZeroMeshSkipped = 0;
    _lastScenario = L"Smart Stable Field " + std::to_wstring(count);
    MarkBenchmarkDirty();
    RequestDeferredRefresh();
}

void StressPanel::SpawnRandomPreset(int count, ActivePreset preset)
{
    if (!_placer) return;
    const auto validTypes = CollectValidTypes();
    if (validTypes.empty()) return;

    _placer->ClearAllBlocks();

    std::mt19937 rng{ kRandomSeed };
    std::uniform_real_distribution<float> distXZ(-kSpawnRange, kSpawnRange);
    std::uniform_int_distribution<int> distType(0, static_cast<int>(validTypes.size()) - 1);

    for (int i = 0; i < count; ++i)
    {
        const float x = distXZ(rng);
        const float z = distXZ(rng);
        const int32 type = validTypes[distType(rng)];
        _placer->PlaceBlock(x, 0.f, z, type);
    }

    SetActivePreset(preset);
    _lastScenario = L"Seed Random " + std::to_wstring(count);
    MarkBenchmarkDirty();
    RequestDeferredRefresh();
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
    MarkBenchmarkDirty();
    RequestDeferredRefresh();
}

void StressPanel::MarkBenchmarkDirty()
{
    if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
    {
        if (Camera* camera = scene->GetMainCamera())
            camera->SetSortDirty();
    }

    auto* instancing = GET_SINGLE(InstancingManager);
    instancing->SetMeshDirty();
    instancing->SetMeshGroupDirty();
    instancing->SetDirty();
}

void StressPanel::MarkVisibilityDirty()
{
    if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
    {
        if (Camera* camera = scene->GetMainCamera())
            camera->SetSortDirty();
    }

    auto* instancing = GET_SINGLE(InstancingManager);
    instancing->SetMeshGroupDirty();
    instancing->SetDirty();
}

void StressPanel::MarkSmartProbeDirty()
{
    auto* instancing = GET_SINGLE(InstancingManager);
    instancing->SetMeshGroupDirty();
    instancing->SetDirty();
}

void StressPanel::RequestDeferredRefresh()
{
    _statsRefreshDelayTicks = 1;
}

void StressPanel::ToggleFrustumCulling()
{
    auto& options = RenderDebugOptions::Get();
    options.bEnableFrustumCulling = !options.bEnableFrustumCulling;
    MarkVisibilityDirty();
    RequestDeferredRefresh();
}

void StressPanel::ToggleFaceOcclusionCulling()
{
    auto& options = RenderDebugOptions::Get();
    options.bEnableFaceOcclusionCulling = !options.bEnableFaceOcclusionCulling;
    MarkVisibilityDirty();
    RequestDeferredRefresh();
}

void StressPanel::ToggleSmartRebuild()
{
    auto& options = RenderDebugOptions::Get();
    options.bEnableSmartRebuild = !options.bEnableSmartRebuild;
    MarkSmartProbeDirty();
    RequestDeferredRefresh();
}

void StressPanel::SetActivePreset(ActivePreset preset)
{
    _activePreset = preset;
}

std::string StressPanel::NarrowScenario() const
{
    std::string scenario;
    scenario.reserve(_lastScenario.size());
    for (wchar_t ch : _lastScenario)
        scenario.push_back(ch < 128 ? static_cast<char>(ch) : '?');
    return scenario;
}

void StressPanel::DumpToLog()
{
    RefreshStats();

    std::wstring text = L"[StressPanel Dump]\n";
    text += L"  Scenario : " + _lastScenario + L"\n";
    for (const StatRow& row : _statRows)
        text += L"  " + row.label + L" : " + row.value + L"\n";
    text += L"  Frame Time (CPU) : " + _frameTimeText + L"\n";

    ::OutputDebugStringW(text.c_str());
}

void StressPanel::ExportCsv()
{
    if (!_placer) return;
    RefreshStats();

    const auto& placed = _placer->GetPlacedBlocks();
    const RenderStats& rs = GET_SINGLE(InstancingManager)->GetStats();
    ChunkManager* cm = GET_SINGLE(ChunkManager);
    DynamicInstancePool* pool = GET_SINGLE(DynamicInstancePool);

    Camera::CullStats cs = {};
    if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
    {
        if (Camera* camera = scene->GetMainCamera())
        {
            camera->SortEntities();
            cs = camera->GetCullStats();
        }
    }

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
            << "render_instances,total_entities,visible_entities,culled_entities,"
            << "mesh_groups_rebuilt,mesh_groups_skipped,pool_instances,pool_max,"
            << "ring_slot,total_chunks,visible_chunks,face_culled,cull_pct,cpu_frame_ms\n";
    }

    out << NarrowScenario() << ','
        << placed.size() << ','
        << rs.totalDrawCalls << ','
        << rs.meshDrawCalls << ','
        << rs.modelDrawCalls << ','
        << rs.totalInstances << ','
        << cs.totalEntities << ','
        << cs.visibleEntities << ','
        << cs.culledEntities << ','
        << rs.meshGroupsRebuilt << ','
        << rs.meshGroupsSkipped << ','
        << pool->GetUsedInstances() << ','
        << DynamicInstancePool::kMaxInstances << ','
        << pool->GetCurrentSlot() << ','
        << totalChunks << ','
        << visibleChunks << ','
        << cm->GetFaceOccludedCount() << ','
        << cullPct << ','
        << GET_SINGLE(TimeManager)->GetDeltaTime() * 1000.f
        << '\n';

    ::OutputDebugStringW(L"[StressPanel] CSV exported: ../StressReports/stress_metrics.csv\n");
}

void StressPanel::Refresh()
{
    if (!_visible) return;
    if (_statsRefreshDelayTicks > 0)
    {
        --_statsRefreshDelayTicks;
        return;
    }

    RefreshStats();
}

void StressPanel::RefreshStats()
{
    int32 blockCount = 0;
    if (_placer)
        blockCount = static_cast<int32>(_placer->GetPlacedBlocks().size());

    const RenderStats& rs = GET_SINGLE(InstancingManager)->GetStats();
    if (rs.meshGroupsRebuilt != 0 || rs.meshGroupsSkipped != 0)
    {
        _lastNonZeroMeshRebuilt = rs.meshGroupsRebuilt;
        _lastNonZeroMeshSkipped = rs.meshGroupsSkipped;
    }

    Camera::CullStats cs = {};
    if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
    {
        if (Camera* camera = scene->GetMainCamera())
        {
            camera->SortEntities();
            cs = camera->GetCullStats();
        }
    }

    ChunkManager* cm = GET_SINGLE(ChunkManager);
    const int32 totalChunks = cm->GetChunkCount();
    const int32 visibleChunks = cm->GetVisibleChunkCount();
    const float cullPct = totalChunks > 0
        ? (1.f - static_cast<float>(visibleChunks) / static_cast<float>(totalChunks)) * 100.f
        : 0.f;

    wchar_t drawBuf[80];
    swprintf_s(drawBuf, L"%s  (Mesh %s / Model %s)",
        FormatInt(static_cast<int32>(rs.totalDrawCalls)).c_str(),
        FormatInt(static_cast<int32>(rs.meshDrawCalls)).c_str(),
        FormatInt(static_cast<int32>(rs.modelDrawCalls)).c_str());

    wchar_t chunkBuf[64];
    swprintf_s(chunkBuf, L"%s  (Cull %.1f%%)", FormatInt(visibleChunks).c_str(), cullPct);

    DynamicInstancePool* pool = GET_SINGLE(DynamicInstancePool);
    std::wstring poolText = L"-";
    std::wstring ringText = L"-";
    if (pool->IsReady())
    {
        poolText = FormatInt(static_cast<int32>(pool->GetUsedInstances()))
            + L" / " + FormatInt(DynamicInstancePool::kMaxInstances);

        wchar_t slotBuf[32];
        swprintf_s(slotBuf, L"%u / %u", pool->GetCurrentSlot(), DynamicInstancePool::kRingCount);
        ringText = slotBuf;
    }

    wchar_t msBuf[24];
    swprintf_s(msBuf, L"%.2f ms", GET_SINGLE(TimeManager)->GetDeltaTime() * 1000.f);
    _frameTimeText = msBuf;

    _statRows.clear();
    _statRows.push_back({ L"Blocks", FormatInt(blockCount) });
    _statRows.push_back({ L"Total Entities", FormatInt(static_cast<int32>(cs.totalEntities)) });
    _statRows.push_back({ L"Visible Entities", FormatInt(static_cast<int32>(cs.visibleEntities)) });
    _statRows.push_back({ L"Culled Entities", FormatInt(static_cast<int32>(cs.culledEntities)) });
    _statRows.push_back({ L"Draw Calls", drawBuf });
    _statRows.push_back({ L"Render Instances", FormatInt(static_cast<int32>(rs.totalInstances)) });
    _statRows.push_back({ L"Pool Instances", poolText });
    _statRows.push_back({ L"Ring Slot", ringText });
    _statRows.push_back({ L"Total Chunks", FormatInt(totalChunks) });
    _statRows.push_back({ L"Visible Chunks", chunkBuf });
    _statRows.push_back({ L"Face Culled", FormatInt(cm->GetFaceOccludedCount()) });
    _statRows.push_back({ L"Mesh Rebuild/Skip",
        FormatInt(static_cast<int32>(rs.meshGroupsRebuilt)) + L" / " + FormatInt(static_cast<int32>(rs.meshGroupsSkipped)) });
    _statRows.push_back({ L"Last Nonzero R/S",
        FormatInt(static_cast<int32>(_lastNonZeroMeshRebuilt)) + L" / " + FormatInt(static_cast<int32>(_lastNonZeroMeshSkipped)) });
}

void StressPanel::BuildButtons(std::vector<HudButton>& outButtons) const
{
    const auto& options = RenderDebugOptions::Get();
    constexpr float x0 = 14.f;
    constexpr float y0 = 54.f;
    constexpr float bw = 104.f;
    constexpr float bh = 26.f;
    constexpr float gap = 8.f;

    auto AddPreset = [&](HudCommand cmd, ActivePreset preset, const wchar_t* label, int col, int row)
    {
        outButtons.push_back({ cmd, preset, label,
            x0 + col * (bw + gap), y0 + row * (bh + gap), bw, bh,
            _activePreset == preset, false, false });
    };

    AddPreset(HudCommand::FrustumLow,  ActivePreset::FrustumLow,  L"Frustum Low",  0, 0);
    AddPreset(HudCommand::FrustumMid,  ActivePreset::FrustumMid,  L"Frustum Mid",  1, 0);
    AddPreset(HudCommand::FrustumHigh, ActivePreset::FrustumHigh, L"Frustum High", 2, 0);
    AddPreset(HudCommand::FaceLow,     ActivePreset::FaceLow,     L"Face Low",     0, 1);
    AddPreset(HudCommand::FaceMid,     ActivePreset::FaceMid,     L"Face Mid",     1, 1);
    AddPreset(HudCommand::FaceHigh,    ActivePreset::FaceHigh,    L"Face High",    2, 1);
    AddPreset(HudCommand::SmartLow,    ActivePreset::SmartLow,    L"Smart Low",    0, 2);
    AddPreset(HudCommand::SmartMid,    ActivePreset::SmartMid,    L"Smart Mid",    1, 2);
    AddPreset(HudCommand::SmartHigh,   ActivePreset::SmartHigh,   L"Smart High",   2, 2);
    AddPreset(HudCommand::RandomLow,   ActivePreset::RandomLow,   L"Rand Low",     0, 3);
    AddPreset(HudCommand::RandomMid,   ActivePreset::RandomMid,   L"Rand Mid",     1, 3);
    AddPreset(HudCommand::RandomHigh,  ActivePreset::RandomHigh,  L"Rand High",    2, 3);

    const float optY = y0 + 4.f * (bh + gap) + 28.f;
    outButtons.push_back({ HudCommand::ToggleFrustum, ActivePreset::None, L"Frustum", x0, optY, bw, bh,
        false, options.bEnableFrustumCulling, false });
    outButtons.push_back({ HudCommand::ToggleFace, ActivePreset::None, L"Face", x0 + bw + gap, optY, bw, bh,
        false, options.bEnableFaceOcclusionCulling, false });
    outButtons.push_back({ HudCommand::ToggleSmart, ActivePreset::None, L"Smart", x0 + 2.f * (bw + gap), optY, bw, bh,
        false, options.bEnableSmartRebuild, false });

    const float actionY = optY + bh + 12.f;
    outButtons.push_back({ HudCommand::ClearAll, ActivePreset::None, L"Clear", x0, actionY, bw, bh, false, false, true });
    outButtons.push_back({ HudCommand::DeleteRandom10Pct, ActivePreset::None, L"Delete 10%", x0 + bw + gap, actionY, bw, bh, false, false, true });
    outButtons.push_back({ HudCommand::Dump, ActivePreset::None, L"Dump", x0 + 2.f * (bw + gap), actionY, bw, bh, false, false, false });
    outButtons.push_back({ HudCommand::ExportCsv, ActivePreset::None, L"CSV", x0 + 3.f * (bw + gap), actionY, 64.f, bh, false, false, false });
}

void StressPanel::ExecuteCommand(HudCommand command)
{
    switch (command)
    {
    case HudCommand::FrustumLow:  SpawnFrustumPreset(1000, 2.0f, ActivePreset::FrustumLow); return;
    case HudCommand::FrustumMid:  SpawnFrustumPreset(5000, 2.4f, ActivePreset::FrustumMid); return;
    case HudCommand::FrustumHigh: SpawnFrustumPreset(10000, 2.8f, ActivePreset::FrustumHigh); return;
    case HudCommand::FaceLow:     SpawnFacePreset(8, ActivePreset::FaceLow); return;
    case HudCommand::FaceMid:     SpawnFacePreset(12, ActivePreset::FaceMid); return;
    case HudCommand::FaceHigh:    SpawnFacePreset(16, ActivePreset::FaceHigh); return;
    case HudCommand::SmartLow:    SpawnSmartPreset(1000, ActivePreset::SmartLow); return;
    case HudCommand::SmartMid:    SpawnSmartPreset(5000, ActivePreset::SmartMid); return;
    case HudCommand::SmartHigh:   SpawnSmartPreset(10000, ActivePreset::SmartHigh); return;
    case HudCommand::RandomLow:   SpawnRandomPreset(1000, ActivePreset::RandomLow); return;
    case HudCommand::RandomMid:   SpawnRandomPreset(5000, ActivePreset::RandomMid); return;
    case HudCommand::RandomHigh:  SpawnRandomPreset(10000, ActivePreset::RandomHigh); return;
    case HudCommand::ToggleFrustum: ToggleFrustumCulling(); return;
    case HudCommand::ToggleFace:    ToggleFaceOcclusionCulling(); return;
    case HudCommand::ToggleSmart:   ToggleSmartRebuild(); return;
    case HudCommand::ClearAll:
        if (_placer)
        {
            _placer->ClearAllBlocks();
            SetActivePreset(ActivePreset::None);
            _lastScenario = L"Clear All";
            MarkBenchmarkDirty();
            RequestDeferredRefresh();
        }
        return;
    case HudCommand::DeleteRandom10Pct: DeleteRandom10Pct(); return;
    case HudCommand::Dump: DumpToLog(); return;
    case HudCommand::ExportCsv: ExportCsv(); return;
    }
}

void StressPanel::Update()
{
    if (!_visible) return;
    if (!GET_SINGLE(InputManager)->IsMainWindowActive()) return;

    std::vector<HudButton> buttons;
    BuildButtons(buttons);

    const POINT mp = GET_SINGLE(InputManager)->GetMousePos();
    const float mx = static_cast<float>(mp.x);
    const float my = static_cast<float>(mp.y);
    const float panelX = GET_SINGLE(DisplayContext)->GetWidthF() - 474.f;
    const float panelY = 42.f;

    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON)) return;

    for (const HudButton& button : buttons)
    {
        const float x = panelX + button.x;
        const float y = panelY + button.y;
        if (mx >= x && mx <= x + button.w && my >= y && my <= y + button.h)
        {
            ExecuteCommand(button.command);
            return;
        }
    }
}

void StressPanel::DrawButton(const HudButton& button, float panelX, float panelY, bool hovered)
{
    auto* ui = GET_SINGLE(UIManager);
    const float x = panelX + button.x;
    const float y = panelY + button.y;

    Color bg = Color(0.14f, 0.16f, 0.19f, 0.96f);
    Color border = Color(0.34f, 0.38f, 0.44f, 0.95f);
    Color text = Color(0.88f, 0.90f, 0.94f, 1.f);

    if (button.enabled)
    {
        bg = Color(0.08f, 0.34f, 0.20f, 0.98f);
        border = Color(0.30f, 0.95f, 0.56f, 1.f);
        text = Color(0.92f, 1.00f, 0.94f, 1.f);
    }
    else if (button.command == HudCommand::ToggleFrustum ||
             button.command == HudCommand::ToggleFace ||
             button.command == HudCommand::ToggleSmart)
    {
        bg = Color(0.34f, 0.10f, 0.12f, 0.98f);
        border = Color(0.95f, 0.34f, 0.34f, 1.f);
        text = Color(1.00f, 0.90f, 0.90f, 1.f);
    }

    if (button.active)
    {
        bg = Color(0.88f, 0.62f, 0.12f, 0.98f);
        border = Color(1.00f, 0.92f, 0.34f, 1.f);
        text = Color(0.08f, 0.08f, 0.08f, 1.f);
    }
    else if (button.danger)
    {
        bg = Color(0.24f, 0.10f, 0.10f, 0.96f);
        border = Color(0.70f, 0.25f, 0.20f, 0.95f);
    }

    if (hovered)
    {
        bg.R(std::min(1.f, bg.R() + 0.08f));
        bg.G(std::min(1.f, bg.G() + 0.08f));
        bg.B(std::min(1.f, bg.B() + 0.08f));
    }

    ui->AddRect(x, y, button.w, button.h, bg);
    ui->AddRectBorder(x, y, button.w, button.h, border, button.active || button.enabled ? 2.f : 1.f);

    std::wstring label = button.label;
    if (button.command == HudCommand::ToggleFrustum ||
        button.command == HudCommand::ToggleFace ||
        button.command == HudCommand::ToggleSmart)
        label += button.enabled ? L" ON" : L" OFF";

    ui->AddText(label, x + 3.f, y + 2.f, button.w - 6.f, button.h - 4.f, text, 13, L"Arial");
}

void StressPanel::DrawStatRow(const StatRow& row, float x, float y, float labelW, float valueW)
{
    auto* ui = GET_SINGLE(UIManager);
    ui->AddText(row.label, x, y, labelW, 17.f, Color(0.64f, 0.70f, 0.78f, 1.f), 12, L"Arial");
    ui->AddText(row.value, x + labelW, y, valueW, 17.f, Color(0.94f, 0.96f, 1.f, 1.f), 12, L"Arial");
}

void StressPanel::DrawUI()
{
    if (!_visible) return;

    RefreshStats();

    auto* ui = GET_SINGLE(UIManager);
    const float scrW = GET_SINGLE(DisplayContext)->GetWidthF();
    const float panelW = 462.f;
    const float panelH = 646.f;
    const float panelX = scrW - panelW - 14.f;
    const float panelY = 42.f;

    ui->AddRect(panelX, panelY, panelW, panelH, Color(0.055f, 0.065f, 0.080f, 0.93f));
    ui->AddRectBorder(panelX, panelY, panelW, panelH, Color(0.30f, 0.36f, 0.44f, 0.95f), 1.5f);
    ui->AddText(L"Stress Benchmark", panelX + 14.f, panelY + 10.f, 190.f, 22.f,
        Color(0.90f, 0.94f, 1.00f, 1.f), 17, L"Arial");
    ui->AddText(L"F3 Hide", panelX + panelW - 76.f, panelY + 10.f, 62.f, 20.f,
        Color(0.56f, 0.62f, 0.70f, 1.f), 12, L"Arial");

    std::vector<HudButton> buttons;
    BuildButtons(buttons);

    const POINT mp = GET_SINGLE(InputManager)->GetMousePos();
    const float mx = static_cast<float>(mp.x);
    const float my = static_cast<float>(mp.y);

    ui->AddText(L"Scenario Presets", panelX + 14.f, panelY + 34.f, 180.f, 18.f,
        Color(0.66f, 0.74f, 0.84f, 1.f), 12, L"Arial");
    ui->AddText(L"Optimization Toggles", panelX + 14.f, panelY + 188.f, 180.f, 18.f,
        Color(0.66f, 0.74f, 0.84f, 1.f), 12, L"Arial");

    for (const HudButton& button : buttons)
    {
        const float x = panelX + button.x;
        const float y = panelY + button.y;
        const bool hovered = mx >= x && mx <= x + button.w && my >= y && my <= y + button.h;
        DrawButton(button, panelX, panelY, hovered);
    }

    const float statX = panelX + 14.f;
    float statY = panelY + 286.f;
    ui->AddText(L"Current Metrics", statX, statY, 160.f, 18.f,
        Color(0.66f, 0.74f, 0.84f, 1.f), 12, L"Arial");
    statY += 22.f;

    for (const StatRow& row : _statRows)
    {
        DrawStatRow(row, statX, statY, 150.f, 280.f);
        statY += 18.f;
    }

    statY += 8.f;
    DrawStatRow({ L"CPU Frame", _frameTimeText }, statX, statY, 150.f, 280.f);
    statY += 22.f;
    DrawStatRow({ L"Scenario", _lastScenario }, statX, statY, 150.f, 280.f);
}
