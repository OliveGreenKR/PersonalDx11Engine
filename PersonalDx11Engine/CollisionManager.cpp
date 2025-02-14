#include "CollisionManager.h"
#include "RigidBodyComponent.h"
#include "Transform.h"
#include <algorithm>
#include "DynamicAABBTree.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventDispatcher.h"

UCollisionManager::~UCollisionManager()
{
	Release();
}

void UCollisionManager::RegisterCollision(std::shared_ptr<UCollisionComponent>& NewComponent, const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	if (!bIsInitialized || !InRigidBody)
	{
		return;
	}

	InRigidBody->AddChild(NewComponent);

	RegisterCollision(NewComponent);

}

void UCollisionManager::RegisterCollision(std::shared_ptr<UCollisionComponent>& NewComponent)
{
	if (!bIsInitialized)
	{
		return;
	}

	// AABB 트리에 등록
	size_t TreeNodeId = CollisionTree->Insert(NewComponent);
	if (TreeNodeId == FDynamicAABBTree::NULL_NODE)
	{
		return;
	}

	// 컴포넌트 데이터 생성 및 등록
	FComponentData ComponentData;
	ComponentData.Component = NewComponent;
	ComponentData.TreeNodeId = TreeNodeId;

	// 벡터에 추가
	RegisteredComponents.push_back(std::move(ComponentData));
}

void UCollisionManager::Tick(const float DeltaTime)
{
	UpdateCollisionTransform();
	UpdateCollisionPairs();
	ProcessCollisions(DeltaTime);
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
	if (!bIsInitialized || !CollisionTree || RegisteredComponents.empty())
	{
		ActiveCollisionPairs.clear();
		return;
	}

	// 새로운 충돌 쌍을 저장할 임시 컨테이너
	std::unordered_set<FCollisionPair> NewCollisionPairs;
	// TODO :: 초기 예약 크기 계산
	size_t EstimatedPairs = std::min(ActiveCollisionPairs.size(),
									 RegisteredComponents.size() * (RegisteredComponents.size() - 1) / 2);
	NewCollisionPairs.reserve(EstimatedPairs);

	for (size_t i = 0; i < RegisteredComponents.size(); ++i)
	{
		const auto& ComponentData = RegisteredComponents[i];
		auto* Component = ComponentData.Component.get();

		// nullptr 체크를 먼저하여 불필요한 멤버 접근 방지
		if (!Component || Component->bDestroyed || !Component->bCollisionEnabled)
		{
			continue;
		}

		// 트리 노드 ID 유효성 검사 추가
		if (ComponentData.TreeNodeId == FDynamicAABBTree::NULL_NODE)
		{
			continue;
		}

		const FDynamicAABBTree::AABB& TargetFatBounds = CollisionTree->GetFatBounds(ComponentData.TreeNodeId);

		CollisionTree->QueryOverlap(
			TargetFatBounds,
			[this, i, &NewCollisionPairs](size_t OtherNodeId) {
				if (OtherNodeId == FDynamicAABBTree::NULL_NODE ||
					RegisteredComponents[i].TreeNodeId >= OtherNodeId)
				{
					return;
				}

				size_t OtherIndex = FindComponentIndex(OtherNodeId);
				if (OtherIndex == SIZE_MAX)
				{
					return;
				}

				const auto& OtherComponent = RegisteredComponents[OtherIndex].Component;
				if (!OtherComponent || OtherComponent->bDestroyed ||
					!OtherComponent->bCollisionEnabled)
				{
					return;
				}

				NewCollisionPairs.insert(FCollisionPair(i, OtherIndex));
			});
	}

	ActiveCollisionPairs = std::move(NewCollisionPairs);
}

void UCollisionManager::UpdateCollisionTransform()
{
	for (size_t i = 0; i < RegisteredComponents.size(); ++i)
	{
		const auto& ComponentData = RegisteredComponents[i];
		auto* Component = ComponentData.Component.get();

		// nullptr 체크를 먼저하여 불필요한 멤버 접근 방지
		if (!Component || Component->bDestroyed || !Component->bCollisionEnabled || Component->GetRigidBody()->IsStatic())
		{
			continue;
		}

		// 트리 노드 ID 유효성 검사 추가
		if (ComponentData.TreeNodeId == FDynamicAABBTree::NULL_NODE)
		{
			continue;
		}

	}

	CollisionTree->UpdateTree();
}

bool UCollisionManager::ShouldUseCCD(const URigidBodyComponent* RigidBody) const
{
	if (!RigidBody)
		return false;
	return RigidBody->GetVelocity().Length() > Config.CCDVelocityThreshold;
}

FCollisionDetectionResult UCollisionManager::DetectCCDCollision(const FCollisionPair& InPair, const float DeltaTime)
{
	FCollisionDetectionResult DectectionResult;

	FComponentData& CompAData = RegisteredComponents[InPair.IndexA];
	FComponentData& CompBData = RegisteredComponents[InPair.IndexB];

	auto CompA = CompAData.Component.get();
	auto CompB = CompBData.Component.get();
	if (!CompA || !CompB)
		return DectectionResult;

	size_t NodeIdA = CompAData.TreeNodeId;
	size_t NodeIdB = CompBData.TreeNodeId;

	DectectionResult = Detector->DetectCollisionCCD(CompA->GetCollisionShape(), CompA->GetPreviousTransform(), *CompA->GetTransform(),
								 CompB->GetCollisionShape(), CompB->GetPreviousTransform(), *CompB->GetTransform(), DeltaTime);

	return DectectionResult;
}


UCollisionComponent* UCollisionManager::FindComponentByTreeNodeId(size_t TreeNodeId) const
{
	for (const auto& Data : RegisteredComponents)
	{
		if (Data.TreeNodeId == TreeNodeId)
		{
			return Data.Component.get();
		}
	}
	return nullptr;
}

size_t UCollisionManager::FindComponentIndex(size_t TreeNodeId) const
{
	for (size_t i = 0; i < RegisteredComponents.size(); ++i)
	{
		if (RegisteredComponents[i].TreeNodeId == TreeNodeId)
		{
			return i;
		}
	}
	return SIZE_MAX;
}

//----------------------------not imple yet
void UCollisionManager::ProcessCollisions(const float DeltaTime)
{
	for (auto ActivePair : ActiveCollisionPairs)
	{
		auto CompA = FindComponentByTreeNodeId(ActivePair.IndexA);
		auto CompB = FindComponentByTreeNodeId(ActivePair.IndexB);

		FCollisionDetectionResult detectResult;

		if (CompA && CompB)
		{
			if (ShouldUseCCD(CompA->GetRigidBody()) || ShouldUseCCD(CompB->GetRigidBody()))
			{
				//ccd
				detectResult = Detector->DetectCollisionCCD(CompA->GetCollisionShape(), CompA->GetPreviousTransform(), *CompA->GetTransform(),
															CompB->GetCollisionShape(), CompB->GetPreviousTransform(), *CompB->GetTransform(), DeltaTime);
			}
			else
			{
				//dcd
				detectResult = Detector->DetectCollisionDiscrete(CompA->GetCollisionShape(),*CompA->GetTransform(),
																 CompB->GetCollisionShape(),*CompB->GetTransform());
			}
		}

		if (!detectResult.bCollided)
			return;


		//TODO : ColliisionResponse

		//TODO : RecordPrevCollisionState
		FCollisionEventData EventData;
		EventData.CollisionDetectResult = detectResult;
		EventDispatcher->DispatchCollisionEvents(CompA, EventData, ECollisionState::Enter);
		EventDispatcher->DispatchCollisionEvents(CompB, EventData, ECollisionState::Enter);
	}
}

FCollisionDetectionResult UCollisionManager::DetectDCDCollision(const FCollisionPair& InPair, const float DeltaTime)
{
	return FCollisionDetectionResult();
}

void UCollisionManager::HandleCollision(const std::shared_ptr<UCollisionComponent>& ComponentA, const std::shared_ptr<UCollisionComponent>& ComponentB, const FCollisionDetectionResult& DetectionResult, const float DeltaTime)
{
}

void UCollisionManager::ApplyCollisionResponse(const std::shared_ptr<UCollisionComponent>& ComponentA, const std::shared_ptr<UCollisionComponent>& ComponentB, const FCollisionDetectionResult& DetectionResult)
{
}

void UCollisionManager::BroadcastCollisionEvents(const std::shared_ptr<UCollisionComponent>& ComponentA, const std::shared_ptr<UCollisionComponent>& ComponentB, const FCollisionDetectionResult& DetectionResult, const float DeltaTime)
{
}
