#include "CollisionProcessor.h"
#include "PhysicsStateInternalInterface.h"
#include "CollisionShapeInternalInterface.h"
#include "PhysicsEventDispatcherInterface.h"
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

    // 4. 인터페이스 해제
    PhysicsStateInterface = nullptr;
    ShapeInterface = nullptr;
    PhysicsEventDispatcher = nullptr;

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
    ApplyCollisionEvents();

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
            GenerateAndSendExitEvent(existingPair);
        }
    }

    // 새로운 쌍들로 교체
    EffectiveCollisionPairs = std::move(NewCollisionPairs);
}

void FCollisionProcessor::GenerateAndSendExitEvent(const FCollisionPair& ExitingPair)
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

#pragma endregion

#pragma region Utility Functions
float FCollisionProcessor::CalculatePositionBiasVelocity(float PenetrationDepth, float BiasFactor, float DeltaTime, float Slop) const
{
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

#pragma endregion