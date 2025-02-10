#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include "CollisionDefines.h"
#include "CollisionComponent.h"

class UCollisionComponent;

//충돌 매니저가 컴포넌트관리할때 사용할 구조체
struct FCollisionComponentState
{
	std::shared_ptr<UCollisionComponent> Component;
	std::vector<std::weak_ptr<UCollisionComponent>> CurrentCollisions;  // 현재 프레임의 충돌 대상들
	std::vector<std::weak_ptr<UCollisionComponent>> PreviousCollisions; // 이전 프레임의 충돌 대상들

	// CCD를 위한 추가 정보
	FTransform PreviousTransform;
	FTransform CurrentTransform;
};

//todo 서브 시스템 구현
class FCollisionDectection;
class FCollisionResponse;
class FCollisionEvent;


class UCollisionManager
{
	//모든 collision component들의 생명 주기를 관리하고

	/*
	* 모든 Collision Component들의 생성 및 소멸을 관리.
	* 모든 콜리전 컴포의 순서쌍과의 충돌 검사 및 충돌 반응을 총괄.
	* 
	* 3가지 서브 시스템으로 나누어서 구성
	* 
	* - Collision Detection System : 충돌 탐지
	* - Collision Response System: 
			충돌 반응을 계산하여 해당 컴포넌트의 상위 리지드 바디에게 힘의 형태로 전달
	* - Collision Event System : 충돌 이벤트 발송
	* 
	* **Broad Phase는 비구현 예정.
	*/

private :
	UCollisionManager() = default;
public:
	~UCollisionManager() { Release(); }
	
	// 복사 및 이동 방지
	UCollisionManager(const UCollisionManager&) = delete;
	UCollisionManager& operator=(const UCollisionManager&) = delete;
	UCollisionManager(UCollisionManager&&) = delete;
	UCollisionManager& operator=(UCollisionManager&&) = delete;

	static UCollisionManager* Get()
	{
		static std::unique_ptr<UCollisionManager> Instance
			= std::unique_ptr<UCollisionManager>(new UCollisionManager());

		return Instance.get();

	}
	void Tick(const float DeltaTime);

	static UCollisionManager* CreateCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody);
	UCollisionManager* GetCollisionByIndex(const unsigned int InIndex) const;

	void Release();

private:
	void DeleteCollisionByIndex(const unsigned itn);


private:
	std::vector<std::shared_ptr<UCollisionManager>> Collisions;

	//std::unique_ptr<class FCollisionDetection> Detector;
	//std::unique_ptr<class FCollisionResponse> ResponseCalculator;
	//std::unique_ptr<class FCollisionEventPublisher> EventPublisher;

};

