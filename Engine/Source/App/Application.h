#pragma once

#include "Pipeline/ConstantBuffer.h"
#include "Graphics/Graphics.h"
#include "Pipeline/IndexBuffer.h"
#include "Pipeline/Shader.h"
#include "Pipeline/VertexBuffer.h"
#include "Types/VertexData.h"

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

	vector<VertexColorData> vertices = {
		{ Vec3(-0.5f, -0.5f, -0.5f), Color(1.f, 0.f, 0.f, 1.f) }, // 0
	    { Vec3(-0.5f,  0.5f, -0.5f), Color(0.f, 1.f, 0.f, 1.f) }, // 1
	    { Vec3( 0.5f,  0.5f, -0.5f), Color(0.f, 0.f, 1.f, 1.f) }, // 2
	    { Vec3( 0.5f, -0.5f, -0.5f), Color(1.f, 1.f, 1.f, 1.f) }, // 3
	    { Vec3(-0.5f, -0.5f,  0.5f), Color(1.f, 0.f, 0.f, 1.f) }, // 4
	    { Vec3(-0.5f,  0.5f,  0.5f), Color(0.f, 1.f, 0.f, 1.f) }, // 5
	    { Vec3( 0.5f,  0.5f,  0.5f), Color(0.f, 0.f, 1.f, 1.f) }, // 6
	    { Vec3( 0.5f, -0.5f,  0.5f), Color(1.f, 1.f, 1.f, 1.f) }, // 7
	};

	vector<uint32> indices = {
	    0, 1, 2, 0, 2, 3, // ¾Õ¸é
	    4, 5, 6, 4, 6, 7, // µÞ¸é
	    4, 5, 1, 4, 1, 0, // ¿ÞÂÊ
	    3, 2, 6, 3, 6, 7, // ¿À¸¥ÂÊ
	    1, 5, 6, 1, 6, 2, // À­¸é
	    0, 4, 7, 0, 7, 3  // ¾Æ·§¸é
	};

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

	Matrix _matWorld = Matrix::Identity;
};