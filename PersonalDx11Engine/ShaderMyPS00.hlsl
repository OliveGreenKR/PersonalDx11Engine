Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float4 color : TEXCOORD1;
};

// 쉐이더 자체에 정의하는 광원 파라미터 (테스트 목적)
// 실제 사용 시 상수로 넘기는 것이 일반적입니다.
static const float3 g_lightDirection = normalize(float3(1.0f, 1.0f, -0.5f)); // 표면에서 빛으로 향하는 방향 (요청된 -1,-1,0 의 반대)
static const float3 g_lightColor = float3(1.0f, 1.0f, 1.0f); // 백색광
static const float g_ambientStrength = 0.15f; // 주변광 강도
static const float3 g_ambientColor = g_lightColor * g_ambientStrength; // 최소 광량 보장을 위한 주변광 색상 (RGB)

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // 텍스처 샘플링
    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);

    // 투명한 검은색 처리 (원래 로직대로 input.color를 반환하려면 이 부분을 수정해야 함)
    if (all(textureColor.rgb == float3(0.0f, 0.0f, 0.0f)) && textureColor.a == 0.0f)
    {
         // 텍스처가 투명 검은색일 때 input.color를 반환
        return input.color;
    }

    // --- 조명 계산 ---

    // 1. 표면의 노멀 벡터를 정규화 (오타 수정됨: input.normal)
    float3 normalizedNormal = normalize(input.normal);

    // 2. 확산광(Diffuse) 계산
    // 표면 노멀과 빛이 오는 방향(g_lightDirection)의 내적
    // max(0, ...)로 음수 내적 방지
    float diffuseFactor = max(0.0f, dot(normalizedNormal, g_lightDirection));

    // 확산광 색상 기여도 = 광원 색상 * 확산 계수
    float3 diffuseContribution = g_lightColor * diffuseFactor;

    // 3. 최종 픽셀 색상 계산
    // 전체 광량 = 주변광 + 확산광
    float3 totalLight = g_ambientColor + diffuseContribution;

    // 표면 기본 색상 (텍스처 색상)에 전체 광량을 곱합니다.
    // 만약 input.color를 기본 색상에 포함시키고 싶다면:
    float3 baseColorRGB = textureColor.rgb * input.color.rgb;
    float3 finalColorRGB = baseColorRGB * totalLight;
    //float3 finalColorRGB = textureColor.rgb * totalLight;


    // 알파 값은 텍스처 색상의 알파 값을 사용
    float finalAlpha = textureColor.a;

    // --- 조명 계산 끝 ---

    // 최종 색상 반환
    return float4(finalColorRGB, finalAlpha);
}