#include "PhysicsStateSoA.h"
#include "Debug.h"

// 생성자
PhysicsStateArrays::PhysicsStateArrays(size_t InitialSize)
{
    if (InitialSize == 0)
    {
        LOG_FUNC_CALL("[Warning] PhysicsStateArrays initialized with size 0");
        InitialSize = 1;
    }

    ResizeAllVectors(InitialSize);
    ActiveFlags.resize(InitialSize, false);
    AllocatedFlags.resize(InitialSize, false);

    // 첫 번째 슬롯(인덱스 0)은 더미로 사용 (INVALID_ID 대응)
    if (InitialSize > 0)
    {
        InitializeSlot(INVALID_IDX);
        // 더미 슬롯은 할당되지 않은 상태로 유지
        AllocatedFlags[INVALID_IDX] = false;
        ActiveFlags[INVALID_IDX] = false;
    }

    LOG_FUNC_CALL("[Info] PhysicsStateArrays initialized with size: %zu", InitialSize);
}

// === 객체 생명주기 관리 ===

// 새 슬롯 할당
SoAID PhysicsStateArrays::AllocateSlot()
{
    // 공간이 부족하면 확장 시도
    if (AllocatedCount >= Size())
    {
        if (!TryResize(Size() * 2))
        {
            LOG_FUNC_CALL("[Error] Failed to resize PhysicsStateArrays");
            return INVALID_ID;
        }
    }

    SoAID NewId;
    SoAIdx NewIndex;

    // 재사용 가능한 ID가 있으면 사용
    if (!ReusableIDs.empty())
    {
        NewId = ReusableIDs.back();
        ReusableIDs.pop_back();

        // 방어적 프로그래밍: 매핑 무결성 검증
        auto It = IdToIdx.find(NewId);
        if (It == IdToIdx.end())
        {
            LOG_FUNC_CALL("[Error] Data corruption: FreeID %u not found in IdToIdx", NewId);
            return INVALID_ID;
        }

        NewIndex = It->second;

        // 방어적 프로그래밍: 이미 할당된 슬롯 재할당 방지
        if (AllocatedFlags[NewIndex])
        {
            LOG_FUNC_CALL("[Error] Data corruption: Attempting to reuse already allocated slot %u", NewIndex);
            return INVALID_ID;
        }

        // 재할당: 기존 매핑 유지, 상태만 변경
        AllocatedFlags[NewIndex] = true;
        ActiveFlags[NewIndex] = true;

        // 해제 카운트 감소
        DeallocatedCount--;

        LOG_FUNC_CALL("[Info] Reused slot - ID: %u, Index: %u", NewId, NewIndex);
    }
    else
    {
        // 새 ID 생성
        NewId = NextNewId++;

        // ID 오버플로우 검사
        if (NewId == INVALID_ID)
        {
            LOG_FUNC_CALL("[Error] ID overflow in PhysicsStateArrays");
            return INVALID_ID;
        }

        // 새 슬롯 할당 (연속적으로 배치)
        NewIndex = AllocatedCount++;

        // 쌍방향 매핑 등록
        IdToIdx[NewId] = NewIndex;
        IdxToId[NewIndex] = NewId;

        // 슬롯 초기화 및 활성화
        InitializeSlot(NewIndex);
        AllocatedFlags[NewIndex] = true;
        ActiveFlags[NewIndex] = true;

        LOG_FUNC_CALL("[Info] Allocated new slot - ID: %u, Index: %u", NewId, NewIndex);
    }

    return NewId;
}

// 슬롯 해제 - 재사용가능한 슬롯으로 만듦
void PhysicsStateArrays::DeallocateSlot(SoAID Id)
{
    if (Id == INVALID_ID)
    {
        LOG_FUNC_CALL("[Warning] Attempting to deallocate INVALID_ID");
        return;
    }

    auto It = IdToIdx.find(Id);
    if (It == IdToIdx.end())
    {
        LOG_FUNC_CALL("[Warning] Attempting to deallocate non-existent ID: %u", Id);
        return;
    }

    SoAIdx Index = It->second;

    // 인덱스 유효성 검사
    if (Index >= AllocatedCount)
    {
        LOG_FUNC_CALL("[Error] Invalid index for ID %u: %u >= %u", Id, Index, AllocatedCount);
        return;
    }

    // 이미 해제된 슬롯 재해제 방지
    if (!AllocatedFlags[Index])
    {
        LOG_FUNC_CALL("[Warning] Attempting to deallocate already freed ID: %u", Id);
        return;
    }

    // 슬롯 해제: 매핑은 유지, 상태만 변경
    AllocatedFlags[Index] = false;
    ActiveFlags[Index] = false;

    // ID를 재사용 풀에 추가
    ReusableIDs.push_back(Id);

    // 해제된 객체 수 증가
    DeallocatedCount++;

    LOG_FUNC_CALL("[Info] Deallocated slot ID: %u, Index: %u", Id, Index);

    // 압축 필요성 검사
    CompactIfNeeded();
}

// 배열 크기 변경
bool PhysicsStateArrays::TryResize(uint32_t NewSize)
{
    // 현재 할당된 크기보다 작게 축소 시도 시 거부
    if (NewSize < AllocatedCount)
    {
        LOG_FUNC_CALL("[Warning] Cannot resize below allocated count: %u < %u", NewSize, AllocatedCount);
        return false;
    }

    // 이미 충분한 크기인 경우
    if (NewSize <= Size())
    {
        LOG_FUNC_CALL("[Info] Resize skipped - already sufficient size: %u <= %zu", NewSize, Size());
        return true;
    }

    try
    {
        size_t OldSize = Size();

        // 모든 SoA 벡터들 크기 조정
        ResizeAllVectors(NewSize);

        // 플래그 벡터들 크기 조정
        ActiveFlags.resize(NewSize, false);
        AllocatedFlags.resize(NewSize, false);

        LOG_FUNC_CALL("[Info] PhysicsStateArrays resized from %zu to %u", OldSize, NewSize);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_FUNC_CALL("[Error] Failed to resize PhysicsStateArrays: %s", e.what());
        return false;
    }
}

// 압축 필요 시 수행
void PhysicsStateArrays::CompactIfNeeded()
{
    if (NeedsCompaction())
    {
        LOG_FUNC_CALL("[Info] Auto-compaction triggered: %u/%u deallocated objects (%.1f%%)",
                      DeallocatedCount, AllocatedCount,
                      100.0f * DeallocatedCount / AllocatedCount);
        PerformCompaction();
    }
}

// 명시적 압축 수행
void PhysicsStateArrays::ForceCompact()
{
    if (DeallocatedCount > 0)
    {
        LOG_FUNC_CALL("[Info] Force compaction requested: %u deallocated objects", DeallocatedCount);
        PerformCompaction();
    }
    else
    {
        LOG_FUNC_CALL("[Info] Force compaction skipped: no deallocated objects");
    }
}

// === 활성화 상태 관리 ===

// 객체 비활성화
void PhysicsStateArrays::DeactivateObject(SoAID Id)
{
    if (Id == INVALID_ID)
    {
        LOG_FUNC_CALL("[Warning] Attempting to deactivate INVALID_ID");
        return;
    }

    auto It = IdToIdx.find(Id);
    if (It == IdToIdx.end())
    {
        LOG_FUNC_CALL("[Warning] Attempting to deactivate non-existent ID: %u", Id);
        return;
    }

    SoAIdx Index = It->second;

    // 인덱스 유효성 및 할당 상태 검사
    if (!IsValidAllocatedIndex(Index))
    {
        LOG_FUNC_CALL("[Warning] Attempting to deactivate invalid or deallocated ID: %u", Id);
        return;
    }

    if (!ActiveFlags[Index])
    {
        LOG_FUNC_CALL("[Info] ID %u is already inactive", Id);
        return;
    }

    ActiveFlags[Index] = false;
    LOG_FUNC_CALL("[Info] Deactivated object ID: %u", Id);
}

// 객체 활성화
void PhysicsStateArrays::ActivateObject(SoAID Id)
{
    if (Id == INVALID_ID)
    {
        LOG_FUNC_CALL("[Warning] Attempting to activate INVALID_ID");
        return;
    }

    auto It = IdToIdx.find(Id);
    if (It == IdToIdx.end())
    {
        LOG_FUNC_CALL("[Warning] Attempting to activate non-existent ID: %u", Id);
        return;
    }

    SoAIdx Index = It->second;

    // 인덱스 유효성 및 할당 상태 검사
    if (!IsValidAllocatedIndex(Index))
    {
        LOG_FUNC_CALL("[Warning] Attempting to activate invalid or deallocated ID: %u", Id);
        return;
    }

    if (ActiveFlags[Index])
    {
        LOG_FUNC_CALL("[Info] ID %u is already active", Id);
        return;
    }

    ActiveFlags[Index] = true;
    LOG_FUNC_CALL("[Info] Activated object ID: %u", Id);
}

// === 상태 조회 ===

// ID가 할당된 유효한 슬롯인지 확인
bool PhysicsStateArrays::IsAllocatedSlot(SoAID Id) const
{
    if (Id == INVALID_ID)
    {
        return false;
    }

    auto It = IdToIdx.find(Id);
    if (It == IdToIdx.end())
    {
        return false;
    }

    SoAIdx Index = It->second;
    return IsValidAllocatedIndex(Index);
}

// ID 객체가 활성 상태인지 확인 (할당되고 활성화된 경우만 true)
bool PhysicsStateArrays::IsActiveObject(SoAID Id) const
{
    if (!IsAllocatedSlot(Id))
    {
        return false;
    }

    SoAIdx Index = GetIndex(Id);
    return ActiveFlags[Index];
}

// 활성 객체 수 계산
uint32_t PhysicsStateArrays::GetActiveObjectCount() const
{
    uint32_t ActiveCount = 0;

    // 할당된 범위 내에서만 순회
    for (uint32_t i = 0; i < AllocatedCount; ++i)
    {
        if (AllocatedFlags[i] && ActiveFlags[i])
        {
            ActiveCount++;
        }
    }

    return ActiveCount;
}

// 압축이 필요한지 확인
bool PhysicsStateArrays::NeedsCompaction() const
{
    if (AllocatedCount == 0 || DeallocatedCount == 0)
    {
        return false;
    }

    float DeallocatedRatio = static_cast<float>(DeallocatedCount) / static_cast<float>(AllocatedCount);
    return DeallocatedRatio >= COMPACTION_THRESHOLD;
}

// === 내부 헬퍼 함수들 ===

// ID를 내부 인덱스로 변환 (할당된 ID만, 내부 전용)
SoAIdx PhysicsStateArrays::GetIndex(SoAID Id) const
{
    auto It = IdToIdx.find(Id);
    if (It == IdToIdx.end())
    {
        LOG_FUNC_CALL("[Error] GetIndex called with non-existent ID: %u", Id);
        return INVALID_IDX;
    }
    return It->second;
}

// 인덱스가 할당된 범위 내에 있고 실제로 할당되었는지 확인
bool PhysicsStateArrays::IsValidAllocatedIndex(SoAIdx Index) const
{
    return Index < AllocatedCount && AllocatedFlags[Index];
}

// 슬롯을 기본값으로 초기화
void PhysicsStateArrays::InitializeSlot(SoAIdx Index)
{
    if (Index >= Size())
    {
        LOG_FUNC_CALL("[Error] InitializeSlot: Index %u out of bounds (%zu)", Index, Size());
        return;
    }

    // 운동 상태 초기화
    Velocities[Index] = XMVectorZero();
    AngularVelocities[Index] = XMVectorZero();
    AccumulatedForces[Index] = XMVectorZero();
    AccumulatedTorques[Index] = XMVectorZero();

    // 트랜스폼 정보 초기화
    WorldPosition[Index] = XMVectorZero();
    WorldScale[Index] = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);  // 기본 스케일 1
    WorldRotationQuat[Index] = XMQuaternionIdentity();

    // 물리 속성 초기화
    InvRotationalInertias[Index] = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);  // 기본 관성
    InvMasses[Index] = 1.0f;              // 기본 역질량
    FrictionKinetics[Index] = 0.3f;       // 기본 운동 마찰
    FrictionStatics[Index] = 0.5f;        // 기본 정지 마찰
    Restitutions[Index] = 0.2f;           // 기본 반발 계수

    // 제한 및 설정 초기화
    MaxSpeeds[Index] = 3.0f * ONE_METER;            // 기본 최대 속도
    MaxAngularSpeeds[Index] = XM_PIDIV2;      // 기본 최대 각속도
    GravityScales[Index] = 9.81f;          // 기본 중력 스케일
    PhysicsTypes[Index] = EPhysicsType::Dynamic;  // 기본 동적 타입
    PhysicsFlags[Index] = 0;              // 플래그 초기화
}

// 모든 SoA 벡터들을 동시에 크기 조정
void PhysicsStateArrays::ResizeAllVectors(uint32_t NewSize)
{
    try
    {
        // 운동 상태 벡터들 크기 조정
        Velocities.resize(NewSize);
        AngularVelocities.resize(NewSize);
        AccumulatedForces.resize(NewSize);
        AccumulatedTorques.resize(NewSize);

        // 트랜스폼 정보 벡터들 크기 조정
        WorldPosition.resize(NewSize);
        WorldScale.resize(NewSize);
        WorldRotationQuat.resize(NewSize);

        // 물리 속성 벡터들 크기 조정
        InvRotationalInertias.resize(NewSize);
        InvMasses.resize(NewSize);
        FrictionKinetics.resize(NewSize);
        FrictionStatics.resize(NewSize);
        Restitutions.resize(NewSize);

        // 제한 및 설정 벡터들 크기 조정
        MaxSpeeds.resize(NewSize);
        MaxAngularSpeeds.resize(NewSize);
        GravityScales.resize(NewSize);
        PhysicsTypes.resize(NewSize);
        PhysicsFlags.resize(NewSize);
    }
    catch (const std::exception& e)
    {
        LOG_FUNC_CALL("[Error] Failed to resize vectors: %s", e.what());
        throw;  // 예외 재전파
    }
}

// === 압축 관련 함수들 ===

// 압축 실행: 유효 객체들을 앞쪽으로 이동, FreeIDs 초기화
void PhysicsStateArrays::PerformCompaction()
{
    if (DeallocatedCount == 0)
    {
        LOG_FUNC_CALL("[Info] Compaction skipped: no deallocated objects");
        return;
    }

    uint32_t WriteIndex = 0;  // 압축된 배열의 쓰기 위치
    uint32_t ValidObjectCount = 0;

    LOG_FUNC_CALL("[Info] Starting compaction: %u allocated, %u deallocated",
                  AllocatedCount, DeallocatedCount);

    // 1단계: 유효한 객체들을 앞쪽으로 압축
    for (uint32_t ReadIndex = 0; ReadIndex < AllocatedCount; ++ReadIndex)
    {
        // 할당된 슬롯만 처리
        if (AllocatedFlags[ReadIndex])
        {
            // 자기 자리가 아니면 이동 필요
            if (ReadIndex != WriteIndex)
            {
                // 데이터 이동
                MoveSlotData(ReadIndex, WriteIndex);

                // 매핑 업데이트
                UpdateMappingAfterMove(ReadIndex, WriteIndex);

                // 이전 위치 정리
                AllocatedFlags[ReadIndex] = false;
                ActiveFlags[ReadIndex] = false;
            }

            WriteIndex++;
            ValidObjectCount++;
        }
    }

    // 2단계: 압축 후 상태 업데이트
    AllocatedCount = WriteIndex;
    DeallocatedCount = 0;

    // 4단계: 재사용 ID 정리
    RemoveInvalidIDs(ReusableIDs);

    // 재사용 아이디 초기화
    ReusableIDs.clear();

    LOG_FUNC_CALL("[Info] Compaction completed: %u valid objects compacted, FreeIDs cleared",
                  ValidObjectCount);
}

// 슬롯 데이터를 한 위치에서 다른 위치로 이동
void PhysicsStateArrays::MoveSlotData(SoAIdx FromIndex, SoAIdx ToIndex)
{
    if (FromIndex >= Size() || ToIndex >= Size())
    {
        LOG_FUNC_CALL("[Error] MoveSlotData: Invalid indices From=%u, To=%u, Size=%zu",
                      FromIndex, ToIndex, Size());
        return;
    }

    if (FromIndex == ToIndex)
    {
        return;  // 같은 위치로 이동 불필요
    }

    // 운동 상태 이동
    Velocities[ToIndex] = Velocities[FromIndex];
    AngularVelocities[ToIndex] = AngularVelocities[FromIndex];
    AccumulatedForces[ToIndex] = AccumulatedForces[FromIndex];
    AccumulatedTorques[ToIndex] = AccumulatedTorques[FromIndex];

    // 트랜스폼 정보 이동
    WorldPosition[ToIndex] = WorldPosition[FromIndex];
    WorldScale[ToIndex] = WorldScale[FromIndex];
    WorldRotationQuat[ToIndex] = WorldRotationQuat[FromIndex];

    // 물리 속성 이동
    InvRotationalInertias[ToIndex] = InvRotationalInertias[FromIndex];
    InvMasses[ToIndex] = InvMasses[FromIndex];
    FrictionKinetics[ToIndex] = FrictionKinetics[FromIndex];
    FrictionStatics[ToIndex] = FrictionStatics[FromIndex];
    Restitutions[ToIndex] = Restitutions[FromIndex];

    // 제한 및 설정 이동
    MaxSpeeds[ToIndex] = MaxSpeeds[FromIndex];
    MaxAngularSpeeds[ToIndex] = MaxAngularSpeeds[FromIndex];
    GravityScales[ToIndex] = GravityScales[FromIndex];
    PhysicsTypes[ToIndex] = PhysicsTypes[FromIndex];
    PhysicsFlags[ToIndex] = PhysicsFlags[FromIndex];

    // 상태 플래그 이동
    ActiveFlags[ToIndex] = ActiveFlags[FromIndex];
    AllocatedFlags[ToIndex] = AllocatedFlags[FromIndex];
}

// 두 슬롯의 데이터를 교환
void PhysicsStateArrays::SwapSlotData(SoAIdx Index1, SoAIdx Index2)
{
    if (Index1 >= Size() || Index2 >= Size())
    {
        LOG_FUNC_CALL("[Error] SwapSlotData: Invalid indices %u, %u, Size=%zu",
                      Index1, Index2, Size());
        return;
    }

    if (Index1 == Index2)
    {
        return;  // 같은 위치 교환 불필요
    }

    // 운동 상태 교환
    std::swap(Velocities[Index1], Velocities[Index2]);
    std::swap(AngularVelocities[Index1], AngularVelocities[Index2]);
    std::swap(AccumulatedForces[Index1], AccumulatedForces[Index2]);
    std::swap(AccumulatedTorques[Index1], AccumulatedTorques[Index2]);

    // 트랜스폼 정보 교환
    std::swap(WorldPosition[Index1], WorldPosition[Index2]);
    std::swap(WorldScale[Index1], WorldScale[Index2]);
    std::swap(WorldRotationQuat[Index1], WorldRotationQuat[Index2]);

    // 물리 속성 교환
    std::swap(InvRotationalInertias[Index1], InvRotationalInertias[Index2]);
    std::swap(InvMasses[Index1], InvMasses[Index2]);
    std::swap(FrictionKinetics[Index1], FrictionKinetics[Index2]);
    std::swap(FrictionStatics[Index1], FrictionStatics[Index2]);
    std::swap(Restitutions[Index1], Restitutions[Index2]);

    // 제한 및 설정 교환
    std::swap(MaxSpeeds[Index1], MaxSpeeds[Index2]);
    std::swap(MaxAngularSpeeds[Index1], MaxAngularSpeeds[Index2]);
    std::swap(GravityScales[Index1], GravityScales[Index2]);
    std::swap(PhysicsTypes[Index1], PhysicsTypes[Index2]);
    std::swap(PhysicsFlags[Index1], PhysicsFlags[Index2]);

    // 상태 플래그 교환
    bool tempActive = ActiveFlags[Index1];
    ActiveFlags[Index1] = ActiveFlags[Index2];
    ActiveFlags[Index2] = tempActive;

    bool tempAllocated = AllocatedFlags[Index1];
    AllocatedFlags[Index1] = AllocatedFlags[Index2];
    AllocatedFlags[Index2] = tempAllocated;
}

// === 매핑 관리 함수들 구현 ===

// 매핑 관계를 업데이트 (압축 시 사용)
void PhysicsStateArrays::UpdateMappingAfterMove(SoAIdx OldIndex, SoAIdx NewIndex)
{
    if (OldIndex >= Size() || NewIndex >= Size())
    {
        LOG_FUNC_CALL("[Error] UpdateMappingAfterMove: Invalid indices Old=%u, New=%u, Size=%zu",
                      OldIndex, NewIndex, Size());
        return;
    }

    if (OldIndex == NewIndex)
    {
        return;  // 같은 위치로 이동시 매핑 업데이트 불필요
    }

    // OldIndex에 해당하는 ID 찾기
    auto OldIt = IdxToId.find(OldIndex);
    if (OldIt == IdxToId.end())
    {
        LOG_FUNC_CALL("[Error] UpdateMappingAfterMove: No ID found for OldIndex %u", OldIndex);
        return;
    }

    SoAID MovedId = OldIt->second;

    // ID → Index 매핑 일관성 검증
    auto IdIt = IdToIdx.find(MovedId);
    if (IdIt == IdToIdx.end() || IdIt->second != OldIndex)
    {
        LOG_FUNC_CALL("[Error] UpdateMappingAfterMove: Mapping inconsistency for ID %u", MovedId);
        return;
    }

    // NewIndex 위치에 기존 매핑이 있는지 확인 (덮어쓰기 방지)
    auto NewIt = IdxToId.find(NewIndex);
    if (NewIt != IdxToId.end())
    {
        LOG_FUNC_CALL("[Warning] UpdateMappingAfterMove: Overwriting existing mapping at NewIndex %u", NewIndex);
        // 기존 매핑의 역방향도 정리
        SoAID ExistingId = NewIt->second;
        IdToIdx.erase(ExistingId);
    }

    // 매핑 업데이트
    IdToIdx[MovedId] = NewIndex;    // ID → 새로운 Index
    IdxToId[NewIndex] = MovedId;    // 새로운 Index → ID
    IdxToId.erase(OldIndex);        // 이전 Index 매핑 제거

    LOG_FUNC_CALL("[Debug] Updated mapping: ID %u moved from Index %u to %u",
                  MovedId, OldIndex, NewIndex);
}

// === 최적화된 매핑 관리 함수들 ===

// 해제된 ID의 매핑을 완전히 제거 (압축 시 사용) - 최적화된 버전
void PhysicsStateArrays::RemoveInvalidIDs(std::vector<SoAID>& ToRemove)
{
    if (ToRemove.empty())
    {
        LOG_FUNC_CALL("[Info] No ID to remove");
        return;
    }

    LOG_FUNC_CALL("[Info] Removing %zu IDs from mappings",
                  ToRemove.size());

    size_t RemovedCount = 0;

    for (SoAID Id : ToRemove)
    {
        if (Id == INVALID_ID)
        {
            LOG_FUNC_CALL("[Warning] RemoveIDs: Skipping INVALID_ID");
            continue;
        }

        // ID → Index 매핑 찾기 및 제거
        auto IdIt = IdToIdx.find(Id);
        if (IdIt != IdToIdx.end())
        {
            SoAIdx Index = IdIt->second;

            // 쌍방향 매핑 완전 제거
            IdToIdx.erase(IdIt);
            IdxToId.erase(Index);

            RemovedCount++;

            LOG_FUNC_CALL("[Debug] Removed mapping: ID %u → Index %u", Id, Index);
        }
        else
        {
            LOG_FUNC_CALL("[Warning] RemoveIDs: ID %u not found in mappings", Id);
        }
    }

    LOG_FUNC_CALL("[Info] RemoveIDs completed: %zu mappings removed from %zu requested",
                  RemovedCount, ToRemove.size());

    // 디버그 빌드에서 매핑 무결성 검증
#ifdef _DEBUG
    ValidateMappingIntegrity();
#endif
}


// 매핑 무결성 검증 (디버그용)
#ifdef _DEBUG
void PhysicsStateArrays::ValidateMappingIntegrity() const
{
    // ID → Index 매핑 검증
    for (const auto& [Id, Index] : IdToIdx)
    {
        // 역매핑 존재 검증
        auto ReverseIt = IdxToId.find(Index);
        if (ReverseIt == IdxToId.end() || ReverseIt->second != Id)
        {
            LOG_FUNC_CALL("[Error] Mapping integrity violation: ID %u → Index %u lacks valid reverse mapping",
                          Id, Index);
        }

        // 인덱스 유효성 검증
        if (Index >= AllocatedCount)
        {
            LOG_FUNC_CALL("[Error] Mapping integrity violation: ID %u → Index %u out of allocated range",
                          Id, Index);
        }

        // 할당 상태 검증
        if (Index < AllocatedCount && !AllocatedFlags[Index])
        {
            LOG_FUNC_CALL("[Error] Mapping integrity violation: ID %u → Index %u points to deallocated slot",
                          Id, Index);
        }
    }

    // Index → ID 매핑 검증
    for (const auto& [Index, Id] : IdxToId)
    {
        // 역매핑 존재 검증
        auto ReverseIt = IdToIdx.find(Id);
        if (ReverseIt == IdToIdx.end() || ReverseIt->second != Index)
        {
            LOG_FUNC_CALL("[Error] Mapping integrity violation: Index %u → ID %u lacks valid reverse mapping",
                          Index, Id);
        }
    }

    LOG_FUNC_CALL("[Debug] Mapping integrity validation completed: %zu mappings verified",
                  IdToIdx.size());
}
#endif