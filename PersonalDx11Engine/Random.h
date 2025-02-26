#pragma once
#include <random>
#include <type_traits>
#include <chrono>
#include <cstdint>

/**
 * ������Ʈ ��ü���� ����� �� �ִ� ���� ��ƿ��Ƽ Ŭ����
 * ���� �̱��� �ν��Ͻ��� ���� �ϰ��� ���� ������ ����
 * �ܺο����� ���� �޼��带 ���ؼ��� ���� ����
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
        // ���� �ð��� ������� �õ� ���� (������ �������� ���)
        uint64_t TimeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        // �߰� ��Ʈ���Ǹ� ���� �ּҰ� �ؽ� ����
        uintptr_t AddressSeed = reinterpret_cast<uintptr_t>(this);

        // ���յ� �õ尪 ����
        uint64_t CombinedSeed = TimeSeed ^ (AddressSeed << 32);

        Generator.seed(static_cast<uint32_t>(CombinedSeed));
    }

    // ����/�̵� �����ڿ� �Ҵ� ������ ���� (�̱��� ����)
    FRandom(const FRandom&) = delete;
    FRandom(FRandom&&) = delete;
    FRandom& operator=(const FRandom&) = delete;
    FRandom& operator=(FRandom&&) = delete;


    // �ν��Ͻ� ���� ���� �޼���� (���� ����)
    /**
     * ������ �������� ������ ������ ������ ����
     */
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    T GetInt(T Min = 0, T Max = 1)
    {
        static std::uniform_int_distribution<T> Distribution;
        Distribution.param(typename std::uniform_int_distribution<T>::param_type(Min, Max));
        return Distribution(Generator);
    }

    /**
     * �Ǽ��� �������� ������ ������ ������ ����
     */
    template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    T GetFloat(T Min = 0.0f, T Max = 1.0f)
    {
        static std::uniform_real_distribution<T> Distribution;
        Distribution.param(typename std::uniform_real_distribution<T>::param_type(Min, Max));
        return Distribution(Generator);
    }

    /**
     * ���� �������� ������ ����
     */
    template <typename T = float, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    T GetNomalFloat(T Mean = 0.0, T StdDev = 1.0)
    {
        static std::normal_distribution<T> Distribution;
        Distribution.param(typename std::normal_distribution<T>::param_type(Min, Max));
        return Distribution(Generator);
    }

    std::mt19937 Generator; // �޸��� Ʈ������ ���� ������

public:
    /**
     * �̱��� �ν��Ͻ��� �õ带 �缳��
     *
     * @param NewSeed - ���ο� �õ尪
     */
    static void SetSeed(uint32_t NewSeed)
    {
        GetInstance().SetSeed(NewSeed);
    }

    /**
     * ������ �������� ���� ������ �������� ����
     *
     * @param Min - �ּҰ� (����)
     * @param Max - �ִ밪 (����)
     * @return Min�� Max ������ ���� ������
     */
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    static T RandI(T Min = 0, T Max = 1)
    {
        return GetInstance().GetInt<T>(Min, Max);
    }

    /**
     * �Ǽ��� �������� ���� ������ �������� ����
     *
     * @param Min - �ּҰ� (����)
     * @param Max - �ִ밪 (������)
     * @return Min�� Max ������ ���� �Ǽ���
     */
    template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    static T RandF(T Min = 0.0, T Max = 1.0)
    {
        return GetInstance().GetFloat<T>(Min, Max);
    }

    /**
     * ���� ������ �������� ����
     *
     * @param Mean - ��հ�
     * @param StdDev - ǥ�� ����
     * @return ���� ������ ������ ������
     */
    template <typename T = float, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    static T RandNF(T Mean = 0.0, T StdDev = 1.0)
    {
        return GetInstance().GetNomalFloat<T>(Mean, StdDev);
    }
};