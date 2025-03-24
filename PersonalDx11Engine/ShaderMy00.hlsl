Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

cbuffer MATRIX_BUFFER : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};
cbuffer COLOR_BUFFER : register(b1)
{
    float4 InColor;
};
struct VS_INPUT
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};
struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 color : TEXCOORD1;
};
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    input.position.w = 1.0f;
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    output.tex = input.tex;
    output.color = InColor;
    return output;
}
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
    return textureColor * input.color;
}