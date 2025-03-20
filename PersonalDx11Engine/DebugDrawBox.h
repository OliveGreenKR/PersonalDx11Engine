#pragma once
#include "DebugDrawElement.h"

class FDebugDrawBox : public FDebugDrawElement
{
private:
    Vector3 Center;
    Vector3 HalfExtents;    // 박스 반경 (각 축의 절반 길이)
    Quaternion Rotation;

public:
    FDebugDrawBox(const Vector3& InCenter, const Vector3& InExtents,
                  const Quaternion& InRotation, const Vector4& InColor,
                  float InDuration = 0.0f)
        : FDebugDrawElement(InColor, InDuration)
        , Center(InCenter)
        , HalfExtents(InExtents)
        , Rotation(InRotation)
    {
        Initialize();
    }

    virtual void Initialize() override
    {
        Vertices.clear();
        Indices.clear();

        // 박스의 8개 모서리 생성
        Vector3 Corners[8];
        Corners[0] = Vector3(-HalfExtents.x, -HalfExtents.y, -HalfExtents.z);
        Corners[1] = Vector3(HalfExtents.x, -HalfExtents.y, -HalfExtents.z);
        Corners[2] = Vector3(HalfExtents.x, HalfExtents.y, -HalfExtents.z);
        Corners[3] = Vector3(-HalfExtents.x, HalfExtents.y, -HalfExtents.z);

        Corners[4] = Vector3(-HalfExtents.x, -HalfExtents.y, HalfExtents.z);
        Corners[5] = Vector3(HalfExtents.x, -HalfExtents.y, HalfExtents.z);
        Corners[6] = Vector3(HalfExtents.x, HalfExtents.y, HalfExtents.z);
        Corners[7] = Vector3(-HalfExtents.x, HalfExtents.y, HalfExtents.z);

        // 회전 적용 및 중심으로 이동
        for (int i = 0; i < 8; ++i)
        {
            // 회전 적용
            XMVECTOR V = XMLoadFloat3(&Corners[i]);
            XMVECTOR Q = XMLoadFloat4(&Rotation);
            V = XMVector3Rotate(V, Q);

            // 중심으로 이동
            V = XMVectorAdd(V, XMLoadFloat3(&Center));

            // 정점 추가
            FDebugVertex Vertex;
            XMStoreFloat3(&Vertex.Position, V);
            Vertex.TexCoord = Vector2(0.0f, 0.0f);

            Vertices.push_back(Vertex);
        }

        // 박스의 12개 엣지 정의 (인덱스)
        // 하단 사각형
        AddEdge(0, 1);
        AddEdge(1, 2);
        AddEdge(2, 3);
        AddEdge(3, 0);

        // 상단 사각형
        AddEdge(4, 5);
        AddEdge(5, 6);
        AddEdge(6, 7);
        AddEdge(7, 4);

        // 연결 엣지
        AddEdge(0, 4);
        AddEdge(1, 5);
        AddEdge(2, 6);
        AddEdge(3, 7);
    }

private:
    void AddEdge(UINT Start, UINT End)
    {
        Indices.push_back(Start);
        Indices.push_back(End);
    }
};