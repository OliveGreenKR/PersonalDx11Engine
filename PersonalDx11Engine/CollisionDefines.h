#pragma once
#pragma once
#include "Math.h"
#include <memory>
#include "Transform.h"

// 충돌체 형태 정의
enum class ECollisionShapeType
{
    Box,
    Sphere
};

// 충돌체 기하 정보
struct FCollisionShapeData
{
    inline float GetSphereRadius() const {
        assert(Type == ECollisionShapeType::Sphere);
        return HalfExtent.x;  // 구는 x 성분만 사용
    }

    inline const Vector3& GetBoxHalfExtents() const {
        assert(Type == ECollisionShapeType::Box);
        return HalfExtent;    // 박스는 전체 벡터 사용
    }

    ECollisionShapeType Type = ECollisionShapeType::Box;
    Vector3 HalfExtent = Vector3::Zero;  //Sphere는 x값만 사용
};

// 충돌 감지 결과
struct FCollisionDetectionResult
{
    bool bCollided = false;
    Vector3 Normal = Vector3::Zero;      // 충돌 법선
    Vector3 Point = Vector3::Zero;       // 충돌 지점
    float PenetrationDepth = 0.0f;       // 침투 깊이
    float TimeOfImpact = 0.0f;           // CCD용 충돌 시점
};


// 충돌 응답 계산을 위한 매개변수, 충돌 반응에 필요한 물리 속성 정보
struct FCollisionResponseParameters
{
    float Mass = 0.0f;
    float RotationalInertia = 0.0f;
    
    Vector3 Position = Vector3::Zero;
    Vector3 Velocity = Vector3::Zero;
    Vector3 AngularVelocity = Vector3::Zero;

    float Restitution = 0.5f; // 반발계수
    float FrictionStatic = 0.8f;
    float FrictionKinetic = 0.5f;
};

struct FCollisionResponseResult
{
    Vector3 NetImpulse = Vector3::Zero; // 모든 물리적 효과를 통합한 최종 충격량
    Vector3 ApplicationPoint = Vector3::Zero;
};
//충돌 컴포넌트 쌍 구조체
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

// std 해시 함수 특수화 
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

// 충돌 이벤트 정보 (컴포넌트의 델리게이트에서 사용)
struct FCollisionEventData
{
    std::weak_ptr<class UCollisionComponent> OtherComponent;
    FCollisionDetectionResult CollisionResult;
    float TimeStep;
};

// 시스템 설정
struct FCollisionSystemConfig
{
    float MinimumTimeStep = 0.0016f;     // 최소 시간 간격 (약 600fps)
    float MaximumTimeStep = 0.0333f;     // 최대 시간 간격 (약 30fps)
    int32_t MaxIterations = 8;           // 최대 반복 횟수
    float CCDMotionThreshold = 1.0f;     // CCD 활성화 속도 임계값
};