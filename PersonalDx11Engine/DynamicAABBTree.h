#pragma once
#include "Math.h"
#include "Transform.h"
#include "DynamicBoundableInterface.h"
#include <vector>
#include <unordered_map>
#include <memory>

class FDynamicAABBTree
{
public:
    // 16����Ʈ ������ ���� ���
    static constexpr size_t NULL_NODE = static_cast<size_t>(-1);
    static constexpr float AABB_EXTENSION = 0.1f;    // AABB Ȯ�� ���
    static constexpr float AABB_MULTIPLIER = 2.0f;   // ���� ���� ����ȭ�� �¼�

    struct AABB
    {
        Vector3 Min;
        Vector3 Max;

        bool Contains(const AABB& Other) const {
            // SIMD ����ȭ�� ���� XMVECTOR ���
            XMVECTOR vMin = XMLoadFloat3(&Min);
            XMVECTOR vMax = XMLoadFloat3(&Max);
            XMVECTOR vOtherMin = XMLoadFloat3(&Other.Min);
            XMVECTOR vOtherMax = XMLoadFloat3(&Other.Max);

            // ��ġ�� �������� ���� epsilon ���
            XMVECTOR epsilon = XMVectorReplicate(KINDA_SMALL);
            return XMVector3LessOrEqual(XMVectorSubtract(vMin, epsilon), vOtherMin)
                && XMVector3GreaterOrEqual(XMVectorAdd(vMax, epsilon), vOtherMax);
        }

        bool Overlaps(const AABB& Other) const {
            XMVECTOR vMin = XMLoadFloat3(&Min);
            XMVECTOR vMax = XMLoadFloat3(&Max);
            XMVECTOR vOtherMin = XMLoadFloat3(&Other.Min);
            XMVECTOR vOtherMax = XMLoadFloat3(&Other.Max);

            XMVECTOR epsilon = XMVectorReplicate(KINDA_SMALL);
            return XMVector3LessOrEqual(XMVectorSubtract(vMin, epsilon), vOtherMax)
                && XMVector3GreaterOrEqual(XMVectorAdd(vMax, epsilon), vOtherMin);
        }

        AABB& Extend(float Margin) {
            XMVECTOR vMin = XMLoadFloat3(&Min);
            XMVECTOR vMax = XMLoadFloat3(&Max);
            XMVECTOR vMargin = XMVectorReplicate(Margin);

            XMStoreFloat3(&Min, XMVectorSubtract(vMin, vMargin));
            XMStoreFloat3(&Max, XMVectorAdd(vMax, vMargin));
            return *this;
        }
    };

    // ĳ�� ������ ����� ��� ����ü
    struct alignas(16) Node
    {
        // 24����Ʈ ���� ������
        AABB Bounds;                 // ���� AABB
        AABB FatBounds;             // ���� �ִ� AABB (���� ���� ����ȭ��)
        // 12 ����Ʈ
        Vector3 LastPosition;     // ���� �������� ��ġ
        Vector3 LastHalfExtent;   // ���� �������� HalfExtent

        // 8����Ʈ ������
        size_t Parent = NULL_NODE;
        size_t Left = NULL_NODE;
        size_t Right = NULL_NODE;
        IDynamicBoundable* BoundableObject = nullptr;

       
        // 4����Ʈ ������
        int32_t Height = 0;

        //�е�
        int32_t Padding[3];      //�� 108 + 12 

        bool IsLeaf() const { return Left == NULL_NODE; }
        bool NeedsUpdate(const Vector3& CurrentPosition, const Vector3& CurrentHalfExtent) const
        {
            // ���� ���·� AABB ����
            AABB CurrentBounds;
            CurrentBounds.Min = CurrentPosition - CurrentHalfExtent;
            CurrentBounds.Max = CurrentPosition + CurrentHalfExtent;

            // ���� ������ AABB�� FatBounds�� ������� �˻�
            return !FatBounds.Contains(CurrentBounds);
        }

    };

public:
    FDynamicAABBTree(size_t InitialCapacity = 1024);
    ~FDynamicAABBTree();

    // �ٽ� ���
    size_t Insert(const std::shared_ptr<IDynamicBoundable>& Object);
    void Remove(size_t NodeId);
    void UpdateTree();

    // ���� ���
    template<typename Callback>
    void QueryOverlap(const AABB& QueryBounds, Callback&& Func);

private:
    // ��� Ǯ ����
    size_t AllocateNode();
    void FreeNode(size_t NodeId);

    // Ʈ�� ��������
    void InsertLeaf(size_t NodeId);
    void RemoveLeaf(size_t NodeId);
    void UpdateNodeBounds(size_t NodeId);
    size_t Rebalance(size_t NodeId);

    // SAH ����
    float ComputeCost(const AABB& Bounds) const;
    float ComputeInheritedCost(size_t NodeId) const;

private:
    std::vector<Node> NodePool;           // ��� �޸� Ǯ
    std::vector<size_t> FreeNodes;        // ���� ������ ��� �ε���
    size_t RootId = NULL_NODE;            // ��Ʈ ��� �ε���
    size_t NodeCount = 0;                 // ���� ��� ���� ��� ��
};