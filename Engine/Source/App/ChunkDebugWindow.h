#pragma once
#include "App/Interfaces/IWindow.h"

class Entity;

class ChunkDebugWindow : public IWindow
{
public:
    virtual ~ChunkDebugWindow() override;

    virtual bool Create(HINSTANCE hInstance, HWND hMainWnd) override;
    virtual void Show()   override;
    virtual void Hide()   override;
    virtual void Toggle() override;

    virtual bool IsVisible() const override { return _visible; }
    virtual HWND GetHWnd()   const override { return _hWnd;    }

    void Refresh(Entity* selectedEntity);

    static constexpr float kRefreshInterval = 0.25f;

private:
    void BuildUI();

    void RefreshCullingSection();
    void RefreshInstancingSection();
    void RefreshSelectedSection(Entity* selectedEntity);
    void RefreshChunkList();

    void RegisterWindowClass(HINSTANCE hInstance);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND      _hWnd      = nullptr;
    HINSTANCE _hInstance = nullptr;
    bool      _visible   = false;
    bool      _created   = false;

    HWND _hTotalChunks   = nullptr;
    HWND _hVisibleChunks = nullptr;
    HWND _hCulledChunks  = nullptr;
    HWND _hCullRate      = nullptr;

    HWND _hUsedInstances = nullptr;
    HWND _hRingSlot      = nullptr;
    HWND _hPoolPct       = nullptr;

    HWND _hSelNone       = nullptr;
    HWND _hSelCoord      = nullptr;
    HWND _hSelEntCount   = nullptr;
    HWND _hSelVisible    = nullptr;
    HWND _hSelAABBCenter = nullptr;
    HWND _hSelAABBExt    = nullptr;

    HWND _hChunkList     = nullptr;
    HFONT _hMonoFont     = nullptr;

    static constexpr wchar_t CLASS_NAME[] = L"JellytoChunkDebugPanel";
};
