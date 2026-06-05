#pragma once
#include "App/Interfaces/IWindow.h"
#include "Scene/BlockPlacerInterface.h"

class IsometricCameraController;

class StressPanel : public IWindow
{
public:
    virtual bool Create(HINSTANCE hInstance, HWND hMainWnd) override;
    virtual void Show()   override;
    virtual void Hide()   override;
    virtual void Toggle() override;

    virtual bool IsVisible() const override { return _visible; }
    virtual HWND GetHWnd()   const override { return nullptr; }
    virtual bool UsesInternalUI() const override { return true; }

    void SetPlacer(IBlockPlacer* placer) { _placer = placer; }
    void SetCameraController(IsometricCameraController* controller) { _cameraController = controller; }
    virtual void Update() override;
    virtual void DrawUI() override;
    virtual bool HitTest(float x, float y) const override;
    void Refresh();

    static constexpr float kRefreshInterval = 0.25f;

private:
    enum class ActivePreset
    {
        None,
        FrustumLow, FrustumMid, FrustumHigh,
        FaceLow, FaceMid, FaceHigh,
        SmartLow, SmartMid, SmartHigh,
        RandomLow, RandomMid, RandomHigh,
    };

    enum class HudCommand
    {
        FrustumLow, FrustumMid, FrustumHigh,
        FaceLow, FaceMid, FaceHigh,
        SmartLow, SmartMid, SmartHigh,
        RandomLow, RandomMid, RandomHigh,
        ToggleFrustum,
        ToggleFace,
        ToggleSmart,
        ClearAll,
        DeleteRandom10Pct,
        ApplyCameraPreset,
        CaptureBaseline,
        CaptureOptimized,
        Dump,
        ExportCsv,
    };

    struct HudButton
    {
        HudCommand command;
        ActivePreset preset = ActivePreset::None;
        std::wstring label;
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
        bool active = false;
        bool enabled = false;
        bool danger = false;
    };

    struct StatRow
    {
        std::wstring label;
        std::wstring value;
    };

    struct MetricSnapshot
    {
        bool valid = false;
        std::wstring scenario = L"-";
        int32 blocks = 0;
        uint32 visibleEntities = 0;
        uint32 culledEntities = 0;
        uint32 drawCalls = 0;
        uint32 instances = 0;
        uint32 meshRebuilt = 0;
        uint32 meshSkipped = 0;
        float cpuMs = 0.f;
    };

    void BuildButtons(std::vector<HudButton>& outButtons) const;
    void ExecuteCommand(HudCommand command);
    void DrawButton(const HudButton& button, float panelX, float panelY, bool hovered);
    void DrawStatRow(const StatRow& row, float x, float y, float labelW, float valueW);
    void DrawSnapshotComparison(float x, float y);

    void SpawnFrustumPreset(int count, float spacing, ActivePreset preset);
    void SpawnFacePreset(int side, ActivePreset preset);
    void SpawnSmartPreset(int count, ActivePreset preset);
    void SpawnRandomPreset(int count, ActivePreset preset);
    void DeleteRandom10Pct();
    void DumpToLog();
    void ExportCsv();
    void ToggleFrustumCulling();
    void ToggleFaceOcclusionCulling();
    void ToggleSmartRebuild();
    void ApplyCameraPreset();
    void CaptureBaseline();
    void CaptureOptimized();
    MetricSnapshot CaptureMetrics() const;
    void ShowNotice(const std::wstring& message, bool warn = false);
    void MarkBenchmarkDirty();
    void MarkVisibilityDirty();
    void MarkSmartProbeDirty();
    void RequestDeferredRefresh();

    void RefreshStats();
    void SetActivePreset(ActivePreset preset);
    std::vector<int32> CollectValidTypes() const;
    std::vector<int32> CollectBenchmarkBlockTypes() const;

    std::wstring FormatInt(int32 value) const;
    std::string  NarrowScenario() const;

    IBlockPlacer* _placer = nullptr;
    IsometricCameraController* _cameraController = nullptr;
    bool _visible = false;
    bool _created = false;

    std::wstring _lastScenario = L"Idle";
    ActivePreset _activePreset = ActivePreset::None;
    uint32 _lastNonZeroMeshRebuilt = 0;
    uint32 _lastNonZeroMeshSkipped = 0;
    int _statsRefreshDelayTicks = 0;

    std::vector<StatRow> _statRows;
    std::wstring _frameTimeText = L"-";
    MetricSnapshot _baselineSnapshot;
    MetricSnapshot _optimizedSnapshot;
    std::wstring _noticeMessage;
    float _noticeTimer = 0.f;
    bool _noticeWarn = false;

    static constexpr float kSpawnRange = 48.f;
    static constexpr uint32 kRandomSeed = 20260522u;
};
