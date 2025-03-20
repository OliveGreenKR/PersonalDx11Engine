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

        // 원점에서의 구체 좌표 생성 (정점)
        // X, Y, Z 평면의 원을 그려서 와이어프레임 구체 표현
        GenerateCircle(Vector3::Right, Vector3::Up, Segments);     // XY 평면
        GenerateCircle(Vector3::Forward, Vector3::Up, Segments);   // ZY 평면
        GenerateCircle(Vector3::Right, Vector3::Forward, Segments);// XZ 평면
    }

private:
    void GenerateCircle(const Vector3& Axis1, const Vector3& Axis2, int Segments)
    {
        const float StepAngle = 2.0f * PI / Segments;
        const UINT StartIndex = static_cast<UINT>(Vertices.size());

        // 현재 원의 정점 생성
        for (int i = 0; i < Segments; ++i)
        {
            const float Angle = i * StepAngle;
            const float X = cos(Angle);
            const float Y = sin(Angle);

            Vector3 Position = Center + (Axis1 * X + Axis2 * Y) * Radius;

            FDebugVertex Vertex;
            Vertex.Position = Position;
            Vertex.TexCoord = Vector2(0.0f, 0.0f);  // 텍스처 좌표는 사용하지 않음

            Vertices.push_back(Vertex);

            // 인덱스 추가 (라인 세그먼트)
            if (i < Segments - 1)
            {
                Indices.push_back(StartIndex + i);
                Indices.push_back(StartIndex + i + 1);
            }
            else
            {
                // 마지막 정점에서 첫 정점으로 연결
                Indices.push_back(StartIndex + i);
                Indices.push_back(StartIndex);
            }
        }
    }
};