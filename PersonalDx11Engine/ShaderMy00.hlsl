Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);
//vertex 쉐이더에서 반드시 모든 컨스탄틉 버퍼를 참조해야 정상 동작함
cbuffer MATRIX_BUFFER : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer DEBUG_BUFFER : register(b1)
{
    float4 debugColor;
};

struct VS_INPUT
{
    float4 position : POSITION; // Input position from vertex buffer
    float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float2 tex : TEXCOORD0;
    float4 debugValues : TEXCOORD1;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    input.position.w = 1.0f;
    
    output.position = mul(input.position, worldMatrix);
    float4 worldPos = output.position;
    output.position = mul(output.position, viewMatrix);
    float4 viewPos = output.position;
    output.position = mul(output.position, projectionMatrix);
    float4 projPos = output.position;
    
    output.tex = input.tex;
    output.debugValues = debugColor;
 
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 outputColor;
    float4 color = input.debugValues;
    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
    
    // 디버그 컬러와 텍스처 색상을 혼합
    outputColor = textureColor * color;
    
    return outputColor;
    
}
