#include "Global.hlsli"
#include "Light.hlsli"

struct TweenFrame
{
    int animIndex;
    int currFrame;
    int nextFrame;
    float ratio;
    float speed;
    float sumTime;
    float padding[2];
};

struct TweenDesc
{
    float tweenDuration;
    float tweenRatio;
    float tweenSumTime;
    float padding;
    TweenFrame curr;
    TweenFrame next;
};

StructuredBuffer<TweenDesc> Tweens : register(t2);

Texture2DArray TransformMap : register(t1);

struct VS_INST_INPUT
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 indices : BLENDINDICES;
    float4 weights : BLENDWEIGHTS;
    matrix instWorld : INSTWORLD;

    uint instanceID : SV_InstanceID;
};

matrix GetMatrix(int animIndex, int currFrame, int nextFrame, float ratio, int boneIndex)
{
    // Texture Load 좌표: int4(X, Y, TextureArrayIndex, MipLevel)
    float4 c0 = TransformMap.Load(int4(boneIndex * 4 + 0, currFrame, animIndex, 0));
    float4 c1 = TransformMap.Load(int4(boneIndex * 4 + 1, currFrame, animIndex, 0));
    float4 c2 = TransformMap.Load(int4(boneIndex * 4 + 2, currFrame, animIndex, 0));
    float4 c3 = TransformMap.Load(int4(boneIndex * 4 + 3, currFrame, animIndex, 0));
    matrix currMat = matrix(c0, c1, c2, c3);

    float4 n0 = TransformMap.Load(int4(boneIndex * 4 + 0, nextFrame, animIndex, 0));
    float4 n1 = TransformMap.Load(int4(boneIndex * 4 + 1, nextFrame, animIndex, 0));
    float4 n2 = TransformMap.Load(int4(boneIndex * 4 + 2, nextFrame, animIndex, 0));
    float4 n3 = TransformMap.Load(int4(boneIndex * 4 + 3, nextFrame, animIndex, 0));
    matrix nextMat = matrix(n0, n1, n2, n3);

    return currMat * (1.0f - ratio) + nextMat * ratio;
}

MeshOutput VS(VS_INST_INPUT input)
{
    MeshOutput output;

    // ★ StructuredBuffer에서 내 번호에 맞는 데이터 꺼내기
    TweenDesc desc = Tweens[input.instanceID];

    matrix m = 0;
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        if (input.weights[i] <= 0.0f)
            continue;

        matrix mat = GetMatrix(desc.curr.animIndex, desc.curr.currFrame, desc.curr.nextFrame, desc.curr.ratio, (int) input.indices[i]);
        
        if (desc.next.animIndex >= 0)
        {
            matrix nextAnimMat = GetMatrix(desc.next.animIndex, desc.next.currFrame, desc.next.nextFrame, desc.next.ratio, (int) input.indices[i]);
            mat = mat * (1.0f - desc.tweenRatio) + nextAnimMat * desc.tweenRatio;
        }

        m += mul(input.weights[i], mat);
    }

    float4 skinnedPos = mul(input.position, m);
    output.position = mul(skinnedPos, input.instWorld);
    output.worldPosition = output.position.xyz;
    output.position = mul(output.position, VP);

    output.uv = input.uv;
    output.normal = normalize(mul(mul(input.normal, (float3x3) m), (float3x3) input.instWorld));
    output.tangent = normalize(mul(mul(input.tangent, (float3x3) m), (float3x3) input.instWorld));

    return output;
}

float4 PS(MeshOutput input) : SV_TARGET
{
    return DiffuseMap.Sample(LinearSampler, input.uv);
}

technique11 T0
{
    PASS_VP(P0, VS, PS)
}