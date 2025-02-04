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
    Vector3 HalfExtent = Vector3::Zero;  // Box용 - Sphere는 x값만 사용
};

// 개별 충돌체 정보
struct FCollisionTestData
{
    FCollisionShapeData ShapeData;
    FTransform Transform;
};

// 연속 충돌 검사용 데이터
struct FCollisionSweptTestData
{
    FCollisionShapeData ShapeData;
    FTransform PrevTransform;
    FTransform CurrentTransform;
};

// 충돌 쌍 데이터
struct FCollisionPairData
{
    FCollisionTestData A;
    FCollisionTestData B;
    float TimeStep = 0.0f;
};

// 연속 충돌 쌍 데이터 
struct FCollisionSweptPairData
{
    FCollisionSweptTestData A;
    FCollisionSweptTestData B;
    float TimeStep = 0.0f;
};

// 충돌 감지 결과
struct FCollisionDetectionResult
{
    bool bCollided = false;
    Vector3 Normal = Vector3::Zero;      // 충돌 법선
    Vector3 Point = Vector3::Zero;       // 충돌 지점
    float PenetrationDepth = 0.0f;       // 침투 깊이
    float TimeOfImpact = 0.0f;          // CCD용 충돌 시점
};

// 충돌 이벤트 정보 (컴포넌트의 델리게이트에서 사용)
struct FCollisionEventData
{
    std::weak_ptr<class UCollisionComponent> OtherComponent;
    FCollisionDetectionResult CollisionResult;
    float TimeStep;
};

// 디버그 시각화 설정
struct FCollisionDebugParams
{
    bool bShowCollisionShapes = false;
    bool bShowCollisionEvents = false;
    Vector4 ShapeColor = { 0.0f, 1.0f, 0.0f, 0.5f };  // 반투명 녹색
};

// 시스템 설정
struct FCollisionSystemConfig
{
    float MinimumTimeStep = 0.0016f;     // 최소 시간 간격 (약 600fps)
    float MaximumTimeStep = 0.0333f;     // 최대 시간 간격 (약 30fps)
    int32_t MaxIterations = 8;           // 최대 반복 횟수
    float CCDMotionThreshold = 1.0f;     // CCD 활성화 속도 임계값
    FCollisionDebugParams DebugParams;
};