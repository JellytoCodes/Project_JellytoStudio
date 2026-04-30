
#include "Framework.h"
#include "Application.h"

#include "Audio/AudioDataTable.h"
#include "Audio/AudioManager.h"

#include "Core/Interfaces/IExecute.h"
#include "Core/DisplayContext.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "DetailWindow.h"
#include "Graphics/Graphics.h"
#include "ItemWindow.h"
#include "Managers/WindowManager.h"
#include "Scene/SceneManager.h"
#include "ToolWindow.h"
#include "UI/UIManager.h"

bool Application::Initialize(const ApplicationDesc& desc)
{
    _desc = desc;
    assert(_desc.app != nullptr);

    MyRegisterClass();
    if (!InitInstance()) return false;

    GET_SINGLE(DisplayContext)->SetSize(_desc.width, _desc.height);

    GET_SINGLE(WindowManager)->Init(_desc.hInstance, _desc.hWnd);
    GET_SINGLE(WindowManager)->RegisterWindow<ToolWindow>(L"ToolWindow");
    GET_SINGLE(WindowManager)->RegisterWindow<ItemWindow>(L"ItemWindow");
    GET_SINGLE(WindowManager)->RegisterWindow<DetailWindow>(L"DetailWindow");

    GET_SINGLE(TimeManager)->Init();
    GET_SINGLE(InputManager)->Init(_desc.hWnd);

    if (auto detail = GET_SINGLE(WindowManager)->GetWindow<DetailWindow>(L"DetailWindow"))
        GET_SINGLE(InputManager)->AddAllowedWindow(detail->GetHWnd());

    if (auto item = GET_SINGLE(WindowManager)->GetWindow<ItemWindow>(L"ItemWindow"))
        GET_SINGLE(InputManager)->AddAllowedWindow(item->GetHWnd());

    GET_SINGLE(UIManager)->Init(GET_SINGLE(DisplayContext)->GetWidthF(), GET_SINGLE(DisplayContext)->GetHeightF());

    GET_SINGLE(AudioDataTable)->Load(L"../Resources/Data/AudioData.xml");
    GET_SINGLE(AudioManager)->Init(L"../Resources/Audio/");
    GET_SINGLE(AudioManager)->PlayBGM(L"BGM_Main");

    _desc.app->Init();
    return true;
}

WPARAM Application::Run()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        else
        {
            Update();
        }
    }
    return msg.wParam;
}

void Application::Shutdown() {}

void Application::Update()
{
    GET_SINGLE(Graphics)->RenderBegin();
    
	/*----------- Begin ------------*/
    GET_SINGLE(TimeManager)->Update();
    GET_SINGLE(InputManager)->Update();
    
	UpdateWindowTitle();
    
	GET_SINGLE(SceneManager)->Update();
    
	HandleShortcuts();
    
	_desc.app->Update();
    _desc.app->Render();
    
	GET_SINGLE(SceneManager)->Render();
    GET_SINGLE(AudioManager)->Update();
    /*------------ End -------------*/

	GET_SINGLE(Graphics)->RenderEnd();
}

void Application::OnResize(UINT width, UINT height)
{
    if (width == 0 || height == 0) return;
    if (width  == GET_SINGLE(DisplayContext)->GetWidth() &&
        height == GET_SINGLE(DisplayContext)->GetHeight()) return;

    GET_SINGLE(DisplayContext)->SetSize(width, height);
    GET_SINGLE(UIManager)->SetScreenSize(static_cast<float>(width), static_cast<float>(height));
    GET_SINGLE(Graphics)->ResizeBuffers(width, height);
}

void Application::UpdateWindowTitle()
{
    float   fps  = GET_SINGLE(TimeManager)->GetFps();
    wchar_t text[100];
    swprintf_s(text, L"%s (FPS: %.2f)", _desc.appName.c_str(), fps);
    ::SetWindowTextW(_desc.hWnd, text);
}

void Application::CreateMainMenu()
{
    if (_desc.isCreateWindow == false) return;

    HMENU hBar  = ::CreateMenu();
    HMENU hFile = ::CreatePopupMenu();
    ::AppendMenuW(hFile, MF_STRING, (UINT_PTR)AppMenuCmd::Exit, L"종료(&X)\tAlt+F4");
    ::AppendMenuW(hBar,  MF_POPUP,  (UINT_PTR)hFile, L"파일(&F)");

    HMENU hWin = ::CreatePopupMenu();
    ::AppendMenuW(hWin, MF_STRING, (UINT_PTR)AppMenuCmd::ToggleToolWindow,   L"툴 창\tCtrl+T");
    ::AppendMenuW(hWin, MF_STRING, (UINT_PTR)AppMenuCmd::ToggleItemWindow,   L"아이템 창\tCtrl+I");
    ::AppendMenuW(hWin, MF_STRING, (UINT_PTR)AppMenuCmd::ToggleDetailWindow, L"디테일 창\tCtrl+D");
    ::AppendMenuW(hBar, MF_POPUP,  (UINT_PTR)hWin, L"창(&W)");

    ::SetMenu(_desc.hWnd, hBar);
}

void Application::HandleShortcuts()
{
    if (!(::GetKeyState(VK_CONTROL) & 0x8000)) return;
    if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::T)) { ToggleToolWindow();   return; }
    if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::I)) { ToggleItemWindow();   return; }
    if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::D)) { ToggleDetailWindow(); return; }
}

void Application::ToggleToolWindow()   { GET_SINGLE(WindowManager)->ToggleWindow(L"ToolWindow");   }
void Application::ToggleItemWindow()   { GET_SINGLE(WindowManager)->ToggleWindow(L"ItemWindow");   }
void Application::ToggleDetailWindow() { GET_SINGLE(WindowManager)->ToggleWindow(L"DetailWindow"); }

LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

    Application* self = reinterpret_cast<Application*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (self)
    {
        switch (msg)
        {
        case WM_SIZE:
        {
            const UINT w = LOWORD(lParam);
            const UINT h = HIWORD(lParam);
            if (wParam != SIZE_MINIMIZED)
                self->OnResize(w, h);
            return 0;
        }
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;

        case WM_COMMAND:
            switch ((AppMenuCmd)LOWORD(wParam))
            {
            case AppMenuCmd::ToggleToolWindow:   self->ToggleToolWindow();   return 0;
            case AppMenuCmd::ToggleItemWindow:   self->ToggleItemWindow();   return 0;
            case AppMenuCmd::ToggleDetailWindow: self->ToggleDetailWindow(); return 0;
            case AppMenuCmd::Exit:               ::PostQuitMessage(0);       return 0;
            }
            break;
        }
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

ATOM Application::MyRegisterClass()
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = Application::WndProc;
    wcex.hInstance     = _desc.hInstance;
    wcex.hIcon         = ::LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor       = ::LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = _desc.appName.c_str();
    wcex.hIconSm       = ::LoadIcon(NULL, IDI_APPLICATION);
    return ::RegisterClassExW(&wcex);
}

BOOL Application::InitInstance()
{
    RECT wr = { 0, 0,
        static_cast<LONG>(_desc.width),
        static_cast<LONG>(_desc.height) };
    ::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, TRUE);

    _desc.hWnd = ::CreateWindowW(
        _desc.appName.c_str(), _desc.appName.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, _desc.hInstance, this);

    if (!_desc.hWnd) return FALSE;

    CreateMainMenu();
    ::ShowWindow(_desc.hWnd, SW_SHOWNORMAL);
    ::UpdateWindow(_desc.hWnd);

    GET_SINGLE(Graphics)->Initialize(_desc.hWnd, _desc.width, _desc.height);
    return TRUE;
}
