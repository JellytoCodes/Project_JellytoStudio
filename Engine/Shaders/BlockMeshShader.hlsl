#include "Global.hlsli"
#include "Light.hlsli"

Texture2DArray g_BlockTextures : register(t0);

struct VertexBlockMesh
{
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    
    uint   instanceID    : SV_INSTANCEID;
    matrix world         : INST;         
    uint   materialIndex : INST_MATERIAL;
    // _pad[3] (12B at offset 68) → Input Layout 에 선언 없음, 자동 스킵
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
    
    float4 texColor = g_BlockTextures.Sample(LinearSampler, float3(input.uv, (float)input.materialIndex));
        
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
    // Pass 0: 불투명 블록 (기본)
    PASS_VP(P0, VS, PS)

    // Pass 1: 와이어프레임 디버그
    PASS_RS_VP(P1, FillModeWireFrame, VS, PS)

    // Pass 2: 반투명 블록 (향후 유리 블록 등 확장용)
    // PASS_BS_VP(P2, AlphaBlend, VS, PS)
}
