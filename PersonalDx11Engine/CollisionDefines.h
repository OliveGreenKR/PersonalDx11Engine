#pragma once
#pragma once
#include "Math.h"
#include <memory>
#include "Transform.h"

// 충돌 감지 결과
struct FCollisionDetectionResult
{
	bool bCollided = false;
	XMVECTOR Normal = XMVectorSet(0, 0, 0, 0);     // 충돌 법선
	XMVECTOR Point = XMVectorSet(0, 0, 0, 0);    // 충돌 지점
	float PenetrationDepth = 0.0f;       // 침투 깊이
	float TimeOfImpact = 0.0f;           // 정규화된 충돌 시점 [0,1] == [이전프레임,현재프레임]
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
	XMVECTOR NetImpulse = XMVectorSet(0, 0, 0, 0); // 모든 물리적 효과를 통합한 최종 충격량
	XMVECTOR ApplicationPoint = XMVectorSet(0, 0, 0, 0);
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

// 충돌체 형태 정의
enum class ECollisionShapeType
{
	None,
	Box,
	Sphere
};
