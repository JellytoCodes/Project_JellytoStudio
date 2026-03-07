
#include "Framework.h"
#include "Application.h"

vector<Vertex> vertices = {
    { Vec3(-0.5f,  1.f, 0.0f), Color(1.0f, 0.0f, 0.0f, 1.0f) }, // 빨강
    { Vec3( 1.f, -0.5f, 0.0f), Color(0.0f, 1.0f, 0.0f, 1.0f) }, // 초록
    { Vec3(-0.5f, -0.5f, 0.0f), Color(0.0f, 0.0f, 1.0f, 1.0f) }
};

vector<uint32> indices = { 0, 1, 2 };

struct TransformData
{
    Vec4 offset;
};

bool Application::Initialize(const ApplicationDesc& desc)
{
	_desc = desc;

	if (!MyRegisterClass())
		return false;

	if (!InitInstance())
		return false;

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

	_graphics = std::make_unique<Graphics>();
	_graphics->Initialize(_desc.hWnd, _desc.width, _desc.height);

	_shader = std::make_unique<Shader>();
	_shader->Create(_graphics->GetDevice(), L"Default.hlsl");

	_vertexBuffer = std::make_unique<VertexBuffer>();
	_vertexBuffer->Create<Vertex>(_graphics->GetDevice(), vertices); // 위에서 정의한 전역 vertices

	_indexBuffer = std::make_unique<IndexBuffer>();
	_indexBuffer->Create(_graphics->GetDevice(), indices);

	_constantBuffer = std::make_unique<ConstantBuffer<TransformData>>();
	_constantBuffer->Create(_graphics->GetDevice());

    return TRUE;
}

void Application::Update()
{
	
}

void Application::Render()
{
	_graphics->BeginRender(); // 청소 시작

    auto deviceContext = _graphics->GetDeviceContext(); // 작업반장 호출

    // 1. 재료 장착
    _vertexBuffer->PushData(deviceContext);
    _indexBuffer->PushData(deviceContext);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 2. 조리법 세팅
    _shader->Bind(deviceContext);

    // 3. (옵션) 데이터 전송
    TransformData data = { Vec4(0.0f, 0.0f, 0.0f, 0.0f) };
    _constantBuffer->CopyData(deviceContext, data);

	auto bufferPtr = _constantBuffer->GetComPtr();
	deviceContext->VSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());

    // 4. 발사!
    deviceContext->DrawIndexed(_indexBuffer->GetCount(), 0, 0);

    _graphics->EndRender(); // 전광판 교체
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