#include "Framework.h"
#include "Application.h"

#include "Core/Interfaces/IExecute.h"
#include "Scene/SceneManager.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Managers/WindowManager.h"

#include "ToolWindow.h"
#include "ItemWindow.h"
#include "DetailWindow.h"
#include "Graphics/Graphics.h"


bool Application::Initialize(const ApplicationDesc& desc)
{
	_desc = desc;
	assert(_desc.app != nullptr);

	MyRegisterClass();
	if (!InitInstance()) return false;

	GET_SINGLE(WindowManager)->Init(_desc.hInstance, _desc.hWnd);

	GET_SINGLE(WindowManager)->RegisterWindow<ToolWindow>(L"ToolWindow");
	GET_SINGLE(WindowManager)->RegisterWindow<ItemWindow>(L"ItemWindow");
	GET_SINGLE(WindowManager)->RegisterWindow<DetailWindow>(L"DetailWindow");

	GET_SINGLE(TimeManager)->Init();
	GET_SINGLE(InputManager)->Init(_desc.hWnd);

	// DetailWindow / ItemWindow가 포커스를 가져도 키 입력이 동작하도록 등록
	if (auto detail = GET_SINGLE(WindowManager)->GetWindow<DetailWindow>(L"DetailWindow"))
		GET_SINGLE(InputManager)->AddAllowedWindow(detail->GetHWnd());
	if (auto item = GET_SINGLE(WindowManager)->GetWindow<ItemWindow>(L"ItemWindow"))
		GET_SINGLE(InputManager)->AddAllowedWindow(item->GetHWnd());

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
	HandleShortcuts();
	_desc.app->Update();
	_desc.app->Render();
	GET_SINGLE(SceneManager)->Render();
	Graphics::Get()->RenderEnd();
}

void Application::UpdateWindowTitle()
{
	float fps = GET_SINGLE(TimeManager)->GetFps();
	float totalTime = GET_SINGLE(TimeManager)->GetTotalTime();
	wchar_t text[100];
	swprintf_s(text, L"%s (FPS: %.2f, TotalTime: %.2f s)",
		_desc.appName.c_str(), fps, totalTime);
	::SetWindowTextW(_desc.hWnd, text);
}

// ── 메뉴바 ──────────────────────────────────────────────────────────────
void Application::CreateMainMenu()
{
	HMENU hBar = ::CreateMenu();

	// [파일] → 종료
	HMENU hFile = ::CreatePopupMenu();
	::AppendMenuW(hFile, MF_STRING, (UINT_PTR)AppMenuCmd::Exit, L"종료(&X)\tAlt+F4");
	::AppendMenuW(hBar, MF_POPUP, (UINT_PTR)hFile, L"파일(&F)");

	// [창] → 3개 윈도우 토글
	HMENU hWin = ::CreatePopupMenu();
	::AppendMenuW(hWin, MF_STRING,
		(UINT_PTR)AppMenuCmd::ToggleToolWindow, L"툴 윈도우\tCtrl+T");
	::AppendMenuW(hWin, MF_STRING,
		(UINT_PTR)AppMenuCmd::ToggleItemWindow, L"아이템 배치\tCtrl+I");
	::AppendMenuW(hWin, MF_STRING,
		(UINT_PTR)AppMenuCmd::ToggleDetailWindow, L"오브젝트 상세\tCtrl+D");
	::AppendMenuW(hBar, MF_POPUP, (UINT_PTR)hWin, L"창(&W)");

	::SetMenu(_desc.hWnd, hBar);
}

// ── 단축키 처리 (InputManager 기반) ────────────────────────────────────
void Application::HandleShortcuts()
{
	// Ctrl은 KEY_TYPE에 없으므로 GetKeyState로 체크
	if (!(::GetKeyState(VK_CONTROL) & 0x8000)) return;

	if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::T)) { ToggleToolWindow();   return; }
	if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::I)) { ToggleItemWindow();   return; }
	if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::D)) { ToggleDetailWindow(); return; }
}

// ── 창 토글 ──────────────────────────────────────────────────────────────
void Application::ToggleToolWindow()
{
	GET_SINGLE(WindowManager)->ToggleWindow(L"ToolWindow");
}

void Application::ToggleItemWindow()
{
	GET_SINGLE(WindowManager)->ToggleWindow(L"ItemWindow");
}

void Application::ToggleDetailWindow()
{
	GET_SINGLE(WindowManager)->ToggleWindow(L"DetailWindow");
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
			case AppMenuCmd::ToggleToolWindow:   self->ToggleToolWindow();   return 0;
			case AppMenuCmd::ToggleItemWindow:   self->ToggleItemWindow();   return 0;
			case AppMenuCmd::ToggleDetailWindow: self->ToggleDetailWindow(); return 0;
			case AppMenuCmd::Exit:               ::PostQuitMessage(0);       return 0;
			}
			break;

		// 단축키는 Application::Update()의 InputManager에서 처리
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

ATOM Application::MyRegisterClass()
{
	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = Application::WndProc;
	wcex.hInstance = _desc.hInstance;
	wcex.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = _desc.appName.c_str();
	wcex.hIconSm = ::LoadIcon(NULL, IDI_APPLICATION);
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