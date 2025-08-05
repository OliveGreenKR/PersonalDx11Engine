#pragma once
#pragma once
#include "Math.h"
#include <memory>
#include "Transform.h"
#include <DirectXMath.h>
using PhysicsID = std::uint32_t;

#pragma warning(disable: 4996)

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

/// <summary>
/// 게임 로직에서 사용하는 충돌 이벤트 데이터
/// 물리 시스템의 FPhysicsCollisionEvent를 게임 좌표계로 변환한 결과
/// Vector3 좌표계 및 게임 단위 시스템 적용
/// </summary>
struct FCollisionEvent
{
	ECollisionState CollisionState = ECollisionState::None;

	/// <summary>
	/// 현재 미구현 추후 게임 시스템 객체 매니저를 추가해 로직 단위에서 PhyscisId를 통한 빠른 검색이 가능하도록 구현 예정
	/// </summary>
	[[deprecated("Do not use this member. It will be removed in future versions.")]]
	class UCollisionComponentBase* Other = nullptr;

	Vector3 CollisionPoint = Vector3::Zero();
	Vector3 Normal = Vector3::Zero();
	float PenetrationDepth = 0.0f;

	/// <summary>
	/// 충돌 이벤트 비교 연산자
	/// 동일한 충돌인지 판단하기 위한 비교
	/// </summary>
	bool operator==(const FCollisionEvent& OtherEvent) const
	{
		return OtherEvent.Other == Other &&
			OtherEvent.Normal == Normal &&
			OtherEvent.CollisionPoint == CollisionPoint;
	}
};

