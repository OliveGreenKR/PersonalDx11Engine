#pragma once
#include "DebugDrawElement.h"

class FDebugDrawLine : public FDebugDrawElement
{
private:
    Vector3 Start;
    Vector3 End;
    float Thickness;

public:
    FDebugDrawLine(const Vector3& InStart, const Vector3& InEnd,
                   const Vector4& InColor, float InThickness = 1.0f,
                   float InDuration = 0.0f)
        : FDebugDrawElement(InColor, InDuration)
        , Start(InStart)
        , End(InEnd)
        , Thickness(InThickness)
    {
        Initialize();
    }

    virtual void Initialize() override
    {
        Vertices.clear();
        Indices.clear();

        // 시작 정점
        FDebugVertex StartVertex;
        StartVertex.Position = Start;
        StartVertex.TexCoord = Vector2(0.0f, 0.0f);
        Vertices.push_back(StartVertex);

        // 끝 정점
        FDebugVertex EndVertex;
        EndVertex.Position = End;
        EndVertex.TexCoord = Vector2(0.0f, 0.0f);
        Vertices.push_back(EndVertex);

        // 인덱스
        Indices.push_back(0);
        Indices.push_back(1);
    }

    float GetThickness() const { return Thickness; }
};