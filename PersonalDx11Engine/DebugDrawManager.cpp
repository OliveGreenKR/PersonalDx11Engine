// DebugDrawManager.cpp
#include "DebugDrawManager.h"
#include "Camera.h"

UDebugDrawManager::UDebugDrawManager()
    : VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , MaxVertices(1024)
    , MaxIndices(2048)
{
}

UDebugDrawManager::~UDebugDrawManager()
{
    if (VertexBuffer)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }

    if (IndexBuffer)
    {
        IndexBuffer->Release();
        IndexBuffer = nullptr;
    }
}

void UDebugDrawManager::Initialize(IRenderHardware* RenderHardware)
{
    ID3D11Device* Device = RenderHardware->GetDevice();

    // 디버그 와이어프레임용 HLSL 쉐이더 파일 경로
    const wchar_t* DebugShaderPath = L"ShaderDebug.hlsl";

    // 기존 쉐이더 클래스를 사용하여 쉐이더 로드
    D3D11_INPUT_ELEMENT_DESC DebugInputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    // UShader 클래스의 인스턴스 생성
    DebugShader = std::make_shared<UShader>();
    DebugShader->Load(Device, DebugShaderPath, DebugShaderPath, DebugInputLayout, 2);

    // 와이어프레임 렌더 스테이트 생성
    D3D11_RASTERIZER_DESC WireframeDesc = {};
    WireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    WireframeDesc.CullMode = D3D11_CULL_NONE;
    WireframeDesc.DepthClipEnable = TRUE;
    Device->CreateRasterizerState(&WireframeDesc, &WireframeState);

    // 정점 버퍼 생성
    D3D11_BUFFER_DESC VertexBufferDesc = {};
    VertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    VertexBufferDesc.ByteWidth = sizeof(FDebugDrawElement::FDebugVertex) * MaxVertices;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    Device->CreateBuffer(&VertexBufferDesc, nullptr, &VertexBuffer);

    // 인덱스 버퍼 생성
    D3D11_BUFFER_DESC IndexBufferDesc = {};
    IndexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    IndexBufferDesc.ByteWidth = sizeof(UINT) * MaxIndices;
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    Device->CreateBuffer(&IndexBufferDesc, nullptr, &IndexBuffer);
}

void UDebugDrawManager::DrawSphere(const Vector3& Center, float Radius,
                                   const Vector4& Color, float Duration, int Segments)
{
    DrawElements.push_back(std::make_unique<FDebugDrawSphere>(
        Center, Radius, Color, Duration, Segments));
}

void UDebugDrawManager::DrawBox(const Vector3& Center, const Vector3& Extents,
                                const Quaternion& Rotation, const Vector4& Color,
                                float Duration)
{
    DrawElements.push_back(std::make_unique<FDebugDrawBox>(
        Center, Extents, Rotation, Color, Duration));
}

void UDebugDrawManager::DrawLine(const Vector3& Start, const Vector3& End,
                                 const Vector4& Color, float Thickness,
                                 float Duration)
{
    DrawElements.push_back(std::make_unique<FDebugDrawLine>(
        Start, End, Color, Thickness, Duration));
}

void UDebugDrawManager::Tick(float DeltaTime)
{
    // 만료된 요소 제거
    DrawElements.erase(
        std::remove_if(
            DrawElements.begin(),
            DrawElements.end(),
            [](const auto& Element) { return Element->IsExpired(); }
        ),
        DrawElements.end()
    );

    // 남은 요소들의 시간 업데이트
    for (auto& Element : DrawElements)
    {
        Element->Update(DeltaTime);
    }
}

void UDebugDrawManager::UpdateBuffers(URenderer* Renderer)
{
    ID3D11DeviceContext* DeviceContext = Renderer->GetDeviceContext();

    // 모든 드로우 요소의 정점과 인덱스 데이터 수집
    std::vector<FDebugDrawElement::FDebugVertex> AllVertices;
    std::vector<UINT> AllIndices;
    UINT BaseVertexIndex = 0;

    for (const auto& Element : DrawElements)
    {
        const auto& ElementVertices = Element->GetVertices();
        const auto& ElementIndices = Element->GetIndices();

        // 인덱스 오프셋 조정
        for (UINT Index : ElementIndices)
        {
            AllIndices.push_back(BaseVertexIndex + Index);
        }

        // 정점 추가
        AllVertices.insert(AllVertices.end(), ElementVertices.begin(), ElementVertices.end());

        // 다음 요소의 베이스 인덱스 업데이트
        BaseVertexIndex += static_cast<UINT>(ElementVertices.size());
    }

    // 버퍼 업데이트
    if (!AllVertices.empty())
    {
        // 정점 버퍼 업데이트
        D3D11_MAPPED_SUBRESOURCE MappedVB;
        DeviceContext->Map(VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedVB);
        memcpy(MappedVB.pData, AllVertices.data(),
               AllVertices.size() * sizeof(FDebugDrawElement::FDebugVertex));
        DeviceContext->Unmap(VertexBuffer, 0);
    }

    if (!AllIndices.empty())
    {
        // 인덱스 버퍼 업데이트
        D3D11_MAPPED_SUBRESOURCE MappedIB;
        DeviceContext->Map(IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedIB);
        memcpy(MappedIB.pData, AllIndices.data(),
               AllIndices.size() * sizeof(UINT));
        DeviceContext->Unmap(IndexBuffer, 0);
    }
}

void UDebugDrawManager::Render(UCamera* Camera, URenderer* Renderer, IRenderHardware* RenderHardware)
{
    if (DrawElements.empty() || !Camera || !Renderer || !RenderHardware)
        return;

    // 버퍼 업데이트
    UpdateBuffers(Renderer);

    ID3D11DeviceContext* DeviceContext = RenderHardware->GetDeviceContext();

    // 현재 렌더 스테이트 저장
    ID3D11RasterizerState* PrevState = nullptr;
    DeviceContext->RSGetState(&PrevState);

    // 와이어프레임 모드 설정
    DeviceContext->RSSetState(WireframeState);

    // 버퍼 바인딩
    UINT Stride = sizeof(FDebugDrawElement::FDebugVertex);
    UINT Offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
    DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    // 기본 셰이더 바인딩
    DebugShader->Bind(DeviceContext);

    // 뷰 및 투영 행렬 설정
    Matrix IdentityMatrix = XMMatrixIdentity();  // 월드 행렬은 Identity 사용
    Matrix ViewMatrix = Camera->GetViewMatrix();
    Matrix ProjectionMatrix = Camera->GetProjectionMatrix();

    FMatrixBufferData MatrixData(IdentityMatrix, ViewMatrix, ProjectionMatrix);
    DebugShader->BindMatrix(DeviceContext, MatrixData);

    // 각 드로우 요소 그리기
    UINT StartIndexLocation = 0;

    for (const auto& Element : DrawElements)
    {
        const auto& ElementVertices = Element->GetVertices();
        const auto& ElementIndices = Element->GetIndices();

        // 색상 설정
        FColorBufferData ColorData(Element->GetColor());
        DebugShader->BindColor(DeviceContext, ColorData);

        // 그리기
        DeviceContext->DrawIndexed(
            static_cast<UINT>(ElementIndices.size()),
            StartIndexLocation,
            0
        );

        // 다음 요소의 인덱스 위치 업데이트
        StartIndexLocation += static_cast<UINT>(ElementIndices.size());
    }

    // 이전 렌더 스테이트 복원
    DeviceContext->RSSetState(PrevState);
    if (PrevState) PrevState->Release();

    // 기본 프리미티브 토폴로지 복원
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void UDebugDrawManager::ClearAll()
{
    DrawElements.clear();
}