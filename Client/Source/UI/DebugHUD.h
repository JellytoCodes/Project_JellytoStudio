#pragma once

class Camera;

class DebugHUD
{
public:
    void Init(Camera* camera);
    void Update();
    void Render();

    bool IsVisible() const { return _visible; }

private:
    Camera* _pCamera = nullptr;
    bool    _visible = true;

    struct CachedLine
    {
        std::wstring        text;
        ComPtr<ID3D11ShaderResourceView> srv;
    };
    std::vector<CachedLine> _lines;

    void RebuildLines();

    static constexpr float kPanelX      = 10.f;
    static constexpr float kPanelY      = 10.f;
    static constexpr float kPanelW      = 310.f;
    static constexpr float kLineH       = 22.f;
    static constexpr float kPadX        = 10.f;
    static constexpr float kPadY        = 8.f;
    static constexpr int   kFontSize    = 15;
    static constexpr int   kLineCount   = 5;
};