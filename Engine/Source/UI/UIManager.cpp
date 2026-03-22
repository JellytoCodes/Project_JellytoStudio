#include "Framework.h"
#include "UIManager.h"
#include "Pipeline/Shader.h"
#include "Graphics/Graphics.h"

// ── Init ─────────────────────────────────────────────────────────────────
void UIManager::Init(float screenW, float screenH)
{
    _screenW = screenW;
    _screenH = screenH;

    _shader = std::make_shared<Shader>(L"../Engine/Shaders/UI.hlsl");

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,          0,  0,                           D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    D3DX11_PASS_DESC pd = {};
    _shader->Effect()->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&pd);
    HRESULT hr = Graphics::Get()->GetDevice()->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        pd.pIAInputSignature, pd.IAInputSignatureSize,
        _inputLayout.GetAddressOf());
    CHECK(hr);

    CreateBuffers();
}

void UIManager::SetScreenSize(float w, float h) { _screenW = w; _screenH = h; }

// ── 버퍼 생성/업데이트 ────────────────────────────────────────────────────
void UIManager::CreateBuffers()
{
    _vbCap = 4096;
    _ibCap = 8192;

    D3D11_BUFFER_DESC vbd = {};
    vbd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    vbd.ByteWidth      = _vbCap * sizeof(VertexUI);
    vbd.Usage          = D3D11_USAGE_DYNAMIC;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    CHECK(Graphics::Get()->GetDevice()->CreateBuffer(&vbd, nullptr, _vb.GetAddressOf()));

    D3D11_BUFFER_DESC ibd = {};
    ibd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    ibd.ByteWidth      = _ibCap * sizeof(uint32);
    ibd.Usage          = D3D11_USAGE_DYNAMIC;
    ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    CHECK(Graphics::Get()->GetDevice()->CreateBuffer(&ibd, nullptr, _ib.GetAddressOf()));
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
    // Init이 아직 안 됐거나 커맨드 없으면 조기 종료
    if (!_shader || _cmds.empty()) { _vertices.clear(); _indices.clear(); _cmds.clear(); return; }

    UpdateBuffers();

    auto dc = Graphics::Get()->GetDeviceContext();
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dc->IASetInputLayout(_inputLayout.Get());

    uint32 stride = sizeof(VertexUI), offset = 0;
    dc->IASetVertexBuffers(0, 1, _vb.GetAddressOf(), &stride, &offset);
    dc->IASetIndexBuffer(_ib.Get(), DXGI_FORMAT_R32_UINT, 0);

    // 화면 크기 상수버퍼 전달
    Vec4 ss(_screenW, _screenH, 0.f, 0.f);
    _shader->GetVector("ScreenSize")->SetFloatVector(reinterpret_cast<float*>(&ss));

    for (const auto& cmd : _cmds)
    {
        if (cmd.pass == 1 && cmd.srv)
            _shader->GetSRV("UITexture")->SetResource(cmd.srv.Get());

        _shader->DrawIndexed(0, cmd.pass, cmd.indexCount, cmd.indexOffset, 0);
    }

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

    if (!_cmds.empty() && _cmds.back().pass == pass && _cmds.back().srv == srv)
        _cmds.back().indexCount += 6;
    else
        _cmds.push_back({ idxBase, 6, pass, srv });
}

void UIManager::AddRect(float x, float y, float w, float h, Color color)
{
    PushQuad(x, y, w, h, color, Vec2(0,0), Vec2(1,1), 0, nullptr);
}

void UIManager::AddRectBorder(float x, float y, float w, float h,
                               Color color, float t)
{
    AddRect(x,         y,         w, t, color);
    AddRect(x,         y + h - t, w, t, color);
    AddRect(x,         y,         t, h, color);
    AddRect(x + w - t, y,         t, h, color);
}

void UIManager::AddTexturedRect(float x, float y, float w, float h,
                                 Color tint,
                                 ComPtr<ID3D11ShaderResourceView> srv)
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
    bmi.bmiHeader.biHeight      = -static_cast<LONG>(th);
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void*   pBits = nullptr;
    HDC     hdc   = ::CreateCompatibleDC(nullptr);
    HBITMAP hBmp  = ::CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    ::SelectObject(hdc, hBmp);

    RECT rc = { 0, 0, (LONG)tw, (LONG)th };
    HBRUSH hBr = ::CreateSolidBrush(RGB(0,0,0));
    ::FillRect(hdc, &rc, hBr);
    ::DeleteObject(hBr);

    COLORREF cr = RGB((BYTE)(color.R()*255), (BYTE)(color.G()*255), (BYTE)(color.B()*255));
    HFONT hFont = ::CreateFontW(fontSize, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, fontName.c_str());
    HFONT hOld = (HFONT)::SelectObject(hdc, hFont);
    ::SetBkMode(hdc, TRANSPARENT);
    ::SetTextColor(hdc, cr);
    ::DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
    ::SelectObject(hdc, hOld);
    ::DeleteObject(hFont);

    std::vector<uint8_t> rgba(tw * th * 4);
    uint8_t* src = static_cast<uint8_t*>(pBits);
    for (uint32 i = 0; i < tw * th; i++)
    {
        uint8_t b=src[i*4+0], g=src[i*4+1], r=src[i*4+2];
        uint8_t a=(uint8_t)((r+g+b)/3);
        rgba[i*4+0]=r; rgba[i*4+1]=g; rgba[i*4+2]=b; rgba[i*4+3]=a;
    }
    ::DeleteObject(hBmp);
    ::DeleteDC(hdc);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width=tw; td.Height=th; td.MipLevels=1; td.ArraySize=1;
    td.Format=DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count=1;
    td.Usage=D3D11_USAGE_IMMUTABLE; td.BindFlags=D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem=rgba.data(); sd.SysMemPitch=tw*4;

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(Graphics::Get()->GetDevice()->CreateTexture2D(&td, &sd, tex.GetAddressOf())))
        return nullptr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    srvd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels=1;

    ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(Graphics::Get()->GetDevice()->CreateShaderResourceView(tex.Get(), &srvd, srv.GetAddressOf())))
        return nullptr;
    return srv;
}