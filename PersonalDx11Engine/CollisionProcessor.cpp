#include "CollisionProcessor.h"
#include "Transform.h"
#include <algorithm>
#include "DynamicAABBTree.h"
#include "CollisionComponent.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventDispatcher.h"
#include "Debug.h"
#include "ConfigReadManager.h"
#include "PhysicsStateInternalInterface.h"

void FCollisionProcessor::LoadConfigFromIni()
{
	UConfigReadManager::Get()->GetValue("CCDVelocityThreshold", Config.CCDVelocityThreshold);
	UConfigReadManager::Get()->GetValue("InitialCollisionCapacity", Config.InitialCollisonCapacity);
	UConfigReadManager::Get()->GetValue("FatBoundsExtentRatio", Config.FatBoundsExtentRatio);
}

FCollisionProcessor::~FCollisionProcessor()
{
	Release();
}

void FCollisionProcessor::RegisterCollision(std::shared_ptr<UCollisionComponentBase>& NewComponent)
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

void FCollisionProcessor::UnRegisterCollision(std::shared_ptr<UCollisionComponentBase>& InComponent)
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

float FCollisionProcessor::SimulateCollision(const float DeltaTime)
{
	CleanupDestroyedComponents();
	UpdateCollisionTransform();
	UpdateCollisionPairs();
	float minSimulTime = ProcessCollisions(DeltaTime);
	return minSimulTime;
}

void FCollisionProcessor::UnRegisterAll()
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

void FCollisionProcessor::Initialize()
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
		CollisionTree = new FDynamicAABBTree(Config.InitialCollisonCapacity);
		CollisionTree->AABB_Extension = std::max(0.1f,Config.FatBoundsExtentRatio); //기본은 0.1f

		RegisteredComponents.reserve(Config.InitialCollisonCapacity);
		ActiveCollisionPairs.reserve(Config.InitialCollisonCapacity);
	}
	catch (...)
	{
		Release();
	}
}

void FCollisionProcessor::Release()
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

void FCollisionProcessor::CleanupDestroyedComponents()
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

void FCollisionProcessor::UpdateCollisionPairs()
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

			// 새 쌍 상태 
			auto ExistingPair = ActiveCollisionPairs.find(NewPair);
			if (ExistingPair != ActiveCollisionPairs.end()) {
				NewPair.bPrevCollided = ExistingPair->bPrevCollided;
				NewPair.PrevConstraints = ExistingPair->PrevConstraints;
			}

			NewCollisionPairs.insert(std::move(NewPair));
									});
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

void FCollisionProcessor::UpdateCollisionTransform()
{
	CollisionTree->UpdateTree();
}

bool FCollisionProcessor::ShouldUseCCD(const IPhysicsStateInternal* PhysicsState) const
{
	if (!PhysicsState)
		return false;
	return PhysicsState->P_GetVelocity().Length() > Config.CCDVelocityThreshold;
}

float FCollisionProcessor::ProcessCollisions(const float DeltaTime)
{
	const float TotalDeltaTime = DeltaTime;
	float minCollideTime = 1.0f;
	
	for (auto& ActivePair : ActiveCollisionPairs)
	{

		auto CompA = RegisteredComponents[ActivePair.TreeIdA].lock();
		auto CompB = RegisteredComponents[ActivePair.TreeIdB].lock();

		//Collision detection
		FCollisionDetectionResult DetectResult;
		if (CompA && CompB)
		{
			if (ShouldUseCCD(CompA->GetPhysicsStateInternal()) || ShouldUseCCD(CompB->GetPhysicsStateInternal()))
			{
				//ccd
				DetectResult = Detector->DetectCollisionCCD(*CompA.get(), CompA->GetPreviousWorldTransform(), CompA->GetWorldTransform(),
															*CompB.get(), CompB->GetPreviousWorldTransform(), CompB->GetWorldTransform(), DeltaTime);
			}
			else
			{
				//dcd
				DetectResult = Detector->DetectCollisionDiscrete(*CompA.get(), CompA->GetWorldTransform(),
																 *CompB.get(), CompB->GetWorldTransform());
			}
		}
		//충돌 시 최저 ToI 갱신
		if (DetectResult.bCollided)
		{
			minCollideTime = std::min(minCollideTime, DetectResult.TimeOfImpact);

			//response
			ApplyCollisionResponseByContraints(ActivePair, DetectResult);

			float CorrectionRatio = ActivePair.bPrevCollided ? 0.8f : 0.5f;
			//position correction
			ApplyPositionCorrection(CompA, CompB, DetectResult, DeltaTime, CorrectionRatio);
			
		}
		//dispatch event
		BroadcastCollisionEvents(ActivePair, DetectResult);
		//충돌 정보 저장
		ActivePair.bPrevCollided = DetectResult.bCollided;
	}

	return minCollideTime;
}

void FCollisionProcessor::GetPhysicsParams(const std::shared_ptr<UCollisionComponentBase>& InComp, FPhysicsParameters& OutParams ) const
{
	auto CompPtr = InComp.get();
	if (!CompPtr)
		return;
	auto PhysicsState = CompPtr->GetPhysicsStateInternal();
	if (!PhysicsState)
		return;

	OutParams.Mass = PhysicsState->P_GetMass();

	Vector3 RotInerteria = PhysicsState->P_GetRotationalInertia();
    OutParams.RotationalInertia = XMLoadFloat3(&RotInerteria);  

    Vector3 Position = PhysicsState->P_GetWorldPosition();  
    OutParams.Position = XMLoadFloat3(&Position);  

    Vector3 Velocity = PhysicsState->P_GetVelocity();  
    OutParams.Velocity = XMLoadFloat3(&Velocity);
	Vector3 AngularVelocity = PhysicsState->P_GetAngularVelocity();
	OutParams.AngularVelocity = XMLoadFloat3(&(AngularVelocity));

	OutParams.Restitution = PhysicsState->P_GetRestitution();
	OutParams.FrictionKinetic = PhysicsState->P_GetFrictionKinetic();
	OutParams.FrictionStatic = PhysicsState->P_GetFrictionStatic();
	Quaternion Rotation = PhysicsState->P_GetWorldRotation();
	OutParams.Rotation = XMLoadFloat4(&Rotation);
	return;
}

void FCollisionProcessor::ApplyPositionCorrection(const std::shared_ptr<UCollisionComponentBase>& CompA, const std::shared_ptr<UCollisionComponentBase>& CompB,
												  const FCollisionDetectionResult& DetectResult, const float DeltaTime, const float CorrectionRatio)
{
	if (!CompA || !CompB || DetectResult.PenetrationDepth <= KINDA_SMALL)
		return;

	auto RigidA = CompA->GetPhysicsStateInternal();
	auto RigidB = CompB->GetPhysicsStateInternal();

	if (!RigidA || !RigidB )
		return;

	Vector3 correction = DetectResult.Normal * DetectResult.PenetrationDepth;
	// 각 물체를 반대 방향으로 밀어냄
	Vector3 newPosA = RigidA->P_GetWorldPosition() - correction * CorrectionRatio;
	RigidA->P_SetWorldPosition(newPosA);

	Vector3 newPosB = RigidB->P_GetWorldPosition() + correction * CorrectionRatio;
	RigidB->P_SetWorldPosition(newPosB);
}

void FCollisionProcessor::ApplyCollisionResponseByContraints(const FCollisionPair& CollisionPair, const FCollisionDetectionResult& DetectResult)
{

	auto ComponentA = RegisteredComponents[CollisionPair.TreeIdA].lock();
	auto ComponentB = RegisteredComponents[CollisionPair.TreeIdB].lock();

	if (!ComponentA || !ComponentA->GetPhysicsStateInternal() ||
		!ComponentB || !ComponentB->GetPhysicsStateInternal() ||
		!DetectResult.bCollided)
		return;

	FPhysicsParameters ParamsA, ParamsB;
	GetPhysicsParams(ComponentA, ParamsA);
	GetPhysicsParams(ComponentB, ParamsB);

	FAccumulatedConstraint Accumulation;

	// Warm Starting - 이전 프레임 람다 재사용
	if (CollisionPair.bPrevCollided)
	{
		Accumulation = CollisionPair.PrevConstraints;
		Accumulation.Scale(0.8f);  // 안정성을 위한 스케일링
	}

	FCollisionResponseResult collisionResponse =
		ResponseCalculator->CalculateResponseByContraints(DetectResult, ParamsA, ParamsB, Accumulation);
	auto RigidPtrA = ComponentA.get()->GetPhysicsStateInternal();
	auto RigidPtrB = ComponentB.get()->GetPhysicsStateInternal();
	//A->B 방향의 법선벡터이므로 반대로 적용
	RigidPtrA->P_ApplyImpulse(-collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
	RigidPtrB->P_ApplyImpulse(collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);

	//반응 결과 저장
	CollisionPair.PrevConstraints = Accumulation;
}

bool FCollisionProcessor::IsPersistentContact(const FCollisionPair& CollisionPair, const FCollisionDetectionResult& DetectResult)
{
	auto ComponentA = RegisteredComponents[CollisionPair.TreeIdA].lock();
	auto ComponentB = RegisteredComponents[CollisionPair.TreeIdB].lock();

	if (!ComponentA || !ComponentA->GetPhysicsStateInternal() ||
		!ComponentB || !ComponentB->GetPhysicsStateInternal() ||
		!DetectResult.bCollided)
		return false;

	//여러 프레임간 충돌중 + 상대 속도가 일정 수준 이하면 -> 접촉상황

	auto RigidA = ComponentA->GetPhysicsStateInternal();
	auto RigidB = ComponentB->GetPhysicsStateInternal();

	auto VeloA = RigidA->P_GetVelocity();
	auto VeloB = RigidB->P_GetVelocity();

	auto VeloAB = VeloB - VeloA;

	if (VeloAB.LengthSquared() < 1.0f && std::fabs(DetectResult.PenetrationDepth) < 1.0f)
	{
		return true;
	}

	return  false;
}

void FCollisionProcessor::BroadcastCollisionEvents(const FCollisionPair& InPair, const FCollisionDetectionResult& DetectionResult)
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

void FCollisionProcessor::PrintTreeStructure() const
{
#if defined(_DEBUG) || defined(DEBUG)
//test
	CollisionTree->PrintTreeStructure(std::cout);
	LOG("Collision Comps : %02d", CollisionTree->GetLeafNodeCount());
	LOG("Node : %02d", CollisionTree->GetNodeCount());
	LOG("-------------------------------------");
#endif
}
