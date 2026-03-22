#pragma once

class UIManager
{
    DECLARE_SINGLE(UIManager)

public:
    void Init(float screenW, float screenH);
    void Render();                          // Scene::Render() 이후 호출
    void SetScreenSize(float w, float h);

    // ── DrawList API (Widget/UIText/UIButton 에서 호출) ──────────────
    void AddRect(float x, float y, float w, float h, Color color);
    void AddRectBorder(float x, float y, float w, float h, Color color, float thickness = 1.f);
    void AddTexturedRect(float x, float y, float w, float h, Color tint,
                         ComPtr<ID3D11ShaderResourceView> srv);
    void AddText(const std::wstring& text,
                 float x, float y, float w, float h,
                 Color color, int fontSize = 18,
                 const std::wstring& fontName = L"Arial");

private:
    // ── 내부 드로우 커맨드 ──────────────────────────────────────────
    struct DrawCmd
    {
        uint32 indexOffset = 0;
        uint32 indexCount  = 0;
        uint32 pass        = 0;   // 0=색상, 1=텍스처
        ComPtr<ID3D11ShaderResourceView> srv = nullptr;
    };

    void PushQuad(float x, float y, float w, float h, Color color,
                  Vec2 uvMin, Vec2 uvMax, uint32 pass,
                  ComPtr<ID3D11ShaderResourceView> srv);

    ComPtr<ID3D11ShaderResourceView> BuildTextSRV(
        const std::wstring& text, uint32 tw, uint32 th,
        Color color, int fontSize, const std::wstring& fontName);

    void CreateBuffers();
    void UpdateBuffers();

    // DrawList
    std::vector<VertexUI>  _vertices;
    std::vector<uint32>    _indices;
    std::vector<DrawCmd>   _cmds;

    // GPU 버퍼 (DYNAMIC)
    ComPtr<ID3D11Buffer>      _vb;
    ComPtr<ID3D11Buffer>      _ib;
    uint32 _vbCap = 0, _ibCap = 0;

    // InputLayout + Shader
    ComPtr<ID3D11InputLayout>     _inputLayout;
    std::shared_ptr<class Shader> _shader;

    float _screenW = 1280.f;
    float _screenH =  720.f;
};