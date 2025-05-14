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

    static FAABB Create(const Vector3& LocalHalfExtent, const FTransform& WorldTransform)
    {
        FAABB New;

        // 월드 행렬 가져오기
        Matrix worldMatrix = WorldTransform.GetModelingMatrix();

        // 월드 행렬의 스케일 및 회전 성분만 추출 (위치 제외)
        // 각 축 방향의 변환된 벡터 계산
        XMVECTOR xAxis = XMVector3TransformNormal(XMVectorSet(LocalHalfExtent.x, 0, 0, 0), worldMatrix);
        XMVECTOR yAxis = XMVector3TransformNormal(XMVectorSet(0, LocalHalfExtent.y, 0, 0), worldMatrix);
        XMVECTOR zAxis = XMVector3TransformNormal(XMVectorSet(0, 0, LocalHalfExtent.z, 0), worldMatrix);

        // 각 축의 절대값 계산
        xAxis = XMVectorAbs(xAxis);
        yAxis = XMVectorAbs(yAxis);
        zAxis = XMVectorAbs(zAxis);

        // 세 축의 합이 AABB의 "반경" 벡터가 됨
        XMVECTOR radius = XMVectorAdd(XMVectorAdd(xAxis, yAxis), zAxis);

        // 중심점 위치
        XMVECTOR center = XMLoadFloat3(&WorldTransform.Position);

        // min, max 계산
        XMVECTOR minV = XMVectorSubtract(center, radius);
        XMVECTOR maxV = XMVectorAdd(center, radius);

        // 결과 저장
        New.vMin = minV;
        New.vMax = maxV;

        return New;
    }
};