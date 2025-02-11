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

	ProcessCollisions(DeltaTime);
}

void UCollisionManager::UnRegisterAll()
{
	ActiveCollisionPairs.clear();
	RegisteredComponents.clear();
}

void UCollisionManager::Initialize()
{
	Detector = new FCollisionDetector();
	ResponseCalculator = new FCollisionResponseCalculator();
	EventDispatcher = new FCollisionEventDispatcher();
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
