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
#include "DebugDrawManager.h"

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
		if (!Component || Component->bDestroyed || !Component->GetCollisionEnabled())
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
					!OtherComponent->GetCollisionEnabled())
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
		if (!Component || Component->bDestroyed || !Component->GetCollisionEnabled() || Component->GetRigidBody()->IsStatic())
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

void UCollisionManager::ProcessCollisions(const float DeltaTime)
{
	for (const auto& ActivePair : ActiveCollisionPairs)
	{
		auto& CompAData = RegisteredComponents[ActivePair.IndexA];
		auto& CompBData = RegisteredComponents[ActivePair.IndexB];

		auto CompA = CompAData.Component;
		auto CompB = CompBData.Component;

		const float PersistentThreshold = 0.1f;

		//Collision detection
		FCollisionDetectionResult DetectResult;
		if (CompA && CompB)
		{
			if (ShouldUseCCD(CompA->GetRigidBody()) || ShouldUseCCD(CompB->GetRigidBody()))
			{
				//ccd
				DetectResult = Detector->DetectCollisionCCD(CompA->GetCollisionShape(), CompA->GetPreviousTransform(), *CompA->GetTransform(),
															CompB->GetCollisionShape(), CompB->GetPreviousTransform(), *CompB->GetTransform(), DeltaTime);
			}
			else
			{
				//dcd
				DetectResult = Detector->DetectCollisionDiscrete(CompA->GetCollisionShape(), *CompA->GetTransform(),
																 CompB->GetCollisionShape(), *CompB->GetTransform());
			}
		}

		//response
		ApplyCollisionResponseByContraints(CompA, CompB, DetectResult);
		//position correction

		//dispatch event

	}
	
}



void UCollisionManager::GetCollisionDetectionParams(const std::shared_ptr<UCollisionComponent>& InComp, FCollisionResponseParameters& DetectResult ) const
{
	auto CompPtr = InComp.get();
	if (!CompPtr)
		return;
	auto RigidPtr = CompPtr->GetRigidBody();
	if (!RigidPtr)
		return;

	DetectResult.Mass = RigidPtr->GetMass();
	DetectResult.RotationalInertia = RigidPtr->GetRotationalInertia();
	DetectResult.Position = RigidPtr->GetTransform()->GetPosition();
	DetectResult.Velocity = RigidPtr->GetVelocity();
	DetectResult.AngularVelocity = RigidPtr->GetAngularVelocity();
	DetectResult.Restitution = RigidPtr->GetRestitution();
	DetectResult.FrictionKinetic = RigidPtr->GetFrictionKinetic();
	DetectResult.FrictionStatic = RigidPtr->GetFrictionStatic();
	DetectResult.Rotation = RigidPtr->GetTransform()->GetRotation();

	return;
}

void UCollisionManager::ApplyCollisionResponseByImpulse(const std::shared_ptr<UCollisionComponent>& ComponentA, const std::shared_ptr<UCollisionComponent>& ComponentB, const FCollisionDetectionResult& DetectResult)
{
	if (!ComponentA.get() || !ComponentA.get()->GetRigidBody() ||
		!ComponentB.get() || !ComponentB.get()->GetRigidBody())
		return;

	FCollisionResponseParameters ParamsA, ParamsB;
	GetCollisionDetectionParams(ComponentA, ParamsA);
	GetCollisionDetectionParams(ComponentB , ParamsB);
	FCollisionResponseResult collisionResponse = ResponseCalculator->CalculateResponseByImpulse(DetectResult, ParamsA, ParamsB);

	auto RigidPtrA = ComponentA.get()->GetRigidBody();
	auto RigidPtrB = ComponentB.get()->GetRigidBody();

	//DX 규칙에따른 법선으로 계산한 임펄스이므로 방향을 반대로 적용해야함
	RigidPtrA->ApplyImpulse(-collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
	RigidPtrB->ApplyImpulse(collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
}

void UCollisionManager::HandlePersistentCollision(const FCollisionPair& InPair, const FCollisionDetectionResult& DetectResult, const float DeltaTime)
{
	auto CompA = RegisteredComponents[InPair.IndexA].Component;
	auto CompB = RegisteredComponents[InPair.IndexB].Component;

	auto RigidA = CompA->GetRigidBody();
	auto RigidB = CompB->GetRigidBody();

	if (!RigidA || !RigidB) return;

	const float BiasFactor = 0.2f;
	const float AngularBiasFactor = 0.2f;
	const float Slop = 0.005f;

	// 목표 : 상대속도 0으로 만들기

	// 선형 속도 처리
	Vector3 RelativeVel = RigidB->GetVelocity() - RigidA->GetVelocity();
	float NormalVelocity = Vector3::Dot(RelativeVel, DetectResult.Normal);

	float desiredDeltaVelocity = 0.0f;
	if (DetectResult.PenetrationDepth > Slop)
	{
		desiredDeltaVelocity = (DetectResult.PenetrationDepth - Slop) * BiasFactor;
	}

	// 선형 속도 보정
	float velocityError = -NormalVelocity + desiredDeltaVelocity;

	float invMassA = RigidA->IsStatic() ? 0.0f : 1.0f / RigidA->GetMass();
	float invMassB = RigidB->IsStatic() ? 0.0f : 1.0f / RigidB->GetMass();

	if (invMassA + invMassB > 0.0f)
	{
		Vector3 velocityChange = DetectResult.Normal * velocityError;

		if (!RigidA->IsStatic())
		{
			RigidA->SetVelocity(RigidA->GetVelocity() - velocityChange * (invMassA / (invMassA + invMassB)));
		}

		if (!RigidB->IsStatic())
		{
			RigidB->SetVelocity(RigidB->GetVelocity() + velocityChange * (invMassB / (invMassA + invMassB)));
		}
	}

	// 각속도 처리
	Vector3 RelativeAngularVel = RigidB->GetAngularVelocity() - RigidA->GetAngularVelocity();

	// 각속도 보정
	if (!RigidA->IsStatic())
	{
		RigidA->SetAngularVelocity(RigidA->GetAngularVelocity() - RelativeAngularVel * AngularBiasFactor);
	}

	if (!RigidB->IsStatic())
	{
		RigidB->SetAngularVelocity(RigidB->GetAngularVelocity() + RelativeAngularVel * AngularBiasFactor);
	}
}

void UCollisionManager::ApplyPositionCorrection(const std::shared_ptr<UCollisionComponent>& CompA, const std::shared_ptr<UCollisionComponent>& CompB, 
												const FCollisionDetectionResult& DetectResult, const float DeltaTime)
{
	if (!CompA || !CompB || DetectResult.PenetrationDepth <= KINDA_SMALL)
		return;

	auto RigidA = CompA->GetRigidBody();
	auto RigidB = CompB->GetRigidBody();

	if (!RigidA || !RigidB)
		return;

	// 정적/동적 상태에 따른 보정 비율 결정
	float ratioA = RigidA->IsStatic() ? 0.0f : 1.0f;
	float ratioB = RigidB->IsStatic() ? 0.0f : 1.0f;

	if (ratioA + ratioB > 0.0f)
	{
		if (ratioA > 0.0f) ratioA /= (ratioA + ratioB);
		if (ratioB > 0.0f) ratioB /= (ratioA + ratioB);

		Vector3 correction = DetectResult.Normal * DetectResult.PenetrationDepth;

		// 각 물체를 반대 방향으로 밀어냄
		if (!RigidA->IsStatic())
		{
			auto TransA = RigidA->GetTransform();
			Vector3 newPos = TransA->GetPosition() - correction * ratioA;
			//TransA->SetPosition(Math::Lerp(TransA->GetPosition(), newPos,DeltaTime));
			TransA->SetPosition(newPos);
		}

		if (!RigidB->IsStatic())
		{
			auto TransB = RigidB->GetTransform();
			Vector3 newPos = TransB->GetPosition() + correction * ratioB;
			//TransB->SetPosition(Math::Lerp(TransB->GetPosition(), newPos, DeltaTime));
			TransB->SetPosition(newPos);
		}
	}
}

void UCollisionManager::ApplyCollisionResponseByContraints(const std::shared_ptr<UCollisionComponent>& ComponentA, const std::shared_ptr<UCollisionComponent>& ComponentB, const FCollisionDetectionResult& DetectResult)
{
	if (!ComponentA.get() || !ComponentA.get()->GetRigidBody() ||
		!ComponentB.get() || !ComponentB.get()->GetRigidBody())
		return;

	FCollisionResponseParameters ParamsA, ParamsB;
	GetCollisionDetectionParams(ComponentA, ParamsA);
	GetCollisionDetectionParams(ComponentB, ParamsB);


	for (int i = 0; i < Config.PhysicsIteration ; ++i)
	{
		FCollisionResponseResult collisionResponse = ResponseCalculator->CalculateResponseByContraints(DetectResult, ParamsA, ParamsB);

		auto RigidPtrA = ComponentA.get()->GetRigidBody();
		auto RigidPtrB = ComponentB.get()->GetRigidBody();

		//DX 규칙에따른 법선으로 계산한 임펄스이므로 방향을 반대로 적용해야함
		RigidPtrA->ApplyImpulse(-collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
		RigidPtrB->ApplyImpulse(collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
	}

}

void UCollisionManager::BroadcastCollisionEvents(const FCollisionPair& InPair, const FCollisionDetectionResult& DetectionResult)
{
	auto CompAData = RegisteredComponents[InPair.IndexA];
	auto CompBData = RegisteredComponents[InPair.IndexB];

	auto CompA = CompAData.Component;
	auto CompB = CompBData.Component;
	if (!CompA || !CompB)
		return;

	FCollisionEventData EventData;
	EventData.CollisionDetectResult = DetectionResult;

	ECollisionState NowState = ECollisionState::None;
	if (InPair.bPrevCollided)
	{
		if (DetectionResult.bCollided)
		{
			NowState = ECollisionState::Stay;
		}
		else
		{
			NowState = ECollisionState::Exit;
		}

	}
	else
	{
		if (DetectionResult.bCollided)
		{
			NowState = ECollisionState::Enter;
		}
	}

	if (NowState == ECollisionState::None)
	{
		return;
	}

	EventData.OtherComponent = CompB;
	EventDispatcher->DispatchCollisionEvents(CompA, EventData, NowState);
	EventData.OtherComponent = CompA;
	EventDispatcher->DispatchCollisionEvents(CompB, EventData, NowState);
}
