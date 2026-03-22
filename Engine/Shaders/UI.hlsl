// UI.hlsl
// IMGUI 방식과 동일: 동일 DeviceContext, DepthWrite OFF, AlphaBlend ON
// 픽셀 좌표 → NDC 변환은 VS에서 처리 (별도 카메라 불필요)

cbuffer UIBuffer
{
    float2 ScreenSize; // (width, height)
    float2 _pad;
};

Texture2D UITexture;

SamplerState LinearSampler
{
    Filter   = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

BlendState AlphaBlend
{
    BlendEnable[0]           = true;
    SrcBlend                 = SRC_ALPHA;
    DestBlend                = INV_SRC_ALPHA;
    BlendOp                  = ADD;
    SrcBlendAlpha            = ONE;
    DestBlendAlpha           = ZERO;
    BlendOpAlpha             = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

DepthStencilState NoDepth
{
    DepthEnable    = false;
    DepthWriteMask = Zero;
};

RasterizerState NoCull
{
    CullMode = None;
};

struct VS_IN
{
    float2 pos   : POSITION;
    float2 uv    : TEXCOORD;
    float4 color : COLOR;
};

struct VS_OUT
{
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD;
    float4 color : COLOR;
};

VS_OUT VS(VS_IN input)
{
    VS_OUT output;
    // 픽셀 좌표 → NDC: x=px/w*2-1, y=1-py/h*2
    output.pos   = float4(
        input.pos.x / ScreenSize.x * 2.0f - 1.0f,
        1.0f - input.pos.y / ScreenSize.y * 2.0f,
        0.0f, 1.0f);
    output.uv    = input.uv;
    output.color = input.color;
    return output;
}

// Pass 0: 단색 (버튼 배경, 패널)
float4 PS_Color(VS_OUT input) : SV_TARGET
{
    return input.color;
}

// Pass 1: 텍스처 (텍스트 비트맵)
float4 PS_Tex(VS_OUT input) : SV_TARGET
{
    return UITexture.Sample(LinearSampler, input.uv) * input.color;
}

technique11 T0
{
    pass P0   // 단색
    {
        SetRasterizerState(NoCull);
        SetDepthStencilState(NoDepth, 0);
        SetBlendState(AlphaBlend, float4(0,0,0,0), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader (CompileShader(ps_5_0, PS_Color()));
    }
    pass P1   // 텍스처
    {
        SetRasterizerState(NoCull);
        SetDepthStencilState(NoDepth, 0);
        SetBlendState(AlphaBlend, float4(0,0,0,0), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader (CompileShader(ps_5_0, PS_Tex()));
    }
}
