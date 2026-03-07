#pragma once

#include "Viewport.h"

class Graphics
{
public:
	void Initialize(HWND hwnd);

	void RenderBegin();
	void RenderEnd();

	ComPtr<ID3D11Device> GetDevice()				{ return _device; }
	ComPtr<ID3D11DeviceContext> GetDeviceContext()	{ return _deviceContext; }

	void SetViewport(float width, float height, float x= 0, float y = 0, float minDepth = 0, float maxDepth = 1);
	Viewport& GetViewport() { return _vp; }

private:
	void CreateDeviceAndSwapChain();
	void CreateRenderTargetView();
	void CreateDepthStencilView();

	HWND _hwnd = { };

	// Device & SwapChain
	ComPtr<ID3D11Device>			_device;
	ComPtr<ID3D11DeviceContext>		_deviceContext;
	ComPtr<IDXGISwapChain>			_swapChain;

	// RTV
	ComPtr<ID3D11RenderTargetView>	_renderTargetView;

	// DSV
	ComPtr<ID3D11Texture2D>			_depthStencilTexture;
	ComPtr<ID3D11DepthStencilView>	_depthStencilView;

	// Misc
	Viewport _vp;

	FLOAT _clearColor[4] = {0.f, 0.f, 0.f, 1.f};
};
