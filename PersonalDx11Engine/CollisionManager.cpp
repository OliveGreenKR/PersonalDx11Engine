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

void UCollisionManager::CleanupDestroyedComponents()
{

	ActiveCollisionPairs.erase(std::remove_if(ActiveCollisionPairs.begin(),
											  ActiveCollisionPairs.end(),
											  [this](const FCollisionPair& pair)
											  {
												  return IsDestroyedComponent(pair.IndexA) ||
													  IsDestroyedComponent(pair.IndexB);
											  }));

	RegisteredComponents.erase(std::remove_if(RegisteredComponents.begin(),
											  RegisteredComponents.end(),
											  [](const UCollisionComponent* Comp)
											  {
												  assert(Comp);
												  return Comp->bDestroyed;
											  }));
}
