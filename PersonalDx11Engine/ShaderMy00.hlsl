Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

cbuffer MATRIX_BUFFER : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
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
    float3 debugValues : TEXCOORD1;
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
    
    output.debugValues.x = viewPos.z;
    output.debugValues.y = worldPos.z;
    output.debugValues.z = projPos.z;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 outputColor;

    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
    outputColor = textureColor;
    //float r = (input.debugValues.y - 0) / 30.0f;
    //outputColor = float4(input.debugValues.y, 0,0,1.0f);
    
    return outputColor;
    
}
