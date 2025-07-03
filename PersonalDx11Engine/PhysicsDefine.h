#pragma once

//1m에 해당하는 수치
#define ONE_METER (100.0f)

using PhysicsID = std::uint32_t;

enum class EPhysicsType
{
	Dynamic = 0,
	Static,
};

struct FPhysicsState
{
	// 물리 상태 변수
	Vector3 Velocity = Vector3::Zero();
	Vector3 AngularVelocity = Vector3::Zero();

	//월드 트랜스폼
	FTransform WorldTransform;

	// 물리 속성
	float InvMass = 1.0f;
	Vector3 InvRotationalInertia = Vector3::Zero();
	float FrictionKinetic = 0.3f;
	float FrictionStatic = 0.5f;
	float Restitution = 0.5f;

	float MaxSpeed = 0.0f;
	float MaxAngularSpeed = 0.0f;
	EPhysicsType PhysicsType = EPhysicsType::Dynamic;
    FPhysicsMask PhysicsMasks = FPhysicsMask();
};

struct FPhysicsMask
{
	// === 비트 마스크 영역 정의  ===
	static constexpr uint32_t MASK_NONE = 0;

	// 기본 활성화 상태 
	static constexpr uint32_t MASK_ACTIVATION = 1 << 0;        // 물리 시뮬레이션 활성화
	static constexpr uint32_t MASK_GRAVITY_AFFECTED = 1 << 1;  // 중력 영향

	// 그룹 마스크(편의성)
	static constexpr uint32_t GROUP_BASIC_SIMULATION = MASK_ACTIVATION | MASK_GRAVITY_AFFECTED;

private:
	uint32_t Mask = MASK_NONE;
public:
    // === 생성자 ===
    constexpr FPhysicsMask() = default;
    constexpr explicit FPhysicsMask(uint32_t InMask) : Mask(InMask) {}

    // === 개별 플래그 조작 (타입 안전) ===
    constexpr FPhysicsMask& SetFlag(uint32_t Flag) noexcept
    {
        Mask |= Flag;
        return *this;
    }

    constexpr FPhysicsMask& ClearFlag(uint32_t Flag) noexcept
    {
        Mask &= ~Flag;
        return *this;
    }

    constexpr FPhysicsMask& ToggleFlag(uint32_t Flag) noexcept
    {
        Mask ^= Flag;
        return *this;
    }

    // === 상태 쿼리 ===
    constexpr bool HasFlag(uint32_t Flag) const noexcept
    {
        return (Mask & Flag) == Flag;
    }

    constexpr bool HasAnyFlag(uint32_t Flags) const noexcept
    {
        return (Mask & Flags) != 0;
    }

    constexpr bool HasAllFlags(uint32_t Flags) const noexcept
    {
        return (Mask & Flags) == Flags;
    }

    // === 그룹 단위 조작 ===
    constexpr FPhysicsMask& SetGroup(uint32_t GroupMask) noexcept
    {
        Mask |= GroupMask;
        return *this;
    }

    constexpr FPhysicsMask& ClearGroup(uint32_t GroupMask) noexcept
    {
        Mask &= ~GroupMask;
        return *this;
    }

    // === 전체 마스크 조작 ===
    constexpr uint32_t GetRawMask() const noexcept { return Mask; }
    constexpr void SetRawMask(uint32_t InMask) noexcept { Mask = InMask; }
    constexpr void ClearAll() noexcept { Mask = MASK_NONE; }
    constexpr bool IsEmpty() const noexcept { return Mask == MASK_NONE; }

    // === 비트 연산자 오버로딩 ===
    constexpr FPhysicsMask operator|(const FPhysicsMask& Other) const noexcept
    {
        return FPhysicsMask(Mask | Other.Mask);
    }

    constexpr FPhysicsMask operator&(const FPhysicsMask& Other) const noexcept
    {
        return FPhysicsMask(Mask & Other.Mask);
    }

    constexpr FPhysicsMask operator^(const FPhysicsMask& Other) const noexcept
    {
        return FPhysicsMask(Mask ^ Other.Mask);
    }

    constexpr FPhysicsMask operator~() const noexcept
    {
        return FPhysicsMask(~Mask);
    }

    constexpr FPhysicsMask& operator|=(const FPhysicsMask& Other) noexcept
    {
        Mask |= Other.Mask;
        return *this;
    }

    constexpr FPhysicsMask& operator&=(const FPhysicsMask& Other) noexcept
    {
        Mask &= Other.Mask;
        return *this;
    }

    constexpr FPhysicsMask& operator^=(const FPhysicsMask& Other) noexcept
    {
        Mask ^= Other.Mask;
        return *this;
    }

    // === 비교 연산자 ===
    constexpr bool operator==(const FPhysicsMask& Other) const noexcept
    {
        return Mask == Other.Mask;
    }

    constexpr bool operator!=(const FPhysicsMask& Other) const noexcept
    {
        return Mask != Other.Mask;
    }

    // === 유틸리티 함수 ===

    /// 설정된 플래그 수 계산
    uint32_t GetFlagCount() const noexcept
    {
        return __popcnt(Mask);  // MSVC 내장 함수
    }

    /// 가장 낮은 설정된 비트의 인덱스 반환 (-1 if none)
    int32_t GetLowestSetBitIndex() const noexcept
    {
        if (Mask == 0) return -1;
        unsigned long index;
        _BitScanForward(&index, Mask);  // MSVC 내장 함수
        return static_cast<int32_t>(index);
    }
};