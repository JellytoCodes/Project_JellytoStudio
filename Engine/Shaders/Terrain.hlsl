#include "Global.hlsli"
#include "Light.hlsli"

// 인스턴싱 렌더링을 위한 입력 구조체
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

// 정점 셰이더 (Vertex Shader)
MeshOutput VS(VertexMesh input)
{
    MeshOutput output;
    
    // 1. World 연산
    output.position = mul(input.position, input.world);
    output.worldPosition = output.position.xyz; // 4차원을 3차원으로 명시적 변환
    
    // 2. View-Projection 연산 (Global.hlsli의 VP 사용)
    output.position = mul(output.position, VP);
    
    // 3. UV 복사
    output.uv = input.uv;
    
    // 4. 법선(Normal)과 탄젠트(Tangent) 회전 적용
    output.normal = mul(input.normal, (float3x3)input.world);
    output.tangent = mul(input.tangent, (float3x3)input.world);
        
    return output;
}

// 픽셀 셰이더 (Pixel Shader)
float4 PS(MeshOutput input) : SV_TARGET
{
    // 형님의 바닥 텍스처를 입혀서 출력
    float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
    return color;
}

// Global.hlsli에 만들어두신 기깔난 매크로를 사용해서 테크닉 정의
technique11 T0
{
    // PASS_VP 매크로가 내부적으로 vs_5_0, ps_5_0 컴파일을 수행합니다.
    PASS_VP(P0, VS, PS)
}