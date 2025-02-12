#pragma once
#include "Math.h"
#include "Transform.h"
#include "DynamicBoundableInterface.h"
#include <vector>
#include <unordered_map>

class FDynamicAABBTree
{
public:
    struct AABB
    {
        Vector3 Min;
        Vector3 Max;
    };
    struct Node
    {
        Vector3 HalfExtent;         // ���� ���� (���� ũ�⺸�� �ణ ũ��)
        Vector3 ActualHalfExtent;   // ���� ����
        const FTransform* Transform;  // ���� ��ü�� Transform ����
        Node* Parent;
        Node* Left;
        Node* Right;
        IDynamicBoundable* Object;  // ���� ����� ��츸 ��ȿ
        int Height;                 // ���� = 0, �������� 1 + max(left->Height, right->Height)

        AABB GetActualAABB() const {
            assert(Transform);
            Vector3 center = Transform->Position;
            return{ center - ActualHalfExtent , center + ActualHalfExtent };
        }
    };

    void Insert(IDynamicBoundable* Object);
    void Remove(IDynamicBoundable* Object);
    void UpdateTree();  // ����� ���� ������Ʈ

    std::vector<IDynamicBoundable*> QueryOverlaps(const Vector3& Bounds);
private:
    Node* Root = nullptr;
    std::unordered_map<IDynamicBoundable*, Node*> ObjectToNode;
    std::vector<Node*> ChangedNodes;  // ������ �ʿ��� ���� ����
};