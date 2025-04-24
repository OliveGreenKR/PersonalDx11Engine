#include "CollisionManager.h"
#include "RigidBodyComponent.h"
#include "Transform.h"
#include <algorithm>
#include "DynamicAABBTree.h"
#include "CollisionComponent.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventDispatcher.h"
#include "Debug.h"
#include "ConfigReadManager.h"

UCollisionManager::~UCollisionManager()
{
	Release();
}

void UCollisionManager::RegisterCollision(std::shared_ptr<UCollisionComponentBase>& NewComponent, const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	if (!InRigidBody)
	{
		return;
	}

	InRigidBody->AddChild(NewComponent);

	RegisterCollision(NewComponent);

}

void UCollisionManager::RegisterCollision(std::shared_ptr<UCollisionComponentBase>& NewComponent)
{
	if (!CollisionTree || !NewComponent )
		return;

	// AABB 트리에 등록
	size_t TreeNodeId = CollisionTree->Insert(NewComponent);
	if (TreeNodeId == FDynamicAABBTree::NULL_NODE)
		return;

	// 맵에 추가
	RegisteredComponents[TreeNodeId] = NewComponent; 
}

void UCollisionManager::UnRegisterCollision(std::shared_ptr<UCollisionComponentBase>& InComponent)
{
	if (!InComponent || !CollisionTree)
		return;

	UCollisionComponentBase* targetPtr = InComponent.get();

	// 관리 중인 컴포넌트인지 확인
	auto targetIt = std::find_if(RegisteredComponents.begin(), RegisteredComponents.end(),
								 [targetPtr](const auto& RegisteredPair) {
									 return targetPtr == RegisteredPair.second.lock().get();
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
		LoadConfigFromIni();

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

	if (CollisionTree)
	{
		delete CollisionTree;
		CollisionTree = nullptr;
	}
	if (EventDispatcher)
	{
		delete EventDispatcher;
		EventDispatcher = nullptr;
	}
	if (ResponseCalculator)
	{
		delete ResponseCalculator;
		ResponseCalculator = nullptr;
	}
	if (Detector)
	{
		delete Detector;
		Detector = nullptr;
	}
	ActiveCollisionPairs.clear();
	RegisteredComponents.clear();
}

void UCollisionManager::CleanupDestroyedComponents()
{
	if (RegisteredComponents.empty() || !CollisionTree)
		return;

	std::vector<size_t> componentsToRemove;

	// 제거될 컴포넌트 식별 
	for (auto& pair : RegisteredComponents)
	{
		auto comp = pair.second.lock();
		if (!comp)
		{
			componentsToRemove.push_back(pair.first);
		}
	}

	if (componentsToRemove.size() > 0)
	{
		LOG_FUNC_CALL("[WARNING] Unregistered Destroyed CollisionComponents : [% zu] were found.", componentsToRemove.size());
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
		auto component = compData.second.lock();
		size_t treeNodeId = compData.first;

		if (!component || !component->IsActive())
			continue;

		FDynamicAABBTree::AABB bounds = CollisionTree->GetFatBounds(treeNodeId);

		// 특정 AABB와 겹치는 모든 노드 찾기
		CollisionTree->QueryOverlap(bounds, [&](size_t otherNodeId) {
			// 자기 자신과의 충돌 무시 및 중복 충돌 쌍 방지
			if (treeNodeId >= otherNodeId ||
				!RegisteredComponents.count(otherNodeId) ||
				!RegisteredComponents[otherNodeId].lock() ||
				!RegisteredComponents[otherNodeId].lock()->IsActive())
				return;

			// 새 충돌 쌍 생성
			FCollisionPair NewPair(treeNodeId, otherNodeId);

			// 기존 충돌 쌍에서 상태 복사
			auto ExistingPair = ActiveCollisionPairs.find(NewPair);
			if (ExistingPair != ActiveCollisionPairs.end()) {
				NewPair.bPrevCollided = ExistingPair->bPrevCollided;
				NewPair.PrevConstraints = ExistingPair->PrevConstraints;
			}

			NewCollisionPairs.insert(std::move(NewPair));});
	}

	// Exit 상황이 확실한 쌍을 찾음
	for (const auto& ExistingPair : ActiveCollisionPairs)
	{
		// 컴포넌트가 여전히 유효한지 확인
		auto CompA = RegisteredComponents[ExistingPair.TreeIdA].lock();
		auto CompB = RegisteredComponents[ExistingPair.TreeIdB].lock();

		if (CompA && CompB && CompA->IsActive() && CompB->IsActive())
		{
			// 기존의 쌍에 존재 + 이전프레임 충돌 + 새로운 쌍에 없음
			if (ExistingPair.bPrevCollided && 
				NewCollisionPairs.find(ExistingPair) == NewCollisionPairs.end())
			{
				FCollisionDetectionResult ExitResult;
				ExitResult.bCollided = false;
				BroadcastCollisionEvents(ExistingPair, ExitResult);
			}
		}
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
		auto CompA = RegisteredComponents[ActivePair.TreeIdA].lock();
		auto CompB = RegisteredComponents[ActivePair.TreeIdB].lock();

		const float PersistentThreshold = 0.1f;

		//Collision detection
		FCollisionDetectionResult DetectResult;
		if (CompA && CompB)
		{
			if (ShouldUseCCD(CompA->GetRigidBody()) || ShouldUseCCD(CompB->GetRigidBody()))
			{
				//ccd
				DetectResult = Detector->DetectCollisionCCD(*CompA.get(), CompA->GetPreviousTransform(), CompA->GetWorldTransform(),
															*CompB.get(), CompB->GetPreviousTransform(), CompB->GetWorldTransform(), DeltaTime);
			}
			else
			{
				//dcd
				DetectResult = Detector->DetectCollisionDiscrete(*CompA.get(), CompA->GetWorldTransform(),
																 *CompB.get(), CompB->GetWorldTransform());
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

void UCollisionManager::GetPhysicsParams(const std::shared_ptr<UCollisionComponentBase>& InComp, FPhysicsParameters& ResponseResult ) const
{
	auto CompPtr = InComp.get();
	if (!CompPtr)
		return;
	auto RigidPtr = CompPtr->GetRigidBody();
	if (!RigidPtr)
		return;

	ResponseResult.Mass = RigidPtr->GetMass();

	Vector3 RotInerteria = RigidPtr->GetRotationalInertia();
    ResponseResult.RotationalInertia = XMLoadFloat3(&RotInerteria);  

    Vector3 Position = RigidPtr->GetWorldTransform().Position;  
    ResponseResult.Position = XMLoadFloat3(&Position);  

    Vector3 Velocity = RigidPtr->GetVelocity();  
    ResponseResult.Velocity = XMLoadFloat3(&Velocity);

	ResponseResult.AngularVelocity = XMLoadFloat3(&(RigidPtr->GetAngularVelocity()));

	ResponseResult.Restitution = RigidPtr->GetRestitution();
	ResponseResult.FrictionKinetic = RigidPtr->GetFrictionKinetic();
	ResponseResult.FrictionStatic = RigidPtr->GetFrictionStatic();
	ResponseResult.Rotation = XMLoadFloat4(&(RigidPtr->GetWorldTransform().Rotation));
	return;
}

void UCollisionManager::ApplyPositionCorrection(const std::shared_ptr<UCollisionComponentBase>& CompA, const std::shared_ptr<UCollisionComponentBase>& CompB, 
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
			auto TransA = RigidA->GetWorldTransform();
			Vector3 newPos = TransA.Position - correction * ratioA;
			RigidA->SetWorldPosition(newPos);
		}

		if (!RigidB->IsStatic())
		{
			auto TransB = RigidB->GetWorldTransform();
			Vector3 newPos = TransB.Position + correction * ratioB;
			RigidB->SetWorldPosition(newPos);
		}
	}
}

void UCollisionManager::ApplyCollisionResponseByContraints(const FCollisionPair& CollisionPair, const FCollisionDetectionResult& DetectResult)
{

	auto ComponentA = RegisteredComponents[CollisionPair.TreeIdA].lock();
	auto ComponentB = RegisteredComponents[CollisionPair.TreeIdB].lock();

	if (!ComponentA || !ComponentA->GetRigidBody() ||
		!ComponentB || !ComponentB->GetRigidBody())
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
	auto CompA = RegisteredComponents[InPair.TreeIdA].lock();
	auto CompB = RegisteredComponents[InPair.TreeIdB].lock();

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

void UCollisionManager::LoadConfigFromIni()
{
	UConfigReadManager::Get()->GetValue("bPhysicsSimulated", Config.bPhysicsSimulated);
	UConfigReadManager::Get()->GetValue("MinimumTimeStep", Config.MinimumTimeStep);
	UConfigReadManager::Get()->GetValue("MaximumTimeStep", Config.MaximumTimeStep);
	UConfigReadManager::Get()->GetValue("CCDVelocityThreshold", Config.CCDVelocityThreshold);
	UConfigReadManager::Get()->GetValue("ConstraintInterations", Config.ConstraintInterations);
	UConfigReadManager::Get()->GetValue("bUseFixedTimestep", Config.bUseFixedTimestep);
	UConfigReadManager::Get()->GetValue("FixedTimeStep", Config.FixedTimeStep);
	UConfigReadManager::Get()->GetValue("MaxSubSteps", Config.MaxSubSteps);
	UConfigReadManager::Get()->GetValue("InitialCapacity", Config.InitialCapacity);
	UConfigReadManager::Get()->GetValue("AABBMargin", Config.AABBMargin);
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
