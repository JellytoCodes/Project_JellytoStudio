#pragma once
#include "Viewport.h"

class Graphics
{
    DECLARE_SINGLE(Graphics);

public:
    void Initialize(HWND hwnd);

    void RenderBegin();
    void RenderEnd();

    ComPtr<ID3D11Device>        GetDevice() { return _device; }
    ComPtr<ID3D11DeviceContext> GetDeviceContext() { return _deviceContext; }

    void     SetViewport(float width, float height,
        float x = 0, float y = 0,
        float minDepth = 0, float maxDepth = 1);
    Viewport& GetViewport() { return _vp; }

    void SetRasterizerState(ID3D11RasterizerState* state);
    void SetDepthStencilState(ID3D11DepthStencilState* state, UINT stencilRef);
    void SetBlendState(ID3D11BlendState* state,
        const FLOAT* blendFactor,
        UINT                     sampleMask);

    void InvalidateStateCache();

private:
    void CreateDeviceAndSwapChain();
    void CreateRenderTargetView();
    void CreateDepthStencilView();

    HWND _hwnd = {};

    ComPtr<ID3D11Device>           _device;
    ComPtr<ID3D11DeviceContext>    _deviceContext;
    ComPtr<IDXGISwapChain>         _swapChain;
    ComPtr<ID3D11RenderTargetView> _renderTargetView;
    ComPtr<ID3D11Texture2D>        _depthStencilTexture;
    ComPtr<ID3D11DepthStencilView> _depthStencilView;
    Viewport _vp;
    FLOAT    _clearColor[4] = { 1.f, 1.f, 1.f, 1.f };

    struct ShadowStateCache
    {
        ID3D11RasterizerState* rsState = nullptr;
        ID3D11DepthStencilState* dssState = nullptr;
        UINT                     stencilRef = 0;
        ID3D11BlendState* blendState = nullptr;
        FLOAT                    blendFactor[4] = { 0, 0, 0, 0 };
        UINT                     sampleMask = 0xFFFFFFFF;

        // 포인터 비교로 충분 — 같은 State 객체면 같은 설정
        bool rsValid = false;
        bool dssValid = false;
        bool blendValid = false;
    };
    ShadowStateCache _stateCache;
};