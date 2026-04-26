#include "Framework.h"
#include "ShadowPass.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Graphics.h"
#include "Types/ShaderDesc.h"

#pragma region kDepthVS_HLSL
static const char* kDepthVS_HLSL = R"(
cbuffer ShadowCB : register(b0)
{
    matrix LightVP;
    float  ShadowBias;
    float3 Pad;
}
struct VS_IN
{
    float4 position     : POSITION;
    float2 uv           : TEXCOORD;
    float3 normal       : NORMAL;
    float3 tangent      : TANGENT;
    float4 blendIndices : BLEND_INDICES;
    float4 blendWeights : BLEND_WEIGHTS;
    float4 world0       : INST0;
    float4 world1       : INST1;
    float4 world2       : INST2;
    float4 world3       : INST3;
};
float4 main(VS_IN input) : SV_POSITION
{
    matrix world    = matrix(input.world0, input.world1, input.world2, input.world3);
    float4 worldPos = mul(input.position, world);
    return mul(worldPos, LightVP);
}
)";
#pragma endregion

#pragma region kSkinnedDepthVS_HLSL
static const char* kSkinnedDepthVS_HLSL = R"(
#define MAX_MODEL_TRANSFORMS 250
#define MAX_MODEL_INSTANCE   250

cbuffer ShadowCB : register(b0)
{
    matrix LightVP;
    float  ShadowBias;
    float3 Pad;
}

struct KeyframeDesc
{
    int   animIndex;
    uint  currFrame;
    uint  nextFrame;
    float ratio;
    float sumTime;
    float speed;
    float2 padding;
};

struct TweenFrameDesc
{
    float tweenDuration;
    float tweenRatio;
    float tweenSumTime;
    float tweenPad;
    KeyframeDesc curr;
    KeyframeDesc next;
};

cbuffer TweenCB : register(b1)
{
    TweenFrameDesc TweenFrames[MAX_MODEL_INSTANCE];
}

Texture2DArray TransformMap : register(t0);

struct VS_IN
{
    float4 position     : POSITION;
    float2 uv           : TEXCOORD;
    float3 normal       : NORMAL;
    float3 tangent      : TANGENT;
    float4 blendIndices : BLEND_INDICES;
    float4 blendWeights : BLEND_WEIGHTS;
    uint   instanceID   : SV_InstanceID;
    float4 world0       : INST0;
    float4 world1       : INST1;
    float4 world2       : INST2;
    float4 world3       : INST3;
};

float4 main(VS_IN input) : SV_POSITION
{
    float indices[4] = { input.blendIndices.x, input.blendIndices.y,
                         input.blendIndices.z, input.blendIndices.w };
    float weights[4] = { input.blendWeights.x, input.blendWeights.y,
                         input.blendWeights.z, input.blendWeights.w };

    int   animIdx0  = TweenFrames[input.instanceID].curr.animIndex;
    uint  currF0    = TweenFrames[input.instanceID].curr.currFrame;
    uint  nextF0    = TweenFrames[input.instanceID].curr.nextFrame;
    float ratio0    = TweenFrames[input.instanceID].curr.ratio;

    int   animIdx1  = TweenFrames[input.instanceID].next.animIndex;
    uint  currF1    = TweenFrames[input.instanceID].next.currFrame;
    uint  nextF1    = TweenFrames[input.instanceID].next.nextFrame;
    float ratio1    = TweenFrames[input.instanceID].next.ratio;
    float tweenR    = TweenFrames[input.instanceID].tweenRatio;

    matrix transform = 0;
    float4 c0, c1, c2, c3, n0, n1, n2, n3;

    for (int i = 0; i < 4; i++)
    {
        c0 = TransformMap.Load(int4(indices[i] * 4 + 0, currF0, animIdx0, 0));
        c1 = TransformMap.Load(int4(indices[i] * 4 + 1, currF0, animIdx0, 0));
        c2 = TransformMap.Load(int4(indices[i] * 4 + 2, currF0, animIdx0, 0));
        c3 = TransformMap.Load(int4(indices[i] * 4 + 3, currF0, animIdx0, 0));
        n0 = TransformMap.Load(int4(indices[i] * 4 + 0, nextF0, animIdx0, 0));
        n1 = TransformMap.Load(int4(indices[i] * 4 + 1, nextF0, animIdx0, 0));
        n2 = TransformMap.Load(int4(indices[i] * 4 + 2, nextF0, animIdx0, 0));
        n3 = TransformMap.Load(int4(indices[i] * 4 + 3, nextF0, animIdx0, 0));

        matrix result = lerp(matrix(c0,c1,c2,c3), matrix(n0,n1,n2,n3), ratio0);

        if (animIdx1 >= 0)
        {
            c0 = TransformMap.Load(int4(indices[i] * 4 + 0, currF1, animIdx1, 0));
            c1 = TransformMap.Load(int4(indices[i] * 4 + 1, currF1, animIdx1, 0));
            c2 = TransformMap.Load(int4(indices[i] * 4 + 2, currF1, animIdx1, 0));
            c3 = TransformMap.Load(int4(indices[i] * 4 + 3, currF1, animIdx1, 0));
            n0 = TransformMap.Load(int4(indices[i] * 4 + 0, nextF1, animIdx1, 0));
            n1 = TransformMap.Load(int4(indices[i] * 4 + 1, nextF1, animIdx1, 0));
            n2 = TransformMap.Load(int4(indices[i] * 4 + 2, nextF1, animIdx1, 0));
            n3 = TransformMap.Load(int4(indices[i] * 4 + 3, nextF1, animIdx1, 0));

            matrix nextResult = lerp(matrix(c0,c1,c2,c3), matrix(n0,n1,n2,n3), ratio1);
            result = lerp(result, nextResult, tweenR);
        }

        transform += mul(weights[i], result);
    }

    float4 worldPos = mul(input.position, transform);
    matrix world    = matrix(input.world0, input.world1, input.world2, input.world3);
    worldPos        = mul(worldPos, world);
    return mul(worldPos, LightVP);
}
)";
#pragma endregion

static const D3D11_INPUT_ELEMENT_DESC kStaticShadowLayout[] =
{
    {"POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0,                            D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"BLEND_INDICES",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"BLEND_WEIGHTS",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"INST",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"INST",         1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"INST",         2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"INST",         3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
};

static const D3D11_INPUT_ELEMENT_DESC kSkinnedShadowLayout[] =
{
    {"POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0,                            D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"BLEND_INDICES",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"BLEND_WEIGHTS",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"INST",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"INST",         1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"INST",         2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"INST",         3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
};

void ShadowPass::Init()
{
    CompileDepthShaders();
    CreateShadowMapResources();
    CreateStates();

    _shadowVP.TopLeftX = 0.f;
    _shadowVP.TopLeftY = 0.f;
    _shadowVP.Width    = static_cast<float>(kShadowMapSize);
    _shadowVP.Height   = static_cast<float>(kShadowMapSize);
    _shadowVP.MinDepth = 0.f;
    _shadowVP.MaxDepth = 1.f;

    _shadowDesc.texelSize = 1.0f / static_cast<float>(kShadowMapSize);
}

void ShadowPass::CompileDepthShaders()
{
    auto  device       = GET_SINGLE(Graphics)->GetDevice();
    const UINT flags   = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

    auto compile = [&](const char* src, const char* entry,
                       ComPtr<ID3D11VertexShader>& outVS,
                       ComPtr<ID3D11InputLayout>&  outIL,
                       const D3D11_INPUT_ELEMENT_DESC* layout,
                       UINT layoutCount)
    {
        ComPtr<ID3DBlob> vsBlob, errBlob;
        HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
                                entry, "vs_5_0", flags, 0,
                                vsBlob.GetAddressOf(), errBlob.GetAddressOf());
        if (FAILED(hr))
        {
            if (errBlob) ::OutputDebugStringA((char*)errBlob->GetBufferPointer());
            assert(false);
        }

        CHECK(device->CreateVertexShader(vsBlob->GetBufferPointer(),
            vsBlob->GetBufferSize(), nullptr, outVS.GetAddressOf()));

        CHECK(device->CreateInputLayout(layout, layoutCount,
            vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
            outIL.GetAddressOf()));
    };

    compile(kDepthVS_HLSL,        "main", _depthVS,        _inputLayout,
            kStaticShadowLayout,  ARRAYSIZE(kStaticShadowLayout));

    compile(kSkinnedDepthVS_HLSL, "main", _skinnedDepthVS, _skinnedInputLayout,
            kSkinnedShadowLayout, ARRAYSIZE(kSkinnedShadowLayout));

    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage          = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    cbd.ByteWidth = sizeof(ShadowDesc);
    CHECK(device->CreateBuffer(&cbd, nullptr, _shadowCB.GetAddressOf()));

    cbd.ByteWidth = sizeof(InstancedTweenDesc);
    CHECK(device->CreateBuffer(&cbd, nullptr, _tweenCB.GetAddressOf()));
}

void ShadowPass::CreateShadowMapResources()
{
    auto device = GET_SINGLE(Graphics)->GetDevice();

    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = kShadowMapSize;
    td.Height           = kShadowMapSize;
    td.MipLevels        = 1;
    td.ArraySize        = 1;
    td.Format           = DXGI_FORMAT_R32_TYPELESS;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    CHECK(device->CreateTexture2D(&td, nullptr, _shadowTexture.GetAddressOf()));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};
    dsvd.Format        = DXGI_FORMAT_D32_FLOAT;
    dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    CHECK(device->CreateDepthStencilView(_shadowTexture.Get(), &dsvd, _shadowDSV.GetAddressOf()));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format                    = DXGI_FORMAT_R32_FLOAT;
    srvd.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels       = 1;
    CHECK(device->CreateShaderResourceView(_shadowTexture.Get(), &srvd, _shadowSRV.GetAddressOf()));
}

void ShadowPass::CreateStates()
{
    auto device = GET_SINGLE(Graphics)->GetDevice();

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode             = D3D11_FILL_SOLID;
    rd.CullMode             = D3D11_CULL_BACK;
    rd.DepthClipEnable      = TRUE;
    rd.DepthBias            = 1000;
    rd.DepthBiasClamp       = 0.f;
    rd.SlopeScaledDepthBias = 1.5f;
    CHECK(device->CreateRasterizerState(&rd, _rasterState.GetAddressOf()));

    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable    = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc      = D3D11_COMPARISON_LESS;
    CHECK(device->CreateDepthStencilState(&dsd, _depthState.GetAddressOf()));
}

Matrix ShadowPass::ComputeLightVP(const std::vector<Entity*>& entities, const Vec3& lightDir)
{
    BoundingBox sceneBounds;
    bool first = true;
    for (Entity* e : entities)
    {
        auto* aabb = e->GetComponent<AABBCollider>();
        if (!aabb || !aabb->IsStatic()) continue;
        if (first) { sceneBounds = aabb->GetBoundingBox(); first = false; }
        else BoundingBox::CreateMerged(sceneBounds, sceneBounds, aabb->GetBoundingBox());
    }
    if (first)
    {
        sceneBounds.Center  = { 0, 0, 0 };
        sceneBounds.Extents = { 20, 20, 20 };
    }

    const Vec3  center  = { sceneBounds.Center.x,  sceneBounds.Center.y,  sceneBounds.Center.z  };
    const Vec3  extents = { sceneBounds.Extents.x, sceneBounds.Extents.y, sceneBounds.Extents.z };
    const float radius  = extents.Length() * 1.2f;
    const Vec3  lightPos = center - lightDir * radius;

    Vec3 up = Vec3(0.f, 1.f, 0.f);
    if (fabsf(lightDir.Dot(up)) > 0.99f)
        up = Vec3(0.f, 0.f, 1.f);

    const Matrix lightView = XMMatrixLookAtLH(lightPos, center, up);
    const Matrix lightProj = XMMatrixOrthographicLH(
        radius * 2.f, radius * 2.f, 0.1f, radius * 4.f);

    return lightView * lightProj;
}

void ShadowPass::Render(const std::vector<Entity*>& entities, const Vec3& lightDir)
{
    if (entities.empty()) return;

    auto dc     = GET_SINGLE(Graphics)->GetDeviceContext();
    auto device = GET_SINGLE(Graphics)->GetDevice();

    const Matrix lightVP = ComputeLightVP(entities, lightDir);
    _shadowDesc.lightVP  = lightVP;

    {
        D3D11_MAPPED_SUBRESOURCE ms = {};
        dc->Map(_shadowCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        ::memcpy(ms.pData, &_shadowDesc, sizeof(ShadowDesc));
        dc->Unmap(_shadowCB.Get(), 0);
    }

    ID3D11ShaderResourceView* nullSRV = nullptr;
    dc->PSSetShaderResources(3, 1, &nullSRV);

    ComPtr<ID3D11RenderTargetView> savedRTV;
    ComPtr<ID3D11DepthStencilView> savedDSV;
    D3D11_VIEWPORT savedVP;
    uint32 vpCount = 1;
    dc->OMGetRenderTargets(1, savedRTV.GetAddressOf(), savedDSV.GetAddressOf());
    dc->RSGetViewports(&vpCount, &savedVP);

    ID3D11RenderTargetView* nullRTV = nullptr;
    dc->OMSetRenderTargets(1, &nullRTV, _shadowDSV.Get());
    dc->ClearDepthStencilView(_shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    dc->RSSetViewports(1, &_shadowVP);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dc->VSSetConstantBuffers(0, 1, _shadowCB.GetAddressOf());
    dc->PSSetShader(nullptr, nullptr, 0);
    dc->RSSetState(_rasterState.Get());
    dc->OMSetDepthStencilState(_depthState.Get(), 0);

    _staticGroups.clear();
    _skinnedGroups.clear();

    for (Entity* e : entities)
    {
        auto* tr = e->GetComponent<Transform>();
        if (!tr) continue;

        if (auto* modelR = e->GetComponent<ModelRenderer>())
        {
            const InstanceID id = modelR->GetInstanceID();
            auto& g = _staticGroups[id];
            g.pModelR = modelR;
            g.matrices.push_back(modelR->GetModelScaleMatrix() * tr->GetWorldMatrix());
        }
        else if (auto* anim = e->GetComponent<ModelAnimator>())
        {
            const InstanceID id = anim->GetInstanceID();
            auto& g = _skinnedGroups[id];
            if (g.pAnimator == nullptr) g.pAnimator = anim;
            g.matrices.push_back(tr->GetWorldMatrix());

            const uint32 idx = g.instanceCount;
            if (idx < MAX_MODEL_INSTANCE)
            {
                g.tweenDesc.tweens[idx] = anim->GetTweenDesc();
                g.instanceCount++;
            }
        }
    }

    RenderStaticGroups(dc);
    RenderSkinnedGroups(dc);

    dc->OMSetRenderTargets(1, savedRTV.GetAddressOf(), savedDSV.Get());
    dc->RSSetViewports(1, &savedVP);
    dc->RSSetState(nullptr);
    dc->OMSetDepthStencilState(nullptr, 0);
    dc->VSSetShaderResources(0, 1, &nullSRV);
}

void ShadowPass::RenderStaticGroups(const ComPtr<ID3D11DeviceContext>& dc)
{
    dc->IASetInputLayout(_inputLayout.Get());
    dc->VSSetShader(_depthVS.Get(), nullptr, 0);

    for (auto& [id, g] : _staticGroups)
    {
        if (g.matrices.empty()) continue;

        auto& buf = _shadowBuffers[id];
        if (!buf) buf = std::make_unique<InstancingBuffer>();
        buf->ClearData();
        for (const Matrix& m : g.matrices)
        {
            InstancingData data;
            data.world = m;
            buf->AddData(data);
        }
        buf->UploadData();
        g.pModelR->RenderRawInstanced(dc, buf.get());
    }
}

void ShadowPass::RenderSkinnedGroups(const ComPtr<ID3D11DeviceContext>& dc)
{
    dc->IASetInputLayout(_skinnedInputLayout.Get());
    dc->VSSetShader(_skinnedDepthVS.Get(), nullptr, 0);
    dc->VSSetConstantBuffers(0, 1, _shadowCB.GetAddressOf());

    for (auto& [id, g] : _skinnedGroups)
    {
        if (g.matrices.empty() || g.pAnimator == nullptr) continue;

        {
            D3D11_MAPPED_SUBRESOURCE ms = {};
            dc->Map(_tweenCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
            ::memcpy(ms.pData, &g.tweenDesc, sizeof(InstancedTweenDesc));
            dc->Unmap(_tweenCB.Get(), 0);
        }
        dc->VSSetConstantBuffers(1, 1, _tweenCB.GetAddressOf());

        ID3D11ShaderResourceView* transformSRV = g.pAnimator->GetTransformSRV();
        dc->VSSetShaderResources(0, 1, &transformSRV);

        auto& buf = _shadowBuffers[id];
        if (!buf) buf = std::make_unique<InstancingBuffer>();
        buf->ClearData();
        for (const Matrix& m : g.matrices)
        {
            InstancingData data;
            data.world = m;
            buf->AddData(data);
        }
        buf->UploadData();

        g.pAnimator->RenderRawSkinnedInstanced(dc, buf.get());
    }
}