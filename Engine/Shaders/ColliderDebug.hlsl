#include "Global.hlsli"

// 콜라이더 디버그 시각화 셰이더
// WireFrame으로 외곽선만 그려서 언리얼 엔진의 콜리전 뷰어처럼 동작

cbuffer ColliderColorBuffer
{
    float4 ColliderColor; // 기본: 녹색(AABB), 노랑(Sphere)
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
    // P0: WireFrame 외곽선 Pass
    PASS_RS_VP(P0, FillModeWireFrame, VS, PS)
}
