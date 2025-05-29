#pragma once
struct Vector3;
struct FPhysicsParameters;

// 제약조건 기본 인터페이스
class IConstraint
{
public:
    virtual ~IConstraint() = default;

    // 제약조건 해결 (람다 축적)
    virtual Vector3 Solve(const FPhysicsParameters& BodyA,
                          const FPhysicsParameters& BodyB, float & InOutLambda) const = 0;

    // 충돌 지점 관련 데이터 설정
    virtual void SetContactData(const Vector3& ContactPoint,
                                const Vector3& ContactNormal) = 0;
};