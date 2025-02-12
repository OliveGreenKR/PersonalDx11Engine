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

	// 1. ���ŵ� ������Ʈ�� �ε������� ����
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

	// 2. AABB Ʈ������ ����
	for (size_t Index : DestroyedIndices)
	{
		size_t TreeNodeId = RegisteredComponents[Index].TreeNodeId;
		CollisionTree->Remove(TreeNodeId);
	}

	// 3. Ȱ�� �浹 �ֿ��� ����
	for (auto it = ActiveCollisionPairs.begin(); it != ActiveCollisionPairs.end();)
	{
		const FCollisionPair& Pair = *it;
		// ����� �� �� �ϳ��� ���ŵ� �ε����� ���ԵǸ� ����
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

	// 4. RegisteredComponents ���Ϳ��� ����
	// ����: �ڿ������� �����Ͽ� �ε��� ��ȭ �ּ�ȭ
	std::sort(DestroyedIndices.begin(), DestroyedIndices.end(), std::greater<size_t>());
	for (size_t Index : DestroyedIndices)
	{
		if (Index < RegisteredComponents.size())
		{
			// ������ ��ҿ� ��ü �� ���� (swap and pop)
			if (Index < RegisteredComponents.size() - 1)
			{
				std::swap(RegisteredComponents[Index], RegisteredComponents.back());

				// ��ü�� ������Ʈ�� �� �ε����� ���� �浹 �� ������Ʈ
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

	// ���� �ε����� ����ϴ� ��� ���� ã�� �� �ε����� ������Ʈ
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

	// ������Ʈ�� �ֵ��� �ٽ� ����
	for (const auto& Pair : UpdatedPairs)
	{
		ActiveCollisionPairs.insert(Pair);
	}
}

void UCollisionManager::UpdateCollisionPairs()
{

}
