#pragma once

class UIManager
{
    DECLARE_SINGLE(UIManager)

public:
    void Init(float screenW, float screenH);
    void Render();
    void SetScreenSize(float w, float h);

    void AddRect(float x, float y, float w, float h, Color color);
    void AddRectBorder(float x, float y, float w, float h, Color color, float thickness = 1.f);
    void AddTexturedRect(float x, float y, float w, float h, Color tint,
                         ComPtr<ID3D11ShaderResourceView> srv);
    void AddText(const std::wstring& text,
                 float x, float y, float w, float h,
                 Color color, int fontSize = 18,
                 const std::wstring& fontName = L"Arial");

private:
    struct DrawCmd
    {
        uint32 indexOffset = 0;
        uint32 indexCount  = 0;
        uint32 pass        = 0;   // 0=color, 1=texture
        ComPtr<ID3D11ShaderResourceView> srv = nullptr;
    };

    void PushQuad(float x, float y, float w, float h, Color color,
                  Vec2 uvMin, Vec2 uvMax, uint32 pass,
                  ComPtr<ID3D11ShaderResourceView> srv);

    ComPtr<ID3D11ShaderResourceView> BuildTextSRV(
        const std::wstring& text, uint32 tw, uint32 th,
        Color color, int fontSize, const std::wstring& fontName);

    void CreateDeviceObjects();
    void CreateBuffers();
    void UpdateBuffers();

    // DrawList
    std::vector<VertexUI>  _vertices;
    std::vector<uint32>    _indices;
    std::vector<DrawCmd>   _cmds;

    // GPU 버퍼
    ComPtr<ID3D11Buffer>      _vb;
    ComPtr<ID3D11Buffer>      _ib;
    uint32 _vbCap = 0, _ibCap = 0;

    // 직접 D3D11 오브젝트 (IMGUI 방식)
    ComPtr<ID3D11InputLayout>       _inputLayout;
    ComPtr<ID3D11VertexShader>      _vs;
    ComPtr<ID3D11PixelShader>       _psColor;   // pass=0
    ComPtr<ID3D11PixelShader>       _psTex;     // pass=1
    ComPtr<ID3D11Buffer>            _cbuffer;   // ScreenSize
    ComPtr<ID3D11SamplerState>      _sampler;
    ComPtr<ID3D11BlendState>        _blendState;
    ComPtr<ID3D11DepthStencilState> _depthState;
    ComPtr<ID3D11RasterizerState>   _rasterState;

    float _screenW = 1280.f;
    float _screenH =  720.f;
};