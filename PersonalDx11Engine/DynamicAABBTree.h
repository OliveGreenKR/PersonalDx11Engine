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
        Vector3 HalfExtent;         // 여유 영역 (실제 크기보다 약간 크게)
        Vector3 ActualHalfExtent;   // 실제 영역
        const FTransform* Transform;  // 소유 객체의 Transform 참조
        Node* Parent;
        Node* Left;
        Node* Right;
        IDynamicBoundable* Object;  // 리프 노드인 경우만 유효
        int Height;                 // 리프 = 0, 나머지는 1 + max(left->Height, right->Height)

        AABB GetActualAABB() const {
            assert(Transform);
            Vector3 center = Transform->Position;
            return{ center - ActualHalfExtent , center + ActualHalfExtent };
        }
    };

    void Insert(IDynamicBoundable* Object);
    void Remove(IDynamicBoundable* Object);
    void UpdateTree();  // 변경된 노드들 업데이트

    std::vector<IDynamicBoundable*> QueryOverlaps(const Vector3& Bounds);
private:
    Node* Root = nullptr;
    std::unordered_map<IDynamicBoundable*, Node*> ObjectToNode;
    std::vector<Node*> ChangedNodes;  // 변경이 필요한 노드들 추적
};