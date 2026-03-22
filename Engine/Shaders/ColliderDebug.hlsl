#include "Global.hlsli"

cbuffer ColliderColorBuffer
{
    float4 ColliderColor;
};

struct VertexDebug
{
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
};

struct DebugOutput
{
    float4 position : SV_POSITION;
};

DebugOutput VS(VertexDebug input)
{
    DebugOutput output;
    
    float4 worldPos = mul(input.position, W);
    output.position = mul(worldPos, VP);
    return output;
}

float4 PS(DebugOutput input) : SV_TARGET
{
    return ColliderColor;
}

technique11 T0
{
    PASS_RS_VP(P0, FillModeWireFrame, VS, PS)
}
