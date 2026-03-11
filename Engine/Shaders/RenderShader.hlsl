#include "Global.hlsli"
#include "Light.hlsli"

#define MAX_MODEL_TRANSFORM 250

cbuffer BoneBuffer : register(b2)
{
    matrix BoneTransforms[MAX_MODEL_TRANSFORM];
}

struct VS_INST_INPUT
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    
    matrix instWorld : INSTWORLD;
};

MeshOutput VS(VS_INST_INPUT input)
{
    MeshOutput output;
    float4 pos = input.position;

    output.position = mul(pos, input.instWorld);
    output.worldPosition = output.position.xyz;

    output.position = mul(output.position, VP);

    output.uv = input.uv;
    output.normal = mul(input.normal, (float3x3) input.instWorld);
    output.tangent = mul(input.tangent, (float3x3) input.instWorld);

    return output;
}

float4 PS(MeshOutput input) : SV_TARGET
{
    float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
    return color;
}

technique11 T0
{
    PASS_VP(P0, VS, PS)
}