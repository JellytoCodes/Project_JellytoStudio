#pragma once
#include "UIDrawList.h"

class UIRenderer
{
    DECLARE_SINGLE(UIRenderer)

public:
    void Init(ComPtr<ID3D11Device> device,
              ComPtr<ID3D11DeviceContext> context,
              float screenW, float screenH);

    // Widget이 매 프레임 이 DrawList에 커맨드를 누적
    UIDrawList& GetDrawList() { return _drawList; }

    // 프레임 끝에 한 번 호출 — GPU로 제출 후 DrawList 리셋
    void Render();

    // 화면 크기 변경 시
    void SetScreenSize(float w, float h);

private:
    void CreateBuffers();
    void UpdateBuffers();

    ComPtr<ID3D11Device>        _device;
    ComPtr<ID3D11DeviceContext> _context;

    std::shared_ptr<class Shader> _shader;

    // Dynamic VertexBuffer / IndexBuffer (매 프레임 Map/Unmap)
    ComPtr<ID3D11Buffer> _vertexBuffer;
    ComPtr<ID3D11Buffer> _indexBuffer;
    uint32 _vbCapacity = 0;
    uint32 _ibCapacity = 0;

    ComPtr<ID3D11InputLayout> _inputLayout;

    float _screenW = 1280.f;
    float _screenH = 720.f;

    UIDrawList _drawList;
};