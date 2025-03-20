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
    std::shared_ptr<UShader> DebugShader;    // 기존 UShader 클래스 활용

    // DirectX 버퍼
    ID3D11Buffer* VertexBuffer;
    ID3D11Buffer* IndexBuffer;
    ID3D11RasterizerState* WireframeState;  // 와이어프레임 렌더링을 위한 상태 객체

    // 버퍼 정보
    UINT MaxVertices;
    UINT MaxIndices;

    // 싱글톤
    UDebugDrawManager();
    ~UDebugDrawManager();

    void UpdateBuffers(URenderer* Renderer);

public:
    // 인스턴스 접근
    static UDebugDrawManager* Get()
    {
        static UDebugDrawManager Instance;
        return &Instance;
    }

    // 초기화 - 기존 쉐이더 클래스 사용
    void Initialize(IRenderHardware* RenderHardware);

    void DrawSphere(const Vector3& Center, float Radius, const Vector4& Color, float Duration, int Segments);

    void DrawBox(const Vector3& Center, const Vector3& Extents, const Quaternion& Rotation, const Vector4& Color, float Duration);

    void DrawLine(const Vector3& Start, const Vector3& End, const Vector4& Color, float Thickness, float Duration);

    void Tick(float DeltaTime);

    void Render(UCamera* Camera, URenderer* Renderer, IRenderHardware* RenderHardware);

    void ClearAll();

};