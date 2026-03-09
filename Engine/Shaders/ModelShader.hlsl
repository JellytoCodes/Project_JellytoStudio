cbuffer TransformBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
}

cbuffer BoneBuffer : register(b1)
{
    matrix bones[256];
}

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 indices : BLENDINDICES; // »À ¹øÈ£ (x, y, z, w)
    float4 weights : BLENDWEIGHTS; // °¡ÁßÄ¡ (x, y, z, w)
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

Texture2D diffuseMap : register(t0);
SamplerState sampler0 : register(s0);

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;

    matrix skinMatrix = (matrix) 0;
    
    skinMatrix += mul(input.weights.x, bones[(uint) input.indices.x]);
    skinMatrix += mul(input.weights.y, bones[(uint) input.indices.y]);
    skinMatrix += mul(input.weights.z, bones[(uint) input.indices.z]);
    skinMatrix += mul(input.weights.w, bones[(uint) input.indices.w]);
    
    float4 position = mul(float4(input.pos, 1.0f), skinMatrix);
    position = mul(position, world);
    position = mul(position, view);
    position = mul(position, projection);

    output.pos = position;
    output.uv = input.uv;
    
    output.normal = mul(input.normal, (float3x3) skinMatrix);
    output.normal = normalize(mul(output.normal, (float3x3) world));

    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    float4 color = diffuseMap.Sample(sampler0, input.uv);
    
    float3 lightDir = normalize(float3(1, -1, 1));
    float diffuse = saturate(dot(input.normal, -lightDir)) + 0.3f; // 0.3Àº ¾Úºñ¾ðÆ® ±¤

    return color * diffuse;
}