
#include "Framework.h"
#include "ShadowPass.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Graphics/Graphics.h"
#include "Types/ShaderDesc.h"

// ── Depth-Only Vertex Shader HLSL ────────────────────────────────────────
// VertexTextureNormalTangentBlendData + InstancingData(matrix world) 레이아웃과 일치
static const char* kDepthVS_HLSL = R"(
cbuffer ShadowCB : register(b0)
{
    matrix LightVP;
    float  ShadowBias;
    float3 Pad;
};
struct VS_IN
{
    float4 position      : POSITION;
    float2 uv            : TEXCOORD;      // 미사용 — 레이아웃 일치용
    float3 normal        : NORMAL;        // 미사용
    float3 tangent       : TANGENT;       // 미사용
    float4 blendIndices  : BLEND_INDICES; // 미사용 (스키닝 없이 world 행렬만 사용)
    float4 blendWeights  : BLEND_WEIGHTS; // 미사용
    // ★ 인스턴스 버퍼 slot 1 — InstancingData::world (matrix = 4×float4)
    float4 world0        : INST0;
    float4 world1        : INST1;
    float4 world2        : INST2;
    float4 world3        : INST3;
};
float4 main(VS_IN input) : SV_POSITION
{
    matrix world = matrix(input.world0, input.world1, input.world2, input.world3);
    float4 worldPos = mul(input.position, world);
    return mul(worldPos, LightVP);
}
)";

// ── 입력 레이아웃 설명 ────────────────────────────────────────────────────
// slot 0: VertexTextureNormalTangentBlendData
// slot 1: InstancingData (4×float4 = matrix)
static const D3D11_INPUT_ELEMENT_DESC kShadowLayout[] =
{
    {"POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0,                           D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"BLEND_INDICES",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"BLEND_WEIGHTS",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    // slot 1 — InstancingData::world (row0~row3)
    {"INST",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"INST",         1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"INST",         2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"INST",         3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
};

void ShadowPass::Init()
{
    CompileDepthShader();
    CreateShadowMapResources();
    CreateStates();

    _shadowVP.TopLeftX = 0.f;
    _shadowVP.TopLeftY = 0.f;
    _shadowVP.Width    = static_cast<float>(kShadowMapSize);
    _shadowVP.Height   = static_cast<float>(kShadowMapSize);
    _shadowVP.MinDepth = 0.f;
    _shadowVP.MaxDepth = 1.f;
}

void ShadowPass::CompileDepthShader()
{
    auto device = GET_SINGLE(Graphics)->GetDevice();

    ComPtr<ID3DBlob> vsBlob, errBlob;
    HRESULT hr = D3DCompile(kDepthVS_HLSL, strlen(kDepthVS_HLSL),
        nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0,
        vsBlob.GetAddressOf(), errBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errBlob) ::OutputDebugStringA((char*)errBlob->GetBufferPointer());
        assert(false && "ShadowPass depth VS 컴파일 실패");
    }

    CHECK(device->CreateVertexShader(vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), nullptr, _depthVS.GetAddressOf()));

    // 입력 레이아웃
    CHECK(device->CreateInputLayout(
        kShadowLayout, ARRAYSIZE(kShadowLayout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        _inputLayout.GetAddressOf()));

    // Shadow constant buffer (LightVP + bias)
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth      = sizeof(ShadowDesc);
    cbd.Usage          = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    CHECK(device->CreateBuffer(&cbd, nullptr, _cbuffer.GetAddressOf()));
}

void ShadowPass::CreateShadowMapResources()
{
    auto device = GET_SINGLE(Graphics)->GetDevice();

    // R32_TYPELESS: DSV는 D32_FLOAT, SRV는 R32_FLOAT 로 각각 뷰 생성
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
    CHECK(device->CreateDepthStencilView(
        _shadowTexture.Get(), &dsvd, _shadowDSV.GetAddressOf()));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format                    = DXGI_FORMAT_R32_FLOAT;
    srvd.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels       = 1;
    CHECK(device->CreateShaderResourceView(
        _shadowTexture.Get(), &srvd, _shadowSRV.GetAddressOf()));
}

void ShadowPass::CreateStates()
{
    auto device = GET_SINGLE(Graphics)->GetDevice();

    // ★ Depth Bias: 셀프-섀도우 아티팩트 (shadow acne) 방지
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode              = D3D11_FILL_SOLID;
    rd.CullMode              = D3D11_CULL_BACK;
    rd.DepthClipEnable       = TRUE;
    rd.DepthBias             = 1000;       // 정수 bias — 하드웨어 depth unit
    rd.DepthBiasClamp        = 0.f;
    rd.SlopeScaledDepthBias  = 1.5f;       // 경사면 편향
    CHECK(device->CreateRasterizerState(&rd, _rasterState.GetAddressOf()));

    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable    = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc      = D3D11_COMPARISON_LESS;
    CHECK(device->CreateDepthStencilState(&dsd, _depthState.GetAddressOf()));
}

Matrix ShadowPass::ComputeLightVP(const std::vector<Entity*>& entities,
                                    const Vec3& lightDir)
{
    // ── 가시 엔티티 전체를 감싸는 AABB 계산 ──────────────────────────
    BoundingBox sceneBounds;
    bool first = true;

    for (Entity* e : entities)
    {
        if (auto* aabb = e->GetComponent<AABBCollider>())
        {
            if (first) { sceneBounds = aabb->GetBoundingBox(); first = false; }
            else BoundingBox::CreateMerged(sceneBounds, sceneBounds, aabb->GetBoundingBox());
        }
    }

    // AABBCollider 없는 씬 → 기본 범위
    if (first)
    {
        sceneBounds.Center  = { 0, 0, 0 };
        sceneBounds.Extents = { 20, 20, 20 };
    }

    const Vec3 center (sceneBounds.Center.x,  sceneBounds.Center.y,  sceneBounds.Center.z);
    const Vec3 extents(sceneBounds.Extents.x, sceneBounds.Extents.y, sceneBounds.Extents.z);
    const float radius = extents.Length() * 1.2f;  // 여유 20%

    // ── 광원 뷰 행렬 ─────────────────────────────────────────────────
    const Vec3 lightPos = center - lightDir * radius;

    // Up 벡터가 lightDir과 평행하면 gimbal lock → 대안 사용
    Vec3 up = Vec3(0.f, 1.f, 0.f);
    if (fabsf(lightDir.Dot(up)) > 0.99f)
        up = Vec3(0.f, 0.f, 1.f);

    const Matrix lightView = XMMatrixLookAtLH(lightPos, center, up);

    // ── 광원 투영 행렬 (직교) ─────────────────────────────────────────
    // tight-fit: 씬 반경에 맞춘 직교 투영 → 최대 정밀도
    const Matrix lightProj = XMMatrixOrthographicLH(
        radius * 2.f, radius * 2.f,
        0.1f, radius * 4.f);

    return lightView * lightProj;
}

void ShadowPass::Render(const std::vector<Entity*>& entities, const Vec3& lightDir)
{
    if (entities.empty()) return;

    auto  dc     = GET_SINGLE(Graphics)->GetDeviceContext();
    auto  device = GET_SINGLE(Graphics)->GetDevice();

    // ── 1. LightVP 계산 및 cbuffer 업로드 ────────────────────────────
    const Matrix lightVP = ComputeLightVP(entities, lightDir);
    _shadowDesc.lightVP  = lightVP;

    {
        D3D11_MAPPED_SUBRESOURCE ms = {};
        dc->Map(_cbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        memcpy(ms.pData, &_shadowDesc, sizeof(ShadowDesc));
        dc->Unmap(_cbuffer.Get(), 0);
    }

    // ── 2. Shadow Map 렌더 타겟 설정 (RTV=null, DSV=shadow) ──────────
    // 먼저 SRV를 언바인드 — 같은 텍스처를 DSV와 SRV로 동시에 쓸 수 없음
    ID3D11ShaderResourceView* nullSRV = nullptr;
    dc->PSSetShaderResources(3, 1, &nullSRV);  // slot 3 = ShadowMap 위치

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

    // ── 3. 파이프라인 상태 설정 ───────────────────────────────────────
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dc->IASetInputLayout(_inputLayout.Get());
    dc->VSSetShader(_depthVS.Get(), nullptr, 0);
    dc->VSSetConstantBuffers(0, 1, _cbuffer.GetAddressOf());
    dc->PSSetShader(nullptr, nullptr, 0);  // 깊이만 기록 — PS 불필요
    dc->RSSetState(_rasterState.Get());
    dc->OMSetDepthStencilState(_depthState.Get(), 0);

    // ── 4. 엔티티 그룹화 및 인스턴싱 렌더 ───────────────────────────
    // 그룹: InstanceID → (ModelRenderer*, 월드 행렬 목록)
    std::unordered_map<InstanceID, std::pair<ModelRenderer*, std::vector<Matrix>>,
                       InstanceIDHash> groups;

    for (Entity* e : entities)
    {
        auto* modelR = e->GetComponent<ModelRenderer>();
        if (!modelR) continue;

        auto* tr = e->GetComponent<Transform>();
        if (!tr)    continue;

        const InstanceID id = modelR->GetInstanceID();
        auto& [pModelR, matrices] = groups[id];
        pModelR = modelR;
        matrices.push_back(modelR->GetModelScaleMatrix() * tr->GetWorldMatrix());
    }

    for (auto& [id, pair] : groups)
    {
        auto& [pModelR, matrices] = pair;
        if (matrices.empty()) continue;

        // 인스턴스 버퍼 가져오거나 신규 생성
        auto& buf = _shadowBuffers[id];
        if (!buf) buf = std::make_unique<InstancingBuffer>();

        buf->ClearData();
        for (const Matrix& m : matrices)
        {
            InstancingData data;
            data.world = m;
            buf->AddData(data);
        }
        buf->UploadData();

        // ModelRenderer::RenderRawInstanced — 셰이더 설정 없이 순수 지오메트리만 드로우
        pModelR->RenderRawInstanced(dc, buf.get());
    }

    // ── 5. 렌더 상태 복원 ────────────────────────────────────────────
    dc->OMSetRenderTargets(1, savedRTV.GetAddressOf(), savedDSV.Get());
    dc->RSSetViewports(1, &savedVP);
    dc->RSSetState(nullptr);
    dc->OMSetDepthStencilState(nullptr, 0);
}