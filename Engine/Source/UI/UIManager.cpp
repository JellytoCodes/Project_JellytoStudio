#include "Framework.h"
#include "UIManager.h"
#include "Graphics/Graphics.h"

// ── 인라인 HLSL ─────────────────────────────────────────────────────────
// IMGUI와 동일하게 런타임 컴파일

static const char* g_VS_HLSL = R"(
cbuffer UIBuffer : register(b0)
{
    float2 ScreenSize;
    float2 _pad;
};
struct VS_IN  { float2 pos:POSITION; float2 uv:TEXCOORD; float4 color:COLOR; };
struct VS_OUT { float4 pos:SV_POSITION; float2 uv:TEXCOORD; float4 color:COLOR; };
VS_OUT main(VS_IN i)
{
    VS_OUT o;
    o.pos   = float4(i.pos.x / ScreenSize.x * 2.0f - 1.0f,
                     1.0f - i.pos.y / ScreenSize.y * 2.0f,
                     0.0f, 1.0f);
    o.uv    = i.uv;
    o.color = i.color;
    return o;
}
)";

static const char* g_PS_COLOR_HLSL = R"(
struct PS_IN { float4 pos:SV_POSITION; float2 uv:TEXCOORD; float4 color:COLOR; };
float4 main(PS_IN i) : SV_TARGET { return i.color; }
)";

static const char* g_PS_TEX_HLSL = R"(
Texture2D    tex : register(t0);
SamplerState smp : register(s0);
struct PS_IN { float4 pos:SV_POSITION; float2 uv:TEXCOORD; float4 color:COLOR; };
float4 main(PS_IN i) : SV_TARGET { return tex.Sample(smp, i.uv) * i.color; }
)";

// ── Init ─────────────────────────────────────────────────────────────────
void UIManager::Init(float screenW, float screenH)
{
    _screenW = screenW;
    _screenH = screenH;
    CreateDeviceObjects();
    CreateBuffers();
}

void UIManager::SetScreenSize(float w, float h) { _screenW = w; _screenH = h; }

void UIManager::CreateDeviceObjects()
{
    auto device = Graphics::Get()->GetDevice();

    // ── 셰이더 컴파일 ────────────────────────────────────────────────
    ComPtr<ID3DBlob> vsBlob, psColorBlob, psTexBlob, errBlob;

    auto compile = [&](const char* src, const char* target, ComPtr<ID3DBlob>& out)
    {
        HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
            "main", target, 0, 0, out.GetAddressOf(), errBlob.GetAddressOf());
        if (FAILED(hr))
        {
            if (errBlob)
                ::OutputDebugStringA((char*)errBlob->GetBufferPointer());
            assert(false);
        }
    };

    compile(g_VS_HLSL,       "vs_5_0", vsBlob);
    compile(g_PS_COLOR_HLSL, "ps_5_0", psColorBlob);
    compile(g_PS_TEX_HLSL,   "ps_5_0", psTexBlob);

    CHECK(device->CreateVertexShader(
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        nullptr, _vs.GetAddressOf()));
    CHECK(device->CreatePixelShader(
        psColorBlob->GetBufferPointer(), psColorBlob->GetBufferSize(),
        nullptr, _psColor.GetAddressOf()));
    CHECK(device->CreatePixelShader(
        psTexBlob->GetBufferPointer(), psTexBlob->GetBufferSize(),
        nullptr, _psTex.GetAddressOf()));

    // ── InputLayout ───────────────────────────────────────────────────
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0,  0,                           D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    CHECK(device->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        _inputLayout.GetAddressOf()));

    // ── ConstantBuffer (ScreenSize) ───────────────────────────────────
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth      = sizeof(float) * 4; // float2 + float2 pad (16-byte aligned)
    cbd.Usage          = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    CHECK(device->CreateBuffer(&cbd, nullptr, _cbuffer.GetAddressOf()));

    // ── SamplerState ──────────────────────────────────────────────────
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    CHECK(device->CreateSamplerState(&sd, _sampler.GetAddressOf()));

    // ── BlendState (Alpha Blend) ──────────────────────────────────────
    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable           = TRUE;
    bd.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    CHECK(device->CreateBlendState(&bd, _blendState.GetAddressOf()));

    // ── DepthStencilState (Depth OFF) ─────────────────────────────────
    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable   = FALSE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    CHECK(device->CreateDepthStencilState(&dsd, _depthState.GetAddressOf()));

    // ── RasterizerState (No Cull) ─────────────────────────────────────
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode        = D3D11_FILL_SOLID;
    rd.CullMode        = D3D11_CULL_NONE;
    rd.ScissorEnable   = FALSE;
    rd.DepthClipEnable = FALSE;
    CHECK(device->CreateRasterizerState(&rd, _rasterState.GetAddressOf()));
}

void UIManager::CreateBuffers()
{
    _vbCap = 4096;
    _ibCap = 8192;
    auto device = Graphics::Get()->GetDevice();

    D3D11_BUFFER_DESC vbd = {};
    vbd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    vbd.ByteWidth      = _vbCap * sizeof(VertexUI);
    vbd.Usage          = D3D11_USAGE_DYNAMIC;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    CHECK(device->CreateBuffer(&vbd, nullptr, _vb.GetAddressOf()));

    D3D11_BUFFER_DESC ibd = {};
    ibd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    ibd.ByteWidth      = _ibCap * sizeof(uint32);
    ibd.Usage          = D3D11_USAGE_DYNAMIC;
    ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    CHECK(device->CreateBuffer(&ibd, nullptr, _ib.GetAddressOf()));
}

void UIManager::UpdateBuffers()
{
    if (_vertices.size() > _vbCap || _indices.size() > _ibCap)
    {
        _vbCap = static_cast<uint32>(_vertices.size() * 2);
        _ibCap = static_cast<uint32>(_indices.size()  * 2);
        _vb.Reset(); _ib.Reset();
        CreateBuffers();
    }

    auto dc = Graphics::Get()->GetDeviceContext();
    {
        D3D11_MAPPED_SUBRESOURCE ms = {};
        dc->Map(_vb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        memcpy(ms.pData, _vertices.data(), _vertices.size() * sizeof(VertexUI));
        dc->Unmap(_vb.Get(), 0);
    }
    {
        D3D11_MAPPED_SUBRESOURCE ms = {};
        dc->Map(_ib.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        memcpy(ms.pData, _indices.data(), _indices.size() * sizeof(uint32));
        dc->Unmap(_ib.Get(), 0);
    }
}

// ── Render ────────────────────────────────────────────────────────────────
void UIManager::Render()
{
    if (!_vs || _cmds.empty())
    {
        _vertices.clear(); _indices.clear(); _cmds.clear();
        return;
    }

    UpdateBuffers();

    auto dc = Graphics::Get()->GetDeviceContext();

    // ── ConstantBuffer 업데이트 (ScreenSize) ─────────────────────────
    {
        D3D11_MAPPED_SUBRESOURCE ms = {};
        dc->Map(_cbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        float data[4] = { _screenW, _screenH, 0.f, 0.f };
        memcpy(ms.pData, data, sizeof(data));
        dc->Unmap(_cbuffer.Get(), 0);
    }

    // ── IA ──────────────────────────────────────────────────────────
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dc->IASetInputLayout(_inputLayout.Get());
    uint32 stride = sizeof(VertexUI), offset = 0;
    dc->IASetVertexBuffers(0, 1, _vb.GetAddressOf(), &stride, &offset);
    dc->IASetIndexBuffer(_ib.Get(), DXGI_FORMAT_R32_UINT, 0);

    // ── VS ──────────────────────────────────────────────────────────
    dc->VSSetShader(_vs.Get(), nullptr, 0);
    dc->VSSetConstantBuffers(0, 1, _cbuffer.GetAddressOf());

    // ── OM 상태 ─────────────────────────────────────────────────────
    float blendFactor[4] = { 0, 0, 0, 0 };
    dc->OMSetBlendState(_blendState.Get(), blendFactor, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(_depthState.Get(), 0);
    dc->RSSetState(_rasterState.Get());

    // ── 샘플러 ──────────────────────────────────────────────────────
    dc->PSSetSamplers(0, 1, _sampler.GetAddressOf());

    // ── 커맨드별 드로우 ─────────────────────────────────────────────
    for (const auto& cmd : _cmds)
    {
        if (cmd.pass == 1)
        {
            if (!cmd.srv) continue;
            dc->PSSetShader(_psTex.Get(), nullptr, 0);
            dc->PSSetShaderResources(0, 1, cmd.srv.GetAddressOf());
        }
        else
        {
            dc->PSSetShader(_psColor.Get(), nullptr, 0);
            // 이전 텍스처 바인딩 해제
            ID3D11ShaderResourceView* nullSRV = nullptr;
            dc->PSSetShaderResources(0, 1, &nullSRV);
        }

        dc->DrawIndexed(cmd.indexCount, cmd.indexOffset, 0);
    }

    // ── 사용한 SRV 바인딩 해제 (다음 프레임 3D 렌더에 영향 방지) ────
    ID3D11ShaderResourceView* nullSRV = nullptr;
    dc->PSSetShaderResources(0, 1, &nullSRV);

    _vertices.clear();
    _indices.clear();
    _cmds.clear();
}

// ── DrawList API ──────────────────────────────────────────────────────────
void UIManager::PushQuad(float x, float y, float w, float h,
                          Color color, Vec2 uvMin, Vec2 uvMax,
                          uint32 pass, ComPtr<ID3D11ShaderResourceView> srv)
{
    uint32 base    = static_cast<uint32>(_vertices.size());
    uint32 idxBase = static_cast<uint32>(_indices.size());

    _vertices.push_back({ Vec2(x,     y    ), Vec2(uvMin.x, uvMin.y), color });
    _vertices.push_back({ Vec2(x + w, y    ), Vec2(uvMax.x, uvMin.y), color });
    _vertices.push_back({ Vec2(x + w, y + h), Vec2(uvMax.x, uvMax.y), color });
    _vertices.push_back({ Vec2(x,     y + h), Vec2(uvMin.x, uvMax.y), color });

    _indices.push_back(base+0); _indices.push_back(base+1); _indices.push_back(base+2);
    _indices.push_back(base+0); _indices.push_back(base+2); _indices.push_back(base+3);

    // 텍스처 커맨드는 srv가 다르면 배칭 불가 (각각 별도 DrawCall)
    if (!_cmds.empty() && _cmds.back().pass == pass && _cmds.back().srv == srv)
        _cmds.back().indexCount += 6;
    else
        _cmds.push_back({ idxBase, 6, pass, srv });
}

void UIManager::AddRect(float x, float y, float w, float h, Color color)
{
    PushQuad(x, y, w, h, color, Vec2(0,0), Vec2(1,1), 0, nullptr);
}

void UIManager::AddRectBorder(float x, float y, float w, float h, Color color, float t)
{
    AddRect(x,         y,         w, t, color);
    AddRect(x,         y + h - t, w, t, color);
    AddRect(x,         y,         t, h, color);
    AddRect(x + w - t, y,         t, h, color);
}

void UIManager::AddTexturedRect(float x, float y, float w, float h,
                                 Color tint, ComPtr<ID3D11ShaderResourceView> srv)
{
    PushQuad(x, y, w, h, tint, Vec2(0,0), Vec2(1,1), 1, srv);
}

void UIManager::AddText(const std::wstring& text,
                         float x, float y, float w, float h,
                         Color color, int fontSize, const std::wstring& fontName)
{
    if (text.empty()) return;
    uint32 tw = static_cast<uint32>(w);
    uint32 th = static_cast<uint32>(h);
    if (!tw || !th) return;

    auto srv = BuildTextSRV(text, tw, th, color, fontSize, fontName);
    if (srv)
        PushQuad(x, y, w, h, Color(1,1,1,1), Vec2(0,0), Vec2(1,1), 1, srv);
}

// ── CPU DIB → SRV ────────────────────────────────────────────────────────
ComPtr<ID3D11ShaderResourceView> UIManager::BuildTextSRV(
    const std::wstring& text, uint32 tw, uint32 th,
    Color color, int fontSize, const std::wstring& fontName)
{
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = static_cast<LONG>(tw);
    bmi.bmiHeader.biHeight      = -static_cast<LONG>(th); // top-down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void*   pBits = nullptr;
    HDC     hdc   = ::CreateCompatibleDC(nullptr);
    HBITMAP hBmp  = ::CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    if (!hBmp) { ::DeleteDC(hdc); return nullptr; }
    ::SelectObject(hdc, hBmp);

    RECT rc = { 0, 0, (LONG)tw, (LONG)th };
    HBRUSH hBr = ::CreateSolidBrush(RGB(0, 0, 0));
    ::FillRect(hdc, &rc, hBr);
    ::DeleteObject(hBr);

    COLORREF cr = RGB(
        static_cast<BYTE>(color.R() * 255.f),
        static_cast<BYTE>(color.G() * 255.f),
        static_cast<BYTE>(color.B() * 255.f));
    HFONT hFont = ::CreateFontW(fontSize, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, fontName.c_str());
    HFONT hOld = (HFONT)::SelectObject(hdc, hFont);
    ::SetBkMode(hdc, TRANSPARENT);
    ::SetTextColor(hdc, cr);
    ::DrawTextW(hdc, text.c_str(), -1, &rc,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
    ::SelectObject(hdc, hOld);
    ::DeleteObject(hFont);

    // BGRA(DIB) → RGBA(D3D), 텍스트 밝기 = 알파
    std::vector<uint8_t> rgba(tw * th * 4);
    uint8_t* src = static_cast<uint8_t*>(pBits);
    for (uint32 i = 0; i < tw * th; i++)
    {
        uint8_t b = src[i*4+0], g = src[i*4+1], r = src[i*4+2];
        uint8_t a = static_cast<uint8_t>((static_cast<uint32>(r) + g + b) / 3);
        rgba[i*4+0] = r;
        rgba[i*4+1] = g;
        rgba[i*4+2] = b;
        rgba[i*4+3] = a;
    }
    ::DeleteObject(hBmp);
    ::DeleteDC(hdc);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = tw;
    td.Height           = th;
    td.MipLevels        = 1;
    td.ArraySize        = 1;
    td.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_IMMUTABLE;
    td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem     = rgba.data();
    sd.SysMemPitch = tw * 4;

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(Graphics::Get()->GetDevice()->CreateTexture2D(&td, &sd, tex.GetAddressOf())))
        return nullptr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvd.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;

    ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(Graphics::Get()->GetDevice()->CreateShaderResourceView(
        tex.Get(), &srvd, srv.GetAddressOf())))
        return nullptr;

    return srv;
}