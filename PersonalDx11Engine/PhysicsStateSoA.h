#pragma once
#include <vector>
#include <unordered_map>
#include "Math.h"
#include "PhysicsDefine.h"
#include <DirectXMath.h>

using SoAID = uint32_t;
using SoAIdx = uint32_t;

/// <summary>
/// 물리 상태를 SoA 형태로 저장하는 데이터 컨테이너
/// 
/// 역할:
/// - 물리 상태 데이터의 연속 메모리 배치 제공
/// - ID-Index 매핑을 통한 안정적 외부 인터페이스
/// - 지연 압축을 통한 메모리 효율성 관리
/// 
/// 책임:
/// - 메모리 할당/확장 및 SIMD 정렬 보장
/// - 할당된 객체들의 연속 배치 유지 (압축을 통해)
/// - 최소한의 생명주기 관리
/// 
/// 무결성 원칙:
/// - 연속적인 유효/재사용 가능 Slot 범위 = [0:AllocatedCount)
/// - 재사용가능 ID = FreeIDs
/// - 재사용/사용 가능한 ID-Slot의 관계는 항상 지속적으로 유효
/// - 압축 시 FreeIDs는 완전 초기화되어 해제된 ID들은 영구 무효화
/// </summary>
struct PhysicsStateArrays
{
public:
    // === 공개 물리 상태 데이터 (SoA 구조) ===

    // 운동 상태
    std::vector<XMVECTOR> Velocities;
    std::vector<XMVECTOR> AngularVelocities;
    std::vector<XMVECTOR> AccumulatedForces;
    std::vector<XMVECTOR> AccumulatedTorques;

    // 트랜스폼 정보
    std::vector<XMVECTOR> WorldPosition;
    std::vector<XMVECTOR> WorldScale;
    std::vector<XMVECTOR> WorldRotationQuat;

    // 물리 속성
    std::vector<XMVECTOR> InvRotationalInertias;
    std::vector<float> InvMasses;
    std::vector<float> FrictionKinetics;
    std::vector<float> FrictionStatics;
    std::vector<float> Restitutions;

    // 제한 및 설정
    std::vector<float> MaxSpeeds;
    std::vector<float> MaxAngularSpeeds;
    std::vector<float> GravityScales;
    std::vector<EPhysicsType> PhysicsTypes;
    std::vector<uint8_t> PhysicsFlags;

    // === 상태 관리 데이터 ===
    std::vector<bool> ActiveFlags;       // 각 슬롯의 활성/비활성 (외부 연산 참고용)
    std::vector<bool> AllocatedFlags;    // 각 슬롯의 할당 여부
    uint32_t AllocatedCount = 0;         // 할당된 슬롯 수 (순회 범위)
    uint32_t DeallocatedCount = 0;       // 해제된 슬롯 수 (압축 대상)

public:
    // === 생성자 ===
    explicit PhysicsStateArrays(size_t InitialSize);
    ~PhysicsStateArrays() = default;

    PhysicsStateArrays(const PhysicsStateArrays&) = delete;
    PhysicsStateArrays& operator=(const PhysicsStateArrays&) = delete;

    // === 객체 생명주기 관리 ===

    /// 새 슬롯 할당 및 ID 반환 (기본적으로 활성 상태로 생성)
    SoAID AllocateSlot();

    /// 슬롯 해제 (ID 재사용 가능화, 압축 전까지만)
    void DeallocateSlot(SoAID Id);

    /// 배열 크기 확장 시도
    bool TryResize(uint32_t NewSize);

    /// 압축 필요 시 수행 (해제된 객체들 정리, FreeIDs 초기화)
    void CompactIfNeeded();

    /// 명시적 압축 수행
    void ForceCompact();

    // === 활성화 상태 관리 (외부 연산 참고용) ===

    /// 객체 비활성화 (연산에서 제외, 할당 상태는 유지)
    void DeactivateObject(SoAID Id);

    /// 객체 활성화 (연산에 포함)
    void ActivateObject(SoAID Id);

    // === 상태 조회 ===

    /// ID가 할당된 유효한 슬롯인지 확인
    bool IsAllocatedSlot(SoAID Id) const;

    /// ID 객체가 활성 상태인지 확인 (할당되고 활성화된 경우만 true)
    bool IsActiveObject(SoAID Id) const;

    /// 전체 배열 크기
    size_t Size() const { return Velocities.size(); }

    /// 할당된 슬롯 수 (순회 대상 크기)
    uint32_t GetAllocatedCount() const { return AllocatedCount; }

    /// 실제 할당된 객체 수 (해제된 객체 제외)
    uint32_t GetValidObjectCount() const { return AllocatedCount - DeallocatedCount; }

    /// 활성 객체 수 계산
    uint32_t GetActiveObjectCount() const;

    /// 압축이 필요한지 확인
    bool NeedsCompaction() const;

    /// 재사용 가능한 ID 수
    uint32_t GetFreeIDCount() const { return static_cast<uint32_t>(ReusableIDs.size()); }

private:
    static constexpr uint32_t INVALID_ID = 0;
    static constexpr uint32_t INVALID_IDX = 0;
    static constexpr uint32_t FIRST_VALID_ID = 1;
    static constexpr float COMPACTION_THRESHOLD = 0.3f;

    // ID 관리
    std::unordered_map<SoAID, SoAIdx> IdToIdx;   // ID → Index 매핑
    std::unordered_map<SoAIdx, SoAID> IdxToId;   // Index → ID 역매핑
    uint32_t NextNewId = FIRST_VALID_ID;         // 다음 새 ID
    std::vector<SoAID> ReusableIDs;                  // 재사용 가능한 ID 풀 (압축 시 초기화됨)

    // === 내부 헬퍼 함수들 ===

    /// ID를 내부 인덱스로 변환 (할당된 ID만, 내부 전용)
    SoAIdx GetIndex(SoAID Id) const;

    /// 인덱스가 할당된 범위 내에 있고 실제로 할당되었는지 확인
    bool IsValidAllocatedIndex(SoAIdx Index) const;

    /// 슬롯을 기본값으로 초기화
    void InitializeSlot(SoAIdx Index);

    /// 모든 SoA 벡터들을 동시에 크기 조정
    void ResizeAllVectors(uint32_t NewSize);

    /// 압축 실행: 유효 객체들을 앞쪽으로 이동, FreeIDs 초기화
    void PerformCompaction();

    /// 슬롯 데이터를 한 위치에서 다른 위치로 이동
    void MoveSlotData(SoAIdx FromIndex, SoAIdx ToIndex);

    /// 두 슬롯의 데이터를 교환
    void SwapSlotData(SoAIdx Index1, SoAIdx Index2);

    /// 매핑 관계를 업데이트 (압축 시 사용)
    void UpdateMappingAfterMove(SoAIdx OldIndex, SoAIdx NewIndex);

    /// 해제된 ID의 매핑을 완전히 제거 (압축 시 사용)
    void RemoveInvalidIDs(std::vector<SoAID>& ToRemove);

#ifdef _DEBUG
    //Debug
    void ValidateMappingIntegrity() const;
#endif
};