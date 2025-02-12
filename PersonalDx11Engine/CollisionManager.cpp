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

	// 새 컴포넌트 생성
	auto NewComponent = std::shared_ptr<UCollisionComponent>(
		new UCollisionComponent(InRigidBody, InType, InHalfExtents)
	);

	// AABB 트리에 등록
	size_t TreeNodeId = CollisionTree->Insert(NewComponent);
	if (TreeNodeId == FDynamicAABBTree::NULL_NODE)
	{
		return nullptr;
	}

	// 컴포넌트 데이터 생성 및 등록
	FComponentData ComponentData;
	ComponentData.Component = NewComponent;
	ComponentData.TreeNodeId = TreeNodeId;

	// 벡터에 추가
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

