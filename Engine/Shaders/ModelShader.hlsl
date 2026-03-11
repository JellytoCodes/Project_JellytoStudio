#include "Global.hlsli"
#include "Light.hlsli"

#define MAX_MODEL_TRANSFORM 250

cbuffer BoneBuffer : register(b2) // 레지스터 번호 명시 권장
{
    matrix BoneTransforms[MAX_MODEL_TRANSFORM];
};

// 인스턴싱을 위해 구조체를 수정해야 합니다!
struct VS_INST_INPUT
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    // ★ 인스턴싱 행렬 (Slot 1에서 들어옴)
    matrix instWorld : INSTWORLD;
};

MeshOutput VS(VS_INST_INPUT input)
{
    MeshOutput output;
    
    // 1. [임시 땜빵] 스키닝 생략하고 월드 변환만 수행
    // 만약 BoneTransforms가 0이면 여기서 모델이 깨지므로 일단 월드만 곱합니다.
    float4 pos = input.position;
    
    // 2. 인스턴싱 행렬 적용 (W 대신 instWorld 사용!)
    output.position = mul(pos, input.instWorld);
    output.worldPosition = output.position.xyz;
    
    // 3. 뷰/투영 변환
    output.position = mul(output.position, VP);
    
    output.uv = input.uv;
    
    // 4. 노말도 인스턴싱 행렬로 변환
    output.normal = mul(input.normal, (float3x3) input.instWorld);
    output.tangent = mul(input.tangent, (float3x3) input.instWorld);

    return output;
}

float4 PS(MeshOutput input) : SV_TARGET
{
	// 1. 텍스처 샘플링
    float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
    
    // [검증] 만약 텍스처가 안 나오면 투명도(Alpha) 문제일 수 있으니 1.0으로 강제 고정해봅니다.
    // return float4(color.rgb, 1.0f); 
    
    return color;
}

technique11 T0
{
	PASS_VP(P0, VS, PS)
}