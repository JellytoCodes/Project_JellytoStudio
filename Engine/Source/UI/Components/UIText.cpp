#include "Framework.h"
#include "UIText.h"
#include "UI/UIManager.h"
#include "Graphics/Graphics.h"

UIText::UIText()
    : Super(ComponentType::UIComponent)
{
}

void UIText::Update()
{
    if (_getter)
        _text = _getter();
}

void UIText::DrawUI(float ox, float oy)
{
    if (_text.empty()) return;

    float ax = ox + _x;
    float ay = oy + _y;

    // 배경 패널
    if (_useBg)
        GET_SINGLE(UIManager)->AddRect(ax, ay, _w, _h, _bgColor);

    // 텍스트 텍스처
    auto srv = BuildTextSRV();
    if (srv)
        GET_SINGLE(UIManager)->AddTexturedRect(ax, ay, _w, _h, Color(1,1,1,1), srv);
}

ComPtr<ID3D11ShaderResourceView> UIText::BuildTextSRV()
{
    if (_text.empty()) return nullptr;

    uint32 tw = static_cast<uint32>(_w);
    uint32 th = static_cast<uint32>(_h);
    if (tw == 0 || th == 0) return nullptr;

    // 1. DIB Section 생성 (32bit, top-down)
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = static_cast<LONG>(tw);
    bmi.bmiHeader.biHeight      = -static_cast<LONG>(th); // top-down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void*   pBits  = nullptr;
    HDC     hdc    = ::CreateCompatibleDC(nullptr);
    HBITMAP hBmp   = ::CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    ::SelectObject(hdc, hBmp);

    // 2. 배경: 검정
    RECT rc = { 0, 0, static_cast<LONG>(tw), static_cast<LONG>(th) };
    HBRUSH hBrush = ::CreateSolidBrush(RGB(0, 0, 0));
    ::FillRect(hdc, &rc, hBrush);
    ::DeleteObject(hBrush);

    // 3. 텍스트 그리기
    COLORREF cr = RGB(
        static_cast<BYTE>(_textColor.R() * 255.f),
        static_cast<BYTE>(_textColor.G() * 255.f),
        static_cast<BYTE>(_textColor.B() * 255.f));
    HFONT hFont = ::CreateFontW(_fontSize, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, _fontName.c_str());
    HFONT hOld = (HFONT)::SelectObject(hdc, hFont);
    ::SetBkMode(hdc, TRANSPARENT);
    ::SetTextColor(hdc, cr);
    ::DrawTextW(hdc, _text.c_str(), -1, &rc,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
    ::SelectObject(hdc, hOld);
    ::DeleteObject(hFont);

    // 4. BGRA → RGBA + 알파 = 밝기
    std::vector<uint8_t> rgba(tw * th * 4);
    uint8_t* src = static_cast<uint8_t*>(pBits);
    for (uint32 i = 0; i < tw * th; i++)
    {
        uint8_t b = src[i*4+0], g = src[i*4+1], r = src[i*4+2];
        uint8_t a = (uint8_t)((r + g + b) / 3);
        rgba[i*4+0]=r; rgba[i*4+1]=g; rgba[i*4+2]=b; rgba[i*4+3]=a;
    }
    ::DeleteObject(hBmp);
    ::DeleteDC(hdc);

    // 5. D3D Texture2D
    D3D11_TEXTURE2D_DESC td = {};
    td.Width=tw; td.Height=th; td.MipLevels=1; td.ArraySize=1;
    td.Format=DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count=1;
    td.Usage=D3D11_USAGE_IMMUTABLE; td.BindFlags=D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem=rgba.data(); sd.SysMemPitch=tw*4;

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(Graphics::Get()->GetDevice()->CreateTexture2D(&td, &sd, tex.GetAddressOf())))
        return nullptr;

    // 6. SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    srvd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels=1;

    ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(Graphics::Get()->GetDevice()->CreateShaderResourceView(tex.Get(), &srvd, srv.GetAddressOf())))
        return nullptr;

    return srv;
}