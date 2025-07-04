#pragma once
#include "Math.h"
#include "Transform.h"
#include "PhysicsDefine.h"

// PhysicsDataStructures.h에서 해당 부분 수정

#pragma region Dirty Flag System

/// <summary>
/// 물리 데이터 더티 플래그 클래스
/// 변경된 데이터 카테고리를 추적하여 효율적 동기화 지원
/// 타입 안전성과 편의성을 위한 캡슐화 제공
/// </summary>
class FPhysicsDataDirtyFlags
{
public:
    // === 비트 마스크 영역 정의 ===
    static constexpr uint8_t FLAG_NONE = 0;
    static constexpr uint8_t FLAG_HIGH_FREQ = 1 << 0;  // Transform 데이터
    static constexpr uint8_t FLAG_MID_FREQ = 1 << 1;  // Type, Mask 데이터
    static constexpr uint8_t FLAG_LOW_FREQ = 1 << 2;  // Properties 데이터

    // 그룹 플래그 (편의성)
    static constexpr uint8_t FLAG_ALL = FLAG_HIGH_FREQ | FLAG_MID_FREQ | FLAG_LOW_FREQ;

private:
    uint8_t Flags = FLAG_NONE;

public:
    // === 생성자 ===
    constexpr FPhysicsDataDirtyFlags() = default;
    constexpr explicit FPhysicsDataDirtyFlags(uint8_t InFlags) : Flags(InFlags) {}

    // === 개별 플래그 조작 (타입 안전) ===
    constexpr FPhysicsDataDirtyFlags& SetFlag(uint8_t Flag) noexcept
    {
        Flags |= Flag;
        return *this;
    }

    constexpr FPhysicsDataDirtyFlags& ClearFlag(uint8_t Flag) noexcept
    {
        Flags &= ~Flag;
        return *this;
    }

    constexpr FPhysicsDataDirtyFlags& ToggleFlag(uint8_t Flag) noexcept
    {
        Flags ^= Flag;
        return *this;
    }

    // === 상태 쿼리 ===
    constexpr bool HasFlag(uint8_t Flag) const noexcept
    {
        return (Flags & Flag) == Flag;
    }

    constexpr bool HasAnyFlag(uint8_t TestFlags) const noexcept
    {
        return (Flags & TestFlags) != 0;
    }

    constexpr bool HasAllFlags(uint8_t TestFlags) const noexcept
    {
        return (Flags & TestFlags) == TestFlags;
    }

    // === 편의 메서드 ===
    constexpr bool HasHighFreq() const noexcept { return HasFlag(FLAG_HIGH_FREQ); }
    constexpr bool HasMidFreq() const noexcept { return HasFlag(FLAG_MID_FREQ); }
    constexpr bool HasLowFreq() const noexcept { return HasFlag(FLAG_LOW_FREQ); }

    constexpr FPhysicsDataDirtyFlags& SetHighFreq() noexcept { return SetFlag(FLAG_HIGH_FREQ); }
    constexpr FPhysicsDataDirtyFlags& SetMidFreq() noexcept { return SetFlag(FLAG_MID_FREQ); }
    constexpr FPhysicsDataDirtyFlags& SetLowFreq() noexcept { return SetFlag(FLAG_LOW_FREQ); }

    constexpr FPhysicsDataDirtyFlags& ClearHighFreq() noexcept { return ClearFlag(FLAG_HIGH_FREQ); }
    constexpr FPhysicsDataDirtyFlags& ClearMidFreq() noexcept { return ClearFlag(FLAG_MID_FREQ); }
    constexpr FPhysicsDataDirtyFlags& ClearLowFreq() noexcept { return ClearFlag(FLAG_LOW_FREQ); }

    // === 전체 플래그 조작 ===
    constexpr uint8_t GetRawFlags() const noexcept { return Flags; }
    constexpr void SetRawFlags(uint8_t InFlags) noexcept { Flags = InFlags; }
    constexpr void ClearAll() noexcept { Flags = FLAG_NONE; }
    constexpr bool IsEmpty() const noexcept { return Flags == FLAG_NONE; }
    constexpr bool HasAny() const noexcept { return Flags != FLAG_NONE; }

    // === 비트 연산자 오버로딩 ===
    constexpr FPhysicsDataDirtyFlags operator|(const FPhysicsDataDirtyFlags& Other) const noexcept
    {
        return FPhysicsDataDirtyFlags(Flags | Other.Flags);
    }

    constexpr FPhysicsDataDirtyFlags operator&(const FPhysicsDataDirtyFlags& Other) const noexcept
    {
        return FPhysicsDataDirtyFlags(Flags & Other.Flags);
    }

    constexpr FPhysicsDataDirtyFlags operator^(const FPhysicsDataDirtyFlags& Other) const noexcept
    {
        return FPhysicsDataDirtyFlags(Flags ^ Other.Flags);
    }

    constexpr FPhysicsDataDirtyFlags operator~() const noexcept
    {
        return FPhysicsDataDirtyFlags(~Flags);
    }

    constexpr FPhysicsDataDirtyFlags& operator|=(const FPhysicsDataDirtyFlags& Other) noexcept
    {
        Flags |= Other.Flags;
        return *this;
    }

    constexpr FPhysicsDataDirtyFlags& operator&=(const FPhysicsDataDirtyFlags& Other) noexcept
    {
        Flags &= Other.Flags;
        return *this;
    }

    constexpr FPhysicsDataDirtyFlags& operator^=(const FPhysicsDataDirtyFlags& Other) noexcept
    {
        Flags ^= Other.Flags;
        return *this;
    }

    // === 비교 연산자 ===
    constexpr bool operator==(const FPhysicsDataDirtyFlags& Other) const noexcept
    {
        return Flags == Other.Flags;
    }

    constexpr bool operator!=(const FPhysicsDataDirtyFlags& Other) const noexcept
    {
        return Flags != Other.Flags;
    }
};

#pragma endregion

struct FPhysicsToGameData {
    Vector3 Velocity = Vector3::Zero();
    Vector3 AngularVelocity = Vector3::Zero();
    Vector3 ResultPosition = Vector3::Zero();
    Quaternion ResultRotation = Quaternion::Identity();
    Vector3 ResultScale = Vector3::One();

    FPhysicsToGameData() = default;

    FTransform GetResultTransform() const {
        return FTransform(ResultPosition, ResultRotation, ResultScale);
    }
};

#pragma region Frequency-Based Data Structures

/// <summary>
/// 높은 변경 빈도 데이터 - 매 프레임 변경 가능
/// Transform 관련 데이터
/// </summary>
/// 
struct FHighFrequencyData
{
    Vector3 Position = Vector3::Zero();
    Quaternion Rotation = Quaternion::Identity();
    Vector3 Scale = Vector3::One();

    FHighFrequencyData() = default;

    explicit FHighFrequencyData(const FTransform& InTransform)
        : Position(InTransform.Position)
        , Rotation(InTransform.Rotation)
        , Scale(InTransform.Scale)
    {
    }

    FTransform GetTransform() const
    {
        return FTransform(Position, Rotation, Scale);
    }

    bool operator==(const FHighFrequencyData& Other) const
    {
        return FTransform::IsEqual(GetTransform(), Other.GetTransform());
    }

    bool operator!=(const FHighFrequencyData& Other) const
    {
        return !(*this == Other);
    }
};

/// <summary>
/// 중간 변경 빈도 데이터 - 게임플레이 이벤트에 따라 변경
/// 물리 타입 및 상태 제어
/// </summary>
struct FMidFrequencyData
{
    EPhysicsType PhysicsType = EPhysicsType::Dynamic;
    FPhysicsMask PhysicsMask = FPhysicsMask(FPhysicsMask::GROUP_BASIC_SIMULATION);

    FMidFrequencyData() = default;

    FMidFrequencyData(EPhysicsType InType, const FPhysicsMask& InMask)
        : PhysicsType(InType)
        , PhysicsMask(InMask)
    {
    }

    bool operator==(const FMidFrequencyData& Other) const
    {
        return PhysicsType == Other.PhysicsType && PhysicsMask == Other.PhysicsMask;
    }

    bool operator!=(const FMidFrequencyData& Other) const
    {
        return !(*this == Other);
    }
};

/// <summary>
/// 낮은 변경 빈도 데이터 - 초기화 또는 특수 상황에서만 변경
/// 물리 속성 데이터
/// </summary>
struct FLowFrequencyData {
    float InvMass = 1.0f;
    Vector3 InvRotationalInertia = Vector3::One();
    float FrictionKinetic = 0.3f;
    float FrictionStatic = 0.5f;
    float Restitution = 0.2f;
    float MaxSpeed = 3.0f * ONE_METER;
    float MaxAngularSpeed = XM_PIDIV2;
    float GravityScale = 1.0f;

    FLowFrequencyData() = default;

    float GetMass() const {
        return (InvMass > KINDA_SMALL) ? (1.0f / InvMass) : KINDA_LARGE;
    }

    Vector3 GetRotationalInertia() const {
        Vector3 result;
        result.x = (InvRotationalInertia.x > KINDA_SMALL) ? (1.0f / InvRotationalInertia.x) : KINDA_LARGE;
        result.y = (InvRotationalInertia.y > KINDA_SMALL) ? (1.0f / InvRotationalInertia.y) : KINDA_LARGE;
        result.z = (InvRotationalInertia.z > KINDA_SMALL) ? (1.0f / InvRotationalInertia.z) : KINDA_LARGE;
        return result;
    }
};

#pragma endregion



