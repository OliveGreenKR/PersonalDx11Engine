#include "CollisionManager.h"
#include <algorithm>

UCollisionManager::UCollisionManager()
{
	Initialize();
}

UCollisionManager::~UCollisionManager()
{
	Release();
}

void UCollisionManager::Tick(const float DeltaTime)
{
	if (!Config.bPhysicsSimulated)
		return;

	CleanupDestroyedComponents();

	//BraodPhas를 통해 잠재적 충돌 가능성이 있는 쌍 생성
	//이전 프레임 충돌 상태 정보 유지
	UpdateCollisionPairs();

	//각 충돌쌍에 대해 ProcessCollision실행
	ProcessCollisions(DeltaTime);
		//객체 속도를 통한 CCD 필요 검사
		//	객체 속도에 따라 Detector의 timeStep 변화Lerp(minstep,maxstep)
		//충돌 감지 실행
		//충돌 반응
		//충돌 이벤트
		//상태 업데이트
}

void UCollisionManager::UnRegisterAll()
{
	ActiveCollisionPairs.clear();
	RegisteredComponents.clear();
}

void UCollisionManager::Initialize()
{
	try
	{
		Detector = new FCollisionDetector();
		ResponseCalculator = new FCollisionResponseCalculator();
		EventDispatcher = new FCollisionEventDispatcher();
	}
	catch (...)
	{
		delete EventDispatcher;
		delete ResponseCalculator;
		delete Detector;
		std::terminate();
	}
}

void UCollisionManager::Release()
{
	if (EventDispatcher)
	{
		delete EventDispatcher;
	}
	if (ResponseCalculator)
	{
		delete ResponseCalculator;
	}
	if (Detector)
	{
		delete Detector;
	}
	UnRegisterAll();
}

void UCollisionManager::ProcessCollisions(const float DeltaTime)
{
	//TODO : BroadPhase

	//Narrow Phase
	for (size_t i = 0; i < RegisteredComponents.size(); ++i)
	{
		for (size_t j = i+1 ; j < RegisteredComponents.size(); ++j)
		{
			//객체 속도 검사
			
			//임계속도보다 빠르면 CCD, 아니면 DCD
			
			//충돌쌍 검사

			//충돌 반응

			//결과 이벤트 디스패치

			//충돌쌍들 저장
		}
	}
}

void UCollisionManager::CleanupDestroyedComponents()
{

	// erase-remove 구문 수정
	auto it = ActiveCollisionPairs.begin();
	while(it != ActiveCollisionPairs.end())
	{
		if (IsDestroyedComponent(it->IndexA) || IsDestroyedComponent(it->IndexB))
		{
			it = ActiveCollisionPairs.erase(it);
		}
		else
		{
			++it;
		}
	}

	RegisteredComponents.erase(std::remove_if(RegisteredComponents.begin(),
											  RegisteredComponents.end(),
											  [](const std::shared_ptr<UCollisionComponent>& Comp)
											  {
												  assert(Comp.get());
												  return Comp.get()->bDestroyed;
											  }));
}
