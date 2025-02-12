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

	//BraodPhas�� ���� ������ �浹 ���ɼ��� �ִ� �� ����
	//���� ������ �浹 ���� ���� ����
	UpdateCollisionPairs();

	//�� �浹�ֿ� ���� ProcessCollision����
	ProcessCollisions(DeltaTime);
		//��ü �ӵ��� ���� CCD �ʿ� �˻�
		//	��ü �ӵ��� ���� Detector�� timeStep ��ȭLerp(minstep,maxstep)
		//�浹 ���� ����
		//�浹 ����
		//�浹 �̺�Ʈ
		//���� ������Ʈ
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
			//��ü �ӵ� �˻�
			
			//�Ӱ�ӵ����� ������ CCD, �ƴϸ� DCD
			
			//�浹�� �˻�

			//�浹 ����

			//��� �̺�Ʈ ����ġ

			//�浹�ֵ� ����
		}
	}
}

void UCollisionManager::CleanupDestroyedComponents()
{

	// erase-remove ���� ����
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
