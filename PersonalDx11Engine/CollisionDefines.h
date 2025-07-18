#pragma once
#pragma once
#include "Math.h"
#include <memory>
#include "Transform.h"

using PhysicsID = size_t;

// 충돌 감지 결과
struct FCollisionDetectionResult
{
	bool bCollided = false;
	XMVECTOR Normal = XMVectorSet(0, 0, 0, 0);     // 충돌 법선
	XMVECTOR Point = XMVectorSet(0, 0, 0, 0);    // 충돌 지점
	float PenetrationDepth = 0.0f;       // 침투 깊이
	float NormalizedToI = 0.0f;           // 정규화된 충돌 시점 [0,1] == [이전프레임,현재프레임]
};

//충돌 형상 정보
struct FCollisionShapeData
{
	ECollisionShapeType ShapeType = ECollisionShapeType::Box;
	XMVECTOR HalfExtent = XMVectorSet(0, 0, 0, 0);

	XMVECTOR CurrentWorldPosition = XMVectorZero();
	XMVECTOR CurrentWorldRotation = XMQuaternionIdentity();

	XMVECTOR PrevWorldPosition = XMVectorZero();
	XMVECTOR PrevWorldRotation = XMQuaternionIdentity();
};


// 충돌 반응 계산 에 필요한 물리 속성 정보
struct FPhysicsParameters
{
	//if negative mass, it means invalied params
	float InvMass = -1.0f;  

	XMVECTOR InvRotationalInertia = XMVectorZero();
	XMVECTOR Position = XMVectorZero();
	XMVECTOR Velocity = XMVectorZero();
	XMVECTOR AngularVelocity = XMVectorZero();
	XMVECTOR Rotation = XMQuaternionIdentity();

	float Restitution = 0.5f; // 반발계수
	float FrictionStatic = 0.8f;
	float FrictionKinetic = 0.5f;
};

//제한조건 충돌 검사 람다 누적
struct FCollisionAccumulation
{
	float NormalLambda = 0.0f;
	float FrictionLambda = 0.0f;
	float TwistLambda = 0.0f;

	inline void Scale(const float InScale)
	{
		NormalLambda *= InScale;
		FrictionLambda *= InScale;
		TwistLambda *= InScale;
	}

	static bool IsEqual(const FCollisionAccumulation& AccumA, const FCollisionAccumulation& AccumB ,const float Epsilon = KINDA_SMALL)
	{
		return std::fabs(AccumA.NormalLambda - AccumB.NormalLambda) < Epsilon; 
	}

	inline void ApplyWarmStartingDamping(float DampingFactor = 0.8f)
	{
		NormalLambda *= DampingFactor;
		FrictionLambda *= DampingFactor;
		TwistLambda *= DampingFactor;
	}

	inline void Reset()
	{
		NormalLambda = 0.0f;
		FrictionLambda = 0.0f;
		TwistLambda = 0.0f;
	}
};

// 충돌 반응 결과
struct FCollisionResponseResult
{
	XMVECTOR NetImpulse = XMVectorSet(0, 0, 0, 0); // 모든 물리적 효과를 통합한 최종 충격량
	XMVECTOR ApplicationPoint = XMVectorSet(0, 0, 0, 0);
};

// 물리 시스템 내부용 충돌 이벤트 (PhysicsID 기반)
struct FPhysicsCollisionEvent
{
	ECollisionState CollisionState = ECollisionState::None;
	PhysicsID PhysicsIdA = 0;
	PhysicsID PhysicsIdB = 0;

	XMVECTOR CollisionPoint = XMVectorZero();     // 충돌 지점 (SIMD)
	XMVECTOR Normal = XMVectorZero();             // 충돌 법선 (SIMD)
	float PenetrationDepth = 0.0f;               // 침투 깊이
	float NormalizedToI = 0.0f;                   // 충돌 시간

	bool operator==(const FPhysicsCollisionEvent& Other) const
	{
		return (PhysicsIdA == Other.PhysicsIdA && PhysicsIdB == Other.PhysicsIdB && CollisionState == Other.CollisionState) ||
			(PhysicsIdA == Other.PhysicsIdB && PhysicsIdB == Other.PhysicsIdA && CollisionState == Other.CollisionState) ;
	}
};

enum class ECollisionState
{
	None,
	Enter,
	Stay,
	Exit,
};

// 충돌체 형태 정의
enum class ECollisionShapeType
{
	None,
	Box,
	Sphere
};
