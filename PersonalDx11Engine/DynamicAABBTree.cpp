#include "DynamicAABBTree.h"

FDynamicAABBTree::FDynamicAABBTree(size_t InitialCapacity)
{
    NodePool.resize(InitialCapacity);
    FreeNodes.reserve(InitialCapacity);

    // �ʱ� free list ����
    for (size_t i = 0; i < InitialCapacity - 1; ++i)
    {
        FreeNodes.push_back(i);
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

    size_t NodeId = AllocateNode();
    Node& NewNode = NodePool[NodeId];

    // �ʱ� �ٿ�� ����
    const Vector3 Position = Object->GetTransform()->Position;
    const Vector3 HalfExtent = Object->GetHalfExtent();

    // ���� AABB ����
    NewNode.Bounds.Min = Position - HalfExtent;
    NewNode.Bounds.Max = Position + HalfExtent;

    // Fat AABB ���� (���� �߰�)
    Vector3 Margin = HalfExtent * AABB_MULTIPLIER + Vector3(AABB_EXTENSION, AABB_EXTENSION, AABB_EXTENSION);
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
    if (NodeId >= NodePool.size() || NodeId == NULL_NODE)
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

        const Vector3& CurrentPos = Node.BoundableObject->GetTransform()->Position;
        const Vector3& CurrentExtent = Node.BoundableObject->GetHalfExtent();

        if (Node.NeedsUpdate(CurrentPos, CurrentExtent))
        {
            NodesToUpdate.push_back(i);
        }
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
            FreeNodes.push_back(i);
        }
    }

    size_t NodeId = FreeNodes.back();
    FreeNodes.pop_back();
    NodePool[NodeId] = Node();  // �ʱ�ȭ
    NodeCount++;
    return NodeId;
}

void FDynamicAABBTree::FreeNode(size_t NodeId)
{
    if (NodeId >= NodePool.size())
        return;

    NodePool[NodeId] = Node();  // �缳��
    FreeNodes.push_back(NodeId);
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

    size_t ParentId = NodePool[LeafId].Parent;
    size_t GrandParentId = NodePool[ParentId].Parent;
    size_t SiblingId = (NodePool[ParentId].Left == LeafId) ?
        NodePool[ParentId].Right : NodePool[ParentId].Left;

    if (GrandParentId != NULL_NODE)
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
    Node& N = NodePool[NodeId];
    if (N.IsLeaf() || N.Height < 2)
        return NodeId;

    size_t LeftId = N.Left;
    size_t RightId = N.Right;
    Node& LeftChild = NodePool[LeftId];
    Node& RightChild = NodePool[RightId];

    int Balance = RightChild.Height - LeftChild.Height;

    // �������� �� ���� ���
    if (Balance > 1)
    {
        size_t RightLeftId = RightChild.Left;
        size_t RightRightId = RightChild.Right;
        Node& RightLeft = NodePool[RightLeftId];
        Node& RightRight = NodePool[RightRightId];

        // RightChild�� ���ο� ��Ʈ��
        RightChild.Left = NodeId;
        RightChild.Parent = N.Parent;
        N.Parent = RightId;

        if (RightChild.Parent != NULL_NODE)
        {
            if (NodePool[RightChild.Parent].Left == NodeId)
            {
                NodePool[RightChild.Parent].Left = RightId;
            }
            else
            {
                NodePool[RightChild.Parent].Right = RightId;
            }
        }
        else
        {
            RootId = RightId;
        }

        // ���� ����
        N.Height = 1 + std::max(LeftChild.Height, RightLeft.Height);
        RightChild.Height = 1 + std::max(N.Height, RightRight.Height);

        return RightId;
    }
    // ������ �� ���� ���
    if (Balance < -1)
    {
        size_t LeftLeftId = LeftChild.Left;
        size_t LeftRightId = LeftChild.Right;
        Node& LeftLeft = NodePool[LeftLeftId];
        Node& LeftRight = NodePool[LeftRightId];

        // LeftChild�� ���ο� ��Ʈ��
        LeftChild.Right = NodeId;
        LeftChild.Parent = N.Parent;
        N.Parent = LeftId;

        if (LeftChild.Parent != NULL_NODE)
        {
            if (NodePool[LeftChild.Parent].Left == NodeId)
            {
                NodePool[LeftChild.Parent].Left = LeftId;
            }
            else
            {
                NodePool[LeftChild.Parent].Right = LeftId;
            }
        }
        else
        {
            RootId = LeftId;
        }

        // ���� ����
        N.Height = 1 + std::max(RightChild.Height, LeftRight.Height);
        LeftChild.Height = 1 + std::max(LeftLeft.Height, N.Height);

        return LeftId;
    }

    return NodeId;
}

void FDynamicAABBTree::UpdateNodeBounds(size_t NodeId)
{
    Node& UpdateNode = NodePool[NodeId];
    if (!UpdateNode.BoundableObject)
        return;

    const Vector3& Position = UpdateNode.BoundableObject->GetTransform()->Position;
    const Vector3& HalfExtent = UpdateNode.BoundableObject->GetHalfExtent();

    // ���� AABB ������Ʈ
    UpdateNode.Bounds.Min = Position - HalfExtent;
    UpdateNode.Bounds.Max = Position + HalfExtent;

    // Fat AABB ������Ʈ
    Vector3 Margin = HalfExtent * AABB_MULTIPLIER + Vector3(AABB_EXTENSION, AABB_EXTENSION, AABB_EXTENSION);
    UpdateNode.FatBounds.Min = Position - Margin;
    UpdateNode.FatBounds.Max = Position + Margin;

    // ���� ���� ����
    UpdateNode.LastPosition = Position;
    UpdateNode.LastHalfExtent = HalfExtent;
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

template<typename Callback>
void FDynamicAABBTree::QueryOverlap(const AABB& QueryBounds, Callback&& Func)
{
    std::vector<size_t> Stack;
    Stack.reserve(NodeCount);

    // ��Ʈ���� ����
    if (RootId != NULL_NODE)
        Stack.push_back(RootId);

    while (!Stack.empty())
    {
        size_t NodeId = Stack.back();
        Stack.pop_back();

        const Node& CurrentNode = NodePool[NodeId];

        // AABB�� ��ġ�� ������ ��ŵ
        if (!QueryBounds.Overlaps(CurrentNode.Bounds))
            continue;

        if (CurrentNode.IsLeaf())
        {
            // ���� ���� �ݹ� ȣ��
            if (CurrentNode.BoundableObject)
            {
                Func(CurrentNode.BoundableObject);
            }
        }
        else
        {
            // ���� ���� �ڽĵ��� ���ÿ� �߰�
            Stack.push_back(CurrentNode.Left);
            Stack.push_back(CurrentNode.Right);
        }
    }
}