#include "CollisionProcessor.h"
#include "PhysicsStateInternalInterface.h"
#include "CollisionShapeInternalInterface.h"
#include "PhysicsEventDispatcherInterface.h"

#include "ConfigReadManager.h"
#include "Debug.h"
#pragma region Constructor and Initialization

FCollisionProcessor::~FCollisionProcessor()
{
    Release();
}

bool FCollisionProcessor::Initialize(IPhysicsStateInternal* PhysicsStateInterface, ICollisionShapeInternal* ShapeInterface, IPhysicsEventDispatcher* EventDispathcer)
{
    // 1. 입력 검증
    if (!PhysicsStateInterface || !ShapeInterface || !EventDispathcer)
    {
        LOG_ERROR("FCollisionProcessor::Initialize - Invalid interface pointers");
        return false;
    }

    // 2. 이미 초기화된 경우 해제 후 재초기화
    if (bIsInitialized)
    {
        Release();
    }

    try
    {
        // 3. 인터페이스 저장
        this->PhysicsStateInterface = PhysicsStateInterface;
        this->ShapeInterface = ShapeInterface;
        this->PhysicsEventDispatcher = EventDispathcer;

        // 4. 설정 로드
        LoadConfigFromIni();

        // 5. 하부 시스템 초기화
        Detector = std::make_unique<FCollisionDetector>();
        ResponseCalculator = std::make_unique<FCollisionResponseCalculator>();
        EventCalculator = std::make_unique<FCollisionEventCalculator>();
        PositionCorrectionCalculator = std::make_unique<FCollisionPositionCorrectionCalculator>();

        // 6. 공간 분할 트리 초기화
        CollisionTree = std::make_unique<FDynamicAABBTree>(InitialCollisionCapacity);
        CollisionTree->SetFatMarginRatio(std::max(0.1f, FatBoundsExtentRatio));

        // 7. 컨테이너 메모리 예약 (캐시 효율성)
        EffectiveCollisionPairs.reserve(InitialCollisionCapacity);
        PhysicsIdToNodeId.reserve(InitialCollisionCapacity);
        NodeIdToPhysicsId.reserve(InitialCollisionCapacity);

        // 8. 초기화 완료 마킹
        bIsInitialized = true;

        LOG_INFO("FCollisionProcessor initialized successfully - Capacity: %u", InitialCollisionCapacity);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("FCollisionProcessor::Initialize failed: %s", e.what());
        Release();
        return false;
    }
    catch (...)
    {
        LOG_ERROR("FCollisionProcessor::Initialize failed: Unknown exception");
        Release();
        return false;
    }
}

void FCollisionProcessor::Release()
{
    // 1. 모든 등록 해제
    UnRegisterAll();

    // 2. 하부 시스템 해제 (unique_ptr이므로 자동으로 정리됨)
    PositionCorrectionCalculator.reset();
    EventCalculator.reset();
    ResponseCalculator.reset();
    Detector.reset();
    CollisionTree.reset();

    // 3. 컨테이너 정리
    EffectiveCollisionPairs.clear();
    PhysicsIdToNodeId.clear();
    NodeIdToPhysicsId.clear();

    // 4. 임시 저장소 정리
    CurrentCollidingPairs.clear();
    CurrentDetectionResults.clear();
    CurrentParamsA.clear();
    CurrentParamsB.clear();

    // 5. 인터페이스 해제
    PhysicsStateInterface = nullptr;
    ShapeInterface = nullptr;
    PhysicsEventDispatcher = nullptr;

    // 6. 초기화 상태 해제
    bIsInitialized = false;

    LOG_INFO("FCollisionProcessor released successfully");
}

#pragma endregion

#pragma region Public Interface (PhysicsSystem 전용)

void FCollisionProcessor::PrintTreeStructure() const
{
    CollisionTree->PrintTreeStructure();
}

float FCollisionProcessor::ProcessCollisions(const std::vector<PhysicsID>& ActivePhysicsIDs, float DeltaTime)
{
    // 1. 초기화 상태 검증
    if (!IsInitialized() || DeltaTime <= 0.0f)
    {
        LOG_WARNING("FCollisionProcessor::ProcessCollisions - Invalid state or delta time");
        return 1.0f; // 전체 시간 소모
    }

    // 2. 현재 델타 타임 저장 (멤버 기반 접근용)
    CurrentDeltaTime = DeltaTime;

    // 3. 임시 저장소 초기화 및 메모리 예약 (동적 할당 최소화)
    CurrentCollidingPairs.clear();
    CurrentDetectionResults.clear();
    CurrentParamsA.clear();
    CurrentParamsB.clear();

    // 이전 프레임 크기 기반 예약 (성능 최적화)
    size_t expectedPairCount =
        std::max(static_cast<size_t>(64), static_cast<size_t>(EffectiveCollisionPairs.size() * 1.2f)); // 20% 여유분
    CurrentCollidingPairs.reserve(expectedPairCount);
    CurrentDetectionResults.reserve(expectedPairCount);
    CurrentParamsA.reserve(expectedPairCount);
    CurrentParamsB.reserve(expectedPairCount);

    // 4. 공간 분할 트리 업데이트 (활성 PhysicsID 기반)
    UpdateSpatialPartitioning(ActivePhysicsIDs);

    // 5. 브로드페이즈 충돌 쌍 업데이트
    UpdateBroadPhasePairs(ActivePhysicsIDs);

    // 6. 정밀 충돌 감지 및 최소 ToI 계산 (데이터 수집)
    float MinTimeOfImpact = PerformNarrowphaseDetection(DeltaTime);

    // 7. 성능 최적화: 최소 충돌 시간 +2% 까지의 충돌들 일괄 처리
    float TargetTime = MinTimeOfImpact + 0.02f; // 2% 여유분
    FilterCollisionsByToI(TargetTime);

    // 8. 충돌 반응 처리 (통합된 Apply 함수 호출)
    ApplyCollisionResponse(DeltaTime);

    // 9. 충돌 이벤트 생성 (멤버 데이터 기반)
    RequestCollisionEvents();

    // 10. 정규화된 시뮬레이션 시간 반환 (0.0~1.0)
    return MinTimeOfImpact;
}

void FCollisionProcessor::UpdateSpatialPartitioning(const std::vector<PhysicsID>& ActivePhysicsIDs)
{
    if (!CollisionTree || ActivePhysicsIDs.empty())
        return;

    // 1. 기존 등록된 PhysicsID들 중 비활성화된 것들 제거
    std::vector<PhysicsID> ToRemove;
    for (const auto& [physicsId, nodeId] : PhysicsIdToNodeId)
    {
        // ActivePhysicsIDs에 없으면 제거 대상
        if (std::find(ActivePhysicsIDs.begin(), ActivePhysicsIDs.end(), physicsId) == ActivePhysicsIDs.end())
        {
            ToRemove.push_back(physicsId);
        }
    }

    // 1-1. 제거될 PhysicsID가 포함된 충돌 쌍 중 이전에 충돌했던 것들에 대해 Exit 이벤트 생성
    if (!ToRemove.empty())
    {
        for (const auto& existingPair : EffectiveCollisionPairs)
        {
            // 제거될 PhysicsID가 포함된 쌍인지 확인
            bool containsRemovedId = std::find(ToRemove.begin(), ToRemove.end(), existingPair.PhysicsIdA) != ToRemove.end() ||
                std::find(ToRemove.begin(), ToRemove.end(), existingPair.PhysicsIdB) != ToRemove.end();

            // 이전에 충돌 중이었던 쌍이면 Exit 이벤트 생성
            if (containsRemovedId && existingPair.bPrevCollided)
            {
                GenerateAndSendExitEventToPhyscis(existingPair);
            }
        }
    }

    // 2. 비활성 노드들 제거
    for (PhysicsID physicsId : ToRemove)
    {
        auto nodeIt = PhysicsIdToNodeId.find(physicsId);
        if (nodeIt != PhysicsIdToNodeId.end())
        {
            size_t nodeId = nodeIt->second;

            // AABB 트리에서 제거
            if (CollisionTree->IsValidId(nodeId))
            {
                CollisionTree->Remove(nodeId);
            }

            // 매핑 제거
            NodeIdToPhysicsId.erase(nodeId);
            PhysicsIdToNodeId.erase(nodeIt);
        }
    }

    // 3. 활성 PhysicsID들의 AABB 업데이트 또는 새로 등록
    for (PhysicsID physicsId : ActivePhysicsIDs)
    {
        if (!IsValidPhysicsID(physicsId))
            continue;

        // 현재 월드 변환 정보 획득 (인터페이스 직접 호출)
        XMVECTOR currentPos = PhysicsStateInterface->P_GetWorldPosition(physicsId);
        XMVECTOR currentRot = PhysicsStateInterface->P_GetWorldRotationQuat(physicsId);
        XMVECTOR shapeExtent = ShapeInterface->P_GetShapeHalfExtent(physicsId);
        ECollisionShapeType shapeType = ShapeInterface->P_GetShapeType(physicsId);

        // AABB 계산 (형상 + 변환 적용)
        FMAABB currentAABB = CalculateAABBFromShape(currentPos, currentRot, shapeExtent, shapeType);

        auto nodeIt = PhysicsIdToNodeId.find(physicsId);
        if (nodeIt != PhysicsIdToNodeId.end())
        {
            // 4. 기존 등록된 경우: 노드 AABB 업데이트 (Bounds만 업데이트)
            size_t nodeId = nodeIt->second;
            CollisionTree->UpdateNodeBounds(nodeId, currentAABB);
        }
        else
        {
            // 5. 새로 등록: AABB 트리에 추가
            size_t newNodeId = CollisionTree->Insert(currentAABB);
            if (newNodeId != FDynamicAABBTree::NULL_NODE)
            {
                PhysicsIdToNodeId[physicsId] = newNodeId;
                NodeIdToPhysicsId[newNodeId] = physicsId;
            }
        }
    }

    // 6. 트리 구조 최적화 (Fat Bounds 벗어난 노드들 자동 재배치)
    CollisionTree->UpdateTree();
}

void FCollisionProcessor::LoadConfigFromIni()
{
    UConfigReadManager* ConfigManager = UConfigReadManager::Get();
    if (!ConfigManager)
    {
        LOG_WARNING("ConfigReadManager not available - using default collision settings");
        return;
    }

    // 설정값 로드 (실패 시 기본값 유지)
    ConfigManager->GetValue("CCDVelocityThreshold", CCDVelocityThreshold);
    ConfigManager->GetValue("InitialCollisionCapacity", InitialCollisionCapacity);
    ConfigManager->GetValue("MaxConstraintIterations", MaxConstraintIterations);
    ConfigManager->GetValue("FatBoundsExtentRatio", FatBoundsExtentRatio);
    ConfigManager->GetValue("PositionCorrectionBias", PositionCorrectionBias);
    ConfigManager->GetValue("WarmStartingDamping", WarmStartingDamping);
    ConfigManager->GetValue("MinConstraintLambda", MinConstraintLambda);

    // 설정값 유효성 검증
    CCDVelocityThreshold = std::max(0.1f, CCDVelocityThreshold);
    InitialCollisionCapacity = std::max(32u, InitialCollisionCapacity);
    MaxConstraintIterations = std::clamp(MaxConstraintIterations, 1u, 20u);
    FatBoundsExtentRatio = std::clamp(FatBoundsExtentRatio, 0.01f, 1.0f);
    PositionCorrectionBias = std::clamp(PositionCorrectionBias, 0.0f, 1.0f);
    WarmStartingDamping = std::clamp(WarmStartingDamping, 0.0f, 1.0f);
    MinConstraintLambda = std::max(0.1f, MinConstraintLambda);

    LOG_INFO("Collision settings loaded - CCD Threshold: %.2f, Capacity: %u, Iterations: %u",
             CCDVelocityThreshold, InitialCollisionCapacity, MaxConstraintIterations);
}

void FCollisionProcessor::UnRegisterAll()
{
    if (!CollisionTree)
        return;

    // 1. AABB 트리 완전 정리
    CollisionTree->Clear();

    // 2. 모든 컨테이너 정리
    EffectiveCollisionPairs.clear();
    PhysicsIdToNodeId.clear();
    NodeIdToPhysicsId.clear();

    LOG_INFO("All collision registrations cleared");
}

bool FCollisionProcessor::IsInitialized() const
{
    return bIsInitialized && IsInterfaceValid() &&
        Detector && ResponseCalculator && EventCalculator &&
        PositionCorrectionCalculator && CollisionTree;
}


#pragma endregion

#pragma region Data Access Layer

void FCollisionProcessor::GetCollisionShapeData(PhysicsID Id, FCollisionShapeData& OutData) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
    {
        LOG_WARNING("GetCollisionShapeData: Invalid PhysicsID %u or interface", Id);
        return;
    }

    // 형상 타입 및 기하학적 정보 (인터페이스 직접 호출)
    OutData.ShapeType = ShapeInterface->P_GetShapeType(Id);
    OutData.HalfExtent = ShapeInterface->P_GetShapeHalfExtent(Id);

    // 현재 프레임 월드 변환 (인터페이스 직접 호출)
    OutData.CurrentWorldPosition = PhysicsStateInterface->P_GetWorldPosition(Id);
    OutData.CurrentWorldRotation = PhysicsStateInterface->P_GetWorldRotationQuat(Id);

    // 이전 프레임 월드 변환 (CCD용, 인터페이스 직접 호출)
    OutData.PrevWorldPosition = PhysicsStateInterface->P_GetPrevWorldPosition(Id);
    OutData.PrevWorldRotation = PhysicsStateInterface->P_GetPrevWorldRotationQuat(Id);
}

void FCollisionProcessor::GetPhysicsParams(PhysicsID Id, FPhysicsParameters& OutParams) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
    {
        LOG_WARNING("GetPhysicsParams: Invalid PhysicsID %u or interface", Id);
        OutParams.InvMass = -1.0f; // 무효 표시
        return;
    }

    // 질량 정보 (인터페이스 직접 호출)
    OutParams.InvMass = PhysicsStateInterface->P_GetInvMass(Id);
    OutParams.InvRotationalInertia = PhysicsStateInterface->P_GetInvRotationalInertia(Id);

    // 운동 상태 (인터페이스 직접 호출)
    OutParams.Position = PhysicsStateInterface->P_GetWorldPosition(Id);
    OutParams.Velocity = PhysicsStateInterface->P_GetVelocity(Id);
    OutParams.AngularVelocity = PhysicsStateInterface->P_GetAngularVelocity(Id);
    OutParams.Rotation = PhysicsStateInterface->P_GetWorldRotationQuat(Id);

    // 물리적 속성 (인터페이스 직접 호출)
    OutParams.Restitution = PhysicsStateInterface->P_GetRestitution(Id);
    OutParams.FrictionStatic = PhysicsStateInterface->P_GetFrictionStatic(Id);
    OutParams.FrictionKinetic = PhysicsStateInterface->P_GetFrictionKinetic(Id);
}

#pragma endregion

#pragma region Effective Collision Pairs Management

void FCollisionProcessor::UpdateBroadPhasePairs(const std::vector<PhysicsID>& ActivePhysicsIDs)
{
    if (!CollisionTree || ActivePhysicsIDs.empty())
    {
        EffectiveCollisionPairs.clear();
        return;
    }

    std::unordered_set<FCollisionPair, FCollisionPairHash> NewCollisionPairs;

    // AABBTree 기반 브로드페이즈 충돌 검사
    for (PhysicsID physicsId : ActivePhysicsIDs)
    {
        if (!IsValidPhysicsID(physicsId))
            continue;

        auto nodeIt = PhysicsIdToNodeId.find(physicsId);
        if (nodeIt == PhysicsIdToNodeId.end())
            continue;

        size_t nodeId = nodeIt->second;
        const FMAABB& bounds = CollisionTree->GetBounds(nodeId);

        // 특정 AABB와 겹치는 모든 노드 찾기
        CollisionTree->QueryOverlap(bounds, [&](size_t otherNodeId) {
            // 자기 자신과의 충돌 무시
            if (nodeId == otherNodeId)
                return;

            // 다른 노드의 PhysicsID 찾기
            auto otherPhysicsIt = NodeIdToPhysicsId.find(otherNodeId);
            if (otherPhysicsIt == NodeIdToPhysicsId.end())
                return;

            PhysicsID otherPhysicsId = otherPhysicsIt->second;

            // 중복 충돌 쌍 방지 (작은 ID가 A가 되도록)
            FCollisionPair newPair(physicsId, otherPhysicsId);

            // 기존 쌍의 상태 정보 복사 (Warm Starting 지원)
            auto existingPair = EffectiveCollisionPairs.find(newPair);
            if (existingPair != EffectiveCollisionPairs.end())
            {
                newPair.bPrevCollided = existingPair->bPrevCollided;
                newPair.ConstraintsAccumulation = existingPair->ConstraintsAccumulation;
            }

            NewCollisionPairs.insert(newPair);
                                    });
    }

    // Exit 이벤트 생성: 기존 충돌 쌍 중 새로운 쌍에 없는 것들
    for (const auto& existingPair : EffectiveCollisionPairs)
    {
        if (existingPair.bPrevCollided &&
            NewCollisionPairs.find(existingPair) == NewCollisionPairs.end())
        {
            // Exit 이벤트 생성
            GenerateAndSendExitEventToPhyscis(existingPair);
        }
    }

    // 새로운 쌍들로 교체
    EffectiveCollisionPairs = std::move(NewCollisionPairs);
}

#pragma endregion

#pragma region Collision Processing Pipeline

float FCollisionProcessor::PerformNarrowphaseDetection(float DeltaTime)
{
    if (!Detector || EffectiveCollisionPairs.empty())
    {
        return 1.0f; // 충돌 없음, 전체 시간 소모
    }

    float minTimeOfImpact = 1.0f; // 정규화된 시간 (충돌 없음 기본값)

    // 브로드페이즈 충돌 쌍들에 대해 정밀 충돌 검출
    for (const auto& pair : EffectiveCollisionPairs)
    {
        // 형상 데이터 구성 (스택에서 일회성 생성)
        FCollisionShapeData shapeDataA, shapeDataB;
        GetCollisionShapeData(pair.PhysicsIdA, shapeDataA);
        GetCollisionShapeData(pair.PhysicsIdB, shapeDataB);

        // 물리 매개변수 구성 (스택에서 일회성 생성)
        FPhysicsParameters paramsA, paramsB;
        GetPhysicsParams(pair.PhysicsIdA, paramsA);
        GetPhysicsParams(pair.PhysicsIdB, paramsB);

        // CCD 판단
        bool useCCD = ShouldUseCCD(pair.PhysicsIdA) || ShouldUseCCD(pair.PhysicsIdB);

        FCollisionDetectionResult detectionResult;
        if (useCCD)
        {
            // 연속 충돌 검출
            detectionResult = Detector->DetectCollisionCCD(shapeDataA, shapeDataB, DeltaTime);
        }
        else
        {
            // 이산 충돌 검출
            detectionResult = Detector->DetectCollisionDiscrete(shapeDataA, shapeDataB);
        }

        // 충돌이 발생한 경우만 멤버 저장소에 수집
        if (detectionResult.bCollided)
        {
            // 최소 충돌 시간 업데이트
            minTimeOfImpact = std::min(minTimeOfImpact, detectionResult.NormalizedToI);

            // 결과를 멤버 저장소에 저장 (복사 최소화)
            CurrentCollidingPairs.push_back(pair);
            CurrentDetectionResults.push_back(detectionResult);
            CurrentParamsA.push_back(std::move(paramsA));
            CurrentParamsB.push_back(std::move(paramsB));
        }
    }

    return minTimeOfImpact;
}

void FCollisionProcessor::FilterCollisionsByToI(float TargetTime)
{
    if (CurrentCollidingPairs.empty() || CurrentDetectionResults.empty())
        return;

    // TargetTime 이내에 발생하는 충돌들만 필터링
    size_t writeIndex = 0;

    for (size_t readIndex = 0; readIndex < CurrentCollidingPairs.size(); ++readIndex)
    {
        const auto& result = CurrentDetectionResults[readIndex];

        if (result.NormalizedToI <= TargetTime)
        {
            // 조건을 만족하는 경우, 앞쪽으로 이동 (in-place 필터링)
            if (writeIndex != readIndex)
            {
                CurrentCollidingPairs[writeIndex] = CurrentCollidingPairs[readIndex];
                CurrentDetectionResults[writeIndex] = CurrentDetectionResults[readIndex];
                CurrentParamsA[writeIndex] = std::move(CurrentParamsA[readIndex]);
                CurrentParamsB[writeIndex] = std::move(CurrentParamsB[readIndex]);
            }
            ++writeIndex;
        }
    }

    // 벡터 크기 조정 (필터링된 크기로)
    CurrentCollidingPairs.resize(writeIndex);
    CurrentDetectionResults.resize(writeIndex);
    CurrentParamsA.resize(writeIndex);
    CurrentParamsB.resize(writeIndex);
}

void FCollisionProcessor::ApplyCollisionResponse(float DeltaTime)
{
    if (CurrentCollidingPairs.empty() || CurrentDetectionResults.empty())
        return;

    // 1. 직접 위치 보정 적용 (멤버 데이터 기반)
    ApplyDirectPositionCorrections();

    // 2. 반복적 제약 조건 해결 (멤버 데이터 기반)
    ApplyIterativeConstraintSolver();

    // 3. 충돌 상태 업데이트 (멤버 데이터 기반)
    UpdateCollisionStates();
}

void FCollisionProcessor::RequestCollisionEvents()
{
    if (!EventCalculator || !PhysicsEventDispatcher)
        return;


    // 멤버 저장소 기반으로 충돌 이벤트 생성 및 전송
    for (size_t i = 0; i < CurrentCollidingPairs.size(); ++i)
    {
        const auto& pair = CurrentCollidingPairs[i];
        const auto& result = CurrentDetectionResults[i];

        if (result.bCollided)
        {
            GenerateAndSendEventToPhysics(pair, result);
        }
    }
}

#pragma endregion

#pragma region Collision Response Implementation

void FCollisionProcessor::ApplyDirectPositionCorrections()
{
    if (CurrentCollidingPairs.empty() || !PositionCorrectionCalculator)
        return;

    // 멤버 저장소 기반으로 각 충돌 쌍에 대해 위치 보정 적용
    for (size_t i = 0; i < CurrentCollidingPairs.size(); ++i)
    {
        const auto& pair = CurrentCollidingPairs[i];
        const auto& result = CurrentDetectionResults[i];
        const auto& paramsA = CurrentParamsA[i];
        const auto& paramsB = CurrentParamsB[i];

        // AABB 겹침 비율 계산으로 보정 강도 결정
        float overlapRatio = CalculateAABBOverlapRatio(pair);
        float correctionRatio = 0.0f;

        // 겹침 정도에 따른 차등 보정
        if (overlapRatio > 0.7f)
        {
            correctionRatio = 0.45f; // 심각한 겹침
        }
        else if (overlapRatio > 0.4f)
        {
            correctionRatio = 0.2f;  // 중간 겹침
        }
        else
        {
            continue; // 경미한 겹침은 제약 조건으로만 해결
        }

        // 개별 위치 보정 처리 (인덱스 + 참조 기반)
        ProcessSinglePositionCorrection(i, paramsA, paramsB, correctionRatio);
    }
}

void FCollisionProcessor::ApplyIterativeConstraintSolver()
{
    if (CurrentCollidingPairs.empty() || !ResponseCalculator)
        return;

    int convergedCount = 0;
    const size_t totalPairs = CurrentCollidingPairs.size();

    // 반복적 제약 조건 해결 (Projected Gauss-Seidel)
    for (uint32_t iteration = 0; iteration < MaxConstraintIterations; ++iteration)
    {
        convergedCount = 0;

        for (size_t i = 0; i < totalPairs; ++i)
        {
            auto& pair = CurrentCollidingPairs[i]; 

            if (pair.bConverged)
            {
                convergedCount++;
                continue;
            }

            // 개별 제약 조건 반복 처리 (인덱스 + 참조 기반)
            ProcessSingleConstraintIteration(i, CurrentParamsA[i], CurrentParamsB[i], iteration);

            // 수렴 여부는 ProcessSingleConstraintIteration에서 판단하여 pair.bConverged 설정
            if (pair.bConverged)
            {
                convergedCount++;
            }
        }

        // 모든 쌍이 수렴했으면 조기 종료
        if (convergedCount == static_cast<int>(totalPairs))
        {
            LOG_INFO("Collision constraints converged early at iteration %u", iteration + 1);
            break;
        }
    }

    // 수렴하지 못한 쌍에 대한 경고
    if (convergedCount < static_cast<int>(totalPairs))
    {
        LOG_WARNING("Collision constraints did not fully converge: %d/%zu pairs",
                    convergedCount, totalPairs);
    }
}

void FCollisionProcessor::UpdateCollisionStates()
{
    if (CurrentCollidingPairs.empty())
        return;

    // 멤버 저장소 기반으로 충돌 상태 업데이트
    for (size_t i = 0; i < CurrentCollidingPairs.size(); ++i)
    {
        auto& pair = CurrentCollidingPairs[i]; // non-const로 상태 수정
        const auto& result = CurrentDetectionResults[i];

        // 이전 충돌 상태 업데이트
        pair.bPrevCollided = result.bCollided;

        // 수렴 상태 리셋 (다음 프레임을 위해)
        pair.bConverged = false;

        // EffectiveCollisionPairs에서도 동기화 업데이트 (mutable 멤버 직접 수정)
        auto activeIt = EffectiveCollisionPairs.find(pair);
        if (activeIt != EffectiveCollisionPairs.end())
        {
            // mutable 멤버들을 직접 수정 (erase + insert 불필요)
            activeIt->bPrevCollided = result.bCollided;
            activeIt->ConstraintsAccumulation = pair.ConstraintsAccumulation;
            activeIt->bConverged = false;
        }
    }
}

void FCollisionProcessor::ProcessSinglePositionCorrection(size_t Index,
                                                          const FPhysicsParameters& ParamsA,
                                                          const FPhysicsParameters& ParamsB,
                                                          float CorrectionRatio)
{
    if (Index >= CurrentDetectionResults.size() || !PositionCorrectionCalculator)
        return;

    const auto& pair = CurrentCollidingPairs[Index];
    const auto& result = CurrentDetectionResults[Index];

    // 질량 비례 분리 계산
    XMVECTOR correctionA, correctionB;
    PositionCorrectionCalculator->CalculateMassProportionalSeparation(
        ParamsA.InvMass, ParamsB.InvMass,
        result.PenetrationDepth,
        result.Normal,
        0.01f, // 최소 분리 거리
        correctionA, correctionB
    );

    // 현재 위치 획득 및 보정 적용
    XMVECTOR posA = PhysicsStateInterface->P_GetWorldPosition(pair.PhysicsIdA);
    XMVECTOR posB = PhysicsStateInterface->P_GetWorldPosition(pair.PhysicsIdB);

    XMVECTOR newPosA = XMVectorAdd(posA, XMVectorScale(correctionA, CorrectionRatio));
    XMVECTOR newPosB = XMVectorAdd(posB, XMVectorScale(correctionB, CorrectionRatio));

    // 물리 상태 인터페이스를 통해 위치 업데이트
    PhysicsStateInterface->P_SetWorldPosition(pair.PhysicsIdA, newPosA);
    PhysicsStateInterface->P_SetWorldPosition(pair.PhysicsIdB, newPosB);
}

void FCollisionProcessor::ProcessSingleConstraintIteration(size_t Index,
                                                           FPhysicsParameters& ParamsA,
                                                           FPhysicsParameters& ParamsB,
                                                           uint32_t Iteration)
{
    if (Index >= CurrentDetectionResults.size() || !ResponseCalculator)
        return;

    auto& pair = CurrentCollidingPairs[Index];
    const auto& result = CurrentDetectionResults[Index];

    // 이전 제약 누적값 저장 (수렴 판단용)
    FCollisionAccumulation prevAccumulation = pair.ConstraintsAccumulation;

    // 위치 편향 속도 계산 (침투 보정용)
    float biasSpeed = CalculatePositionBiasVelocity(
        result.PenetrationDepth,
        PositionCorrectionBias,
        CurrentDeltaTime,
        0.01f // Slop
    );

    // 충돌 반응 계산 (제약 조건 기반)
    FCollisionResponseResult responseResult = ResponseCalculator->CalculateCollisionResponse(
        result,
        ParamsA, ParamsB,
        pair.ConstraintsAccumulation, // 입출력 매개변수
        biasSpeed
    );

    // 충돌 반응 적용 (임펄스 기반)
    PhysicsStateInterface->P_ApplyImpulse(pair.PhysicsIdA, responseResult.NetImpulse, responseResult.ApplicationPoint);
    PhysicsStateInterface->P_ApplyImpulse(pair.PhysicsIdB, XMVectorNegate(responseResult.NetImpulse), responseResult.ApplicationPoint);

    // 수렴 확인 (제약 누적값 변화량 기준)
    if (FCollisionAccumulation::IsEqual(prevAccumulation, pair.ConstraintsAccumulation, MinConstraintLambda))
    {
        pair.bConverged = true;
    }
}
#pragma endregion

#pragma region Event Generation
void FCollisionProcessor::GenerateAndSendExitEventToPhyscis(const FCollisionPair& ExitingPair)
{
    if (!EventCalculator)
        return;

    // Exit 이벤트 생성 (이전 충돌 상태가 true였던 쌍만 처리)
    if (ExitingPair.bPrevCollided)
    {
        auto EventData = EventCalculator->GenerateExitEvent(ExitingPair.PhysicsIdA, ExitingPair.PhysicsIdB);
        PhysicsEventDispatcher->AddCollisionEvent(EventData);
    }
}

void FCollisionProcessor::GenerateAndSendEventToPhysics(const FCollisionPair& ExitingPair, const FCollisionDetectionResult& Result)
{
    auto& pair = ExitingPair;
    const auto& result = Result;

    // 이벤트 생성 (스택에서 일회성 생성)
    FPhysicsCollisionEvent collisionEvent = EventCalculator->GenerateCollisionEvent(
        result, pair.bPrevCollided, pair.PhysicsIdA, pair.PhysicsIdB);

    //이벤트 없음
    if (collisionEvent.CollisionState == ECollisionState::None)
    {
        return;
    }

    //이벤트 전송 요청
    PhysicsEventDispatcher->AddCollisionEvent(collisionEvent);
}
#pragma endregion

#pragma region Utility Functions

bool FCollisionProcessor::IsInterfaceValid() const
{
    return PhysicsStateInterface != nullptr &&
        ShapeInterface != nullptr &&
        PhysicsEventDispatcher != nullptr;
}

bool FCollisionProcessor::ShouldUseCCD(PhysicsID Id) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
        return false;

    XMVECTOR velocity = PhysicsStateInterface->P_GetVelocity(Id);
    float speed = XMVectorGetX(XMVector3Length(velocity));

    return speed > CCDVelocityThreshold;
}

float FCollisionProcessor::CalculateAABBOverlapRatio(const FCollisionPair& Pair) const
{
    // 노드 ID 찾기
    auto nodeAIt = PhysicsIdToNodeId.find(Pair.PhysicsIdA);
    auto nodeBIt = PhysicsIdToNodeId.find(Pair.PhysicsIdB);

    if (nodeAIt == PhysicsIdToNodeId.end() || nodeBIt == PhysicsIdToNodeId.end())
        return 0.0f;

    if (!CollisionTree->IsValidId(nodeAIt->second) || !CollisionTree->IsValidId(nodeBIt->second))
        return 0.0f;

    const FMAABB& boundsA = CollisionTree->GetBounds(nodeAIt->second);
    const FMAABB& boundsB = CollisionTree->GetBounds(nodeBIt->second);

    // 효율적인 XMVECTOR 기반 겹침 검사
    XMVECTOR minA = boundsA.vMin;
    XMVECTOR maxA = boundsA.vMax;
    XMVECTOR minB = boundsB.vMin;
    XMVECTOR maxB = boundsB.vMax;

    // 겹침 검사: minA > maxB 또는 minB > maxA 이면 겹치지 않음
    XMVECTOR overlapCheck1 = XMVectorGreater(minA, maxB);
    XMVECTOR overlapCheck2 = XMVectorGreater(minB, maxA);
    XMVECTOR noOverlap = XMVectorOrInt(overlapCheck1, overlapCheck2);

    // 각 축별로 겹침 여부 확인 (XMVector3AnyTrue 대신 컴포넌트 직접 확인)
    if (XMVectorGetX(noOverlap) || XMVectorGetY(noOverlap) || XMVectorGetZ(noOverlap))
    {
        return 0.0f; // AABB가 겹치지 않음
    }

    // 겹침 영역 계산
    XMVECTOR overlapMin = XMVectorMax(minA, minB);
    XMVECTOR overlapMax = XMVectorMin(maxA, maxB);
    XMVECTOR overlapSize = XMVectorSubtract(overlapMax, overlapMin);

    // 각 축의 겹침 크기 확인
    float overlapX = XMVectorGetX(overlapSize);
    float overlapY = XMVectorGetY(overlapSize);
    float overlapZ = XMVectorGetZ(overlapSize);

    if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
    {
        return 0.0f;
    }

    // 겹침 볼륨 계산
    float overlapVolume = overlapX * overlapY * overlapZ;

    // 각 AABB의 볼륨 계산
    XMVECTOR sizeA = XMVectorSubtract(maxA, minA);
    XMVECTOR sizeB = XMVectorSubtract(maxB, minB);

    float volumeA = XMVectorGetX(sizeA) * XMVectorGetY(sizeA) * XMVectorGetZ(sizeA);
    float volumeB = XMVectorGetX(sizeB) * XMVectorGetY(sizeB) * XMVectorGetZ(sizeB);

    // 기준 볼륨 결정 (두 객체 볼륨 중 작은 값)
    float referenceVolume = std::min(volumeA, volumeB);

    // 0으로 나누는 것을 방지하기 위한 최소 볼륨 보장
    constexpr float MinVolume = 0.001f; // 1cm³
    referenceVolume = std::max(referenceVolume, MinVolume);

    // 겹침 비율 계산
    float overlapRatio = overlapVolume / referenceVolume;

    // 부드러운 포화(0~1 범위)를 위한 시그모이드 함수 적용
    return std::clamp(1.0f - std::exp(-overlapRatio * 3.0f), 0.0f, 1.0f);
}

float FCollisionProcessor::CalculatePositionBiasVelocity(float PenetrationDepth, float BiasFactor, float DeltaTime, float Slop) const
{
    // 슬롭(Slop)을 초과하는 침투만 고려
    float biasPenetration = std::max(0.0f, PenetrationDepth - Slop);

    if (biasPenetration < KINDA_SMALL) // 아주 작은 값은 무시
    {
        return 0.0f;
    }
    
    // Baumgarte 안정화 항: (위치 오류 * 보정 계수) / DeltaTime  
    // 위치 보정을 나타낼 속도 편향
    return (biasPenetration * BiasFactor) / DeltaTime;
}

FMAABB FCollisionProcessor::CalculateAABBFromShape(XMVECTOR position, XMVECTOR rotation, XMVECTOR halfExtent, ECollisionShapeType shapeType) const
{
    FMAABB result;

    switch (shapeType)
    {
        case ECollisionShapeType::Sphere:
        {
            // 구체의 경우: halfExtent.x를 반지름으로 사용
            float radius = XMVectorGetX(halfExtent);
            XMVECTOR radiusVec = XMVectorReplicate(radius);

            result.vMin = (XMVectorSubtract(position, radiusVec));
            result.vMax = (XMVectorAdd(position, radiusVec));
            break;
        }
        case ECollisionShapeType::Box:
        {
            // 박스의 경우: 회전 적용된 AABB 계산
            XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

            // 회전된 박스의 각 꼭짓점을 고려한 AABB 계산
            XMVECTOR signs[8] = {
                XMVectorSet(-1, -1, -1, 0), XMVectorSet(1, -1, -1, 0),
                XMVectorSet(-1,  1, -1, 0), XMVectorSet(1,  1, -1, 0),
                XMVectorSet(-1, -1,  1, 0), XMVectorSet(1, -1,  1, 0),
                XMVectorSet(-1,  1,  1, 0), XMVectorSet(1,  1,  1, 0)
            };

            XMVECTOR minBounds = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0);
            XMVECTOR maxBounds = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0);

            // 8개 꼭짓점을 모두 변환하여 최소/최대값 계산
            for (int i = 0; i < 8; ++i)
            {
                XMVECTOR localCorner = XMVectorMultiply(halfExtent, signs[i]);
                XMVECTOR worldCorner = XMVector3Transform(localCorner, rotationMatrix);
                worldCorner = XMVectorAdd(worldCorner, position);

                minBounds = XMVectorMin(minBounds, worldCorner);
                maxBounds = XMVectorMax(maxBounds, worldCorner);
            }

            result.vMin = (minBounds);
            result.vMax = (maxBounds);
            break;
        }
        default:
        {
            LOG_ERROR("CalculateAABBFromShape: Unknown shape type %d", static_cast<int>(shapeType));
            // 기본값: 위치 중심의 작은 AABB
            XMVECTOR smallExtent = XMVectorReplicate(0.1f);
            result.vMin = (XMVectorSubtract(position, smallExtent));
            result.vMax = (XMVectorAdd(position, smallExtent));
            break;
        }
    }

    return result;
}

#pragma endregion