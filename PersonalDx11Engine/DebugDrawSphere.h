#pragma once
#include "DebugDrawElement.h"

class FDebugDrawSphere : public FDebugDrawElement
{
private:
    Vector3 Center;
    float Radius;
    int Segments;

public:
    FDebugDrawSphere(const Vector3& InCenter, float InRadius,
                     const Vector4& InColor, float InDuration = 0.0f, int InSegments = 16)
        : FDebugDrawElement(InColor, InDuration)
        , Center(InCenter)
        , Radius(InRadius)
        , Segments(InSegments)
    {
        Initialize();
    }

    virtual void Initialize() override
    {
        Vertices.clear();
        Indices.clear();

        // ���������� ��ü ��ǥ ���� (����)
        // X, Y, Z ����� ���� �׷��� ���̾������� ��ü ǥ��
        GenerateCircle(Vector3::Right, Vector3::Up, Segments);     // XY ���
        GenerateCircle(Vector3::Forward, Vector3::Up, Segments);   // ZY ���
        GenerateCircle(Vector3::Right, Vector3::Forward, Segments);// XZ ���
    }

private:
    void GenerateCircle(const Vector3& Axis1, const Vector3& Axis2, int Segments)
    {
        const float StepAngle = 2.0f * PI / Segments;
        const UINT StartIndex = static_cast<UINT>(Vertices.size());

        // ���� ���� ���� ����
        for (int i = 0; i < Segments; ++i)
        {
            const float Angle = i * StepAngle;
            const float X = cos(Angle);
            const float Y = sin(Angle);

            Vector3 Position = Center + (Axis1 * X + Axis2 * Y) * Radius;

            FDebugVertex Vertex;
            Vertex.Position = Position;
            Vertex.TexCoord = Vector2(0.0f, 0.0f);  // �ؽ�ó ��ǥ�� ������� ����

            Vertices.push_back(Vertex);

            // �ε��� �߰� (���� ���׸�Ʈ)
            if (i < Segments - 1)
            {
                Indices.push_back(StartIndex + i);
                Indices.push_back(StartIndex + i + 1);
            }
            else
            {
                // ������ �������� ù �������� ����
                Indices.push_back(StartIndex + i);
                Indices.push_back(StartIndex);
            }
        }
    }
};