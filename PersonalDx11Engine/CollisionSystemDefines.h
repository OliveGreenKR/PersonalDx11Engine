#pragma once
#pragma once
#include "Math.h"
#include <memory>
#include "Transform.h"

// �浹ü ���� ����
enum class ECollisionShapeType
{
    Box,
    Sphere
};

// �浹ü ���� ����
struct FCollisionShapeData
{
    inline float GetSphereRadius() const {
        assert(Type == ECollisionShapeType::Sphere);
        return HalfExtent.x;  // ���� x ���и� ���
    }

    inline const Vector3& GetBoxHalfExtents() const {
        assert(Type == ECollisionShapeType::Box);
        return HalfExtent;    // �ڽ��� ��ü ���� ���
    }

    ECollisionShapeType Type = ECollisionShapeType::Box;
    Vector3 HalfExtent = Vector3::Zero;  // Box�� - Sphere�� x���� ���
};

// ���� �浹ü ����
struct FCollisionTestData
{
    FCollisionShapeData ShapeData;
    FTransform Transform;
};

// ���� �浹 �˻�� ������
struct FCollisionSweptTestData
{
    FCollisionShapeData ShapeData;
    FTransform PrevTransform;
    FTransform CurrentTransform;
};

// �浹 �� ������
struct FCollisionPairData
{
    FCollisionTestData A;
    FCollisionTestData B;
    float TimeStep = 0.0f;
};

// ���� �浹 �� ������ 
struct FCollisionSweptPairData
{
    FCollisionSweptTestData A;
    FCollisionSweptTestData B;
    float TimeStep = 0.0f;
};

// �浹 ���� ���
struct FCollisionDetectionResult
{
    bool bCollided = false;
    Vector3 Normal = Vector3::Zero;      // �浹 ����
    Vector3 Point = Vector3::Zero;       // �浹 ����
    float PenetrationDepth = 0.0f;       // ħ�� ����
    float TimeOfImpact = 0.0f;          // CCD�� �浹 ����
};

// �浹 �̺�Ʈ ���� (������Ʈ�� ��������Ʈ���� ���)
struct FCollisionEventData
{
    std::weak_ptr<class UCollisionComponent> OtherComponent;
    FCollisionDetectionResult CollisionResult;
    float TimeStep;
};

// ����� �ð�ȭ ����
struct FCollisionDebugParams
{
    bool bShowCollisionShapes = false;
    bool bShowCollisionEvents = false;
    Vector4 ShapeColor = { 0.0f, 1.0f, 0.0f, 0.5f };  // ������ ���
};

// �ý��� ����
struct FCollisionSystemConfig
{
    float MinimumTimeStep = 0.0016f;     // �ּ� �ð� ���� (�� 600fps)
    float MaximumTimeStep = 0.0333f;     // �ִ� �ð� ���� (�� 30fps)
    int32_t MaxIterations = 8;           // �ִ� �ݺ� Ƚ��
    float CCDMotionThreshold = 1.0f;     // CCD Ȱ��ȭ �ӵ� �Ӱ谪
    FCollisionDebugParams DebugParams;
};