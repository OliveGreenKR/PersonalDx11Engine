#include "CollisionProcessor.h"
#include "PhysicsStateInternalInterface.h"
#include "CollisionShapeInternalInterface.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventCalculator.h"
#include "CollisionPositionCorrectionCalculator.h"
#include "DynamicAABBTree.h"
#include "ConfigReadManager.h"
#include "Debug.h"

#pragma region Constructor and Initialization

FCollisionProcessor::~FCollisionProcessor()
{
    Release();
}

bool FCollisionProcessor::Initialize(IPhysicsStateInternal* PhysicsStateInterface, ICollisionShapeInternal* ShapeInterface)
{
    // 1. 입력 검증
    if (!PhysicsStateInterface || !ShapeInterface)
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
        ActiveCollisionPairs.reserve(InitialCollisionCapacity);
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
    ActiveCollisionPairs.clear();
    PhysicsIdToNodeId.clear();
    NodeIdToPhysicsId.clear();

    // 4. 인터페이스 해제
    PhysicsStateInterface = nullptr;
    ShapeInterface = nullptr;

    // 5. 초기화 상태 해제
    bIsInitialized = false;

    LOG_INFO("FCollisionProcessor released successfully");
}

#pragma endregion

#pragma region Public Interface (PhysicsSystem 전용)

float FCollisionProcessor::ProcessCollisions(const std::vector<PhysicsID>& ActivePhysicsIDs, float DeltaTime)
{
    // 1. 초기화 상태 검증
    if (!IsInitialized() || DeltaTime <= 0.0f)
    {
        LOG_WARNING("FCollisionProcessor::ProcessCollisions - Invalid state or delta time");
        return 1.0f; // 전체 시간 소모
    }

    // 2. 공간 분할 트리 업데이트 (활성 PhysicsID 기반)
    UpdateSpatialPartitioning(ActivePhysicsIDs);

    // 3. 브로드페이즈 충돌 쌍 업데이트
    UpdateCollisionPairs(ActivePhysicsIDs);

    // 4. 실제 충돌 처리 파이프라인 실행
    std::vector<FCollisionPair> CollidingPairs;
    std::vector<FCollisionDetectionResult> DetectionResults;

    // 5. 정밀 충돌 감지 및 최소 ToI 계산
    float MinTimeOfImpact = PerformNarrowphaseDetection(ActivePhysicsIDs, DeltaTime,
                                                        CollidingPairs, DetectionResults);

    // 6. 성능 최적화: 최소 충돌 시간 +2% 까지의 충돌들 일괄 처리
    float TargetTime = MinTimeOfImpact + 0.02f; // 2% 여유분
    FilterCollisionsByTime(CollidingPairs, DetectionResults, TargetTime);

    // 7. 충돌 반응 처리 (제약 조건 + 위치 보정)
    ProcessCollisionResponse(CollidingPairs, DetectionResults, DeltaTime);

    // 8. 충돌 이벤트 생성 (PhysicsSystem에 전달용)
    GenerateCollisionEvents(CollidingPairs, DetectionResults);

    // 9. 정규화된 시뮬레이션 시간 반환 (0.0~1.0)
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

        // 현재 월드 변환 정보 획득 (인터페이스를 통해)
        XMVECTOR currentPos = GetCurrentWorldPosition(physicsId);
        XMVECTOR currentRot = GetCurrentWorldRotation(physicsId);
        XMVECTOR shapeExtent = ShapeInterface->P_GetShapeHalfExtent(physicsId);
        ECollisionShapeType shapeType = ShapeInterface->P_GetShapeType(physicsId);

        // AABB 계산 (형상 + 변환 적용)
        FMAABB currentAABB = CalculateAABBFromShape(currentPos, currentRot, shapeExtent, shapeType);

        auto nodeIt = PhysicsIdToNodeId.find(physicsId);
        if (nodeIt != PhysicsIdToNodeId.end())
        {
            // 4. 기존 등록된 경우: 노드 AABB 업데이트 (Bouds만 업데이트)
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
    ActiveCollisionPairs.clear();
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

    // 형상 타입 및 기하학적 정보
    OutData.ShapeType = ShapeInterface->P_GetShapeType(Id);
    OutData.HalfExtent = ShapeInterface->P_GetShapeHalfExtent(Id);

    // 현재 프레임 월드 변환
    OutData.CurrentWorldPosition = GetCurrentWorldPosition(Id);
    OutData.CurrentWorldRotation = GetCurrentWorldRotation(Id);

    // 이전 프레임 월드 변환 (CCD용)
    OutData.PrevWorldPosition = GetPrevWorldPosition(Id);
    OutData.PrevWorldRotation = GetPrevWorldRotation(Id);
}

void FCollisionProcessor::GetPhysicsParams(PhysicsID Id, FPhysicsParameters& OutParams) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
    {
        LOG_WARNING("GetPhysicsParams: Invalid PhysicsID %u or interface", Id);
        OutParams.InvMass = -1.0f; // 무효 표시
        return;
    }

    // 질량 정보
    OutParams.InvMass = PhysicsStateInterface->P_GetInvMass(Id);
    OutParams.InvRotationalInertia = PhysicsStateInterface->P_GetInvRotationalInertia(Id);

    // 운동 상태
    OutParams.Position = PhysicsStateInterface->P_GetWorldPosition(Id);
    OutParams.Velocity = PhysicsStateInterface->P_GetVelocity(Id);
    OutParams.AngularVelocity = PhysicsStateInterface->P_GetAngularVelocity(Id);
    OutParams.Rotation = PhysicsStateInterface->P_GetWorldRotationQuat(Id);

    // 물리적 속성
    OutParams.Restitution = PhysicsStateInterface->P_GetRestitution(Id);
    OutParams.FrictionStatic = PhysicsStateInterface->P_GetFrictionStatic(Id);
    OutParams.FrictionKinetic = PhysicsStateInterface->P_GetFrictionKinetic(Id);
}

XMVECTOR FCollisionProcessor::GetCurrentWorldPosition(PhysicsID Id) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
        return XMVectorZero();

    return PhysicsStateInterface->P_GetWorldPosition(Id);
}

XMVECTOR FCollisionProcessor::GetCurrentWorldRotation(PhysicsID Id) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
        return XMQuaternionIdentity();

    return PhysicsStateInterface->P_GetWorldRotationQuat(Id);
}

XMVECTOR FCollisionProcessor::GetCurrentWorldScale(PhysicsID Id) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
        return XMVectorSplatOne(); // (1,1,1,1)

    return PhysicsStateInterface->P_GetWorldScale(Id);
}

XMVECTOR FCollisionProcessor::GetPrevWorldPosition(PhysicsID Id) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
        return XMVectorZero();

    return ShapeInterface->P_GetPrevWorldPosition(Id);
}

XMVECTOR FCollisionProcessor::GetPrevWorldRotation(PhysicsID Id) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
        return XMQuaternionIdentity();

    return ShapeInterface->P_GetPrevWorldRotationQuat(Id);
}

XMVECTOR FCollisionProcessor::GetPrevWorldScale(PhysicsID Id) const
{
    if (!IsValidPhysicsID(Id) || !IsInterfaceValid())
        return XMVectorSplatOne(); // (1,1,1,1)

    return ShapeInterface->P_GetPrevWorldScale(Id);
}

#pragma endregion


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
            XMVECTOR corners[8];
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


#pragma region Collision Processing Pipeline

void FCollisionProcessor::UpdateCollisionPairs(const std::vector<PhysicsID>& ActivePhysicsIDs)
{
    if (!CollisionTree || ActivePhysicsIDs.empty())
    {
        ActiveCollisionPairs.clear();
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

            // 기존 쌍의 상태 정보 복사
            auto existingPair = ActiveCollisionPairs.find(newPair);
            if (existingPair != ActiveCollisionPairs.end())
            {
                newPair.bPrevCollided = existingPair->bPrevCollided;
                newPair.PrevConstraints = existingPair->PrevConstraints;
            }

            NewCollisionPairs.insert(newPair);
                                    });
    }

    // Exit 이벤트 생성: 기존 충돌 쌍 중 새로운 쌍에 없는 것들
    for (const auto& existingPair : ActiveCollisionPairs)
    {
        if (existingPair.bPrevCollided &&
            NewCollisionPairs.find(existingPair) == NewCollisionPairs.end())
        {
            // Exit 이벤트 생성
            GenerateCollisionExitEvent(existingPair);
        }
    }

    //새로운 쌍들로 교체
    ActiveCollisionPairs = std::move(NewCollisionPairs);
}

float FCollisionProcessor::PerformNarrowphaseDetection(const std::vector<PhysicsID>& ActivePhysicsIDs,
                                                       float DeltaTime,
                                                       std::vector<FCollisionPair>& OutCollidingPairs,
                                                       std::vector<FCollisionDetectionResult>& OutDetectionResults)
{
    OutCollidingPairs.clear();
    OutDetectionResults.clear();

    float minTimeOfImpact = 1.0f; // 정규화된 시간 (충돌 없음 기본값)

    // 브로드페이즈 충돌 쌍들에 대해 정밀 충돌 검출
    for (const auto& pair : ActiveCollisionPairs)
    {
        // 형상 데이터 구성
        FCollisionShapeData shapeDataA, shapeDataB;
        GetCollisionShapeData(pair.PhysicsIdA, shapeDataA);
        GetCollisionShapeData(pair.PhysicsIdB, shapeDataB);

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

        // 충돌이 발생한 경우만 수집
        if (detectionResult.bCollided)
        {
            // 최소 충돌 시간 업데이트
            minTimeOfImpact = std::min(minTimeOfImpact, detectionResult.NormalizedToI);

            // 결과 저장
            OutCollidingPairs.push_back(pair);
            OutDetectionResults.push_back(detectionResult);
        }
    }

    return minTimeOfImpact;
}

void FCollisionProcessor::FilterCollisionsByTime(std::vector<FCollisionPair>& CollidingPairs,
                                                 std::vector<FCollisionDetectionResult>& DetectionResults,
                                                 float TargetTime)
{
    if (CollidingPairs.empty() || DetectionResults.empty())
        return;

    std::vector<FCollisionPair> filteredPairs;
    std::vector<FCollisionDetectionResult> filteredResults;

    // TargetTime 이내에 발생하는 충돌들만 필터링
    for (size_t i = 0; i < CollidingPairs.size(); ++i)
    {
        const auto& result = DetectionResults[i];
        if (result.NormalizedToI <= TargetTime)
        {
            filteredPairs.push_back(CollidingPairs[i]);
            filteredResults.push_back(result);
        }
    }

    // 원본 벡터 교체
    CollidingPairs = std::move(filteredPairs);
    DetectionResults = std::move(filteredResults);
}

void FCollisionProcessor::ProcessCollisionResponse(const std::vector<FCollisionPair>& CollidingPairs,
                                                   const std::vector<FCollisionDetectionResult>& DetectionResults,
                                                   float DeltaTime)
{
    if (CollidingPairs.empty() || DetectionResults.empty())
        return;

    // 1. 직접 위치 보정 (겹침 비율 기반)
    ApplyDirectPositionCorrections(CollidingPairs, DetectionResults);

    // 2. 반복적 제약 조건 해결
    ApplyIterativeConstraintSolver(CollidingPairs, DetectionResults, DeltaTime);

    // 3. 충돌 상태 업데이트
    UpdateCollisionStates(CollidingPairs, DetectionResults);
}

void FCollisionProcessor::GenerateCollisionEvents(const std::vector<FCollisionPair>& CollidingPairs,
                                                  const std::vector<FCollisionDetectionResult>& DetectionResults)
{
    for (size_t i = 0; i < CollidingPairs.size(); ++i)
    {
        const auto& pair = CollidingPairs[i];
        const auto& result = DetectionResults[i];

        if (result.bCollided)
        {
            GenerateCollisionEvent(pair, result);
        }
    }
}

void FCollisionProcessor::ApplyDirectPositionCorrections(const std::vector<FCollisionPair>& CollidingPairs,
                                                         const std::vector<FCollisionDetectionResult>& DetectionResults)
{
    for (size_t i = 0; i < CollidingPairs.size(); ++i)
    {
        const auto& pair = CollidingPairs[i];
        const auto& result = DetectionResults[i];

        // AABB 겹침 비율 계산
        float overlapRatio = CalculateAABBOverlapRatio(pair);

        // 겹침 정도에 따른 위치 보정
        if (overlapRatio > 0.7f)
        {
            ApplyDirectPositionCorrection(pair, result, 0.45f);
        }
        else if (overlapRatio > 0.4f)
        {
            ApplyDirectPositionCorrection(pair, result, 0.2f);
        }
    }
}

void FCollisionProcessor::ApplyIterativeConstraintSolver(const std::vector<FCollisionPair>& CollidingPairs,
                                                         const std::vector<FCollisionDetectionResult>& DetectionResults,
                                                         float EffectiveDeltaTime)
{
    int ConvergedCount = 0;

    // 반복적 제약 조건 해결 (Projected Gauss-Seidel)
    for (std::uint32_t iteration = 0; iteration < MaxConstraintIterations; ++iteration)
    {
        for (size_t i = 0; i < CollidingPairs.size(); ++i)
        {
            auto& pair = const_cast<FCollisionPair&>(CollidingPairs[i]);
            const auto& result = DetectionResults[i];

            if (pair.bConverged)
            {
                continue;
            }

            // 제약 조건 기반 충돌 반응 적용
            ApplyCollisionResponseByConstraints(pair, result, EffectiveDeltaTime);

            if (pair.bConverged)
            {
                ConvergedCount++;
            }
        }

        // 모든 쌍이 수렴했으면 조기 종료
        if (ConvergedCount == CollidingPairs.size())
        {
            break;
        }
    }
}

void FCollisionProcessor::UpdateCollisionStates(const std::vector<FCollisionPair>& CollidingPairs,
                                                const std::vector<FCollisionDetectionResult>& DetectionResults)
{
    for (size_t i = 0; i < CollidingPairs.size(); ++i)
    {
        auto& pair = const_cast<FCollisionPair&>(CollidingPairs[i]);
        const auto& result = DetectionResults[i];

        // 이전 충돌 상태 업데이트
        pair.bPrevCollided = result.bCollided;

        // 수렴 상태 리셋 (다음 프레임을 위해)
        pair.bConverged = false;

        // ActiveCollisionPairs에서도 업데이트
        auto activeIt = ActiveCollisionPairs.find(pair);
        if (activeIt != ActiveCollisionPairs.end())
        {
            const_cast<FCollisionPair&>(*activeIt).bPrevCollided = result.bCollided;
            const_cast<FCollisionPair&>(*activeIt).PrevConstraints = pair.PrevConstraints;
            const_cast<FCollisionPair&>(*activeIt).bConverged = false;
        }
    }
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

    // AABB가 겹치지 않으면 0 반환
    if (!boundsA.IsOverlapping(boundsB))
        return 0.0f;

    // 겹침 볼륨 계산
    Vector3 minA, maxA, minB, maxB;
    boundsA.GetMinV(minA);
    boundsA.GetMaxV(maxA);
    boundsB.GetMinV(minB);
    boundsB.GetMaxV(maxB);

    // 겹침 영역 계산
    Vector3 overlapMin = Vector3::Max(minA, minB);
    Vector3 overlapMax = Vector3::Min(maxA, maxB);
    Vector3 overlapSize = overlapMax - overlapMin;

    // 겹침이 없으면 0 반환
    if (overlapSize.x <= 0.0f || overlapSize.y <= 0.0f || overlapSize.z <= 0.0f)
        return 0.0f;

    float overlapVolume = overlapSize.x * overlapSize.y * overlapSize.z;

    // 더 작은 객체의 볼륨을 기준으로 비율 계산
    Vector3 sizeA = maxA - minA;
    Vector3 sizeB = maxB - minB;
    float volumeA = sizeA.x * sizeA.y * sizeA.z;
    float volumeB = sizeB.x * sizeB.y * sizeB.z;
    float referenceVolume = std::min(volumeA, volumeB);

    // 최소 볼륨 보장
    constexpr float MinVolume = 0.001f; // 1cm³
    referenceVolume = std::max(referenceVolume, MinVolume);

    // 겹침 비율 계산 및 정규화
    float overlapRatio = overlapVolume / referenceVolume;

    // Sigmoid 함수로 부드러운 포화 (0~1 범위)
    return std::clamp(1.0f - std::exp(-overlapRatio * 3.0f), 0.0f, 1.0f);
}

#pragma endregion