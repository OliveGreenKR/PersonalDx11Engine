#pragma once
#include "Math.h"
#include "AABB.h"
#include <vector>
#include <unordered_set>
#include <functional>
#include <iostream>

/// <summary>
/// 외부 의존성 없는 순수 데이터 기반 동적 AABB 트리
/// IDynamicBoundable 인터페이스 의존성 제거
/// </summary>
class FDynamicAABBTree
{
#pragma region Constants
public:
    static constexpr size_t NULL_NODE = static_cast<size_t>(-1);
    static constexpr float DEFAULT_FAT_MARGIN = 0.1f;
    static constexpr float MIN_MARGIN = 0.01f;

#pragma endregion

#pragma region Node Structure
public:
    struct Node
    {
        // 공간 데이터
        FMAABB Bounds;                // 실제 AABB
        FMAABB FatBounds;            // 확장된 AABB

        // 트리 구조
        size_t Parent = NULL_NODE;
        size_t Left = NULL_NODE;
        size_t Right = NULL_NODE;

        // 트리 속성
        int32_t Height = 0;

        // 메서드
        bool IsLeaf() const { return Left == NULL_NODE; }
        bool IsInternal() const { return Left != NULL_NODE; }
    };

#pragma endregion

#pragma region Constructor and Destructor
public:
    FDynamicAABBTree(size_t initialCapacity = 1024);
    ~FDynamicAABBTree();

#pragma endregion

#pragma region Core Tree Operations
public:
    /// <summary>
    /// 객체를 트리에 삽입
    /// </summary>
    /// <param name="bounds">객체의 AABB</param>
    /// <returns>노드 ID (실패 시 NULL_NODE)</returns>
    size_t Insert(const FMAABB& bounds);

    /// <summary>
    /// 객체를 트리에서 제거
    /// </summary>
    /// <param name="nodeId">제거할 노드 ID</param>
    void Remove(size_t nodeId);

    /// <summary>
    /// 트리 전체 업데이트 (변경된 객체들 재배치)
    /// </summary>
    void UpdateTree();

    /// <summary>
    /// 트리 정리
    /// </summary>
    void Clear();

#pragma endregion

#pragma region Query Operations  
public:
    /// <summary>
    /// AABB와 겹치는 모든 객체 조회 (기존 인터페이스 유지)
    /// </summary>
    /// <param name="queryBounds">검색할 AABB</param>
    /// <param name="callback">각 겹치는 노드에 대해 호출될 함수 (nodeId 전달)</param>
    void QueryOverlap(const FMAABB& queryBounds, const std::function<void(size_t)>& callback) const;

#pragma endregion

#pragma region Data Access
public:
    /// <summary>
    /// 노드의 현재 AABB 조회 (기존 GetBounds와 동일)
    /// </summary>
    /// <param name="nodeId">노드 ID</param>
    /// <returns>AABB</returns>
    const FMAABB& GetBounds(size_t nodeId) const;

    /// <summary>
    /// 노드의 Fat AABB 조회
    /// </summary>
    /// <param name="nodeId">노드 ID</param>
    /// <returns>Fat AABB</returns>
    const FMAABB& GetFatBounds(size_t nodeId) const;

    /// <summary>
    /// 노드 ID 유효성 검사 (기존 IsValidId와 동일)
    /// </summary>
    /// <param name="nodeId">노드 ID</param>
    /// <returns>유효 여부</returns>
    bool IsValidId(size_t nodeId) const;

#pragma endregion

#pragma region Statistics and Debug
public:
    size_t GetNodeCount() const { return NodeCount; }
    size_t GetLeafCount() const;
    size_t GetMaxDepth() const;

    void PrintTreeStructure(std::ostream& os = std::cout) const;

private:
    void PrintBinaryTree(size_t nodeId, std::ostream& os, std::string prefix, bool isLeft) const;

#pragma endregion

#pragma region Internal Operations
private:
    size_t AllocateNode();
    void FreeNode(size_t nodeId);
    void InsertLeaf(size_t leafId);
    void RemoveLeaf(size_t leafId);
    size_t Rebalance(size_t nodeId);
    void UpdateNodeBounds(size_t nodeId, const FMAABB& bounds);
    void CreateFatBounds(size_t nodeId);

    bool IsValidNodeId(size_t nodeId) const;
    void QueryOverlapRecursive(size_t nodeId, const FMAABB& queryBounds,
                               const std::function<void(size_t)>& callback) const;

#pragma endregion

#pragma region Member Variables
private:
    std::vector<Node> NodePool;
    std::unordered_set<size_t> FreeNodes;

    size_t RootId = NULL_NODE;
    size_t NodeCount = 0;
    float FatMarginRatio = DEFAULT_FAT_MARGIN;

#pragma endregion
};