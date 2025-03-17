#include "CollisionManager.h"
#include "RigidBodyComponent.h"
#include "Transform.h"
#include <algorithm>
#include "DynamicAABBTree.h"
#include "CollisionComponent.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventDispatcher.h"
#include "DebugDrawManager.h"
#include "Debug.h"

UCollisionManager::~UCollisionManager()
{
	Release();
}

void UCollisionManager::RegisterCollision(std::shared_ptr<UCollisionComponent>& NewComponent, const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	if (!InRigidBody)
	{
		return;
	}

	InRigidBody->AddChild(NewComponent);

	RegisterCollision(NewComponent);

}

void UCollisionManager::RegisterCollision(std::shared_ptr<UCollisionComponent>& NewComponent)
{
	if (!CollisionTree || !NewComponent || NewComponent->bDestroyed)
		return;

	// AABB 트리에 등록
	size_t TreeNodeId = CollisionTree->Insert(NewComponent);
	if (TreeNodeId == FDynamicAABBTree::NULL_NODE)
		return;

	// 맵에 추가
	RegisteredComponents[TreeNodeId] = NewComponent; 
}

void UCollisionManager::UnRegisterCollision(std::shared_ptr<UCollisionComponent>& InComponent)
{
	if (!InComponent || !CollisionTree)
		return;

	// shared_ptr 비교보다 raw 포인터 비교가 더 안정적임
	UCollisionComponent* targetPtr = InComponent.get();

	// 관리 중인 컴포넌트인지 확인
	auto targetIt = std::find_if(RegisteredComponents.begin(), RegisteredComponents.end(),
								 [targetPtr](const auto& RegisteredPair) {
									 return targetPtr == RegisteredPair.second.get();
								 });

	if (targetIt == RegisteredComponents.end())
	{
		LOG("[WARNING] Attempt to unregister collision component that is not registered");
		return;
	}

	// 트리 노드 ID 저장
	size_t unregisteredId = targetIt->first;

	try {
		// 충돌 쌍에서 해당 컴포넌트 관련 항목 제거
		auto it = ActiveCollisionPairs.begin();
		while (it != ActiveCollisionPairs.end())
		{
			const FCollisionPair& Pair = *it;
			if (Pair.TreeIdA == unregisteredId || Pair.TreeIdB == unregisteredId)
				it = ActiveCollisionPairs.erase(it);
			else
				++it;
		}

		// AABB 트리에서 제거
		CollisionTree->Remove(unregisteredId);

		// RegisteredComponents에서 제거
		RegisteredComponents.erase(targetIt);
	}
	catch (const std::exception& e) {
		LOG("[ERROR] Exception during collision unregistration: %s", e.what());
	}
}

void UCollisionManager::Tick(const float DeltaTime)
{
	if (Config.bUseFixedTimestep)
	{
		// 누적 시간 업데이트
		AccumulatedTime += DeltaTime;

		// 최대 처리 가능한 시간 (일정 시간 이상 누적되면 추적 포기)
		const float maxProcessTime = Config.MaximumTimeStep * Config.MaxSubSteps;
		if (AccumulatedTime > maxProcessTime) {
			// 심각한 지연 발생 - 시뮬레이션 타임스케일 조정 또는 경고
			LOG("Physics simulation falling behind, skipping %f seconds",
				AccumulatedTime - maxProcessTime);
			AccumulatedTime = maxProcessTime;
		}

		// TODO : 이전 상태 저장 (보간용)
		//StoreComponentStates();

		int steps = 0;
		while (AccumulatedTime >= Config.FixedTimeStep && steps < Config.MaxSubSteps)
		{
			// 고정 간격 물리 업데이트
			CleanupDestroyedComponents();
			UpdateCollisionTransform();
			UpdateCollisionPairs();
			ProcessCollisions(Config.FixedTimeStep);
			AccumulatedTime -= Config.FixedTimeStep;
			steps++;
		}

		// TODO : 남은 시간 비율로 현재 상태와 이전 상태 사이 보간
		float alpha = AccumulatedTime / Config.FixedTimeStep;
		//InterpolateComponentStates(alpha);
	}
	else
	{
		// 기존 가변 타임스텝 방식
		CleanupDestroyedComponents();
		UpdateCollisionTransform();
		UpdateCollisionPairs();
		ProcessCollisions(DeltaTime);
	}
}

void UCollisionManager::UnRegisterAll()
{
	if (!CollisionTree)
		return;

	for (const auto& Registered : RegisteredComponents)
	{
		CollisionTree->Remove(Registered.first);
	}
	ActiveCollisionPairs.clear();
	RegisteredComponents.clear();
}

void UCollisionManager::Initialize()
{
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
	}
	catch (...)
	{
		Release();
	}
}

void UCollisionManager::Release()
{
	UnRegisterAll();

	delete CollisionTree;
	delete EventDispatcher;
	delete ResponseCalculator;
	delete Detector;

	CollisionTree = nullptr;
	EventDispatcher = nullptr;
	ResponseCalculator = nullptr;
	Detector = nullptr;
}

void UCollisionManager::CleanupDestroyedComponents()
{
	if (RegisteredComponents.empty() || !CollisionTree)
		return;

	std::vector<size_t> componentsToRemove;

	// 제거될 컴포넌트 식별 (첫 단계에서는 제거할 ID만 수집)
	for (auto& pair : RegisteredComponents)
	{
		auto comp = pair.second;
		if (!comp || comp->bDestroyed)
		{
			componentsToRemove.push_back(pair.first);
		}
	}

	// 제거 작업 수행
	for (size_t nodeId : componentsToRemove)
	{
		// AABB 트리에서 제거
		if (CollisionTree->IsValidId(nodeId))
			CollisionTree->Remove(nodeId);

		// 활성 충돌 쌍에서 관련 항목 제거
		auto pairIt = ActiveCollisionPairs.begin();
		while (pairIt != ActiveCollisionPairs.end())
		{
			const FCollisionPair& Pair = *pairIt;
			if (Pair.TreeIdA == nodeId || Pair.TreeIdB == nodeId)
				pairIt = ActiveCollisionPairs.erase(pairIt);
			else
				++pairIt;
		}

		// RegisteredComponents에서 제거
		RegisteredComponents.erase(nodeId);
	}
}

void UCollisionManager::UpdateCollisionPairs()
{
	if (!CollisionTree || RegisteredComponents.empty())
	{
		ActiveCollisionPairs.clear();
		return;
	}

	std::unordered_set<FCollisionPair> NewCollisionPairs;

	// AABBTree 기반 broad-phase 충돌 검사
	for (const auto& compData : RegisteredComponents)
	{
		auto& component = compData.second;
		size_t treeNodeId = compData.first;

		if (!component || !component->IsActive())
			continue;

		FDynamicAABBTree::AABB bounds = CollisionTree->GetFatBounds(treeNodeId);

		// 특정 AABB와 겹치는 모든 노드 찾기
		CollisionTree->QueryOverlap(bounds, [&](size_t otherNodeId) {
			// 자기 자신과의 충돌 무시 및 중복 충돌 쌍 방지
			if (treeNodeId >= otherNodeId ||
				!RegisteredComponents.count(otherNodeId) ||
				!RegisteredComponents[otherNodeId]->IsActive())
				return;

			// 새 충돌 쌍 생성
			FCollisionPair NewPair(treeNodeId, otherNodeId);

			// 기존 충돌 쌍에서 상태 복사
			auto ExistingPair = ActiveCollisionPairs.find(NewPair);
			if (ExistingPair != ActiveCollisionPairs.end()) {
				NewPair.bPrevCollided = ExistingPair->bPrevCollided;
				NewPair.PrevConstraints = ExistingPair->PrevConstraints;
			}

			NewCollisionPairs.insert(std::move(NewPair));
									});
	}

	ActiveCollisionPairs = std::move(NewCollisionPairs);
}

void UCollisionManager::UpdateCollisionTransform()
{
	CollisionTree->UpdateTree();
}

bool UCollisionManager::ShouldUseCCD(const URigidBodyComponent* RigidBody) const
{
	if (!RigidBody)
		return false;
	return RigidBody->GetVelocity().Length() > Config.CCDVelocityThreshold;
}

void UCollisionManager::ProcessCollisions(const float DeltaTime)
{
	for (auto& ActivePair : ActiveCollisionPairs)
	{
		auto& CompA = RegisteredComponents[ActivePair.TreeIdA];
		auto& CompB = RegisteredComponents[ActivePair.TreeIdB];

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
		ApplyCollisionResponseByContraints(ActivePair, DetectResult);
		//position correction
		ApplyPositionCorrection(CompA, CompB, DetectResult, DeltaTime);
		//dispatch event
		BroadcastCollisionEvents(ActivePair, DetectResult);

		//충돌정보 저장
		ActivePair.bPrevCollided = DetectResult.bCollided;

	}
}

void UCollisionManager::GetPhysicsParams(const std::shared_ptr<UCollisionComponent>& InComp, FPhysicsParameters& ResponseResult ) const
{
	auto CompPtr = InComp.get();
	if (!CompPtr)
		return;
	auto RigidPtr = CompPtr->GetRigidBody();
	if (!RigidPtr)
		return;

	ResponseResult.Mass = RigidPtr->GetMass();
	ResponseResult.RotationalInertia = RigidPtr->GetRotationalInertia();
	ResponseResult.Position = RigidPtr->GetTransform()->GetPosition();
	ResponseResult.Velocity = RigidPtr->GetVelocity();
	ResponseResult.AngularVelocity = RigidPtr->GetAngularVelocity();
	ResponseResult.Restitution = RigidPtr->GetRestitution();
	ResponseResult.FrictionKinetic = RigidPtr->GetFrictionKinetic();
	ResponseResult.FrictionStatic = RigidPtr->GetFrictionStatic();
	ResponseResult.Rotation = RigidPtr->GetTransform()->GetRotation();

	return;
}

void UCollisionManager::ApplyCollisionResponseByImpulse(const std::shared_ptr<UCollisionComponent>& ComponentA, const std::shared_ptr<UCollisionComponent>& ComponentB, const FCollisionDetectionResult& DetectResult)
{
	if (!ComponentA.get() || !ComponentA.get()->GetRigidBody() ||
		!ComponentB.get() || !ComponentB.get()->GetRigidBody())
		return;

	FPhysicsParameters ParamsA, ParamsB;
	GetPhysicsParams(ComponentA, ParamsA);
	GetPhysicsParams(ComponentB , ParamsB);
	FCollisionResponseResult collisionResponse = ResponseCalculator->CalculateResponseByImpulse(DetectResult, ParamsA, ParamsB);

	auto RigidPtrA = ComponentA.get()->GetRigidBody();
	auto RigidPtrB = ComponentB.get()->GetRigidBody();

	//DX 규칙에따른 법선으로 계산한 임펄스이므로 방향을 반대로 적용해야함
	RigidPtrA->ApplyImpulse(-collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
	RigidPtrB->ApplyImpulse(collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
}

void UCollisionManager::HandlePersistentCollision(const FCollisionPair& InPair, const FCollisionDetectionResult& DetectResult, const float DeltaTime)
{
	auto CompA = RegisteredComponents[InPair.TreeIdA];
	auto CompB = RegisteredComponents[InPair.TreeIdB];

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

void UCollisionManager::ApplyCollisionResponseByContraints(const FCollisionPair& CollisionPair, const FCollisionDetectionResult& DetectResult)
{

	auto ComponentA = RegisteredComponents[CollisionPair.TreeIdA];
	auto ComponentB = RegisteredComponents[CollisionPair.TreeIdB];

	if (!ComponentA.get() || !ComponentA.get()->GetRigidBody() ||
		!ComponentB.get() || !ComponentB.get()->GetRigidBody())
		return;

	FPhysicsParameters ParamsA, ParamsB;
	GetPhysicsParams(ComponentA, ParamsA);
	GetPhysicsParams(ComponentB, ParamsB);

	FAccumulatedConstraint Accumulation;

	// Warm Starting - 이전 프레임 람다 재사용
	if (CollisionPair.bPrevCollided)
	{
		Accumulation = CollisionPair.PrevConstraints;
		Accumulation.Scale(0.65f);  // 안정성을 위한 스케일링
	}

	for (int i = 0; i < Config.ConstraintInterations ; ++i)
	{
		FCollisionResponseResult collisionResponse = 
			ResponseCalculator->CalculateResponseByContraints(DetectResult, ParamsA, ParamsB, Accumulation);

		auto RigidPtrA = ComponentA.get()->GetRigidBody();
		auto RigidPtrB = ComponentB.get()->GetRigidBody();

		//중간단계 적용
		RigidPtrA->ApplyImpulse(-collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
		RigidPtrB->ApplyImpulse(collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
	}

	CollisionPair.PrevConstraints = Accumulation;
}

void UCollisionManager::BroadcastCollisionEvents(const FCollisionPair& InPair, const FCollisionDetectionResult& DetectionResult)
{
	auto CompA = RegisteredComponents[InPair.TreeIdA];
	auto CompB = RegisteredComponents[InPair.TreeIdB];

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

void UCollisionManager::PrintTreeStructure()
{
#if defined(_DEBUG) || defined(DEBUG)
//test
	CollisionTree->PrintTreeStructure(std::cout);
	LOG("Collision Comps : %02d", CollisionTree->GetLeafNodeCount());
	LOG("Node : %02d", CollisionTree->GetNodeCount());
	LOG("-------------------------------------");
#endif
}
