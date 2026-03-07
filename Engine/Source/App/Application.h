#pragma once

#include "Graphics/Graphics.h"
#include "Types/VertexData.h"

class MeshRenderer;

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
};