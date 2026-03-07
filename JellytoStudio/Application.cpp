
#include "Framework.h"
#include "Application.h"

bool Application::Initialize(const ApplicationDesc& desc)
{
	_desc = desc;

	// 1. 윈도우 클래스 등록
	if (!MyRegisterClass())
		return false;

	// 2. 윈도우 창 생성
	if (!InitInstance())
		return false;

	// 3. (나중에 여기에 DX11 장치 초기화 코드가 들어올 예정입니다)

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
			// 게임 메인 루프 가동
			Update();
			Render();
		}
	}

	return msg.wParam;
}

void Application::Shutdown()
{
	// 나중에 생성한 DX11 객체들을 여기서 안전하게 Release 해줄 겁니다.
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
	// 작업 영역(Client Area) 크기를 맞추기 위한 계산
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
		this // 중요: 'this' 포인터를 넘겨서 클래스와 윈도우를 연결
	);

	if (!_desc.hWnd) return FALSE;

	::ShowWindow(_desc.hWnd, SW_SHOWNORMAL);
	::UpdateWindow(_desc.hWnd);

	_graphics = std::make_unique<Graphics>();
    return _graphics->Initialize(_desc.hWnd, _desc.width, _desc.height);
}

void Application::Update()
{
	
}

void Application::Render()
{
	if (_graphics)
		_graphics->Render();	
}

LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// 창 생성 시 넘겨준 'this' 포인터를 보관/추출하는 로직
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