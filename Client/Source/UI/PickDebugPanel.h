#pragma once
#include "App/Interfaces/IWindow.h"

class BlockPlacer;

class PickDebugPanel : public IWindow
{
public:
    virtual bool Create(HINSTANCE hInstance, HWND hMainWnd) override;
    virtual void Show()   override;
    virtual void Hide()   override;
    virtual void Toggle() override;

    virtual bool IsVisible() const override { return _visible; }
    virtual HWND GetHWnd()   const override { return _hWnd;    }

    void SetPlacer(BlockPlacer* placer) { _placer = placer; }
    void Refresh();

    static constexpr float kRefreshInterval = 0.10f;

private:
    void BuildUI();
    void RegisterWindowClass(HINSTANCE hInstance);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    BlockPlacer* _placer    = nullptr;
    HWND         _hWnd      = nullptr;
    HINSTANCE    _hInstance = nullptr;
    bool         _visible   = false;
    bool         _created   = false;

    HWND _hValMode      = nullptr;
    HWND _hValMouse     = nullptr;
    HWND _hValSlot      = nullptr;
    HWND _hValResult    = nullptr;
    HWND _hValReason    = nullptr;
    HWND _hValChannel   = nullptr;
    HWND _hValTarget    = nullptr;
    HWND _hValStock     = nullptr;
    HWND _hValPriming   = nullptr;
    HWND _hValFloor     = nullptr;
    HWND _hValMushroom  = nullptr;

    static constexpr wchar_t CLASS_NAME[] = L"JellytoPickDebugPanel";
};
