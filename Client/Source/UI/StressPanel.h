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

    void SpawnBlocks(int count);
    void SpawnFlatPreset(int count);
    void SpawnDenseCubePreset(int side);
    void SpawnGridPreset(int count);
    void SpawnRandomPreset(int count);
    void DeleteRandom10Pct();
    void DumpToLog();
    void ExportCsv();
    void ToggleFrustumCulling();
    void ToggleFaceOcclusionCulling();
    void ToggleSmartRebuild();
    void MarkBenchmarkDirty();

    void RefreshStats();
    void RefreshOptionButtons();
    std::vector<int32> CollectValidTypes() const;
    std::vector<int32> CollectValidMeshTypes() const;

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
    HWND _hValMeshGroups  = nullptr;
    HWND _hValFrameMs     = nullptr;
    HWND _hValScenario    = nullptr;
    HWND _hBtnFrustum     = nullptr;
    HWND _hBtnFace        = nullptr;
    HWND _hBtnSmart       = nullptr;

    std::wstring _lastScenario = L"Idle";

    static constexpr float kSpawnRange = 48.f;
    static constexpr float kGridSpacing = 1.15f;
    static constexpr uint32 kRandomSeed = 20260522u;

    static constexpr int ID_BTN_SPAWN100  = 701;
    static constexpr int ID_BTN_SPAWN1K   = 702;
    static constexpr int ID_BTN_SPAWN10K  = 703;
    static constexpr int ID_BTN_CLEAR     = 704;
    static constexpr int ID_BTN_DEL10     = 705;
    static constexpr int ID_BTN_DUMP      = 706;
    static constexpr int ID_BTN_GRID10K   = 707;
    static constexpr int ID_BTN_RANDOM10K = 708;
    static constexpr int ID_BTN_EXPORT    = 709;
    static constexpr int ID_BTN_FLAT1K    = 710;
    static constexpr int ID_BTN_DENSE16   = 711;
    static constexpr int ID_BTN_FRUSTUM   = 712;
    static constexpr int ID_BTN_FACE      = 713;
    static constexpr int ID_BTN_SMART     = 714;

    static constexpr wchar_t CLASS_NAME[] = L"JellytoStressPanel";
};
