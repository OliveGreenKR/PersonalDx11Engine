#pragma once
#include "DebugDrawElement.h"
#include "DebugDrawSphere.h"
#include "DebugDrawBox.h"
#include "DebugDrawLine.h"
#include "Renderer.h"
#include "D3DShader.h"
#include <vector>
#include <memory>

class UCamera;

class UDebugDrawManager
{
private:
    std::vector<std::unique_ptr<FDebugDrawElement>> DrawElements;
    std::shared_ptr<UShader> DebugShader;    // ���� UShader Ŭ���� Ȱ��

    // DirectX ����
    ID3D11Buffer* VertexBuffer;
    ID3D11Buffer* IndexBuffer;
    ID3D11RasterizerState* WireframeState;  // ���̾������� �������� ���� ���� ��ü

    // ���� ����
    UINT MaxVertices;
    UINT MaxIndices;

    // �̱���
    UDebugDrawManager();
    ~UDebugDrawManager();

    void UpdateBuffers(URenderer* Renderer);

public:
    // �ν��Ͻ� ����
    static UDebugDrawManager* Get()
    {
        static UDebugDrawManager Instance;
        return &Instance;
    }

    // �ʱ�ȭ - ���� ���̴� Ŭ���� ���
    void Initialize(IRenderHardware* RenderHardware);

    void DrawSphere(const Vector3& Center, float Radius, const Vector4& Color, float Duration, int Segments);

    void DrawBox(const Vector3& Center, const Vector3& Extents, const Quaternion& Rotation, const Vector4& Color, float Duration);

    void DrawLine(const Vector3& Start, const Vector3& End, const Vector4& Color, float Thickness, float Duration);

    void Tick(float DeltaTime);

    void Render(UCamera* Camera, URenderer* Renderer, IRenderHardware* RenderHardware);

    void ClearAll();

};