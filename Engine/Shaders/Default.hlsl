cbuffer TransformBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
}

struct VS_INPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 position = float4(input.pos, 1.0f);
    
    position = mul(position, world);
    position = mul(position, view);
    position = mul(position, projection);

    output.pos = position;
    output.color = input.color;

    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    return input.color;
}