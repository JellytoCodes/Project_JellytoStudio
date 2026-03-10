cbuffer GlobalBuffer : register(b0)
{
    matrix V;
    matrix P;
    matrix VP;
    matrix VInv;
};

cbuffer LightBuffer : register(b1)
{
    float4 lightAmbient;
    float4 lightDiffuse;
    float4 lightSpecular;
    float4 lightEmissive;
    float3 lightDir;
    float lightPadding;
};

cbuffer BoneBuffer : register(b2)
{
    matrix bones[250];
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 indices : BLENDINDICES;
    float4 weights : BLENDWEIGHTS;
    
    matrix instWorld : INSTWORLD; 
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

Texture2D diffuseMap : register(t0);
SamplerState sampler0 : register(s0);

// Vertex Shader
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;

    /*
    matrix skinMatrix = (matrix)0;
    skinMatrix += mul(input.weights.x, bones[(uint)input.indices.x]);
    skinMatrix += mul(input.weights.y, bones[(uint)input.indices.y]);
    skinMatrix += mul(input.weights.z, bones[(uint)input.indices.z]);
    skinMatrix += mul(input.weights.w, bones[(uint)input.indices.w]);

    float4 position = mul(float4(input.pos, 1.0f), skinMatrix);
    position = mul(position, input.instWorld);
    position = mul(position, V);
    position = mul(position, P);

    output.pos = position;
    output.uv = input.uv;

    output.normal = mul(input.normal, (float3x3)skinMatrix);
    output.normal = normalize(mul(output.normal, (float3x3)input.instWorld));

    return output;
	*/

    float4 position = float4(input.pos, 1.0f);

    position = mul(position, input.instWorld); 
    position = mul(position, V);
    position = mul(position, P);

    output.pos = position;
    output.uv = input.uv;
    output.normal = input.normal;

    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
	float4 color = diffuseMap.Sample(sampler0, input.uv);

    float3 tempLightDir = normalize(float3(1, -1, 1));
    float diffuse = saturate(dot(input.normal, -tempLightDir));
    
    return color * (diffuse + 0.3f);
}

technique11 T0
{
	pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
};