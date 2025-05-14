#pragma once
#pragma once
#include "Math.h"
#include <memory>
#include "Transform.h"

// 충돌 감지 결과
struct FCollisionDetectionResult
{
	bool bCollided = false;
	Vector3 Normal = Vector3::Zero();      // 충돌 법선
	Vector3 Point = Vector3::Zero();       // 충돌 지점
	float PenetrationDepth = 0.0f;       // 침투 깊이
	float TimeOfImpact = 1.0f;           // 정규화된 충돌 시점 [0,1] == [이전프레임,현재프레임]
};


// 충돌 반응 계산 에 필요한 물리 속성 정보
struct FPhysicsParameters
{
	//if negative mass, it means invalied params
	float Mass = -1.0f;  

	XMVECTOR RotationalInertia = XMVectorZero();
	XMVECTOR Position = XMVectorZero();
	XMVECTOR Velocity = XMVectorZero();
	XMVECTOR AngularVelocity = XMVectorZero();
	XMVECTOR Rotation = XMQuaternionIdentity();

	float Restitution = 0.5f; // 반발계수
	float FrictionStatic = 0.8f;
	float FrictionKinetic = 0.5f;
};

//제한조건 충돌 검사 람다 누적
struct FAccumulatedConstraint
{
	float normalLambda = 0.0f;
	float frictionLambda = 0.0f;

	inline void Scale(const float InScale)
	{
		normalLambda *= InScale;
		frictionLambda *= InScale;
	}
};

struct FCollisionResponseResult
{
	Vector3 NetImpulse = Vector3::Zero(); // 모든 물리적 효과를 통합한 최종 충격량
	Vector3 ApplicationPoint = Vector3::Zero();
};

// 단일 충돌쌍 충돌 이벤트 정보, 충돌 결과 델리게이트 전파
struct FCollisionEventData
{
	std::weak_ptr<class UCollisionComponentBase> OtherComponent;
	FCollisionDetectionResult CollisionDetectResult;
};

enum class ECollisionState
{
	None,
	Enter,
	Stay,
	Exit,
};