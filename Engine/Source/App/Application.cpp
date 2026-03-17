#include "Framework.h"
#include "Application.h"
#include "Core/Interfaces/IExecute.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Graphics/Graphics.h"
#include "Scene/SceneManager.h"

bool Application::Initialize(const ApplicationDesc& desc)
{
	_desc = desc;
	assert(_desc.app != nullptr);

	MyRegisterClass();
	if (!InitInstance()) return false;

	GET_SINGLE(TimeManager)->Init();
	GET_SINGLE(InputManager)->Init(_desc.hWnd);
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
	Graphics::Get()->RenderBegin();
	GET_SINGLE(TimeManager)->Update();
	GET_SINGLE(InputManager)->Update();
	UpdateWindowTitle();
	GET_SINGLE(SceneManager)->Update();
	_desc.app->Update();
	_desc.app->Render();
	GET_SINGLE(SceneManager)->Render();
	Graphics::Get()->RenderEnd();
}

void Application::UpdateWindowTitle()
{
	float fps       = GET_SINGLE(TimeManager)->GetFps();
	float totalTime = GET_SINGLE(TimeManager)->GetTotalTime();
	wchar_t text[100];
	swprintf_s(text, L"%s (FPS: %.2f, TotalTime: %.2f s)",
		_desc.appName.c_str(), fps, totalTime);
	::SetWindowTextW(_desc.hWnd, text);
}

// ── 메인 윈도우 메뉴: 파일 / 창 ────────────────────────────────────────
void Application::CreateMainMenu()
{
	HMENU hBar = ::CreateMenu();

	// [파일] → 종료
	HMENU hFile = ::CreatePopupMenu();
	::AppendMenuW(hFile, MF_STRING, (UINT_PTR)AppMenuCmd::Exit, L"종료(&X)\tAlt+F4");
	::AppendMenuW(hBar, MF_POPUP, (UINT_PTR)hFile, L"파일(&F)");

	// [창] → 서브 윈도우 열기/닫기
	HMENU hWin = ::CreatePopupMenu();
	::AppendMenuW(hWin, MF_STRING, (UINT_PTR)AppMenuCmd::ToggleSubWindow,
		L"서브 윈도우 열기/닫기\tCtrl+W");
	::AppendMenuW(hBar, MF_POPUP, (UINT_PTR)hWin, L"창(&W)");

	::SetMenu(_desc.hWnd, hBar);
}

void Application::ToggleSubWindow()
{
	if (!_subWindow.GetHWnd())
	{
		_subWindow.Create(_desc.hInstance, _desc.hWnd,
			L"Jellyto Studio - 툴 패널",
			SUB_WINDOW_WIDTH, SUB_WINDOW_HEIGHT);
	}
	_subWindow.Toggle();
}

// ── WndProc ──────────────────────────────────────────────────────────────
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
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;

		case WM_COMMAND:
			switch ((AppMenuCmd)LOWORD(wParam))
			{
			case AppMenuCmd::ToggleSubWindow: self->ToggleSubWindow(); return 0;
			case AppMenuCmd::Exit:            ::PostQuitMessage(0);    return 0;
			}
			break;

		case WM_KEYDOWN:
			if ((::GetKeyState(VK_CONTROL) & 0x8000) && wParam == 'W')
			{
				self->ToggleSubWindow();
				return 0;
			}
			break;
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

ATOM Application::MyRegisterClass()
{
	WNDCLASSEXW wcex   = {};
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
	Graphics::Get()->Initialize(_desc.hWnd);
	return TRUE;
}