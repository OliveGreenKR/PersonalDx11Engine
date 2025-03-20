// ��� ���� (CBuffer)
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

// Vertex Shader �Է� ����ü
struct VS_INPUT
{
    float4 Pos : POSITION;
};

// Vertex Shader ��� ����ü (Geometry Shader�� ����)
struct GS_INPUT
{
    float4 Pos : SV_POSITION;
};

// Geometry Shader ��� ����ü (Pixel Shader�� ����)
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
};

// Vertex Shader
GS_INPUT mainVS(VS_INPUT input)
{
    GS_INPUT output = (GS_INPUT) 0;
    output.Pos = mul(input.Pos, World); // ���� ��ȯ
    output.Pos = mul(output.Pos, View); // �� ��ȯ
    output.Pos = mul(output.Pos, Projection); // ���� ��ȯ
    return output;
}

// Geometry Shader: �ﰢ���� �޾� ������ ��ȯ
[maxvertexcount(6)] // �ִ� 6���� ���� ��� (�ﰢ���� 3���� ��)
void mainGS(triangle GS_INPUT input[3], inout LineStream<PS_INPUT> output)
{
    PS_INPUT lineVert = (PS_INPUT) 0;

    // ù ��° �� (0 -> 1)
    lineVert.Pos = input[0].Pos;
    output.Append(lineVert);
    lineVert.Pos = input[1].Pos;
    output.Append(lineVert);
    output.RestartStrip();

    // �� ��° �� (1 -> 2)
    lineVert.Pos = input[1].Pos;
    output.Append(lineVert);
    lineVert.Pos = input[2].Pos;
    output.Append(lineVert);
    output.RestartStrip();

    // �� ��° �� (2 -> 0)
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