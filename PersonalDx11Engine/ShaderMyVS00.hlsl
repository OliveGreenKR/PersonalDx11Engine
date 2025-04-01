cbuffer MATRIX_WORLD : register(b0)
{
    matrix worldMatrix;
};
cbuffer MATRIX_CAMERA : register(b1)
{
    matrix viewMatrix;
    matrix projectionMatrix;
};
cbuffer COLOR_BUFFER : register(b2)
{
    float4 InColor;
};
struct VS_INPUT
{
    float3 position : POSITION;
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
    
    float4 Inposition = input.position.xyzx;
    Inposition.w = 1.0f;
    
    output.position = mul(Inposition, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    output.tex = input.tex;
    output.color = InColor;
    return output;
}
