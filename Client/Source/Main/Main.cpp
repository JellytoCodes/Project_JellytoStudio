
#include "pch.h"
#include "App/Application.h"

// ── 빌드 타겟 스위치 ──────────────────────────────────────────────────────
// EDITOR_MODE를 정의하면 EditorApp 실행
// 주석 처리하면 게임용 MainApp 실행
//#define EDITOR_MODE

#ifdef EDITOR_MODE
    #include "EditorApp.h"
    using AppType = EditorApp;
	static constexpr  bool isCreateWindow = true;
    static const wchar_t* APP_NAME = L"Jellyto Studio v0.1 [EDITOR]";
#else
    #include "MainApp.h"
    using AppType = MainApp;
	static constexpr  bool isCreateWindow = false;
    static const wchar_t* APP_NAME = L"Jellyto Studio v0.1";
#endif

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    ApplicationDesc desc;
    desc.appName   = APP_NAME;
    desc.hInstance = hInstance;
    desc.width     = MAIN_WINDOW_WIDTH;
    desc.height    = MAIN_WINDOW_HEIGHT;
    desc.isCreateWindow = isCreateWindow;
    desc.app       = std::make_shared<AppType>();

    Application app;
    if (app.Initialize(desc))
        app.Run();

    app.Shutdown();
    return 0;
}