#pragma once
#include "DebugDrawElement.h"

class FDebugDrawBox : public FDebugDrawElement
{
private:
    Vector3 Center;
    Vector3 HalfExtents;    // �ڽ� �ݰ� (�� ���� ���� ����)
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

        // �ڽ��� 8�� �𼭸� ����
        Vector3 Corners[8];
        Corners[0] = Vector3(-HalfExtents.x, -HalfExtents.y, -HalfExtents.z);
        Corners[1] = Vector3(HalfExtents.x, -HalfExtents.y, -HalfExtents.z);
        Corners[2] = Vector3(HalfExtents.x, HalfExtents.y, -HalfExtents.z);
        Corners[3] = Vector3(-HalfExtents.x, HalfExtents.y, -HalfExtents.z);

        Corners[4] = Vector3(-HalfExtents.x, -HalfExtents.y, HalfExtents.z);
        Corners[5] = Vector3(HalfExtents.x, -HalfExtents.y, HalfExtents.z);
        Corners[6] = Vector3(HalfExtents.x, HalfExtents.y, HalfExtents.z);
        Corners[7] = Vector3(-HalfExtents.x, HalfExtents.y, HalfExtents.z);

        // ȸ�� ���� �� �߽����� �̵�
        for (int i = 0; i < 8; ++i)
        {
            // ȸ�� ����
            XMVECTOR V = XMLoadFloat3(&Corners[i]);
            XMVECTOR Q = XMLoadFloat4(&Rotation);
            V = XMVector3Rotate(V, Q);

            // �߽����� �̵�
            V = XMVectorAdd(V, XMLoadFloat3(&Center));

            // ���� �߰�
            FDebugVertex Vertex;
            XMStoreFloat3(&Vertex.Position, V);
            Vertex.TexCoord = Vector2(0.0f, 0.0f);

            Vertices.push_back(Vertex);
        }

        // �ڽ��� 12�� ���� ���� (�ε���)
        // �ϴ� �簢��
        AddEdge(0, 1);
        AddEdge(1, 2);
        AddEdge(2, 3);
        AddEdge(3, 0);

        // ��� �簢��
        AddEdge(4, 5);
        AddEdge(5, 6);
        AddEdge(6, 7);
        AddEdge(7, 4);

        // ���� ����
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