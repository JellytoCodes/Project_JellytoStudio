#include "ShaderCommon.hlsli"
#include "Lighting.hlsli"

Texture2D                g_BlockAtlas  : register(t0);
StructuredBuffer<float4> g_AtlasRects  : register(t1);

struct VertexBlockMesh
{
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    
    uint   instanceID    : SV_INSTANCEID;
    matrix world         : INST;
    uint   materialIndex : INST_MATERIAL;
};

struct BlockMeshOutput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : POSITION1;
    float2 uv            : TEXCOORD;
    float3 normal        : NORMAL;
    float3 tangent       : TANGENT;
    
    nointerpolation uint materialIndex : MATERIAL_IDX;
};

BlockMeshOutput VS(VertexBlockMesh input)
{
    BlockMeshOutput output;
    
    output.position      = mul(input.position, input.world);
    output.worldPosition = output.position.xyz;
    output.position      = mul(output.position, VP);

    output.uv            = input.uv;
    output.normal        = normalize(mul(input.normal,  (float3x3)input.world));
    output.tangent       = normalize(mul(input.tangent, (float3x3)input.world));
    
    output.materialIndex = input.materialIndex;

    return output;
}

float4 PS(BlockMeshOutput input) : SV_TARGET
{
    float3 N = normalize(input.normal);

    float4 rect    = g_AtlasRects[input.materialIndex];
    float2 atlasUV = input.uv * rect.zw + rect.xy;

    float4 texColor = g_BlockAtlas.Sample(LinearSampler, atlasUV);
        
    bool   hasTex    = (texColor.a > 0.001f) || any(texColor.rgb > 0.001f);
    float4 baseColor = hasTex ? texColor : Material.diffuse;
    
    float4 ambient = baseColor * GlobalLight.ambient * Material.ambient;

    float  NdotL   = saturate(dot(N, -GlobalLight.direction));
    float4 diffuse = baseColor * NdotL * GlobalLight.diffuse * Material.diffuse;

    float3 camPos  = CameraPosition();
    float3 E       = normalize(camPos - input.worldPosition);
    float3 R       = reflect(GlobalLight.direction, N);
    float  spec    = pow(saturate(dot(R, E)), 16.0f);
    float4 specular = GlobalLight.specular * Material.specular * spec;

    float  rim     = 1.0f - saturate(dot(E, N));
    float4 emissive = GlobalLight.emissive * Material.emissive * pow(rim, 2.0f);
    
    float  shadow     = ComputeShadowFactor(input.worldPosition);
    float4 finalColor = ambient + (diffuse + specular) * shadow + emissive;
    finalColor.a = 1.0f;

    return finalColor;
}

technique11 T0
{
    PASS_VP(P0, VS, PS)
    PASS_RS_VP(P1, FillModeWireFrame, VS, PS)
}
