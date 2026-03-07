
#include "Framework.h"
#include "Application.h"

#include "Entity/Components/MeshRenderer.h"
#include "Entity/Entity.h"
#include "Scene/SceneManager.h"

bool Application::Initialize(const ApplicationDesc& desc)
{
	_desc = desc;

	if (!MyRegisterClass())
		return false;

	if (!InitInstance())
		return false;

	auto scene = make_shared<Scene>();

    auto box = make_shared<Entity>();
    
    box->AddComponent(make_shared<Transform>());
    box->AddComponent(make_shared<MeshRenderer>());

    scene->Add(box);

    GET_SINGLE(SceneManager)->ChangeScene(scene);

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

void Application::Update()
{
	GET_SINGLE(SceneManager)->Update();
}

void Application::Render()
{
	Graphics::Get()->RenderBegin();

	GET_SINGLE(SceneManager)->Render();

    Graphics::Get()->RenderEnd();
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