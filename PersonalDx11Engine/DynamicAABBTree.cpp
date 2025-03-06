#include "DynamicAABBTree.h"
#include <iostream>
#include <queue>


FDynamicAABBTree::FDynamicAABBTree(size_t InitialCapacity)
{
    NodePool.resize(InitialCapacity);
    FreeNodes.reserve(InitialCapacity);

    // 초기 free list 구성
    for (size_t i = 0; i < InitialCapacity - 1; ++i)
    {
        FreeNodes.insert(i);
    }
}

FDynamicAABBTree::~FDynamicAABBTree()
{
    NodePool.clear();
    FreeNodes.clear();
}

size_t FDynamicAABBTree::Insert(const std::shared_ptr<IDynamicBoundable>& Object)
{
    if (!Object)
        return NULL_NODE;
    // 중복 검사
    for (size_t i = 0; i < NodePool.size(); ++i)
    {
        const Node& ExistingNode = NodePool[i];
        // FreeNodes에 포함되지 않은 노드 중에서만 검사
        if (FreeNodes.find(i) == FreeNodes.end())
        {
            if (ExistingNode.BoundableObject == Object.get())
            {
                return FDynamicAABBTree::NULL_NODE; // 이미 존재하는 객체면 추가 실패
            }
        }
    }
    size_t NodeId = AllocateNode();
    Node& NewNode = NodePool[NodeId];

    // 초기 바운드 설정
    auto OwnerTrans = Object->GetTransform();
    const Vector3 Position = Object->GetTransform()->GetPosition();
    const Vector3 HalfExtent = Object->GetHalfExtent();
    
    // 실제 AABB 설정
    NewNode.Bounds.Min = Position - HalfExtent;
    NewNode.Bounds.Max = Position + HalfExtent;

    // Fat AABB 설정 (마진 추가)
    Vector3 Margin = HalfExtent * (1.0f + AABB_Extension) + Vector3::One * (MIN_MARGIN);  // 매우 작은 AABB를 위한 최소 여유
    NewNode.FatBounds.Min = Position - Margin;
    NewNode.FatBounds.Max = Position + Margin;

    // 추적을 위한 마지막 상태 저장
    NewNode.LastPosition = Position;
    NewNode.LastHalfExtent = HalfExtent;
    NewNode.BoundableObject = Object.get();
    NewNode.Height = 0;

    InsertLeaf(NodeId);
    return NodeId;
}

void FDynamicAABBTree::Remove(size_t NodeId)
{
    if (!IsValidId(NodeId))
        return;

    RemoveLeaf(NodeId);
    FreeNode(NodeId);
}

void FDynamicAABBTree::UpdateTree()
{
    // 루트가 없으면 종료
    if (RootId == NULL_NODE)
        return;

    std::vector<size_t> NodesToUpdate;
    NodesToUpdate.reserve(NodeCount);

    // 리프 노드들의 바운드 체크 및 업데이트 필요 노드 수집
    for (size_t i = 0; i < NodePool.size(); ++i)
    {
        Node& Node = NodePool[i];
        if (!Node.IsLeaf() || !Node.BoundableObject)
            continue;

        const Vector3& CurrentPos = Node.BoundableObject->GetTransform()->GetPosition();
        const Vector3& CurrentExtent = Node.BoundableObject->GetHalfExtent();

        if (Node.NeedsUpdate(CurrentPos, CurrentExtent))
        {
            NodesToUpdate.push_back(i);
        }

        //if (Node.BoundableObject->IsTransformChanged())
        //{
        //    NodesToUpdate.push_back(i);
        //}
    }

    // 수집된 노드들 업데이트
    for (size_t NodeId : NodesToUpdate)
    {
        RemoveLeaf(NodeId);
        UpdateNodeBounds(NodeId);
        InsertLeaf(NodeId);
    }
}

size_t FDynamicAABBTree::AllocateNode()
{
    if (FreeNodes.empty())
    {
        // 노드 풀 확장
        size_t OldSize = NodePool.size();
        size_t NewSize = OldSize * 2;
        NodePool.resize(NewSize);

        // 새로운 free 노드들 추가
        FreeNodes.reserve(NewSize - OldSize);
        for (size_t i = OldSize; i < NewSize; ++i)
        {
            FreeNodes.insert(i);
        }
    }

    auto it = FreeNodes.begin();
    size_t NodeId = *it;
    FreeNodes.erase(it);
    NodePool[NodeId] = Node();  // 초기화
    NodeCount++;
    return NodeId;
}

void FDynamicAABBTree::FreeNode(size_t NodeId)
{
    if (NodeId >= NodePool.size())
        return;

    NodePool[NodeId] = Node();  // 재설정
    FreeNodes.insert(NodeId);
    NodeCount--;
}

void FDynamicAABBTree::InsertLeaf(size_t LeafId)
{
    // 첫 노드면 루트로 설정
    if (RootId == NULL_NODE)
    {
        RootId = LeafId;
        NodePool[RootId].Parent = NULL_NODE;
        return;
    }

    //FreeNode에 없는 경우 이미 트리에 포함되어 있음
    if (FreeNodes.find(LeafId) == FreeNodes.end())
        return;

    // 삽입 위치 찾기
    Node& Leaf = NodePool[LeafId];
    size_t CurrentId = RootId;

    while (!NodePool[CurrentId].IsLeaf())
    {
        Node& Current = NodePool[CurrentId];
        size_t LeftId = Current.Left;
        size_t RightId = Current.Right;

        // SAH 비용 계산
        float CurrentCost = ComputeCost(Current.Bounds);
        AABB CombinedBounds;
        CombinedBounds.Min = Vector3::Min(Current.Bounds.Min, Leaf.Bounds.Min);
        CombinedBounds.Max = Vector3::Max(Current.Bounds.Max, Leaf.Bounds.Max);
        float CombinedCost = ComputeCost(CombinedBounds);

        // 왼쪽, 오른쪽 자식과의 결합 비용 계산
        Node& LeftChild = NodePool[LeftId];
        Node& RightChild = NodePool[RightId];

        AABB LeftCombined;
        LeftCombined.Min = Vector3::Min(LeftChild.Bounds.Min, Leaf.Bounds.Min);
        LeftCombined.Max = Vector3::Max(LeftChild.Bounds.Max, Leaf.Bounds.Max);
        float LeftCost = ComputeCost(LeftCombined);

        AABB RightCombined;
        RightCombined.Min = Vector3::Min(RightChild.Bounds.Min, Leaf.Bounds.Min);
        RightCombined.Max = Vector3::Max(RightChild.Bounds.Max, Leaf.Bounds.Max);
        float RightCost = ComputeCost(RightCombined);

        // 최소 비용 경로 선택
        if (LeftCost < RightCost && LeftCost < CombinedCost)
        {
            CurrentId = LeftId;
        }
        else if (RightCost < CombinedCost)
        {
            CurrentId = RightId;
        }
        else 
        {
            // 현재 노드에 직접 합치는 것이 가장 효율적인 경우
            break;
        }
    }

    // 새로운 부모 노드 생성
    size_t NewParentId = AllocateNode();
    Node& NewParent = NodePool[NewParentId];

    size_t OldParentId = NodePool[CurrentId].Parent;
    NewParent.Parent = OldParentId;

    // 새 부모의 AABB 설정
    NewParent.Bounds.Min = Vector3::Min(Leaf.Bounds.Min, NodePool[CurrentId].Bounds.Min);
    NewParent.Bounds.Max = Vector3::Max(Leaf.Bounds.Max, NodePool[CurrentId].Bounds.Max);
    NewParent.Height = NodePool[CurrentId].Height + 1;

    if (OldParentId != NULL_NODE)
    {
        // 기존 부모의 자식 포인터 업데이트
        if (NodePool[OldParentId].Left == CurrentId)
        {
            NodePool[OldParentId].Left = NewParentId;
        }
        else
        {
            NodePool[OldParentId].Right = NewParentId;
        }
    }
    else
    {
        // 루트 업데이트
        RootId = NewParentId;
    }

    // 새 부모의 자식 설정
    NewParent.Left = CurrentId;
    NewParent.Right = LeafId;
    NodePool[CurrentId].Parent = NewParentId;
    NodePool[LeafId].Parent = NewParentId;

    // 조상 노드들의 AABB 업데이트
    CurrentId = NewParentId;
    while (CurrentId != NULL_NODE)
    {
        CurrentId = Rebalance(CurrentId);

        Node& Current = NodePool[CurrentId];
        Node& LeftChild = NodePool[Current.Left];
        Node& RightChild = NodePool[Current.Right];

        Current.Height = 1 + std::max(LeftChild.Height, RightChild.Height);
        Current.Bounds.Min = Vector3::Min(LeftChild.Bounds.Min, RightChild.Bounds.Min);
        Current.Bounds.Max = Vector3::Max(LeftChild.Bounds.Max, RightChild.Bounds.Max);

        CurrentId = Current.Parent;
    }
}

void FDynamicAABBTree::RemoveLeaf(size_t LeafId)
{
    if (LeafId == RootId)
    {
        RootId = NULL_NODE;
        return;
    }

    if (!IsValidId(LeafId))
        return;

    size_t ParentId = NodePool[LeafId].Parent;
    if (!IsValidId(ParentId))
        return;  

    size_t GrandParentId = NodePool[ParentId].Parent;
    size_t SiblingId = (NodePool[ParentId].Left == LeafId) ?
        NodePool[ParentId].Right : NodePool[ParentId].Left;

    if (!IsValidId(SiblingId))
        return;  // 형제가 유효하지 않으면 종료

    if (IsValidId(GrandParentId))
    {
        // 형제를 조부모에 직접 연결
        if (NodePool[GrandParentId].Left == ParentId)
        {
            NodePool[GrandParentId].Left = SiblingId;
        }
        else
        {
            NodePool[GrandParentId].Right = SiblingId;
        }
        NodePool[SiblingId].Parent = GrandParentId;

        // 부모 노드 제거
        FreeNode(ParentId);

        // 조상들의 AABB 업데이트
        size_t CurrentId = GrandParentId;
        while (CurrentId != NULL_NODE)
        {
            CurrentId = Rebalance(CurrentId);

            Node& Current = NodePool[CurrentId];
            Node& LeftChild = NodePool[Current.Left];
            Node& RightChild = NodePool[Current.Right];

            Current.Bounds.Min = Vector3::Min(LeftChild.Bounds.Min, RightChild.Bounds.Min);
            Current.Bounds.Max = Vector3::Max(LeftChild.Bounds.Max, RightChild.Bounds.Max);
            Current.Height = 1 + std::max(LeftChild.Height, RightChild.Height);

            CurrentId = Current.Parent;
        }
    }
    else
    {
        // 형제를 새로운 루트로
        RootId = SiblingId;
        NodePool[SiblingId].Parent = NULL_NODE;
        FreeNode(ParentId);
    }
}

size_t FDynamicAABBTree::Rebalance(size_t NodeId)
{
    // 기본 ID 검증
    if (!IsValidId(NodeId))
        return NULL_NODE;

    Node& N = NodePool[NodeId];
    if (N.IsLeaf() || N.Height < 2)
        return NodeId;

    size_t LeftId = N.Left;
    size_t RightId = N.Right;

    // 자식 노드 유효성 검사
    if (!IsValidId(LeftId) || !IsValidId(RightId))
        return NodeId;

    Node& LeftChild = NodePool[LeftId];
    Node& RightChild = NodePool[RightId];

    int Balance = RightChild.Height - LeftChild.Height;

    // 오른쪽이 더 깊은 경우
    if (Balance > 1)
    {
        size_t RightLeftId = RightChild.Left;
        size_t RightRightId = RightChild.Right;

        // 추가 자식 노드 유효성 검사
        if (!IsValidId(RightLeftId) || !IsValidId(RightRightId))
            return NodeId;

        Node& RightLeft = NodePool[RightLeftId];
        Node& RightRight = NodePool[RightRightId];

        // 부모-자식 관계 업데이트
        N.Right = RightLeftId;              // 1. N의 오른쪽 자식을  RightLeft로 변경
        if (IsValidId(RightLeftId))
            RightLeft.Parent = NodeId;      // 2. RightLeft의 부모를 N으로 설정

        RightChild.Left = NodeId;           // 3. RightChild의 왼쪽 자식을 N으로 설정
        RightChild.Parent = N.Parent;       // 4. RightChild의 부모를 N의 부모로 설정
        N.Parent = RightId;                 // 5. N의 부모를 RightChild로 설정

        // 루트 노드 업데이트
        if (RightChild.Parent != NULL_NODE)
        {
            if (NodePool[RightChild.Parent].Left == NodeId)
                NodePool[RightChild.Parent].Left = RightId;
            else
                NodePool[RightChild.Parent].Right = RightId;
        }
        else
            RootId = RightId;

        // 높이 조정
        N.Height = 1 + std::max(LeftChild.Height,
                                IsValidId(RightLeftId) ? RightLeft.Height : 0);
        RightChild.Height = 1 + std::max(N.Height, RightRight.Height);

        //AABB 업데이트
        // N의 새 AABB 계산 (LeftChild와 RightLeft의 조합)
        if (IsValidId(RightLeftId)) {
            N.Bounds.Min = Vector3::Min(LeftChild.Bounds.Min, RightLeft.Bounds.Min);
            N.Bounds.Max = Vector3::Max(LeftChild.Bounds.Max, RightLeft.Bounds.Max);
        }
        else {
            N.Bounds = LeftChild.Bounds; 
        }

        // RightChild의 새 AABB 계산 (N과 RightRight의 조합)
        RightChild.Bounds.Min = Vector3::Min(N.Bounds.Min, RightRight.Bounds.Min);
        RightChild.Bounds.Max = Vector3::Max(N.Bounds.Max, RightRight.Bounds.Max);

        return RightId;
    }

    // 왼쪽이 더 깊은 경우
    if (Balance < -1)
    {
        size_t LeftLeftId = LeftChild.Left;
        size_t LeftRightId = LeftChild.Right;

        // 추가 자식 노드 유효성 검사
        if (!IsValidId(LeftLeftId) || !IsValidId(LeftRightId))
            return NodeId;

        Node& LeftLeft = NodePool[LeftLeftId];
        Node& LeftRight = NodePool[LeftRightId];

        // 부모-자식 관계 업데이트
        N.Left = LeftRightId;               // 1. N의 왼쪽 자식을 LeftRight으로 변경
        if (IsValidId(LeftRightId))
            LeftRight.Parent = NodeId;      // 2. LeftRight의 부모를 N으로 설정

        LeftChild.Right = NodeId;           // 3. LeftChild의 오른쪽 자식을 N으로 설정
        LeftChild.Parent = N.Parent;        // 4. LeftChild의 부모를 N의 부모로 설정
        N.Parent = LeftId;                  // 5. N의 부모를 LeftChild로 설정

        // 루트 노드 업데이트
        if (LeftChild.Parent != NULL_NODE)
        {
            if (NodePool[LeftChild.Parent].Left == NodeId)
                NodePool[LeftChild.Parent].Left = LeftId;
            else
                NodePool[LeftChild.Parent].Right = LeftId;
        }
        else
            RootId = LeftId;

        // 높이 조정
        N.Height = 1 + std::max(RightChild.Height,
                                IsValidId(LeftRightId) ? LeftRight.Height : 0);
        LeftChild.Height = 1 + std::max(LeftLeft.Height, N.Height);

        // AABB업데이트
        // N의 새 AABB 계산 (RightChild와 LeftRight의 조합)
        if (IsValidId(LeftRightId)) {
            N.Bounds.Min = Vector3::Min(RightChild.Bounds.Min, LeftRight.Bounds.Min);
            N.Bounds.Max = Vector3::Max(RightChild.Bounds.Max, LeftRight.Bounds.Max);
        }
        else {
            N.Bounds = RightChild.Bounds; // 만약 LeftRight가 없다면
        }

        // LeftChild의 새 AABB 계산 (N과 LeftLeft의 조합)
        LeftChild.Bounds.Min = Vector3::Min(N.Bounds.Min, LeftLeft.Bounds.Min);
        LeftChild.Bounds.Max = Vector3::Max(N.Bounds.Max, LeftLeft.Bounds.Max);

        return LeftId;
    }

    return NodeId;
}

void FDynamicAABBTree::UpdateNodeBounds(size_t NodeId)
{
    Node& UpdateNode = NodePool[NodeId];
    if (!UpdateNode.BoundableObject)
        return;

    const Vector3& Position = UpdateNode.BoundableObject->GetTransform()->GetPosition();
    const Vector3& HalfExtent = UpdateNode.BoundableObject->GetHalfExtent();

    // 실제 AABB 업데이트
    UpdateNode.Bounds.Min = Position - HalfExtent;
    UpdateNode.Bounds.Max = Position + HalfExtent;

    // Fat AABB 업데이트
    Vector3 Margin = HalfExtent * (1.0f + AABB_Extension) + Vector3::One * MIN_MARGIN;
    UpdateNode.FatBounds.Min = Position - Margin;
    UpdateNode.FatBounds.Max = Position + Margin;

    // 이전 상태 저장
    UpdateNode.LastPosition = Position;
    UpdateNode.LastHalfExtent = HalfExtent;

    UpdateNode.BoundableObject->SetTransformChagedClean();
}

float FDynamicAABBTree::ComputeCost(const AABB& Bounds) const
{
    Vector3 Dimensions = Bounds.Max - Bounds.Min;
    // 표면적 휴리스틱 (Surface Area Heuristic)
    return 2.0f * (Dimensions.x * Dimensions.y + Dimensions.y * Dimensions.z + Dimensions.z * Dimensions.x);
}

float FDynamicAABBTree::ComputeInheritedCost(size_t NodeId) const
{
    if (NodeId == NULL_NODE)
        return 0.0f;

    float Cost = ComputeCost(NodePool[NodeId].Bounds);
    Cost += ComputeInheritedCost(NodePool[NodeId].Parent);
    return Cost;
}

bool FDynamicAABBTree::IsValidId(const size_t NodeId) const
{
    return NodeId < NodePool.size() 
        && NodeId != NULL_NODE
        && FreeNodes.find(NodeId) == FreeNodes.end();
}

void FDynamicAABBTree::ReBuildTree()
{
    // 현재 활성 노드 백업
    std::vector<std::shared_ptr<IDynamicBoundable>> activeObjects;

    for (size_t i = 0; i < NodePool.size(); i++)
    {
        if (FreeNodes.find(i) == FreeNodes.end() && NodePool[i].BoundableObject)
        {
            activeObjects.push_back(std::shared_ptr<IDynamicBoundable>(
                const_cast<IDynamicBoundable*>(NodePool[i].BoundableObject),
                [](IDynamicBoundable*) {} //임시 참조 shared_ptr
            ));
        }
    }

    // 트리 초기화
    ClearTree();

    // 활성 객체 다시 삽입
    for (const auto& obj : activeObjects)
    {
        Insert(obj);
    }
}

void FDynamicAABBTree::ClearTree(const size_t InitialCapacity)
{
    NodePool.clear();
    FreeNodes.clear();
    RootId = NULL_NODE;
    NodeCount = 0;

    // 초기 용량으로 다시 초기화
    NodePool.resize(InitialCapacity);
    FreeNodes.reserve(InitialCapacity);

    for (size_t i = 0; i < InitialCapacity; i++)
    {
        FreeNodes.insert(i);
    }
}

void FDynamicAABBTree::PrintTreeStructure() const
{
    PrintBinaryTree(RootId);
}

void FDynamicAABBTree::QueryOverlap(const AABB& QueryBounds, const std::function<void(size_t)>& Func)
{
    std::vector<size_t> Stack;
    std::unordered_set<size_t> visited;
    Stack.reserve(NodeCount);

    // 루트부터 시작
    if (RootId != NULL_NODE)
        Stack.push_back(RootId);

    while (!Stack.empty())
    {
        size_t NodeId = Stack.back();
        Stack.pop_back();
        if (visited.find(NodeId) == visited.end())
        {
            visited.insert(NodeId);
        }
        else
        {
            continue;
        }

        if (!IsValidId(NodeId))
            continue;

        const Node& CurrentNode = NodePool[NodeId];

		// AABB가 겹치지 않으면 스킵
		if (!QueryBounds.Overlaps(CurrentNode.Bounds))
			continue;

        if (CurrentNode.IsLeaf())
        {
            // 리프 노드면 콜백 호출
            if (CurrentNode.BoundableObject)
            {
                Func(NodeId);
            }
        }
        else
        {
            // 내부 노드면 자식들을 스택에 추가
            if (CurrentNode.Left != NULL_NODE && CurrentNode.Left < NodePool.size()) {
                Stack.push_back(CurrentNode.Left);
            }
            if (CurrentNode.Right != NULL_NODE && CurrentNode.Right < NodePool.size()) {
                Stack.push_back(CurrentNode.Right);
            }
        }
    }
}


void FDynamicAABBTree::PrintBinaryTree(size_t root, std::string prefix, bool isLeft) const
{
    using namespace std;

    if (!IsValidId(root)) 
        return;

    cout << prefix;

    cout << (isLeft ? "├── " : "└── ");

    auto RootNode = NodePool[root];
    // 노드 데이터 출력 및 부모 정보 추가
    cout << root;
    if (RootNode.Parent != NULL_NODE) {
        cout << " (Parent: " << RootNode.Parent << ")";
    }
    cout << endl;

    // 자식 노드에 대한 새 접두사 계산
    string newPrefix = prefix + (isLeft ? "│   " : "    ");

    // 왼쪽, 오른쪽 자식 출력 (왼쪽 먼저)
    PrintBinaryTree(RootNode.Left, newPrefix, true);
    PrintBinaryTree(RootNode.Right, newPrefix, false);
}