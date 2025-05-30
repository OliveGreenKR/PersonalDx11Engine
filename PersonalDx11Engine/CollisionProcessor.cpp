#include "CollisionProcessor.h"
#include "Transform.h"
#include <algorithm>
#include "DynamicAABBTree.h"
#include "CollisionComponent.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventDispatcher.h"
#include "CollisionPositionalCorrectionCalculator.h"
#include "Debug.h"
#include "ConfigReadManager.h"
#include "PhysicsStateInternalInterface.h"
#include "PhysicsDefine.h"

void FCollisionProcessor::LoadConfigFromIni()
{
	UConfigReadManager::Get()->GetValue("CCDVelocityThreshold", CCDVelocityThreshold);
	UConfigReadManager::Get()->GetValue("InitialCollisionCapacity", InitialCollisonCapacity);
	UConfigReadManager::Get()->GetValue("MaxConstraintIterations", MaxConstraintIterations);
	UConfigReadManager::Get()->GetValue("FatBoundsExtentRatio", FatBoundsExtentRatio);
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
		PositionCorrectionCalculator = new FPositionalCorrectionCalculator();

		CollisionTree = new FDynamicAABBTree(InitialCollisonCapacity);
		CollisionTree->AABB_Extension = std::max(0.1f,FatBoundsExtentRatio); //기본은 0.1f

		RegisteredComponents.reserve(InitialCollisonCapacity);
		ActiveCollisionPairs.reserve(InitialCollisonCapacity);
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
	if(PositionCorrectionCalculator)
	{
		delete PositionCorrectionCalculator;
		PositionCorrectionCalculator = nullptr;
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
				NewPair = *ExistingPair;
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
	return PhysicsState->P_GetVelocity().Length() > CCDVelocityThreshold;
}

float FCollisionProcessor::ProcessCollisions(const float DeltaTime)
{
	const float TotalDeltaTime = DeltaTime;
	float minCollideTime = 1.0f;
	
	std::vector<const FCollisionPair*> CollisionPairs;
	std::vector<FCollisionDetectionResult> DetectionResults;

	CollisionPairs.reserve(ActiveCollisionPairs.size());
	DetectionResults.reserve(ActiveCollisionPairs.size());

	//충돌 감지 및 정보 수집
	for (auto it = ActiveCollisionPairs.begin(); it != ActiveCollisionPairs.end() ; ++it)
	{

		auto& ActivePair = *it;

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
		//충돌 시 최저 ToI 갱신 및 충돌 정보 수집
		if (DetectResult.bCollided)
		{
			minCollideTime = std::min(minCollideTime, DetectResult.TimeOfImpact);

			CollisionPairs.push_back(&(*it));
			DetectionResults.push_back(DetectResult);
		}
	}

	// 겹침 비율에 따른 좌표 기반 위치 보정
	for (int j = 0; j < CollisionPairs.size(); ++j)
	{
		auto& CurrentPair = *CollisionPairs[j];
		auto& CurrentResult = DetectionResults[j];

		float overlapRatio = CalculateAABBOverlapRatio(CurrentPair);
		if (overlapRatio > 0.7f)
		{
			ApplyDirectPositionCorrection(CurrentPair, CurrentResult, 0.45f);
		}
		else if (overlapRatio > 0.4f)
		{
			ApplyDirectPositionCorrection(CurrentPair, CurrentResult, 0.2f);
		}
	}

	//수집 된 정보를 반복적 해결법 적용
	for (int i = 0; i < MaxConstraintIterations; ++i)
	{
		for (int j = 0; j < CollisionPairs.size(); ++j)
		{
			auto& CurrentPair = *CollisionPairs[j];
			auto& CurrentResult = DetectionResults[j];

			if (CurrentPair.bConverged)
				continue;

			auto CompA = RegisteredComponents[CurrentPair.TreeIdA].lock();
			auto CompB = RegisteredComponents[CurrentPair.TreeIdB].lock();

			ApplyCollisionResponseByContraints(CurrentPair, CurrentResult, DeltaTime);
		}
	}

	for (int j = 0; j < CollisionPairs.size(); ++j)
	{
		auto& CurrentPair = *CollisionPairs[j];
		auto& CurrentResult = DetectionResults[j];
		BroadcastCollisionEvents(CurrentPair, CurrentResult);
		//충돌 정보 저장
		CurrentPair.bPrevCollided = CurrentResult.bCollided;
		//수렴 정보 리셋
		CurrentPair.bConverged = false;
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

	OutParams.InvMass = PhysicsState->P_GetInvMass();

	Vector3 RotInvInerteria = PhysicsState->P_GetInvRotationalInertia();
    OutParams.InvRotationalInertia = XMLoadFloat3(&RotInvInerteria);

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

void FCollisionProcessor::ApplyCollisionResponseByContraints(const FCollisionPair& CollisionPair, const FCollisionDetectionResult& DetectResult, 
															 const float DeltaTime)
{
	if (CollisionPair.bConverged)
	{
		return;
	}

	auto ComponentA = RegisteredComponents[CollisionPair.TreeIdA].lock();
	auto ComponentB = RegisteredComponents[CollisionPair.TreeIdB].lock();

	if (!ComponentA || !ComponentA->GetPhysicsStateInternal() ||
		!ComponentB || !ComponentB->GetPhysicsStateInternal() ||
		!DetectResult.bCollided)
		return;

	FPhysicsParameters ParamsA, ParamsB;
	GetPhysicsParams(ComponentA, ParamsA);
	GetPhysicsParams(ComponentB, ParamsB);

	// Warm Starting  람다 재사용
	FAccumulatedConstraint Accumulation = CollisionPair.PrevConstraints;
	
	// 새로운 접촉이면 더 강한 감쇠 적용
	float dampingFactor = CollisionPair.bPrevCollided ? 0.8f : 0.5f;
	Accumulation.Scale(dampingFactor);

	//충돌 반응 제약조건 계산
	FCollisionResponseResult collisionResponse;
	float BiasSpeed = CalculatePositionBiasVelocity(DetectResult.PenetrationDepth, 0.2f, DeltaTime, 0.01f);
	Vector3 NormalImpulse = 
		ResponseCalculator->CalculateNormalImpulse(DetectResult, ParamsA, ParamsB, Accumulation.normalLambda, BiasSpeed);
	Vector3 FrictionImpulse = 
		ResponseCalculator->CalculateFrictionImpulse(DetectResult, ParamsA, ParamsB, Accumulation.normalLambda, Accumulation.frictionLambda);

	// 최종 순수 충격량 합산
	// 마찰에 대한 접선 방향 충격량 약화 계수 (기존 상수 유지)
	constexpr float TangentCoef = 1.0f;
	collisionResponse.NetImpulse = (NormalImpulse + TangentCoef * FrictionImpulse);
	// 충돌 지점 설정
	collisionResponse.ApplicationPoint = DetectResult.Point;
	//수렴 조건 확인
	if (std::fabs(CollisionPair.PrevConstraints.normalLambda - Accumulation.normalLambda) < 1.0f
		|| collisionResponse.NetImpulse.Length () < KINDA_SMALL)
	{
		CollisionPair.bConverged = true;
		return;
	}

	auto RigidPtrA = ComponentA.get()->GetPhysicsStateInternal();
	auto RigidPtrB = ComponentB.get()->GetPhysicsStateInternal();
	//A->B 방향의 법선벡터이므로 반대로 적용
	RigidPtrA->P_ApplyImpulse(-collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);
	RigidPtrB->P_ApplyImpulse(collisionResponse.NetImpulse, collisionResponse.ApplicationPoint);

	//LOG("%s", Debug::ToString(collisionResponse.NetImpulse));

	//반응 결과 저장
	CollisionPair.PrevConstraints = Accumulation;
}

void FCollisionProcessor::ApplyDirectPositionCorrection(const FCollisionPair& CollisionPair, const FCollisionDetectionResult& DetectionResult, float CorrectionRatio)
{
	// 1. 컴포넌트 유효성 검증
	auto CompA = RegisteredComponents[CollisionPair.TreeIdA].lock();
	auto CompB = RegisteredComponents[CollisionPair.TreeIdB].lock();

	// 2. 물리 매개변수 구성
	FPhysicsParameters ParamsA, ParamsB;
	GetPhysicsParams(CompA, ParamsA);
	GetPhysicsParams(CompB, ParamsB);

	// 3. PositionalCorrectionCalculator 활용 - 질량 비례 분리 계산
	Vector3 CorrectionA, CorrectionB;
	bool success = PositionCorrectionCalculator->CalculateMassProportionalSeparation(
		ParamsA, ParamsB, DetectionResult,
		CorrectionA, CorrectionB,
		0.01f  // SafetyMargin
	);

	// 4. 보정 비율 적용 및 위치 갱신
	if (success)
	{
		CorrectionA *= CorrectionRatio;
		CorrectionB *= CorrectionRatio;

		auto PhysicsA = CompA->GetPhysicsStateInternal();
		auto PhysicsB = CompB->GetPhysicsStateInternal();

		FTransform TransA = PhysicsA->P_GetWorldTransform();
		FTransform TransB = PhysicsB->P_GetWorldTransform();

		TransA.Position += CorrectionA;
		TransB.Position +=  CorrectionB;

		PhysicsA->P_SetWorldTransform(TransA);
		PhysicsB->P_SetWorldTransform(TransB);
	}
}

bool FCollisionProcessor::IsPersistentContact(const FCollisionPair& CollisionPair, const FCollisionDetectionResult& DetectResult)
{
	//특정 프레임 이상 연속으로 침투 상태 유지?
	return false;
}
float FCollisionProcessor::CalculatePositionBiasVelocity(
	float PenetrationDepth,
	float BiasFactor,
	float DeltaTime,
	float Slop)
{
	Slop *= ONE_METER; //미터 단위로 변환

	// 슬롭(Slop)을 초과하는 침투만 고려
	float biasPenetration = std::fmaxf(0.0f, PenetrationDepth - Slop);

	if (biasPenetration > KINDA_SMALL) // 아주 작은 값은 무시
	{
		// Baumgarte 안정화 항: (위치 오류 * 보정 계수) / DeltaTime  
		// 위치 보정을 나타낼 속도 편향
		return (biasPenetration * BiasFactor) / DeltaTime;
	}
	return 0.0f;
}

float FCollisionProcessor::CalculateAABBOverlapRatio(const FCollisionPair& CollisionPair) const
{
	// 트리 노드에서 AABB 정보 획득
	if (!CollisionTree->IsValidId(CollisionPair.TreeIdA) ||
		!CollisionTree->IsValidId(CollisionPair.TreeIdB))
	{
		return 0.0f;
	}

	auto BoundsA = CollisionTree->GetBounds(CollisionPair.TreeIdA);
	auto BoundsB = CollisionTree->GetBounds(CollisionPair.TreeIdB);

	// AABB가 겹치지 않으면 0 반환
	if (!BoundsA.Overlaps(BoundsB))
	{
		return 0.0f;
	}

	// 겹침 볼륨 계산
	float OverlapVolume = CalculateAABBOverlapVolume(BoundsA, BoundsB);

	auto CalculateAABBVolume = [](const FDynamicAABBTree::AABB& Bounds) {
		Vector3 Size = Bounds.Max - Bounds.Min;
		return Size.x * Size.y * Size.z;
		};

	// 더 작은 객체의 볼륨을 기준으로 비율 계산
	float VolumeA = CalculateAABBVolume(BoundsA);
	float VolumeB = CalculateAABBVolume(BoundsB);
	float ReferenceVolume = Math::Min(VolumeA, VolumeB);

	// 최소 볼륨 보장 (매우 작은 객체 대응)
	constexpr float MinVolume = 0.001f; // 1cm³
	ReferenceVolume = Math::Max(ReferenceVolume, MinVolume);

	// 겹침 비율 계산 및 정규화
	float OverlapRatio = OverlapVolume / ReferenceVolume;

	// Sigmoid 함수로 부드러운 포화 (0~1 범위)
	return Math::Clamp(1.0f - expf(-OverlapRatio * 3.0f), 0.0f, 1.0f);
}

float FCollisionProcessor::CalculateAABBOverlapVolume(
	const FDynamicAABBTree::AABB& BoundsA,
	const FDynamicAABBTree::AABB& BoundsB
) const
{
	// 겹침 영역의 각 축별 길이 계산
	float OverlapX = Math::Max(0.0f,
							   Math::Min(BoundsA.Max.x, BoundsB.Max.x) - Math::Max(BoundsA.Min.x, BoundsB.Min.x));
	float OverlapY = Math::Max(0.0f,
							   Math::Min(BoundsA.Max.y, BoundsB.Max.y) - Math::Max(BoundsA.Min.y, BoundsB.Min.y));
	float OverlapZ = Math::Max(0.0f,
							   Math::Min(BoundsA.Max.z, BoundsB.Max.z) - Math::Max(BoundsA.Min.z, BoundsB.Min.z));

	return OverlapX * OverlapY * OverlapZ;
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
