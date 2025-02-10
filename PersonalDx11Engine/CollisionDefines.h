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
    Vector3 HalfExtent = Vector3::Zero;  //Sphere�� x���� ���
};

// �浹 ���� ���
struct FCollisionDetectionResult
{
    bool bCollided = false;
    Vector3 Normal = Vector3::Zero;      // �浹 ����
    Vector3 Point = Vector3::Zero;       // �浹 ����
    float PenetrationDepth = 0.0f;       // ħ�� ����
    float TimeOfImpact = 0.0f;           // CCD�� �浹 ����
};


// �浹 ���� ����� ���� �Ű�����, �浹 ������ �ʿ��� ���� �Ӽ� ����
struct FCollisionResponseParameters
{
    float Mass = 0.0f;
    float RotationalInertia = 0.0f;
    
    Vector3 Position = Vector3::Zero;
    Vector3 Velocity = Vector3::Zero;
    Vector3 AngularVelocity = Vector3::Zero;

    float Restitution = 0.5f; // �ݹ߰��
    float FrictionStatic = 0.8f;
    float FrictionKinetic = 0.5f;
};

struct FCollisionResponseResult
{
    Vector3 NetImpulse = Vector3::Zero; // ��� ������ ȿ���� ������ ���� ��ݷ�
    Vector3 ApplicationPoint = Vector3::Zero;
};
//�浹 ������Ʈ �� ����ü
struct FCollisionPair
{
    std::weak_ptr<class UCollisionComponent> ComponentA;
    std::weak_ptr<class UCollisionComponent> ComponentB;

    bool operator==(const FCollisionPair& Other) const {
        auto a1 = ComponentA.lock();
        auto a2 = ComponentB.lock();
        auto b1 = Other.ComponentA.lock();
        auto b2 = Other.ComponentB.lock();

        if (!a1 || !a2 || !b1 || !b2) return false;

        return (a1 == b1 && a2 == b2) || (a1 == b2 && a2 == b1);
    }
};

// std �ؽ� �Լ� Ư��ȭ 
namespace std
{
    template<>
    struct hash<FCollisionPair>
    {
        size_t operator()(const FCollisionPair& Key) const {
            auto comp1 = Key.ComponentA.lock();
            auto comp2 = Key.ComponentB.lock();

            if (!comp1 || !comp2) return 0;

            size_t h1 = std::hash<UCollisionComponent*>()(comp1.get());
            size_t h2 = std::hash<UCollisionComponent*>()(comp2.get());
            return h1 ^ (h2 << 1);
        }
    };
}

// �浹 �̺�Ʈ ���� (������Ʈ�� ��������Ʈ���� ���)
struct FCollisionEventData
{
    std::weak_ptr<class UCollisionComponent> OtherComponent;
    FCollisionDetectionResult CollisionResult;
    float TimeStep;
};

// �ý��� ����
struct FCollisionSystemConfig
{
    float MinimumTimeStep = 0.0016f;     // �ּ� �ð� ���� (�� 600fps)
    float MaximumTimeStep = 0.0333f;     // �ִ� �ð� ���� (�� 30fps)
    int32_t MaxIterations = 8;           // �ִ� �ݺ� Ƚ��
    float CCDMotionThreshold = 1.0f;     // CCD Ȱ��ȭ �ӵ� �Ӱ谪
};