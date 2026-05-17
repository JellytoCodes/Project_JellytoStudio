#pragma once
#include "App/Interfaces/IWindow.h"

class BlockTestPanel : public IWindow
{
public:
    virtual bool Create(HINSTANCE hInstance, HWND hMainWnd) override;
    virtual void Show()   override;
    virtual void Hide()   override;
    virtual void Toggle() override;

    virtual bool IsVisible() const override { return _visible; }
    virtual HWND GetHWnd()   const override { return _hWnd;    }

    void Load();

private:
    void BuildUI();
    void PopulateList();
    void ShowRecord(int idx);

    void RegisterWindowClass(HINSTANCE hInstance);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND      _hWnd      = nullptr;
    HINSTANCE _hInstance = nullptr;
    bool      _visible   = false;
    bool      _created   = false;

    HWND _hCountLabel = nullptr;
    HWND _hList       = nullptr;

    HWND _hValId          = nullptr;
    HWND _hValKey         = nullptr;
    HWND _hValLabel       = nullptr;
    HWND _hValEraser      = nullptr;
    HWND _hValRenderType  = nullptr;
    HWND _hValModelName   = nullptr;
    HWND _hValModelScale  = nullptr;
    HWND _hValCollider    = nullptr;
    HWND _hValOwnChannel  = nullptr;
    HWND _hValPickable    = nullptr;
    HWND _hValFaces       = nullptr;
    HWND _hValColor       = nullptr;

    static constexpr wchar_t CLASS_NAME[]  = L"JellytoBlockTestPanel";
    static constexpr int     ID_LIST_BLOCK = 601;
};