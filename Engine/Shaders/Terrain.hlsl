#include "Global.hlsli"
#include "Light.hlsli"

struct VertexMesh
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    
    // INSTANCING
    uint instanceID : SV_INSTANCEID;
    matrix world : INST;
};

MeshOutput VS(VertexMesh input)
{
    MeshOutput output;

    output.position = mul(input.position, input.world);
    output.worldPosition = output.position.xyz;

    output.position = mul(output.position, VP);

    output.uv = input.uv;

    output.normal = mul(input.normal, (float3x3)input.world);
    output.tangent = mul(input.tangent, (float3x3)input.world);
        
    return output;
}

// «»ºø ºŒ¿Ã¥ı (Pixel Shader)
float4 PS(MeshOutput input) : SV_TARGET
{
    float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
    return color;
}

technique11 T0
{
    PASS_VP(P0, VS, PS)
}