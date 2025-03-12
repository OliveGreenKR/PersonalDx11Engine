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
	if (!NewComponent || NewComponent->bDestroyed)
		return;

	// AABB 트리에 등록
	size_t TreeNodeId = CollisionTree->Insert(NewComponent);
	if (TreeNodeId == FDynamicAABBTree::NULL_NODE)
		return;

	// 벡터에 추가
	RegisteredComponents[TreeNodeId] = NewComponent; 
}

void UCollisionManager::UnRegisterCollision(std::shared_ptr<UCollisionComponent>& InComponent)
{
	if (!InComponent)
		return;

	// 관리 중인 컴포넌트인지 확인
	auto targetIt = std::find_if(RegisteredComponents.begin(), RegisteredComponents.end(),
								 [&InComponent](const auto& RegisteredPair)
								 {
									 return InComponent.get() == RegisteredPair.second.get();
								 });

	if (targetIt == RegisteredComponents.end())
	{
		LOG("[WARNING] Try UnRegister to UnRegisutered Collision");
		return;
	}

	// 트리 노드 ID 저장
	size_t unregistedId = targetIt->first;

	// AABB 트리에서 제거
	if (CollisionTree)
		CollisionTree->Remove(unregistedId);

	// 충돌 쌍에서 해당 컴포넌트 관련 항목 제거
	auto it = ActiveCollisionPairs.begin();
	while (it != ActiveCollisionPairs.end())
	{
		const FCollisionPair& Pair = *it;
		if (Pair.TreeIdA == unregistedId || Pair.TreeIdB == unregistedId)
			it = ActiveCollisionPairs.erase(it);
		else
			++it;
	}

	// RegisteredComponents에서 제거 
	RegisteredComponents.erase(targetIt);
}

void UCollisionManager::Tick(const float DeltaTime)
{
	UpdateCollisionTransform();
	UpdateCollisionPairs();
	ProcessCollisions(DeltaTime);
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
	if (RegisteredComponents.empty())
		return;

	// 제거될 컴포넌트 식별
	auto it = RegisteredComponents.begin();
	while (it != RegisteredComponents.end())
	{
		auto comp = it->second;
		if (!comp || comp->bDestroyed)
		{
			// AABB 트리에서 제거
			if (CollisionTree && CollisionTree->IsValidId(it->first))
				CollisionTree->Remove(it->first);

			// 활성 충돌 쌍에서 관련 항목 제거
			auto pairIt = ActiveCollisionPairs.begin();
			while (pairIt != ActiveCollisionPairs.end())
			{
				const FCollisionPair& Pair = *pairIt;
				if (RegisteredComponents[Pair.TreeIdA].get() == comp.get() ||
					RegisteredComponents[Pair.TreeIdA].get() == comp.get())
					pairIt = ActiveCollisionPairs.erase(pairIt);
				else
					++pairIt;
			}

			// RegisteredComponents에서 제거
			it = RegisteredComponents.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void UCollisionManager::UpdateCollisionPairs()
{
	if (!CollisionTree || RegisteredComponents.empty())
	{
		ActiveCollisionPairs.clear();
		return;
	}

	// 새로운 충돌 쌍을 저장할 임시 컨테이너
	std::unordered_set<FCollisionPair> NewCollisionPairs;

	// 현재 유효한 컴포넌트들만 필터링
	//for (const auto& compData : RegisteredComponents)
	//{
	//	auto& Component = compData.second;
	//	size_t TreeNodeId = compData.first;

	//	if (Component && !Component->bDestroyed &&
	//		Component->IsActive() && CollisionTree->IsLeafNode(TreeNodeId))
	//	{
	//		FDynamicAABBTree::AABB FatBounds = CollisionTree->GetFatBounds(TreeNodeId);
	//		CollisionTree->QueryOverlap(FatBounds, [&NewCollisionPairs, &TreeNodeId]
	//									(const size_t OtherNodeId)
	//									{
	//										FCollisionPair tmpPairs(TreeNodeId,OtherNodeId);
	//										NewCollisionPairs.insert(tmpPairs);
	//									});
	//	}
	//}

	// 가능한 모든 페어 생성 (O(n²) 작업)
	for (auto it = RegisteredComponents.begin(); it != RegisteredComponents.end() ; ++it)
	{
		for (auto jt = it; jt != RegisteredComponents.end(); ++jt)
		{
			if (it->first != jt->first)
			{
				FCollisionPair tmpPairs(it->first, jt->first);
				NewCollisionPairs.insert(tmpPairs);
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

	FCollisionResponseParameters ParamsA, ParamsB;
	GetCollisionDetectionParams(ComponentA, ParamsA);
	GetCollisionDetectionParams(ComponentB, ParamsB);

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
#endif
}
