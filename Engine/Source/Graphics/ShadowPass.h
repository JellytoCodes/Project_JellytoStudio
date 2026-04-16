#pragma once

#include "Pipeline/InstancingBuffer.h"
#include "Graphics/RenderPacket.h"
#include "Managers/InstancingManager.h"

class Entity;
class ModelRenderer;
struct ShadowDesc;

class ShadowPass
{
public:
    static constexpr uint32 kShadowMapSize = 1024;

    void Init();

    void Render(const std::vector<Entity*>& entities, const Vec3& lightDir);

    // 메인 패스에서 PushShadowData 호출 시 사용
    ID3D11ShaderResourceView* GetShadowSRV()  const { return _shadowSRV.Get(); }
    const ShadowDesc&         GetShadowDesc() const { return _shadowDesc; }

private:
    void CompileDepthShader();
    void CreateShadowMapResources();
    void CreateStates();
    void CreateInputLayout(ComPtr<ID3DBlob> vsBlob);

    Matrix ComputeLightVP(const std::vector<Entity*>& entities, const Vec3& lightDir);

	ComPtr<ID3D11Texture2D>                                                             _shadowTexture;
    ComPtr<ID3D11DepthStencilView>                                                      _shadowDSV;
    ComPtr<ID3D11ShaderResourceView>                                                    _shadowSRV;

	ComPtr<ID3D11VertexShader>                                                          _depthVS;
    ComPtr<ID3D11Buffer>                                                                _cbuffer;
    ComPtr<ID3D11InputLayout>                                                           _inputLayout;

	ComPtr<ID3D11RasterizerState>                                                       _rasterState;
    ComPtr<ID3D11DepthStencilState>                                                     _depthState;

    std::unordered_map<InstanceID, std::unique_ptr<InstancingBuffer>, InstanceIDHash>   _shadowBuffers;

    ShadowDesc                                                                          _shadowDesc;
    D3D11_VIEWPORT                                                                      _shadowVP = {};
};