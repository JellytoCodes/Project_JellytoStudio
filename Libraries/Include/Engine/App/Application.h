#pragma once

class IExecute;

enum class AppMenuCmd : UINT
{
    ToggleToolWindow   = 1001,
    ToggleItemWindow   = 1002,
    ToggleDetailWindow = 1003,
    Exit               = 1004,
};

struct ApplicationDesc
{
    std::shared_ptr<IExecute>   app;
    std::wstring                appName;
    HINSTANCE                   hInstance = nullptr;
    HWND                        hWnd      = nullptr;
    UINT                        width     = 1280;
    UINT                        height    = 720;
    bool                        isCreateWindow = false;
    bool                        vsync          = false;
    bool                        windowed       = true;
};

class Application
{
public:
    bool    Initialize(const ApplicationDesc& desc);
    WPARAM  Run();
    void    Shutdown();

private:
    void    Update();
    void    UpdateWindowTitle();
    void    OnResize(UINT width, UINT height);

    ATOM    MyRegisterClass();
    BOOL    InitInstance();
    void    CreateMainMenu();
    void    HandleShortcuts();
    void    ToggleToolWindow();
    void    ToggleItemWindow();
    void    ToggleDetailWindow();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    ApplicationDesc _desc;
};
