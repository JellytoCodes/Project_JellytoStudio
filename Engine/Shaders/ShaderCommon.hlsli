#ifndef _GLOBAL_FX_
#define _GLOBAL_FX_

cbuffer ShadowBuffer : register(b0)
{
    matrix LightVP;
    float  ShadowBias;
    float  ShadowTexelSize;
    float2 ShadowPad;
}

cbuffer GlobalBuffer : register(b1)
{
    matrix V;
    matrix P;
    matrix VP;
    matrix VInv;
}

cbuffer TransformBuffer : register(b2)
{
    matrix W;
}

Texture2D ShadowMap;

SamplerComparisonState ShadowSampler
{
    Filter         = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    AddressU       = BORDER;
    AddressV       = BORDER;
    BorderColor    = float4(1, 1, 1, 1);
    ComparisonFunc = LESS_EQUAL;
};

float ComputeShadowFactor(float3 worldPos)
{
    float4 lsPos = mul(float4(worldPos, 1.0f), LightVP);
    float3 proj  = lsPos.xyz / lsPos.w;

    float2 uv = proj.xy * float2(0.5f, -0.5f) + 0.5f;

    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
        return 1.0f;

    float depth  = proj.z - ShadowBias;
    float shadow = 0.0f;

    [unroll]
    for (int x = -1; x <= 1; x++)
    {
        [unroll]
        for (int y = -1; y <= 1; y++)
        {
            shadow += ShadowMap.SampleCmpLevelZero(
                ShadowSampler,
                uv + float2(x, y) * ShadowTexelSize,
                depth);
        }
    }
    return shadow / 9.0f;
}

struct Vertex
{
    float4 position : POSITION;
};

struct VertexTexture
{
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct VertexColor
{
    float4 Position : POSITION;
    float4 Color    : COLOR;
};

struct VertexTextureNormal
{
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
};

struct VertexTextureNormalTangent
{
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
};

struct VertexTextureNormalTangentBlend
{
    float4 position     : POSITION;
    float2 uv           : TEXCOORD;
    float3 normal       : NORMAL;
    float3 tangent      : TANGENT;
    float4 blendIndices : BLEND_INDICES;
    float4 blendWeights : BLEND_WEIGHTS;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
};

struct MeshOutput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : POSITION1;
    float2 uv            : TEXCOORD;
    float3 normal        : NORMAL;
    float3 tangent       : TANGENT;
};

SamplerState LinearSampler
{
    Filter   = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState PointSampler
{
    Filter   = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
};

RasterizerState FillModeWireFrame
{
    FillMode = WireFrame;
};

RasterizerState FrontCounterClockwiseTrue
{
    FrontCounterClockwise = true;
};

BlendState AlphaBlend
{
    AlphaToCoverageEnable    = false;
    BlendEnable[0]           = true;
    SrcBlend[0]              = SRC_ALPHA;
    DestBlend[0]             = INV_SRC_ALPHA;
    BlendOp[0]               = ADD;
    SrcBlendAlpha[0]         = One;
    DestBlendAlpha[0]        = Zero;
    BlendOpAlpha[0]          = Add;
    RenderTargetWriteMask[0] = 15;
};

BlendState AlphaBlendAlphaToCoverageEnable
{
    AlphaToCoverageEnable    = true;
    BlendEnable[0]           = true;
    SrcBlend[0]              = SRC_ALPHA;
    DestBlend[0]             = INV_SRC_ALPHA;
    BlendOp[0]               = ADD;
    SrcBlendAlpha[0]         = One;
    DestBlendAlpha[0]        = Zero;
    BlendOpAlpha[0]          = Add;
    RenderTargetWriteMask[0] = 15;
};

BlendState AdditiveBlend
{
    AlphaToCoverageEnable    = true;
    BlendEnable[0]           = true;
    SrcBlend[0]              = One;
    DestBlend[0]             = One;
    BlendOp[0]               = ADD;
    SrcBlendAlpha[0]         = One;
    DestBlendAlpha[0]        = Zero;
    BlendOpAlpha[0]          = Add;
    RenderTargetWriteMask[0] = 15;
};

BlendState AdditiveBlendAlphaToCoverageEnable
{
    AlphaToCoverageEnable    = true;
    BlendEnable[0]           = true;
    SrcBlend[0]              = One;
    DestBlend[0]             = One;
    BlendOp[0]               = ADD;
    SrcBlendAlpha[0]         = One;
    DestBlendAlpha[0]        = Zero;
    BlendOpAlpha[0]          = Add;
    RenderTargetWriteMask[0] = 15;
};

#define PASS_VP(name, vs, ps)                           \
pass name                                               \
{                                                       \
    SetVertexShader(CompileShader(vs_5_0, vs()));       \
    SetPixelShader(CompileShader(ps_5_0, ps()));        \
}

#define PASS_RS_VP(name, rs, vs, ps)                    \
pass name                                               \
{                                                       \
    SetRasterizerState(rs);                             \
    SetVertexShader(CompileShader(vs_5_0, vs()));       \
    SetPixelShader(CompileShader(ps_5_0, ps()));        \
}

#define PASS_BS_VP(name, bs, vs, ps)                    \
pass name                                               \
{                                                       \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF);       \
    SetVertexShader(CompileShader(vs_5_0, vs()));       \
    SetPixelShader(CompileShader(ps_5_0, ps()));        \
}

float3 CameraPosition()
{
    return VInv._41_42_43;
}

#endif
