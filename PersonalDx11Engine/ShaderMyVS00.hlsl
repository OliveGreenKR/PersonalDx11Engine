cbuffer MATRIX_BUFFER : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix worldInverseMatrix;
};
cbuffer COLOR_BUFFER : register(b1)
{
    float4 InColor;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};
struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float4 color : TEXCOORD1;
};
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 Inposition = float4(input.position, 1.0);
    
    output.position = mul(Inposition, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    output.normal = normalize(mul(input.normal, (float3x3) transpose(worldInverseMatrix)));
    output.tex = input.tex;
    output.color = InColor;
    return output;
}
