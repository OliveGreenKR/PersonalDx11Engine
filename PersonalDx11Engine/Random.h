#pragma once
#include <random>
#include <type_traits>
#include <chrono>
#include <cstdint>
#include "Math.h"

/**
 * 프로젝트 전체에서 사용할 수 있는 랜덤 유틸리티 클래스
 * 전역 싱글톤 인스턴스를 통해 일관된 랜덤 시퀀스 제공
 * 외부에서는 정적 메서드를 통해서만 접근 가능
 */
class FRandom
{
private:
    static FRandom& GetInstance()
    {
        static FRandom Instance;
        return Instance;
    }

    FRandom()
    {
        // 현재 시간을 기반으로 시드 생성 (나노초 단위까지 사용)
        uint64_t TimeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        // 추가 엔트로피를 위해 주소값 해싱 결합
        uintptr_t AddressSeed = reinterpret_cast<uintptr_t>(this);

        // 결합된 시드값 생성
        uint64_t CombinedSeed = TimeSeed ^ (AddressSeed << 32);

        Generator.seed(static_cast<uint32_t>(CombinedSeed));
    }

    // 복사/이동 생성자와 할당 연산자 삭제 (싱글톤 패턴)
    FRandom(const FRandom&) = delete;
    FRandom(FRandom&&) = delete;
    FRandom& operator=(const FRandom&) = delete;
    FRandom& operator=(FRandom&&) = delete;


    // 인스턴스 레벨 동작 메서드들 (내부 전용)
    /**
     * 정수형 범위에서 균일한 분포의 랜덤값 생성
     */
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    T GetInt(T Min = 0, T Max = 1)
    {
        static std::uniform_int_distribution<T> Distribution;
        Distribution.param(typename std::uniform_int_distribution<T>::param_type(Min, Max));
        return Distribution(Generator);
    }

    /**
     * 실수형 범위에서 균일한 분포의 랜덤값 생성
     */
    template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    T GetFloat(T Min = 0.0f, T Max = 1.0f)
    {
        static std::uniform_real_distribution<T> Distribution;
        Distribution.param(typename std::uniform_real_distribution<T>::param_type(Min, Max));
        return Distribution(Generator);
    }

    /**
     * 정규 분포에서 랜덤값 생성
     */
    template <typename T = float, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    T GetNomalFloat(T Mean = 0.0, T StdDev = 1.0)
    {
        static std::normal_distribution<T> Distribution;
        Distribution.param(typename std::normal_distribution<T>::param_type(Mean, StdDev));
        return Distribution(Generator);
    }

    std::mt19937 Generator; // 메르센 트위스터 랜덤 생성기

public:
    /**
     * 싱글톤 인스턴스의 시드를 재설정
     *
     * @param NewSeed - 새로운 시드값
     */
    static void SetSeed(uint32_t NewSeed)
    {
        GetInstance().SetSeed(NewSeed);
    }
    /// <summary>
    /// Returns a random number within [min,max].
    /// </summary>
    static float RandF(float Min = 0.0f, float Max = 1.0f)
    {
        return GetInstance().GetFloat<float>(Min, Max);
    }
    /// <summary>
    /// Returns a random number within [min,max] based on a normal distribution.
    /// </summary>
    static float RandNF(float Min = 0.0f, float Max = 1.0f)
    {
        return GetInstance().GetNomalFloat<float>(Min, Max);
    }
    /// <summary>
    /// Returns a random number within [min,max].
    /// </summary>
    static int RandI(int Min = 0, int Max = 1)
    {
        return GetInstance().GetInt<int>(Min, Max);
    }

    static Quaternion RandQuat()
    {
        // 구면 좌표에서 랜덤 포인트 생성
        float u1 = RandF(0.0f, 1.0f);
        float u2 = RandF(0.0f, 1.0f);
        float u3 = RandF(0.0f, 1.0f);

        // 균일 분포의 쿼터니언 생성
        float sqrtOneMinusU1 = sqrt(1.0f - u1);
        float sqrtU1 = sqrt(u1);
        float twoPI_1 = 2.0f * PI * u2;
        float twoPI_2 = 2.0f * PI * u3;

        float x = sqrtOneMinusU1 * sin(twoPI_1);
        float y = sqrtOneMinusU1 * cos(twoPI_1);
        float z = sqrtU1 * sin(twoPI_2);
        float w = sqrtU1 * cos(twoPI_2);

        return Quaternion(x, y, z, w);
    }

    static Vector3 RandUnitVector()
    {
        // 균일 분포의 랜덤 방향 벡터
        float theta = RandF(0.0f, PI * 2.0f);
        float phi = acos(RandF(-1.0f, 1.0f)); // 균일한 구면 분포를 위해 acos 사용

        float sinPhi = sin(phi);
        float x = sinPhi * cos(theta);
        float y = sinPhi * sin(theta);
        float z = cos(phi);

        return Vector3(x, y, z);
    }
    /// <summary>
    /// Returns a random vector within [min.xyz,max.xyz].
    /// </summary>
    static Vector3 RandVector(const Vector3& MinVec, const Vector3& MaxVec)
    {
        return Vector3(
            RandF(MinVec.x, MaxVec.x),
            RandF(MinVec.y, MaxVec.y),
            RandF(MinVec.z, MaxVec.z)
        );
    }
    /// <summary>
    /// Returns a random Opaque RGB Color;
    /// </summary>
    static Vector4 RandColor()
    {
        return Vector4(
            RandF(),
            RandF(),
            RandF(),
            1.0f
        );
    }
};