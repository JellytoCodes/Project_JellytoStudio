#pragma once
#include "Types/VertexData.h"

struct UIDrawCmd
{
    uint32 indexOffset  = 0;    // IndexBuffer 시작 오프셋
    uint32 indexCount   = 0;    // 그릴 인덱스 수
    uint32 pass         = 0;    // 0=색상, 1=텍스처
    ComPtr<ID3D11ShaderResourceView> srv = nullptr; // pass=1일 때 사용
};

class UIDrawList
{
public:
    void Reset();

    // 색상 사각형 (배경 패널, 버튼 등)
    void AddRect(float x, float y, float w, float h, Color color);

    // 테두리
    void AddRectBorder(float x, float y, float w, float h,
                       Color color, float thickness = 1.f);

    // 텍스처 사각형 (이미지, 텍스트 아틀라스)
    void AddRectTextured(float x, float y, float w, float h,
                         Color tint,
                         ComPtr<ID3D11ShaderResourceView> srv,
                         Vec2 uvMin = Vec2(0,0), Vec2 uvMax = Vec2(1,1));

    // 텍스트 (Win32 GDI로 CPU 비트맵 생성 → SRV → TexturedRect)
    void AddText(const std::wstring& text,
                 float x, float y, float w, float h,
                 Color color, int fontSize = 18,
                 const std::wstring& fontName = L"Arial");

    // 접근자
    const std::vector<VertexUI>&  GetVertices() const { return _vertices; }
    const std::vector<uint32>&    GetIndices()  const { return _indices;  }
    const std::vector<UIDrawCmd>& GetCmds()     const { return _cmds;     }

private:
    // 쿼드 2삼각형 추가 헬퍼
    void PushQuad(float x, float y, float w, float h,
                  Color color,
                  Vec2 uvMin, Vec2 uvMax,
                  uint32 pass,
                  ComPtr<ID3D11ShaderResourceView> srv);

    // 텍스트 → SRV (CPU 비트맵 → D3D Texture)
    ComPtr<ID3D11ShaderResourceView> BuildTextSRV(
        const std::wstring& text, uint32 width, uint32 height,
        Color color, int fontSize, const std::wstring& fontName);

    std::vector<VertexUI>  _vertices;
    std::vector<uint32>    _indices;
    std::vector<UIDrawCmd> _cmds;
};