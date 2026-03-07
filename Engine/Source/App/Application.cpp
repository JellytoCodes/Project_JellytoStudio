
#include "Framework.h"
#include "Application.h"

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
	_graphics->Initialize(_desc.hWnd);

	_shader = std::make_unique<Shader>();
	_shader->Create(_graphics->GetDevice(), L"../Engine/Shaders/Default.hlsl");

	_vertexBuffer = std::make_unique<VertexBuffer>();
	_vertexBuffer->Create<VertexColorData>(_graphics->GetDevice(), vertices);

	_indexBuffer = std::make_unique<IndexBuffer>();
	_indexBuffer->Create(_graphics->GetDevice(), indices);

	_constantBuffer = std::make_unique<ConstantBuffer<TransformData>>();
	_constantBuffer->Create(_graphics->GetDevice());

    return TRUE;
}

void Application::Update()
{
    static float angle = 0.f;
    angle += 0.001f;

    _matWorld = Matrix::CreateRotationY(angle) * Matrix::CreateRotationX(angle);	
}

void Application::Render()
{
	_graphics->RenderBegin();

    auto deviceContext = _graphics->GetDeviceContext();

    _vertexBuffer->PushData(deviceContext);
    _indexBuffer->PushData(deviceContext);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    _shader->Bind(deviceContext);

	Vec3 eye(0.f, 0.f, -5.f);
	Vec3 focus(0.f, 0.f, 0.f);
	Vec3 up(0.f, 1.f, 0.f);
	Matrix matView = Matrix::CreateLookAt(eye, focus, up);

	float aspectRatio = 800.f / 600.f;
	Matrix matProj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f, aspectRatio, 0.1f, 1000.f);

	TransformData data;
	data.world = _matWorld.Transpose();
	data.view = matView.Transpose();
	data.projection = matProj.Transpose();

    _constantBuffer->CopyData(deviceContext, data);

	auto bufferPtr = _constantBuffer->GetComPtr();
	deviceContext->VSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());

    deviceContext->DrawIndexed(_indexBuffer->GetCount(), 0, 0);

    _graphics->RenderEnd();
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