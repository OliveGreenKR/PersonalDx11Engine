// 상수 버퍼 (CBuffer)
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

cbuffer COLOR_BUFFER : register(b1)
{
    float4 InColor;
};

// Vertex Shader 입력 구조체
struct VS_INPUT
{
    float4 Pos : POSITION;
};

// Vertex Shader 출력 구조체 (Geometry Shader로 전달)
struct GS_INPUT
{
    float4 Pos : SV_POSITION;
};

// Geometry Shader 출력 구조체 (Pixel Shader로 전달)
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
};

// Vertex Shader
GS_INPUT mainVS(VS_INPUT input)
{
    GS_INPUT output = (GS_INPUT) 0;
    output.Pos = mul(input.Pos, World); // 월드 변환
    output.Pos = mul(output.Pos, View); // 뷰 변환
    output.Pos = mul(output.Pos, Projection); // 투영 변환
    return output;
}

// Geometry Shader: 삼각형을 받아 선으로 변환
[maxvertexcount(6)] // 최대 6개의 정점 출력 (삼각형당 3개의 선)
void mainGS(triangle GS_INPUT input[3], inout LineStream<PS_INPUT> output)
{
    PS_INPUT lineVert = (PS_INPUT) 0;

    // 첫 번째 선 (0 -> 1)
    lineVert.Pos = input[0].Pos;
    output.Append(lineVert);
    lineVert.Pos = input[1].Pos;
    output.Append(lineVert);
    output.RestartStrip();

    // 두 번째 선 (1 -> 2)
    lineVert.Pos = input[1].Pos;
    output.Append(lineVert);
    lineVert.Pos = input[2].Pos;
    output.Append(lineVert);
    output.RestartStrip();

    // 세 번째 선 (2 -> 0)
    lineVert.Pos = input[2].Pos;
    output.Append(lineVert);
    lineVert.Pos = input[0].Pos;
    output.Append(lineVert);
    output.RestartStrip();
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 color = InColor;
    return color; 
}