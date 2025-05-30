#include "CollisionResponseCalculator.h"
#include "VelocityConstraint.h"
#include "PhysicsStateInterface.h"
#include "Debug.h"
#include "PhysicsDefine.h"

Vector3 FCollisionResponseCalculator::CalculateNormalImpulse(const FCollisionDetectionResult& DetectionResult,
                                                             const FPhysicsParameters& ParameterA, const FPhysicsParameters& ParameterB,
                                                             float& InOutLambda, float BiasSpeed) const
{
    if (!DetectionResult.bCollided)
        return Vector3::Zero();

    Vector3 Normal = DetectionResult.Normal;

    FVelocityConstraint NormalConstraint(Normal, 0.0f, 0.0f);
    NormalConstraint.SetContactData(DetectionResult.Point, Normal);

    // 반발 계수 적용 (두 물체의 평균)
    float RestitutionCoef = std::min(0.9999f, (ParameterA.Restitution + ParameterB.Restitution) * 0.5f);

    // 상대 속도 계산
    XMVECTOR vRelativeVel = NormalConstraint.CalculateRelativeVelocity(
        ParameterA, ParameterB, XMLoadFloat3(&DetectionResult.Point));
    XMVECTOR vNormal = XMLoadFloat3(&Normal);
    float NormalVelocity = XMVectorGetX(XMVector3Dot(vRelativeVel, vNormal));

    // 속도가 임계값 이하면 반발 무시 (정지 상태 유지)
    const float VelocityThreshold = KINDA_SMALL;
    if (std::abs(NormalVelocity) < VelocityThreshold)
        RestitutionCoef = 0.0f;

    // 반발 속도 목표 설정
    float DesiredVelocity = NormalVelocity < 0.0f ? -NormalVelocity * RestitutionCoef : 0.0f;

    // 편향 속력 추가
    DesiredVelocity += BiasSpeed;
    NormalConstraint.SetDesiredSpeed(DesiredVelocity);

    // 제약 조건 해결 (충격량 계산)
    Vector3 NormalImpulse = NormalConstraint.Solve(ParameterA, ParameterB, InOutLambda);

    return NormalImpulse;
}

Vector3 FCollisionResponseCalculator::CalculateFrictionImpulse(const FCollisionDetectionResult& DetectionResult,
                                                               const FPhysicsParameters& ParameterA, const FPhysicsParameters& ParameterB,
                                                               float NormalLambda, float& InOutFrictionLambda) const
{
    // 법선 방향 성분 제거하여 접선 방향 상대 속도 계산
    XMVECTOR vRelativeVel = FVelocityConstraint::CalculateRelativeVelocity(
        ParameterA, ParameterB, XMLoadFloat3(&DetectionResult.Point));

    XMVECTOR vNormal = XMLoadFloat3(&DetectionResult.Normal);
    float NormalVelocity = XMVectorGetX(XMVector3Dot(vRelativeVel, vNormal));

    XMVECTOR vNormalComponent = XMVectorScale(vNormal, NormalVelocity);
    XMVECTOR vTangentVel = XMVectorSubtract(vRelativeVel, vNormalComponent);

    float TangentLength = XMVectorGetX(XMVector3Length(vTangentVel));
    Vector3 TangentImpulse = Vector3::Zero();

    if (TangentLength > KINDA_SMALL)
    {
        // 접선 방향 정규화
        XMVECTOR vTangent = XMVectorDivide(vTangentVel, XMVectorReplicate(TangentLength));
        Vector3 Tangent;
        XMStoreFloat3(&Tangent, vTangent);

        // 접선 방향 제약 조건 (마찰력)
        // 마찰은 속도를 0으로 만드는 것이 목표이므로 DesiredSpeed는 0.0f
        FVelocityConstraint FrictionConstraint(Tangent, 0.0f);
        FrictionConstraint.SetContactData(DetectionResult.Point, DetectionResult.Normal);

        // Warm Starting: 이전 프레임의 마찰 람다 재사용
        TangentImpulse = FrictionConstraint.Solve(ParameterA, ParameterB, InOutFrictionLambda);

        // 마찰력 크기 제한 (Coulomb 마찰 모델)
        float StaticFriction = (ParameterA.FrictionStatic + ParameterB.FrictionStatic) * 0.5f;
        float KineticFriction = (ParameterA.FrictionKinetic + ParameterB.FrictionKinetic) * 0.5f;

        // 마찰 클램핑
        ClampFriction(TangentLength, NormalLambda, StaticFriction, KineticFriction,
                      InOutFrictionLambda, TangentImpulse);
    }

    return TangentImpulse;
}

void FCollisionResponseCalculator::ClampFriction(const float TangentRelativeVelocityLength,
                                                 const float NormalLambda,
                                                 const float StaticFriction,
                                                 const float KineticFriction,
                                                 float& OutFrictionLambda,
                                                 Vector3& OutTangentImpulse) const
{
    // 정적 마찰과 운동 마찰 적용 (Coulomb Friction Model)
    float maxFrictionImpulse = NormalLambda; // 람다는 이미 스케일링된 값이라고 가정 (충격량 크기)

    if (TangentRelativeVelocityLength < KINDA_SMALL) // 상대 속도가 매우 작으면 정적 마찰 적용
    {
        maxFrictionImpulse *= StaticFriction;
    }
    else // 움직이고 있다면 운동 마찰 적용
    {
        maxFrictionImpulse *= KineticFriction;
    }

    // 마찰 람다를 계산된 최대 임펄스 값으로 클램핑
    OutFrictionLambda = Math::Clamp(OutFrictionLambda, -maxFrictionImpulse, maxFrictionImpulse);
    // 마찰 임펄스도 클램프
    float currentTangentImpulseMagnitude = OutTangentImpulse.Length();
    if (currentTangentImpulseMagnitude > maxFrictionImpulse)
    {
        OutTangentImpulse = OutTangentImpulse * (maxFrictionImpulse / currentTangentImpulseMagnitude);
    }
}