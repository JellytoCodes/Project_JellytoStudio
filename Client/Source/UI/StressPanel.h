#pragma once
#include "App/Interfaces/IWindow.h"
#include "Scene/BlockPlacerInterface.h"

class StressPanel : public IWindow
{
public:
    virtual bool Create(HINSTANCE hInstance, HWND hMainWnd) override;
    virtual void Show()   override;
    virtual void Hide()   override;
    virtual void Toggle() override;

    virtual bool IsVisible() const override { return _visible; }
    virtual HWND GetHWnd()   const override { return _hWnd;    }

    void SetPlacer(IBlockPlacer* placer) { _placer = placer; }
    void Refresh();

    static constexpr float kRefreshInterval = 0.25f;

private:
    void BuildUI();

    enum class ActivePreset
    {
        None,
        FrustumLow, FrustumMid, FrustumHigh,
        FaceLow, FaceMid, FaceHigh,
        SmartLow, SmartMid, SmartHigh,
        RandomLow, RandomMid, RandomHigh,
    };

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
    void MarkBenchmarkDirty();
    void MarkVisibilityDirty();
    void MarkSmartProbeDirty();

    void RefreshStats();
    void RefreshOptionButtons();
    void RefreshPresetButtons();
    void SetActivePreset(ActivePreset preset);
    std::vector<int32> CollectValidTypes() const;
    std::vector<int32> CollectBenchmarkBlockTypes() const;

    void RegisterWindowClass(HINSTANCE hInstance);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    IBlockPlacer* _placer    = nullptr;
    HWND          _hWnd      = nullptr;
    HINSTANCE     _hInstance = nullptr;
    bool          _visible   = false;
    bool          _created   = false;

    HWND _hValBlockCount  = nullptr;
    HWND _hValTotalEnt    = nullptr;
    HWND _hValVisibleEnt  = nullptr;
    HWND _hValCulledEnt   = nullptr;
    HWND _hValDrawCalls   = nullptr;
    HWND _hValRenderInst  = nullptr;
    HWND _hValInstances   = nullptr;
    HWND _hValRingSlot    = nullptr;
    HWND _hValTotalChunks = nullptr;
    HWND _hValVisChunks   = nullptr;
    HWND _hValFaceCulled  = nullptr;
    HWND _hValMeshGroups  = nullptr;
    HWND _hValLastMeshGroups = nullptr;
    HWND _hValFrameMs     = nullptr;
    HWND _hValScenario    = nullptr;
    HWND _hBtnFrustum     = nullptr;
    HWND _hBtnFace        = nullptr;
    HWND _hBtnSmart       = nullptr;
    HWND _hPresetFrustumLow  = nullptr;
    HWND _hPresetFrustumMid  = nullptr;
    HWND _hPresetFrustumHigh = nullptr;
    HWND _hPresetFaceLow     = nullptr;
    HWND _hPresetFaceMid     = nullptr;
    HWND _hPresetFaceHigh    = nullptr;
    HWND _hPresetSmartLow    = nullptr;
    HWND _hPresetSmartMid    = nullptr;
    HWND _hPresetSmartHigh   = nullptr;
    HWND _hPresetRandomLow   = nullptr;
    HWND _hPresetRandomMid   = nullptr;
    HWND _hPresetRandomHigh  = nullptr;

    std::wstring _lastScenario = L"Idle";
    ActivePreset _activePreset = ActivePreset::None;
    uint32 _lastNonZeroMeshRebuilt = 0;
    uint32 _lastNonZeroMeshSkipped = 0;

    static constexpr float kSpawnRange = 48.f;
    static constexpr float kGridSpacing = 1.15f;
    static constexpr uint32 kRandomSeed = 20260522u;

    static constexpr int ID_BTN_CLEAR     = 704;
    static constexpr int ID_BTN_DEL10     = 705;
    static constexpr int ID_BTN_DUMP      = 706;
    static constexpr int ID_BTN_EXPORT    = 709;
    static constexpr int ID_BTN_FRUSTUM   = 712;
    static constexpr int ID_BTN_FACE      = 713;
    static constexpr int ID_BTN_SMART     = 714;
    static constexpr int ID_BTN_FRUSTUM_LOW  = 720;
    static constexpr int ID_BTN_FRUSTUM_MID  = 721;
    static constexpr int ID_BTN_FRUSTUM_HIGH = 722;
    static constexpr int ID_BTN_FACE_LOW     = 723;
    static constexpr int ID_BTN_FACE_MID     = 724;
    static constexpr int ID_BTN_FACE_HIGH    = 725;
    static constexpr int ID_BTN_SMART_LOW    = 726;
    static constexpr int ID_BTN_SMART_MID    = 727;
    static constexpr int ID_BTN_SMART_HIGH   = 728;
    static constexpr int ID_BTN_RANDOM_LOW   = 729;
    static constexpr int ID_BTN_RANDOM_MID   = 730;
    static constexpr int ID_BTN_RANDOM_HIGH  = 731;

    static constexpr wchar_t CLASS_NAME[] = L"JellytoStressPanel";
};
