#pragma once
#include <random>
#include <type_traits>
#include <chrono>
#include <cstdint>
#include "Math.h"

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

    static float RandF(float Min = 0.0f, float Max = 1.0f)
    {
        return GetInstance().GetFloat<float>(Min, Max);
    }

    static float RandNF(float Min = 0.0f, float Max = 1.0f)
    {
        return GetInstance().GetNomalFloat<float>(Min, Max);
    }

    static int RandI(int Min = 0, int Max = 1)
    {
        return GetInstance().GetInt<int>(Min, Max);
    }

    static Quaternion RandQuat()
    {
        // ���� ��ǥ���� ���� ����Ʈ ����
        float u1 = RandF(0.0f, 1.0f);
        float u2 = RandF(0.0f, 1.0f);
        float u3 = RandF(0.0f, 1.0f);

        // ���� ������ ���ʹϾ� ����
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
        // ���� ������ ���� ���� ����
        float theta = RandF(0.0f, PI * 2.0f);
        float phi = acos(RandF(-1.0f, 1.0f)); // ������ ���� ������ ���� acos ���

        float sinPhi = sin(phi);
        float x = sinPhi * cos(theta);
        float y = sinPhi * sin(theta);
        float z = cos(phi);

        return Vector3(x, y, z);
    }

    static Vector3 RandVector(const Vector3& MinVec, const Vector3& MaxVec)
    {
        return Vector3(
            RandF(MinVec.x, MaxVec.x),
            RandF(MinVec.y, MaxVec.y),
            RandF(MinVec.z, MaxVec.z)
        );
    }
};