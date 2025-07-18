#include "CollisionEventCalculator.h"
#include "Debug.h"

using namespace DirectX;

#pragma region Core Event Generation

FPhysicsCollisionEvent FCollisionEventCalculator::GenerateCollisionEvent(
    const FCollisionDetectionResult& DetectResult,
    bool bPrevCollided,
    PhysicsID PhysicsIdA,
    PhysicsID PhysicsIdB,
    float deltaTime)
{
    FPhysicsCollisionEvent Event;

    //상태 결정
    Event.CollisionState = DetermineCollisionState(DetectResult.bCollided, bPrevCollided);

    // 기본 ID 설정
    Event.PhysicsIdA = PhysicsIdA;
    Event.PhysicsIdB = PhysicsIdB;

    // 충돌 데이터 복사 (DetectResult → Event)
    Event.CollisionPoint = DetectResult.Point;
    Event.Normal = DetectResult.Normal;
    Event.PenetrationDepth = DetectResult.PenetrationDepth;
    Event.TimeOfImpact = DetectResult.NormalizedToI * deltaTime;

    return Event;
}

FPhysicsCollisionEvent FCollisionEventCalculator::GenerateExitEvent(
    PhysicsID PhysicsIdA,
    PhysicsID PhysicsIdB)
{
    FPhysicsCollisionEvent Event;

    Event.CollisionState = ECollisionState::Exit;

    // 기본 ID 설정
    Event.PhysicsIdA = PhysicsIdA;
    Event.PhysicsIdB = PhysicsIdB;

    // Exit 이벤트는 충돌 데이터가 없음
    Event.CollisionPoint = XMVectorZero();
    Event.Normal = XMVectorZero();
    Event.PenetrationDepth = 0.0f;
    Event.TimeOfImpact = 0.0f;

    return Event;
}

#pragma endregion

#pragma region Event State Determination

ECollisionState FCollisionEventCalculator::DetermineCollisionState(
    bool bCurrentlyColliding,
    bool bPreviouslyColliding)
{
    if (bCurrentlyColliding && !bPreviouslyColliding)
    {
        return ECollisionState::Enter;
    }
    else if (bCurrentlyColliding && bPreviouslyColliding)
    {
        return ECollisionState::Stay;
    }
    else if (!bCurrentlyColliding && bPreviouslyColliding)
    {
        return ECollisionState::Exit;
    }
    else
    {
        return ECollisionState::None;
    }
}

#pragma endregion