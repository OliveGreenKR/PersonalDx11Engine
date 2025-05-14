#pragma once
#include "Math.h"
//연산을 위해 사용하는 구조체, 내부멤버는 XMVECOTR 16바이트 정렬
struct alignas(16) FAABB
{
    XMVECTOR vMin = XMVectorReplicate(FLT_MAX);
    XMVECTOR vMax = XMVectorReplicate(-FLT_MAX);

    void GetMaxV(Vector3& OutVector) const { XMStoreFloat3(&OutVector, vMax); }
    void GetMinV(Vector3& OutVector) const { XMStoreFloat3(&OutVector, vMin); }
    void SetMax(const Vector3& InVector3) { vMax = XMLoadFloat3(&InVector3); }
    void SetMin(const Vector3& InVector3) { vMin = XMLoadFloat3(&InVector3); }

    //완전히 포함하는지
    bool IsContaining(const FAABB& Other) const {
        // 수치적 안정성을 위한 epsilon 사용
        XMVECTOR epsilon = XMVectorReplicate(KINDA_SMALL);
        return XMVector3LessOrEqual(XMVectorSubtract(vMin, epsilon), Other.vMin)
            && XMVector3GreaterOrEqual(XMVectorAdd(vMax, epsilon), Other.vMax);
    }
    //겹치는지 확인
    bool IsOverlapping(const FAABB& Other) const {
        XMVECTOR epsilon = XMVectorReplicate(KINDA_SMALL);
        return XMVector3LessOrEqual(XMVectorSubtract(vMin, epsilon), Other.vMax)
            && XMVector3GreaterOrEqual(XMVectorAdd(vMax, epsilon), Other.vMin);
    }

    FAABB& Extend(float Margin) {
        XMVECTOR vMargin = XMVectorReplicate(Margin);
        vMin = XMVectorSubtract(vMin, vMargin);
        vMax = XMVectorAdd(vMax, vMargin);
        return *this;
    }

    //점을 포함하도록 확장
    FAABB& Include(const Vector3& InPoint)
    {
        XMVECTOR point = XMLoadFloat3(&InPoint);
        vMin = XMVectorMin(vMin, point);
        vMax = XMVectorMax(vMax, point);
        return *this;
    }

    static FAABB Merge(const FAABB& A, const FAABB& B)
    {
        FAABB New;
        New.vMin = XMVectorMin(A.vMin, B.vMin); // 각 축에서 최소값 선택
        New.vMax = XMVectorMax(A.vMax, B.vMax); // 각 축에서 최대값 선택
        return New;
    }
};