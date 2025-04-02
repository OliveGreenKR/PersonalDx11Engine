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
    //Sample the texture
    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
    //float4 textureColor = float4(input.tex.x, input.tex.y, 0, 1);
    
    if (all(textureColor == float4(0.0, 0.0, 0.0, 0.0)))
    {
        return input.color;
    }
    
    return textureColor * input.color;
}