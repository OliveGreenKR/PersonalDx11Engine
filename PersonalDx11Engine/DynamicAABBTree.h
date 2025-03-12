#pragma once
#include "Math.h"
#include "Transform.h"
#include "DynamicBoundableInterface.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

class FDynamicAABBTree
{
public:
    // 16바이트 정렬을 위한 상수
    static constexpr size_t NULL_NODE = static_cast<size_t>(-1);
    float AABB_Extension = 0.1f;    // AABB 확장 계수
    static constexpr float MIN_MARGIN = 0.01f;

    struct AABB
    {
        Vector3 Min;
        Vector3 Max;

        bool Contains(const AABB& Other) const {
            // SIMD 최적화를 위해 XMVECTOR 사용
            XMVECTOR vMin = XMLoadFloat3(&Min);
            XMVECTOR vMax = XMLoadFloat3(&Max);
            XMVECTOR vOtherMin = XMLoadFloat3(&Other.Min);
            XMVECTOR vOtherMax = XMLoadFloat3(&Other.Max);

            // 수치적 안정성을 위한 epsilon 사용
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

    struct alignas(16) Node
    {
        // 24바이트 정렬 데이터
        AABB Bounds;                 // 실제 AABB
        AABB FatBounds;             // 여유 있는 AABB (동적 갱신 최적화용)
        // 12 바이트
        Vector3 LastPosition;     // 이전 프레임의 위치
        Vector3 LastHalfExtent;   // 이전 프레임의 HalfExtent

        // 8바이트 데이터
        size_t Parent = NULL_NODE;
        size_t Left = NULL_NODE;
        size_t Right = NULL_NODE;
        IDynamicBoundable* BoundableObject = nullptr;

       
        // 4바이트 데이터
        int32_t Height = 0;

        //패딩
        int32_t Padding[3];      //총 108 + 12 

        bool IsLeaf() const { return Left == NULL_NODE; }
        bool NeedsUpdate(const Vector3& CurrentPosition, const Vector3& CurrentHalfExtent) const
        {
            // 현재 상태로 AABB 생성
            AABB CurrentBounds;
            CurrentBounds.Min = CurrentPosition - CurrentHalfExtent;
            CurrentBounds.Max = CurrentPosition + CurrentHalfExtent;

            // 현재 상태의 AABB가 FatBounds를 벗어났는지 검사
            return !FatBounds.Contains(CurrentBounds);
        }

    };

public:
    FDynamicAABBTree(size_t InitialCapacity = 1024);
    ~FDynamicAABBTree();

    // 핵심 기능
    size_t Insert(const std::shared_ptr<IDynamicBoundable>& Object);
    void Remove(size_t NodeId);
    void UpdateTree();

    // 쿼리 기능
    void QueryOverlap(const AABB& QueryBounds, const std::function<void(size_t)>& Func);

    const AABB& GetBounds(const size_t NodeId)
    {
        assert(NodePool[NodeId].BoundableObject);
        return NodePool[NodeId].Bounds;
    }

    const AABB& GetFatBounds(const size_t NodeId)
    {
        assert(NodePool[NodeId].BoundableObject);
        return NodePool[NodeId].FatBounds;
    }
    //사용중인 노드 수 반환
    const size_t GetNodeCount() { return NodeCount; }
private:
    // 노드 풀 관리
    size_t AllocateNode();
    void FreeNode(size_t NodeId);

    // 트리 유지보수
    void InsertLeaf(size_t NodeId);
    void RemoveLeaf(size_t NodeId);
    void UpdateNodeBounds(size_t NodeId);
    size_t Rebalance(size_t NodeId);

    // SAH 관련
    float ComputeCost(const AABB& Bounds) const;
    float ComputeInheritedCost(size_t NodeId) const;

    bool IsValidId(const size_t NodeId) const;

    //트리 재생성
    void ReBuildTree();
    //트리 초기화
    void ClearTree(const size_t InitialCapacity = 1024);

public:
	void PrintTreeStructure() const;
private:
    void PrintBinaryTree(size_t root, std::string prefix = "", bool isLeft = false) const;

private:
    std::vector<Node> NodePool;           // 노드 메모리 풀 - 모든 노드를 보관
    std::unordered_set<size_t> FreeNodes; // 재사용 가능한 노드 인덱스만 보관
    size_t RootId = NULL_NODE;            // 루트 노드 인덱스
    size_t NodeCount = 0;                 // 현재 사용 중인 노드 수
};