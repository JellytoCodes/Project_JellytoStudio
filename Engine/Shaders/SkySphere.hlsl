#include "Global.hlsli"
#include "Light.hlsli"

struct VS_IN
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;

    row_major matrix instanceWorld : W; 
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUT VS(VS_IN input)
{
    VS_OUT output;
    
    float4 viewPos = mul(float4(input.position.xyz, 0.0f), V);
    float4 clipSpacePos = mul(viewPos, P);    
    
    output.position = clipSpacePos;
    
    output.position.z = output.position.w * 0.999999f;
    output.uv = input.uv;
    
    return output;
}

float4 PS(VS_OUT input) : SV_TARGET
{
    return DiffuseMap.Sample(LinearSampler, input.uv);
}

DepthStencilState DepthLessEqual
{
    DepthEnable = true;
    DepthFunc = Less_Equal;
    DepthWriteMask = Zero;
};

technique11 T0
{
    pass P0
    {
        SetRasterizerState(FrontCounterClockwiseTrue);
        
        SetDepthStencilState(DepthLessEqual, 0);
        
        SetVertexShader(CompileShader(vs_5_0, VS()));    
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}