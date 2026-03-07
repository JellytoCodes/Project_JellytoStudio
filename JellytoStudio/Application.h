#pragma once

#include "ConstantBuffer.h"
#include "Graphics.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "VertexBuffer.h"

struct TransformData;

struct ApplicationDesc
{
	std::wstring appName;
	HINSTANCE hInstance;
	HWND hWnd;
	UINT width;
	UINT height;
	bool vsync = false;
	bool windowed = true;
};

class Application
{
public:
	bool Initialize(const ApplicationDesc& desc);
	WPARAM Run();
	void Shutdown();

private:
	void Update();
	void Render();

	ATOM MyRegisterClass();
	BOOL InitInstance();

	static LRESULT CALLBACK WndProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam);
	
private:
	ApplicationDesc _desc;
	std::unique_ptr<Graphics> _graphics;

	std::unique_ptr<Shader> _shader;
    std::unique_ptr<VertexBuffer> _vertexBuffer;
    std::unique_ptr<IndexBuffer> _indexBuffer;
    std::unique_ptr<ConstantBuffer<TransformData>> _constantBuffer;
};