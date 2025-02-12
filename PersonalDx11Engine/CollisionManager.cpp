#include "CollisionManager.h"
#include <algorithm>

UCollisionManager::~UCollisionManager()
{
	Release();
}

std::shared_ptr<UCollisionComponent> UCollisionManager::Create(
	const std::shared_ptr<URigidBodyComponent>& InRigidBody,
	const ECollisionShapeType& InType,
	const Vector3& InHalfExtents)
{
	if (!bIsInitialized || !InRigidBody)
	{
		return nullptr;
	}

	// �� ������Ʈ ����
	auto NewComponent = std::shared_ptr<UCollisionComponent>(
		new UCollisionComponent(InRigidBody, InType, InHalfExtents)
	);

	// AABB Ʈ���� ���
	size_t TreeNodeId = CollisionTree->Insert(NewComponent);
	if (TreeNodeId == FDynamicAABBTree::NULL_NODE)
	{
		return nullptr;
	}

	// ������Ʈ ������ ���� �� ���
	FComponentData ComponentData;
	ComponentData.Component = NewComponent;
	ComponentData.TreeNodeId = TreeNodeId;

	// ���Ϳ� �߰�
	RegisteredComponents.push_back(std::move(ComponentData));

	return NewComponent;
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
		CollisionTree = new FDynamicAABBTree(Config.InitialCapacity);
	}
	catch (...)
	{
		delete EventDispatcher;
		delete ResponseCalculator;
		delete Detector;
		delete CollisionTree;
		std::terminate();
	}

	RegisteredComponents.reserve(Config.InitialCapacity);
	ActiveCollisionPairs.reserve(Config.InitialCapacity);
	bIsInitialized = true;
}

void UCollisionManager::Release()
{
	if (CollisionTree)
	{
		delete CollisionTree;
	}
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

