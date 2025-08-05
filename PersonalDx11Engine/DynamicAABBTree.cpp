#include "DynamicAABBTree.h"
#include <iostream>
#include <queue>
#include "Debug.h"

#pragma region Constructor and Destructor

FDynamicAABBTree::FDynamicAABBTree(size_t initialCapacity)
{
    NodePool.resize(initialCapacity);
    FreeNodes.reserve(initialCapacity);

    // 초기 free list 구성
    for (size_t i = 0; i < initialCapacity; ++i)
    {
        FreeNodes.insert(i);
    }
}

FDynamicAABBTree::~FDynamicAABBTree()
{
    NodePool.clear();
    FreeNodes.clear();
}

#pragma endregion

#pragma region Core Tree Operations

size_t FDynamicAABBTree::Insert(const FMAABB& bounds)
{
    // 새 노드 할당
    size_t nodeId = AllocateNode();
    Node& newNode = NodePool[nodeId];

    // 노드 데이터 설정 (ObjectID 없이 AABB만)
    newNode.Bounds = bounds;
    newNode.Height = 0;

    // Fat AABB 생성
    CreateFatBounds(nodeId);

    // 트리에 리프 노드로 삽입
    InsertLeaf(nodeId);

    return nodeId;
}

void FDynamicAABBTree::Remove(size_t nodeId)
{
    if (!IsValidId(nodeId))
    {
        return;
    }

    RemoveLeaf(nodeId);
    FreeNode(nodeId);
}

void FDynamicAABBTree::UpdateTree()
{
    // 루트가 없으면 종료
    if (RootId == NULL_NODE)
    {
        return;
    }

    // 업데이트가 필요한 노드들 수집
    std::vector<size_t> nodesToUpdate;
    nodesToUpdate.reserve(NodeCount);

    // 모든 리프 노드를 확인하여 Fat AABB를 벗어난 노드 찾기
    for (size_t i = 0; i < NodePool.size(); ++i)
    {
        if (FreeNodes.find(i) == FreeNodes.end()) // 할당된 노드만 검사
        {
            Node& node = NodePool[i];
            if (node.IsLeaf())
            {
                // 현재 Bounds가 FatBounds를 벗어났는지 확인
                if (!node.FatBounds.IsContaining(node.Bounds))
                {
                    nodesToUpdate.push_back(i);
                }
            }
        }
    }

    // 수집된 노드들 업데이트
    for (size_t nodeId : nodesToUpdate)
    {
        Node& node = NodePool[nodeId];

        // 트리에서 제거
        RemoveLeaf(nodeId);

        // Fat AABB 재생성
        CreateFatBounds(nodeId);

        // 트리에 다시 삽입
        InsertLeaf(nodeId);
    }
}

void FDynamicAABBTree::Clear()
{
    NodePool.clear();
    FreeNodes.clear();
    RootId = NULL_NODE;
    NodeCount = 0;

    // 초기 용량으로 다시 초기화
    size_t initialCapacity = 1024;
    NodePool.resize(initialCapacity);
    FreeNodes.reserve(initialCapacity);

    for (size_t i = 0; i < initialCapacity; ++i)
    {
        FreeNodes.insert(i);
    }
}

#pragma endregion

#pragma region Internal Operations

size_t FDynamicAABBTree::AllocateNode()
{
    if (FreeNodes.empty())
    {
        // 노드 풀 확장
        size_t oldSize = NodePool.size();
        size_t newSize = oldSize * 2;
        NodePool.resize(newSize);

        // 새로운 free 노드들 추가
        FreeNodes.reserve(newSize - oldSize);
        for (size_t i = oldSize; i < newSize; ++i)
        {
            FreeNodes.insert(i);
        }
    }

    auto it = FreeNodes.begin();
    size_t nodeId = *it;
    FreeNodes.erase(it);
    NodePool[nodeId] = Node();  // 노드 초기화
    NodeCount++;
    return nodeId;
}

void FDynamicAABBTree::FreeNode(size_t nodeId)
{
    if (nodeId >= NodePool.size())
    {
        return;
    }

    NodePool[nodeId] = Node();  // 노드 재설정
    FreeNodes.insert(nodeId);
    NodeCount--;
}

void FDynamicAABBTree::InsertLeaf(size_t leafId)
{
    // 첫 노드면 루트로 설정
    if (RootId == NULL_NODE)
    {
        RootId = leafId;
        NodePool[RootId].Parent = NULL_NODE;
        return;
    }

    // 삽입할 리프와 현재 루트
    Node& leaf = NodePool[leafId];
    size_t currentId = RootId;

    // 최적의 삽입 위치 찾기 (리프까지 내려가기)
    while (!NodePool[currentId].IsLeaf())
    {
        Node& current = NodePool[currentId];
        Node& leftChild = NodePool[current.Left];
        Node& rightChild = NodePool[current.Right];

        // 각 자식과 합쳤을 때의 확장 비용 계산
        FMAABB leftUnion = FMAABB::Merge(leftChild.Bounds, leaf.Bounds);
        FMAABB rightUnion = FMAABB::Merge(rightChild.Bounds, leaf.Bounds);

        Vector3 leftUnionSize, rightUnionSize;
        leftUnion.GetMaxV(leftUnionSize);
        Vector3 leftUnionMin;
        leftUnion.GetMinV(leftUnionMin);
        leftUnionSize = leftUnionSize - leftUnionMin;

        rightUnion.GetMaxV(rightUnionSize);
        Vector3 rightUnionMin;
        rightUnion.GetMinV(rightUnionMin);
        rightUnionSize = rightUnionSize - rightUnionMin;

        // 표면적 기반 비용 계산 (SAH - Surface Area Heuristic)
        float leftCost = leftUnionSize.x * leftUnionSize.y + leftUnionSize.y * leftUnionSize.z + leftUnionSize.z * leftUnionSize.x;
        float rightCost = rightUnionSize.x * rightUnionSize.y + rightUnionSize.y * rightUnionSize.z + rightUnionSize.z * rightUnionSize.x;

        // 비용이 낮은 쪽으로 이동
        currentId = (leftCost < rightCost) ? current.Left : current.Right;
    }

    // 새로운 부모 노드 생성
    size_t newParentId = AllocateNode();
    Node& newParent = NodePool[newParentId];
    Node& current = NodePool[currentId];

    size_t oldParentId = current.Parent;
    newParent.Parent = oldParentId;
    newParent.Bounds.vMin = XMVectorMin(leaf.Bounds.vMin, current.Bounds.vMin);
    newParent.Bounds.vMax = XMVectorMax(leaf.Bounds.vMax, current.Bounds.vMax);
    newParent.Height = current.Height + 1;

    if (oldParentId != NULL_NODE)
    {
        // 기존 부모의 자식 포인터 업데이트
        if (NodePool[oldParentId].Left == currentId)
        {
            NodePool[oldParentId].Left = newParentId;
        }
        else
        {
            NodePool[oldParentId].Right = newParentId;
        }
    }
    else
    {
        // 루트 업데이트
        RootId = newParentId;
    }

    // 새 부모의 자식 설정
    newParent.Left = currentId;
    newParent.Right = leafId;
    current.Parent = newParentId;
    leaf.Parent = newParentId;

    // 조상 노드들의 AABB 및 높이 업데이트
    currentId = newParentId;
    while (currentId != NULL_NODE)
    {
        currentId = Rebalance(currentId);

        Node& node = NodePool[currentId];
        Node& leftChild = NodePool[node.Left];
        Node& rightChild = NodePool[node.Right];

        node.Height = 1 + std::max(leftChild.Height, rightChild.Height);
        node.Bounds.vMin = XMVectorMin(leftChild.Bounds.vMin, rightChild.Bounds.vMin);
        node.Bounds.vMax = XMVectorMax(leftChild.Bounds.vMax, rightChild.Bounds.vMax);

        currentId = node.Parent;
    }
}

void FDynamicAABBTree::RemoveLeaf(size_t leafId)
{
    if (leafId == RootId)
    {
        RootId = NULL_NODE;
        return;
    }

    if (!IsValidNodeId(leafId))
    {
        return;
    }

    size_t parentId = NodePool[leafId].Parent;
    if (!IsValidNodeId(parentId))
    {
        return;
    }

    size_t grandParentId = NodePool[parentId].Parent;
    size_t siblingId = (NodePool[parentId].Left == leafId) ?
        NodePool[parentId].Right : NodePool[parentId].Left;

    if (grandParentId != NULL_NODE)
    {
        // 형제를 조부모에 직접 연결
        if (NodePool[grandParentId].Left == parentId)
        {
            NodePool[grandParentId].Left = siblingId;
        }
        else
        {
            NodePool[grandParentId].Right = siblingId;
        }
        NodePool[siblingId].Parent = grandParentId;

        // 부모 노드 해제
        FreeNode(parentId);

        // 조상 노드들 업데이트
        size_t currentId = grandParentId;
        while (currentId != NULL_NODE)
        {
            currentId = Rebalance(currentId);

            Node& node = NodePool[currentId];
            Node& leftChild = NodePool[node.Left];
            Node& rightChild = NodePool[node.Right];

            node.Height = 1 + std::max(leftChild.Height, rightChild.Height);
            node.Bounds.vMin = XMVectorMin(leftChild.Bounds.vMin, rightChild.Bounds.vMin);
            node.Bounds.vMax = XMVectorMax(leftChild.Bounds.vMax, rightChild.Bounds.vMax);

            currentId = node.Parent;
        }
    }
    else
    {
        // 부모가 루트였던 경우
        RootId = siblingId;
        NodePool[siblingId].Parent = NULL_NODE;
        FreeNode(parentId);
    }
}

size_t FDynamicAABBTree::Rebalance(size_t nodeId)
{
    if (!IsValidNodeId(nodeId))
    {
        return nodeId;
    }

    Node& node = NodePool[nodeId];

    if (node.IsLeaf() || node.Height < 2)
    {
        return nodeId;
    }

    size_t leftId = node.Left;
    size_t rightId = node.Right;

    if (!IsValidNodeId(leftId) || !IsValidNodeId(rightId))
    {
        return nodeId;
    }

    Node& leftChild = NodePool[leftId];
    Node& rightChild = NodePool[rightId];

    int32_t balance = rightChild.Height - leftChild.Height;

    // 오른쪽이 더 깊은 경우 (우측 회전 필요)
    if (balance > 1)
    {
        size_t rightLeftId = rightChild.Left;
        size_t rightRightId = rightChild.Right;

        if (!IsValidNodeId(rightLeftId) || !IsValidNodeId(rightRightId))
        {
            return nodeId;
        }

        Node& rightLeft = NodePool[rightLeftId];
        Node& rightRight = NodePool[rightRightId];

        // 부모-자식 관계 업데이트
        node.Right = rightLeftId;
        if (IsValidNodeId(rightLeftId))
        {
            rightLeft.Parent = nodeId;
        }

        rightChild.Left = nodeId;
        rightChild.Parent = node.Parent;
        node.Parent = rightId;

        // 루트 노드 업데이트
        if (rightChild.Parent != NULL_NODE)
        {
            if (NodePool[rightChild.Parent].Left == nodeId)
            {
                NodePool[rightChild.Parent].Left = rightId;
            }
            else
            {
                NodePool[rightChild.Parent].Right = rightId;
            }
        }
        else
        {
            RootId = rightId;
        }

        // 높이 및 AABB 조정
        node.Height = 1 + std::max(leftChild.Height, IsValidNodeId(rightLeftId) ? rightLeft.Height : 0);
        rightChild.Height = 1 + std::max(node.Height, rightRight.Height);

        // AABB 업데이트
        if (IsValidNodeId(rightLeftId))
        {
            node.Bounds.vMin = XMVectorMin(leftChild.Bounds.vMin, rightLeft.Bounds.vMin);
            node.Bounds.vMax = XMVectorMax(leftChild.Bounds.vMax, rightLeft.Bounds.vMax);
        }
        else
        {
            node.Bounds = leftChild.Bounds;
        }

        rightChild.Bounds.vMin = XMVectorMin(node.Bounds.vMin, rightRight.Bounds.vMin);
        rightChild.Bounds.vMax = XMVectorMax(node.Bounds.vMax, rightRight.Bounds.vMax);

        return rightId;
    }

    // 왼쪽이 더 깊은 경우 (좌측 회전 필요)
    if (balance < -1)
    {
        size_t leftLeftId = leftChild.Left;
        size_t leftRightId = leftChild.Right;

        if (!IsValidNodeId(leftLeftId) || !IsValidNodeId(leftRightId))
        {
            return nodeId;
        }

        Node& leftLeft = NodePool[leftLeftId];
        Node& leftRight = NodePool[leftRightId];

        // 부모-자식 관계 업데이트
        node.Left = leftRightId;
        if (IsValidNodeId(leftRightId))
        {
            leftRight.Parent = nodeId;
        }

        leftChild.Right = nodeId;
        leftChild.Parent = node.Parent;
        node.Parent = leftId;

        // 루트 노드 업데이트
        if (leftChild.Parent != NULL_NODE)
        {
            if (NodePool[leftChild.Parent].Left == nodeId)
            {
                NodePool[leftChild.Parent].Left = leftId;
            }
            else
            {
                NodePool[leftChild.Parent].Right = leftId;
            }
        }
        else
        {
            RootId = leftId;
        }

        // 높이 및 AABB 조정
        node.Height = 1 + std::max(rightChild.Height, IsValidNodeId(leftRightId) ? leftRight.Height : 0);
        leftChild.Height = 1 + std::max(leftLeft.Height, node.Height);

        // AABB 업데이트
        if (IsValidNodeId(leftRightId))
        {
            node.Bounds.vMin = XMVectorMin(rightChild.Bounds.vMin, leftRight.Bounds.vMin);
            node.Bounds.vMax = XMVectorMax(rightChild.Bounds.vMax, leftRight.Bounds.vMax);
        }
        else
        {
            node.Bounds = rightChild.Bounds;
        }

        leftChild.Bounds.vMin = XMVectorMin(node.Bounds.vMin, leftLeft.Bounds.vMin);
        leftChild.Bounds.vMax = XMVectorMax(node.Bounds.vMax, leftLeft.Bounds.vMax);

        return leftId;
    }

    return nodeId;
}

void FDynamicAABBTree::UpdateNodeBounds(size_t nodeId, const FMAABB& bounds)
{
    if (!IsValidNodeId(nodeId))
    {
        return;
    }

    Node& node = NodePool[nodeId];
    node.Bounds = bounds;
}

void FDynamicAABBTree::CreateFatBounds(size_t nodeId)
{
    if (!IsValidNodeId(nodeId))
    {
        return;
    }

    Node& node = NodePool[nodeId];

    // 현재 Bounds에서 크기 계산
    Vector3 min, max;
    node.Bounds.GetMinV(min);
    node.Bounds.GetMaxV(max);

    Vector3 size = max - min;
    Vector3 margin = size * FatMarginRatio + Vector3::One() * 0.01f;

    // Fat AABB 설정
    node.FatBounds.SetMin(min - margin);
    node.FatBounds.SetMax(max + margin);
}

bool FDynamicAABBTree::IsValidNodeId(size_t nodeId) const
{
    return nodeId < NodePool.size() &&
        nodeId != NULL_NODE &&
        FreeNodes.find(nodeId) == FreeNodes.end();
}

void FDynamicAABBTree::QueryOverlapRecursive(size_t nodeId, const FMAABB& queryBounds,
                                             const std::function<void(size_t)>& callback) const
{
    if (!IsValidNodeId(nodeId))
    {
        return;
    }

    const Node& node = NodePool[nodeId];

    // AABB가 겹치지 않으면 이 서브트리 전체 스킵
    if (!queryBounds.IsOverlapping(node.Bounds))
    {
        return;
    }

    if (node.IsLeaf())
    {
        // 리프 노드면 콜백 호출
        callback(nodeId);
    }
    else
    {
        // 내부 노드면 자식들을 재귀적으로 검사
        if (IsValidNodeId(node.Left))
        {
            QueryOverlapRecursive(node.Left, queryBounds, callback);
        }
        if (IsValidNodeId(node.Right))
        {
            QueryOverlapRecursive(node.Right, queryBounds, callback);
        }
    }
}

#pragma endregion

#pragma region Query Operations

void FDynamicAABBTree::QueryOverlap(const FMAABB& queryBounds, const std::function<void(size_t)>& callback) const
{
    // 루트가 없으면 종료
    if (RootId == NULL_NODE)
    {
        return;
    }

    // 재귀적으로 겹침 검사 수행
    QueryOverlapRecursive(RootId, queryBounds, callback);
}

#pragma endregion

#pragma region Data Access

const FMAABB& FDynamicAABBTree::GetBounds(size_t nodeId) const
{
    static FMAABB defaultAABB; // 기본값 반환용

    if (!IsValidId(nodeId))
    {
        return defaultAABB;
    }

    return NodePool[nodeId].Bounds;
}

const FMAABB& FDynamicAABBTree::GetFatBounds(size_t nodeId) const
{
    static FMAABB defaultAABB; // 기본값 반환용

    if (!IsValidId(nodeId))
    {
        return defaultAABB;
    }

    return NodePool[nodeId].FatBounds;
}

bool FDynamicAABBTree::IsValidId(size_t nodeId) const
{
    return IsValidNodeId(nodeId);
}

#pragma endregion

#pragma region Statistics and Debug

size_t FDynamicAABBTree::GetLeafCount() const
{
    size_t leafCount = 0;

    // 유효한 모든 노드를 순회하며 리프 노드 개수 계산
    for (size_t i = 0; i < NodePool.size(); ++i)
    {
        if (IsValidNodeId(i) && NodePool[i].IsLeaf())
        {
            leafCount++;
        }
    }

    return leafCount;
}

void FDynamicAABBTree::PrintTreeStructure(std::ostream& os) const
{
    if (RootId == NULL_NODE)
    {
        os << "Empty tree" << std::endl;
        return;
    }

    PrintBinaryTree(RootId, os);
}

#pragma endregion

#pragma region Statistics and Debug Helper Functions


void FDynamicAABBTree::PrintBinaryTree(size_t nodeId, std::ostream& os, 
                                       std::string prefix, bool isLeft) const
{
    if (!IsValidNodeId(nodeId))
    {
        return;
    }

    os << prefix;
    os << (isLeft ? "├── " : "└── ");

    const Node& node = NodePool[nodeId];

    // 노드 정보 출력
    if (node.IsLeaf())
    {
        os << "*"; // 리프 표시
    }

    os << nodeId;

    if (node.Parent != NULL_NODE)
    {
        os << " (Parent: " << node.Parent << ")";
    }

    // AABB 정보 출력 (간단하게)
    Vector3 min, max;
    node.Bounds.GetMinV(min);
    node.Bounds.GetMaxV(max);
    os << " AABB[(" << min.x << "," << min.y << "," << min.z << ") - ("
        << max.x << "," << max.y << "," << max.z << ")]";

    os << " H:" << node.Height;
    os << std::endl;

    // 자식 노드에 대한 새 접두사 계산
    std::string newPrefix = prefix + (isLeft ? "│   " : "    ");

    // 왼쪽, 오른쪽 자식 출력
    if (IsValidNodeId(node.Left))
    {
        PrintBinaryTree(node.Left, os, newPrefix, true);
    }

    if (IsValidNodeId(node.Right))
    {
        PrintBinaryTree(node.Right, os, newPrefix, false);
    }
}

#pragma endregion