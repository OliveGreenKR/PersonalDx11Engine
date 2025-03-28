Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 color : TEXCOORD1;
};

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
    //float4 textureColor = float4(input.tex.x, input.tex.y, 0, 1);
    return textureColor * input.color;
}