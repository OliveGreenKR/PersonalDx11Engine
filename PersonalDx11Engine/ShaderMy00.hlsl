Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

struct VS_INPUT
{
    float4 position : POSITION; // Input position from vertex buffer
    float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float2 tex : TEXCOORD0;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    output.position = input.position;
    output.tex = input.tex;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 outputColor;

    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
    
    outputColor = textureColor;
    return outputColor;
}
