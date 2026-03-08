
#include "Framework.h"
#include "Application.h"

#include "Core/Interfaces/IExecute.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/SceneManager.h"

bool Application::Initialize(const ApplicationDesc& desc)
{
	_desc = desc;
	assert(_desc.app != nullptr);

	MyRegisterClass();

	if (!InitInstance())
		return false;

	GET_SINGLE(TimeManager)->Init();
    GET_SINGLE(InputManager)->Init(_desc.hWnd);

	_desc.app->Init();

	return true;
}

WPARAM Application::Run()
{
	MSG msg = { 0 };

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
			Render();
		}
	}

	return msg.wParam;
}

void Application::Shutdown()
{
	
}

void Application::Update()
{
	GET_SINGLE(TimeManager)->Update();
    GET_SINGLE(InputManager)->Update();

	UpdateWindowTitle();
	_desc.app->Update();

	GET_SINGLE(SceneManager)->Update();
}

void Application::Render()
{
	Graphics::Get()->RenderBegin();

	GET_SINGLE(SceneManager)->Render();
	_desc.app->Render();

    Graphics::Get()->RenderEnd();
}

void Application::UpdateWindowTitle()
{
	float fps = GET_SINGLE(TimeManager)->GetFps();
    float totalTime = GET_SINGLE(TimeManager)->GetTotalTime();

    wchar_t text[100];
    swprintf_s(text, L"%s (FPS: %.2f, TotalTime: %.2f s)", _desc.appName.c_str(), fps, totalTime);

    ::SetWindowTextW(_desc.hWnd, text);
}

LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_NCCREATE)
	{
		LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	Application* app = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	if (app)
	{
		switch (message)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

ATOM Application::MyRegisterClass()
{
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = Application::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = _desc.hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = _desc.appName.c_str();
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	return RegisterClassExW(&wcex);
}

BOOL Application::InitInstance()
{
	RECT windowRect = { 0, 0, static_cast<LONG>(_desc.width), static_cast<LONG>(_desc.height) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	_desc.hWnd = CreateWindowW(
		_desc.appName.c_str(),
		_desc.appName.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL, NULL, _desc.hInstance,
		this
	);

	if (!_desc.hWnd) return FALSE;

	::ShowWindow(_desc.hWnd, SW_SHOWNORMAL);
	::UpdateWindow(_desc.hWnd);

	Graphics::Get()->Initialize(_desc.hWnd);

    return TRUE;
}