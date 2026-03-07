#pragma once

class Graphics
{
public:
	bool Initialize(HWND hWnd, UINT width, UINT height);
	void BeginRender();
	void EndRender();
	void Shutdown();

	ComPtr<ID3D11Device> GetDevice() { return _device; }
	ComPtr<ID3D11DeviceContext> GetDeviceContext() { return _deviceContext; }

private:
	ComPtr<ID3D11Device> _device;
	ComPtr<ID3D11DeviceContext> _deviceContext;
	
	ComPtr<IDXGISwapChain> _swapChain;
	
	ComPtr<ID3D11RenderTargetView> _renderTargetView;

	D3D11_VIEWPORT _viewport = { 0 };
};