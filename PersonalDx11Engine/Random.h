#pragma once
#include <random>
#include <type_traits>
#include <chrono>
#include <cstdint>

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
        Distribution.param(typename std::normal_distribution<T>::param_type(Min, Max));
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

    /**
     * 정수형 범위에서 균일 분포의 랜덤값을 생성
     *
     * @param Min - 최소값 (포함)
     * @param Max - 최대값 (포함)
     * @return Min과 Max 사이의 랜덤 정수값
     */
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    static T RandI(T Min = 0, T Max = 1)
    {
        return GetInstance().GetInt<T>(Min, Max);
    }

    /**
     * 실수형 범위에서 균일 분포의 랜덤값을 생성
     *
     * @param Min - 최소값 (포함)
     * @param Max - 최대값 (미포함)
     * @return Min과 Max 사이의 랜덤 실수값
     */
    template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    static T RandF(T Min = 0.0, T Max = 1.0)
    {
        return GetInstance().GetFloat<T>(Min, Max);
    }

    /**
     * 정규 분포의 랜덤값을 생성
     *
     * @param Mean - 평균값
     * @param StdDev - 표준 편차
     * @return 정규 분포를 따르는 랜덤값
     */
    template <typename T = float, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    static T RandNF(T Mean = 0.0, T StdDev = 1.0)
    {
        return GetInstance().GetNomalFloat<T>(Mean, StdDev);
    }
};