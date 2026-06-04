#pragma once

class IExecute;

enum class AppMenuCmd : UINT
{
    ToggleChunkDebugWindow = 1001,
    ToggleBlockTestPanel   = 1002,
    ToggleStressPanel      = 1003,
    TogglePickDebugPanel   = 1004,
    ToggleToolWindow       = 1005,
    ToggleItemWindow       = 1006,
    ToggleDetailWindow     = 1007,
    Exit                   = 1008,
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
    bool    ConsumeFunctionKey(int index, int vk);
    void    ToggleWindowByCommand(AppMenuCmd cmd);
    void    ToggleRegisteredWindow(const std::wstring& name);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    ApplicationDesc _desc;
    bool _functionKeyDown[8] = {};
};
