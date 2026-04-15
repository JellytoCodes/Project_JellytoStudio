#include "Global.hlsli"
#include "Light.hlsli"

struct VertexMesh
{
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;

    // INSTANCING
    uint   instanceID : SV_INSTANCEID;
    matrix world      : INST;
};

MeshOutput VS(VertexMesh input)
{
    MeshOutput output;

    output.position      = mul(input.position, input.world);
    output.worldPosition = output.position.xyz;
    output.position      = mul(output.position, VP);
    output.uv            = input.uv;
    output.normal        = mul(input.normal,  (float3x3)input.world);
    output.tangent       = mul(input.tangent, (float3x3)input.world);

    return output;
}

float4 PS(MeshOutput input) : SV_TARGET
{
	float3 N = normalize(input.normal);

    float4 texColor = DiffuseMap.Sample(LinearSampler, input.uv);
    bool   hasTex   = (texColor.a > 0.001f) || any(texColor.rgb > 0.001f);
    float4 baseColor = hasTex ? texColor : Material.diffuse;

    float4 ambient  = baseColor * GlobalLight.ambient  * Material.ambient;

    float  NdotL    = saturate(dot(N, -GlobalLight.direction));
    float4 diffuse  = baseColor * NdotL * GlobalLight.diffuse * Material.diffuse;

    float3 camPos   = CameraPosition();
    float3 E        = normalize(camPos - input.worldPosition);
    float3 R        = reflect(GlobalLight.direction, N);
    float  spec     = pow(saturate(dot(R, E)), 16.0f);
    float4 specular = GlobalLight.specular * Material.specular * spec;

    float  rim      = 1.0f - saturate(dot(E, N));
    float4 emissive = GlobalLight.emissive * Material.emissive * pow(rim, 2.0f);
    
    float shadow = ComputeShadowFactor(input.worldPosition);

    float4 finalColor = ambient + (diffuse + specular) * shadow + emissive;
    finalColor.a = 1.0f;
    return finalColor;
}

technique11 T0
{
    PASS_VP(P0, VS, PS)
};
