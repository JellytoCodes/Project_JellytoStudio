#include "Framework.h"
#include "Graphics.h"

void Graphics::Initialize(HWND hwnd, UINT width, UINT height)
{
    _hwnd          = hwnd;
    _windowSize.x  = static_cast<float>(width);
    _windowSize.y  = static_cast<float>(height);

    SetViewport(_windowSize.x, _windowSize.y);
    CreateDeviceAndSwapChain();
    CreateRenderTargetView();
    CreateDepthStencilView();
}

void Graphics::ResizeBuffers(UINT width, UINT height)
{
    if (_deviceContext == nullptr) return;
    if (width == 0 || height == 0) return;

    _windowSize.x = static_cast<float>(width);
    _windowSize.y = static_cast<float>(height);

    _deviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    _renderTargetView.Reset();
    _depthStencilView.Reset();
    _depthStencilTexture.Reset();

    CHECK(_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));

    CreateRenderTargetView();
    CreateDepthStencilView();

    SetViewport(_windowSize.x, _windowSize.y);

    InvalidateStateCache();
}

void Graphics::RenderBegin()
{
    InvalidateStateCache();
    _deviceContext->OMSetRenderTargets(
        1,
        _renderTargetView.GetAddressOf(),
        _depthStencilView.Get());
    _deviceContext->ClearRenderTargetView(_renderTargetView.Get(), _clearColor);
    _deviceContext->ClearDepthStencilView(
        _depthStencilView.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1, 0);
    _vp.RSSetViewport(_deviceContext);
}

void Graphics::RenderEnd()
{
    HRESULT hr = _swapChain->Present(1, 0);
    CHECK(hr);
}

void Graphics::CreateDeviceAndSwapChain()
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferDesc.Width                   = static_cast<UINT>(_windowSize.x);
    desc.BufferDesc.Height                  = static_cast<UINT>(_windowSize.y);
    desc.BufferDesc.RefreshRate.Numerator   = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count                   = 1;
    desc.SampleDesc.Quality                 = 0;
    desc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount                        = 1;
    desc.OutputWindow                       = _hwnd;
    desc.Windowed                           = TRUE;
    desc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = ::D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &desc,
        _swapChain.GetAddressOf(),
        _device.GetAddressOf(),
        nullptr,
        _deviceContext.GetAddressOf());
    CHECK(hr);
}

void Graphics::CreateRenderTargetView()
{
    ComPtr<ID3D11Texture2D> backBuffer;
    CHECK(_swapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(backBuffer.GetAddressOf())));
    CHECK(_device->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        _renderTargetView.GetAddressOf()));
}

void Graphics::CreateDepthStencilView()
{
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width            = static_cast<UINT>(_windowSize.x);
        desc.Height           = static_cast<UINT>(_windowSize.y);
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_DEFAULT;
        desc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
        CHECK(_device->CreateTexture2D(&desc, nullptr, _depthStencilTexture.GetAddressOf()));
    }
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;
        CHECK(_device->CreateDepthStencilView(
            _depthStencilTexture.Get(),
            &desc,
            _depthStencilView.GetAddressOf()));
    }
}

void Graphics::SetViewport(float width, float height,
                            float x, float y,
                            float minDepth, float maxDepth)
{
    _vp.Set(width, height, x, y, minDepth, maxDepth);
}

void Graphics::InvalidateStateCache()
{
    _stateCache.rsValid    = false;
    _stateCache.dssValid   = false;
    _stateCache.blendValid = false;
}

void Graphics::SetRasterizerState(ID3D11RasterizerState* state)
{
    if (_stateCache.rsValid && _stateCache.rsState == state) return;
    _deviceContext->RSSetState(state);
    _stateCache.rsState = state;
    _stateCache.rsValid = true;
}

void Graphics::SetDepthStencilState(ID3D11DepthStencilState* state, UINT stencilRef)
{
    if (_stateCache.dssValid
        && _stateCache.dssState  == state
        && _stateCache.stencilRef == stencilRef) return;

    _deviceContext->OMSetDepthStencilState(state, stencilRef);
    _stateCache.dssState   = state;
    _stateCache.stencilRef = stencilRef;
    _stateCache.dssValid   = true;
}

void Graphics::SetBlendState(ID3D11BlendState* state,
                              const FLOAT* blendFactor, UINT sampleMask)
{
    const bool factorSame = (::memcmp(
        _stateCache.blendFactor,
        blendFactor,
        sizeof(FLOAT) * 4) == 0);

    if (_stateCache.blendValid
        && _stateCache.blendState == state
        && _stateCache.sampleMask == sampleMask
        && factorSame) return;

    _deviceContext->OMSetBlendState(state, blendFactor, sampleMask);
    _stateCache.blendState = state;
    _stateCache.sampleMask = sampleMask;
    _stateCache.blendValid = true;
    ::memcpy(_stateCache.blendFactor, blendFactor, sizeof(FLOAT) * 4);
}
