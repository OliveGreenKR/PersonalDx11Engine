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
	if (!bIsInitialized || !CollisionTree)
		return;

	for (const auto& Registered : RegisteredComponents)
	{
		CollisionTree->Remove(Registered.TreeNodeId);
	}
	ActiveCollisionPairs.clear();
	RegisteredComponents.clear();
}

void UCollisionManager::Initialize()
{
	if (bIsInitialized)
		return;

	try
	{
		Detector = nullptr;
		ResponseCalculator = nullptr;
		EventDispatcher = nullptr;
		CollisionTree = nullptr;

		Detector = new FCollisionDetector();
		ResponseCalculator = new FCollisionResponseCalculator();
		EventDispatcher = new FCollisionEventDispatcher();
		CollisionTree = new FDynamicAABBTree(Config.InitialCapacity);

		RegisteredComponents.reserve(Config.InitialCapacity);
		ActiveCollisionPairs.reserve(Config.InitialCapacity);

		bIsInitialized = true;
	}
	catch (...)
	{
		Release();
	}
}

void UCollisionManager::Release()
{
	if (!bIsInitialized)
		return;

	UnRegisterAll();

	delete CollisionTree;
	delete EventDispatcher;
	delete ResponseCalculator;
	delete Detector;

	CollisionTree = nullptr;
	EventDispatcher = nullptr;
	ResponseCalculator = nullptr;
	Detector = nullptr;

	bIsInitialized = false;
}

void UCollisionManager::CleanupDestroyedComponents()
{
	if (!bIsInitialized || RegisteredComponents.empty())
	{
		return;
	}

	// 1. 제거될 컴포넌트의 인덱스들을 수집
	std::vector<size_t> DestroyedIndices;
	for (size_t i = 0; i < RegisteredComponents.size(); ++i)
	{
		if (RegisteredComponents[i].Component->bDestroyed)
		{
			DestroyedIndices.push_back(i);
		}
	}

	if (DestroyedIndices.empty())
	{
		return;
	}

	// 2. AABB 트리에서 제거
	for (size_t Index : DestroyedIndices)
	{
		size_t TreeNodeId = RegisteredComponents[Index].TreeNodeId;
		CollisionTree->Remove(TreeNodeId);
	}

	// 3. 활성 충돌 쌍에서 제거
	for (auto it = ActiveCollisionPairs.begin(); it != ActiveCollisionPairs.end();)
	{
		const FCollisionPair& Pair = *it;
		// 페어의 둘 중 하나라도 제거될 인덱스에 포함되면 제거
		if (std::find(DestroyedIndices.begin(), DestroyedIndices.end(), Pair.IndexA) != DestroyedIndices.end() ||
			std::find(DestroyedIndices.begin(), DestroyedIndices.end(), Pair.IndexB) != DestroyedIndices.end())
		{
			it = ActiveCollisionPairs.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 4. RegisteredComponents 벡터에서 제거
	// 주의: 뒤에서부터 제거하여 인덱스 변화 최소화
	std::sort(DestroyedIndices.begin(), DestroyedIndices.end(), std::greater<size_t>());
	for (size_t Index : DestroyedIndices)
	{
		if (Index < RegisteredComponents.size())
		{
			// 마지막 요소와 교체 후 제거 (swap and pop)
			if (Index < RegisteredComponents.size() - 1)
			{
				std::swap(RegisteredComponents[Index], RegisteredComponents.back());

				// 교체된 컴포넌트의 새 인덱스에 대한 충돌 쌍 업데이트
				size_t OldIndex = RegisteredComponents.size() - 1;
				UpdateCollisionPairIndices(OldIndex, Index);
			}
			RegisteredComponents.pop_back();
		}
	}
}

void UCollisionManager::UpdateCollisionPairIndices(size_t OldIndex, size_t NewIndex)
{
	std::vector<FCollisionPair> UpdatedPairs;

	// 이전 인덱스를 사용하는 모든 쌍을 찾아 새 인덱스로 업데이트
	for (auto it = ActiveCollisionPairs.begin(); it != ActiveCollisionPairs.end();)
	{
		FCollisionPair CurrentPair = *it;
		bool bNeedsUpdate = false;

		if (CurrentPair.IndexA == OldIndex)
		{
			CurrentPair.IndexA = NewIndex;
			bNeedsUpdate = true;
		}
		if (CurrentPair.IndexB == OldIndex)
		{
			CurrentPair.IndexB = NewIndex;
			bNeedsUpdate = true;
		}

		if (bNeedsUpdate)
		{
			it = ActiveCollisionPairs.erase(it);
			UpdatedPairs.push_back(CurrentPair);
		}
		else
		{
			++it;
		}
	}

	// 업데이트된 쌍들을 다시 삽입
	for (const auto& Pair : UpdatedPairs)
	{
		ActiveCollisionPairs.insert(Pair);
	}
}

void UCollisionManager::UpdateCollisionPairs()
{

}
