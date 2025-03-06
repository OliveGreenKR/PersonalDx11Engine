#include "DynamicAABBTree.h"
#include <iostream>
#include <queue>


FDynamicAABBTree::FDynamicAABBTree(size_t InitialCapacity)
{
    NodePool.resize(InitialCapacity);
    FreeNodes.reserve(InitialCapacity);

    // �ʱ� free list ����
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
    // �ߺ� �˻�
    for (size_t i = 0; i < NodePool.size(); ++i)
    {
        const Node& ExistingNode = NodePool[i];
        // FreeNodes�� ���Ե��� ���� ��� �߿����� �˻�
        if (FreeNodes.find(i) == FreeNodes.end())
        {
            if (ExistingNode.BoundableObject == Object.get())
            {
                return FDynamicAABBTree::NULL_NODE; // �̹� �����ϴ� ��ü�� �߰� ����
            }
        }
    }
    size_t NodeId = AllocateNode();
    Node& NewNode = NodePool[NodeId];

    // �ʱ� �ٿ�� ����
    auto OwnerTrans = Object->GetTransform();
    const Vector3 Position = Object->GetTransform()->GetPosition();
    const Vector3 HalfExtent = Object->GetHalfExtent();
    
    // ���� AABB ����
    NewNode.Bounds.Min = Position - HalfExtent;
    NewNode.Bounds.Max = Position + HalfExtent;

    // Fat AABB ���� (���� �߰�)
    Vector3 Margin = HalfExtent * (1.0f + AABB_Extension) + Vector3::One * (MIN_MARGIN);  // �ſ� ���� AABB�� ���� �ּ� ����
    NewNode.FatBounds.Min = Position - Margin;
    NewNode.FatBounds.Max = Position + Margin;

    // ������ ���� ������ ���� ����
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
    // ��Ʈ�� ������ ����
    if (RootId == NULL_NODE)
        return;

    std::vector<size_t> NodesToUpdate;
    NodesToUpdate.reserve(NodeCount);

    // ���� ������ �ٿ�� üũ �� ������Ʈ �ʿ� ��� ����
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

    // ������ ���� ������Ʈ
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
        // ��� Ǯ Ȯ��
        size_t OldSize = NodePool.size();
        size_t NewSize = OldSize * 2;
        NodePool.resize(NewSize);

        // ���ο� free ���� �߰�
        FreeNodes.reserve(NewSize - OldSize);
        for (size_t i = OldSize; i < NewSize; ++i)
        {
            FreeNodes.insert(i);
        }
    }

    auto it = FreeNodes.begin();
    size_t NodeId = *it;
    FreeNodes.erase(it);
    NodePool[NodeId] = Node();  // �ʱ�ȭ
    NodeCount++;
    return NodeId;
}

void FDynamicAABBTree::FreeNode(size_t NodeId)
{
    if (NodeId >= NodePool.size())
        return;

    NodePool[NodeId] = Node();  // �缳��
    FreeNodes.insert(NodeId);
    NodeCount--;
}

void FDynamicAABBTree::InsertLeaf(size_t LeafId)
{
    // ù ���� ��Ʈ�� ����
    if (RootId == NULL_NODE)
    {
        RootId = LeafId;
        NodePool[RootId].Parent = NULL_NODE;
        return;
    }

    //FreeNode�� ���� ��� �̹� Ʈ���� ���ԵǾ� ����
    if (FreeNodes.find(LeafId) == FreeNodes.end())
        return;

    // ���� ��ġ ã��
    Node& Leaf = NodePool[LeafId];
    size_t CurrentId = RootId;

    while (!NodePool[CurrentId].IsLeaf())
    {
        Node& Current = NodePool[CurrentId];
        size_t LeftId = Current.Left;
        size_t RightId = Current.Right;

        // SAH ��� ���
        float CurrentCost = ComputeCost(Current.Bounds);
        AABB CombinedBounds;
        CombinedBounds.Min = Vector3::Min(Current.Bounds.Min, Leaf.Bounds.Min);
        CombinedBounds.Max = Vector3::Max(Current.Bounds.Max, Leaf.Bounds.Max);
        float CombinedCost = ComputeCost(CombinedBounds);

        // ����, ������ �ڽİ��� ���� ��� ���
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

        // �ּ� ��� ��� ����
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
            // ���� ��忡 ���� ��ġ�� ���� ���� ȿ������ ���
            break;
        }
    }

    // ���ο� �θ� ��� ����
    size_t NewParentId = AllocateNode();
    Node& NewParent = NodePool[NewParentId];

    size_t OldParentId = NodePool[CurrentId].Parent;
    NewParent.Parent = OldParentId;

    // �� �θ��� AABB ����
    NewParent.Bounds.Min = Vector3::Min(Leaf.Bounds.Min, NodePool[CurrentId].Bounds.Min);
    NewParent.Bounds.Max = Vector3::Max(Leaf.Bounds.Max, NodePool[CurrentId].Bounds.Max);
    NewParent.Height = NodePool[CurrentId].Height + 1;

    if (OldParentId != NULL_NODE)
    {
        // ���� �θ��� �ڽ� ������ ������Ʈ
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
        // ��Ʈ ������Ʈ
        RootId = NewParentId;
    }

    // �� �θ��� �ڽ� ����
    NewParent.Left = CurrentId;
    NewParent.Right = LeafId;
    NodePool[CurrentId].Parent = NewParentId;
    NodePool[LeafId].Parent = NewParentId;

    // ���� ������ AABB ������Ʈ
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
        return;  // ������ ��ȿ���� ������ ����

    if (IsValidId(GrandParentId))
    {
        // ������ ���θ� ���� ����
        if (NodePool[GrandParentId].Left == ParentId)
        {
            NodePool[GrandParentId].Left = SiblingId;
        }
        else
        {
            NodePool[GrandParentId].Right = SiblingId;
        }
        NodePool[SiblingId].Parent = GrandParentId;

        // �θ� ��� ����
        FreeNode(ParentId);

        // ������� AABB ������Ʈ
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
        // ������ ���ο� ��Ʈ��
        RootId = SiblingId;
        NodePool[SiblingId].Parent = NULL_NODE;
        FreeNode(ParentId);
    }
}

size_t FDynamicAABBTree::Rebalance(size_t NodeId)
{
    // �⺻ ID ����
    if (!IsValidId(NodeId))
        return NULL_NODE;

    Node& N = NodePool[NodeId];
    if (N.IsLeaf() || N.Height < 2)
        return NodeId;

    size_t LeftId = N.Left;
    size_t RightId = N.Right;

    // �ڽ� ��� ��ȿ�� �˻�
    if (!IsValidId(LeftId) || !IsValidId(RightId))
        return NodeId;

    Node& LeftChild = NodePool[LeftId];
    Node& RightChild = NodePool[RightId];

    int Balance = RightChild.Height - LeftChild.Height;

    // �������� �� ���� ���
    if (Balance > 1)
    {
        size_t RightLeftId = RightChild.Left;
        size_t RightRightId = RightChild.Right;

        // �߰� �ڽ� ��� ��ȿ�� �˻�
        if (!IsValidId(RightLeftId) || !IsValidId(RightRightId))
            return NodeId;

        Node& RightLeft = NodePool[RightLeftId];
        Node& RightRight = NodePool[RightRightId];

        // �θ�-�ڽ� ���� ������Ʈ
        N.Right = RightLeftId;              // 1. N�� ������ �ڽ���  RightLeft�� ����
        if (IsValidId(RightLeftId))
            RightLeft.Parent = NodeId;      // 2. RightLeft�� �θ� N���� ����

        RightChild.Left = NodeId;           // 3. RightChild�� ���� �ڽ��� N���� ����
        RightChild.Parent = N.Parent;       // 4. RightChild�� �θ� N�� �θ�� ����
        N.Parent = RightId;                 // 5. N�� �θ� RightChild�� ����

        // ��Ʈ ��� ������Ʈ
        if (RightChild.Parent != NULL_NODE)
        {
            if (NodePool[RightChild.Parent].Left == NodeId)
                NodePool[RightChild.Parent].Left = RightId;
            else
                NodePool[RightChild.Parent].Right = RightId;
        }
        else
            RootId = RightId;

        // ���� ����
        N.Height = 1 + std::max(LeftChild.Height,
                                IsValidId(RightLeftId) ? RightLeft.Height : 0);
        RightChild.Height = 1 + std::max(N.Height, RightRight.Height);

        //AABB ������Ʈ
        // N�� �� AABB ��� (LeftChild�� RightLeft�� ����)
        if (IsValidId(RightLeftId)) {
            N.Bounds.Min = Vector3::Min(LeftChild.Bounds.Min, RightLeft.Bounds.Min);
            N.Bounds.Max = Vector3::Max(LeftChild.Bounds.Max, RightLeft.Bounds.Max);
        }
        else {
            N.Bounds = LeftChild.Bounds; 
        }

        // RightChild�� �� AABB ��� (N�� RightRight�� ����)
        RightChild.Bounds.Min = Vector3::Min(N.Bounds.Min, RightRight.Bounds.Min);
        RightChild.Bounds.Max = Vector3::Max(N.Bounds.Max, RightRight.Bounds.Max);

        return RightId;
    }

    // ������ �� ���� ���
    if (Balance < -1)
    {
        size_t LeftLeftId = LeftChild.Left;
        size_t LeftRightId = LeftChild.Right;

        // �߰� �ڽ� ��� ��ȿ�� �˻�
        if (!IsValidId(LeftLeftId) || !IsValidId(LeftRightId))
            return NodeId;

        Node& LeftLeft = NodePool[LeftLeftId];
        Node& LeftRight = NodePool[LeftRightId];

        // �θ�-�ڽ� ���� ������Ʈ
        N.Left = LeftRightId;               // 1. N�� ���� �ڽ��� LeftRight���� ����
        if (IsValidId(LeftRightId))
            LeftRight.Parent = NodeId;      // 2. LeftRight�� �θ� N���� ����

        LeftChild.Right = NodeId;           // 3. LeftChild�� ������ �ڽ��� N���� ����
        LeftChild.Parent = N.Parent;        // 4. LeftChild�� �θ� N�� �θ�� ����
        N.Parent = LeftId;                  // 5. N�� �θ� LeftChild�� ����

        // ��Ʈ ��� ������Ʈ
        if (LeftChild.Parent != NULL_NODE)
        {
            if (NodePool[LeftChild.Parent].Left == NodeId)
                NodePool[LeftChild.Parent].Left = LeftId;
            else
                NodePool[LeftChild.Parent].Right = LeftId;
        }
        else
            RootId = LeftId;

        // ���� ����
        N.Height = 1 + std::max(RightChild.Height,
                                IsValidId(LeftRightId) ? LeftRight.Height : 0);
        LeftChild.Height = 1 + std::max(LeftLeft.Height, N.Height);

        // AABB������Ʈ
        // N�� �� AABB ��� (RightChild�� LeftRight�� ����)
        if (IsValidId(LeftRightId)) {
            N.Bounds.Min = Vector3::Min(RightChild.Bounds.Min, LeftRight.Bounds.Min);
            N.Bounds.Max = Vector3::Max(RightChild.Bounds.Max, LeftRight.Bounds.Max);
        }
        else {
            N.Bounds = RightChild.Bounds; // ���� LeftRight�� ���ٸ�
        }

        // LeftChild�� �� AABB ��� (N�� LeftLeft�� ����)
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

    // ���� AABB ������Ʈ
    UpdateNode.Bounds.Min = Position - HalfExtent;
    UpdateNode.Bounds.Max = Position + HalfExtent;

    // Fat AABB ������Ʈ
    Vector3 Margin = HalfExtent * (1.0f + AABB_Extension) + Vector3::One * MIN_MARGIN;
    UpdateNode.FatBounds.Min = Position - Margin;
    UpdateNode.FatBounds.Max = Position + Margin;

    // ���� ���� ����
    UpdateNode.LastPosition = Position;
    UpdateNode.LastHalfExtent = HalfExtent;

    UpdateNode.BoundableObject->SetTransformChagedClean();
}

float FDynamicAABBTree::ComputeCost(const AABB& Bounds) const
{
    Vector3 Dimensions = Bounds.Max - Bounds.Min;
    // ǥ���� �޸���ƽ (Surface Area Heuristic)
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
    // ���� Ȱ�� ��� ���
    std::vector<std::shared_ptr<IDynamicBoundable>> activeObjects;

    for (size_t i = 0; i < NodePool.size(); i++)
    {
        if (FreeNodes.find(i) == FreeNodes.end() && NodePool[i].BoundableObject)
        {
            activeObjects.push_back(std::shared_ptr<IDynamicBoundable>(
                const_cast<IDynamicBoundable*>(NodePool[i].BoundableObject),
                [](IDynamicBoundable*) {} //�ӽ� ���� shared_ptr
            ));
        }
    }

    // Ʈ�� �ʱ�ȭ
    ClearTree();

    // Ȱ�� ��ü �ٽ� ����
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

    // �ʱ� �뷮���� �ٽ� �ʱ�ȭ
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

    // ��Ʈ���� ����
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

		// AABB�� ��ġ�� ������ ��ŵ
		if (!QueryBounds.Overlaps(CurrentNode.Bounds))
			continue;

        if (CurrentNode.IsLeaf())
        {
            // ���� ���� �ݹ� ȣ��
            if (CurrentNode.BoundableObject)
            {
                Func(NodeId);
            }
        }
        else
        {
            // ���� ���� �ڽĵ��� ���ÿ� �߰�
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

    cout << (isLeft ? "������ " : "������ ");

    auto RootNode = NodePool[root];
    // ��� ������ ��� �� �θ� ���� �߰�
    cout << root;
    if (RootNode.Parent != NULL_NODE) {
        cout << " (Parent: " << RootNode.Parent << ")";
    }
    cout << endl;

    // �ڽ� ��忡 ���� �� ���λ� ���
    string newPrefix = prefix + (isLeft ? "��   " : "    ");

    // ����, ������ �ڽ� ��� (���� ����)
    PrintBinaryTree(RootNode.Left, newPrefix, true);
    PrintBinaryTree(RootNode.Right, newPrefix, false);
}