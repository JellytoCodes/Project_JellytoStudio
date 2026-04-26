#pragma once
#include "Pipeline/InstancingBuffer.h"
#include "Graphics/RenderPacket.h"
#include "Managers/InstancingManager.h"

class Entity;
class ModelRenderer;
class ModelAnimator;
struct ShadowDesc;

class ShadowPass
{
public:
    static constexpr uint32 kShadowMapSize = 1024;

    void Init();
    void Render(const std::vector<Entity*>& entities, const Vec3& lightDir);

    ID3D11ShaderResourceView* GetShadowSRV()   const { return _shadowSRV.Get(); }
    const ShadowDesc&         GetShadowDesc()  const { return _shadowDesc; }

private:
    void CompileDepthShaders();
    void CreateShadowMapResources();
    void CreateStates();

    Matrix ComputeLightVP(const std::vector<Entity*>& entities, const Vec3& lightDir);

    void RenderStaticGroups (const ComPtr<ID3D11DeviceContext>& dc);
    void RenderSkinnedGroups(const ComPtr<ID3D11DeviceContext>& dc);

    ComPtr<ID3D11Texture2D>            _shadowTexture;
    ComPtr<ID3D11DepthStencilView>     _shadowDSV;
    ComPtr<ID3D11ShaderResourceView>   _shadowSRV;

    ComPtr<ID3D11VertexShader>         _depthVS;
    ComPtr<ID3D11InputLayout>          _inputLayout;

    ComPtr<ID3D11VertexShader>         _skinnedDepthVS;
    ComPtr<ID3D11InputLayout>          _skinnedInputLayout;

    ComPtr<ID3D11Buffer>               _shadowCB;
    ComPtr<ID3D11Buffer>               _tweenCB;

    ComPtr<ID3D11RasterizerState>      _rasterState;
    ComPtr<ID3D11DepthStencilState>    _depthState;

    struct StaticGroup
    {
        ModelRenderer*      pModelR = nullptr;
        std::vector<Matrix> matrices;
    };

    struct SkinnedGroup
    {
        ModelAnimator*      pAnimator       = nullptr;
        std::vector<Matrix> matrices;
        InstancedTweenDesc  tweenDesc       = {};
        uint32              instanceCount   = 0;
    };

    std::unordered_map<InstanceID, std::unique_ptr<InstancingBuffer>, InstanceIDHash> _shadowBuffers;
    std::unordered_map<InstanceID, StaticGroup,  InstanceIDHash>  _staticGroups;
    std::unordered_map<InstanceID, SkinnedGroup, InstanceIDHash>  _skinnedGroups;

    ShadowDesc         _shadowDesc;
    D3D11_VIEWPORT     _shadowVP = {};
};