#pragma once
#include <vector>
#include <iostream>
#include <random>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <queue>
#include "SceneComponent.h"
#include "Transform.h"
#include "Random.h"

namespace TestSceneComponent
{
    // �׽�Ʈ�� Ȯ�� �� ������Ʈ Ŭ����
    class FTestSceneComponent : public USceneComponent
    {
    private:
        int ID;
        Vector3 PrevWorldPosition; // ���� ���� ��ġ ����
        bool bWorldTransformNeedUpdate = false;

    public:
        FTestSceneComponent(int InID) : ID(InID)
        {
            // �ʱ� ���� ��ġ ����
            PrevWorldPosition = GetWorldPosition();
        }

        // ���� Ʈ�������� üũ�ϰ� ���� ���� ������Ʈ
        void CheckWorldTransformChanged()
        {
            Vector3 CurrentWorldPosition = GetWorldPosition();
            bWorldTransformNeedUpdate = (PrevWorldPosition - CurrentWorldPosition).LengthSquared() > KINDA_SMALL;
            PrevWorldPosition = CurrentWorldPosition;
        }

        void ResetFlags()
        {
            bWorldTransformNeedUpdate = false;
            PrevWorldPosition = GetWorldPosition();
        }

        int GetID() const { return ID; }
        bool HasWorldTransformChanged() const { return bWorldTransformNeedUpdate; }
        // ����� ������ ���ڿ� ǥ��
        std::string ToString() const
        {
            std::stringstream ss;

            // Ʈ������ ���� ���¿� ���� ID ǥ��
            std::string idStr;
            if (bWorldTransformNeedUpdate)
                idStr = "[" + std::to_string(ID) + "]";
            else
                idStr = std::to_string(ID);

            ss << idStr;

            return ss.str();
        }

        virtual const char* GetComponentClassName() const override
        {
            return "FTestScene";
        }
    };

    // ���� ���� �׽�Ʈ Ŭ����
    class FSceneHierarchyTester
    {
    private:
        std::vector<std::shared_ptr<FTestSceneComponent>> Components;

    public:
        // ������ ������ ���� ���� ���� ����
        void CreateHierarchy(int NodeCount, int MaxChildrenPerNode = 3)
        {
            if (NodeCount <= 0)
                return;

            // ���� ������Ʈ ����
            Components.clear();

            // ��Ʈ ��� ����
            auto RootNode = UActorComponent::Create<FTestSceneComponent>(0);
            Components.push_back(RootNode);

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> ChildCountDist(0, MaxChildrenPerNode);
            std::uniform_int_distribution<> ParentSelectionDist(0, 0); // ó������ ��Ʈ�� ����

            // ������ ��� ���� �� ���� ���� ����
            for (int i = 1; i < NodeCount; ++i)
            {
                auto NewNode = UActorComponent::Create<FTestSceneComponent>(i);

                // 0���� i-1 ������ ������ �θ� ����
                ParentSelectionDist = std::uniform_int_distribution<>(0, i - 1);
                int ParentIndex = ParentSelectionDist(gen);

                // �θ��� �ڽ� ��� ���� �ִ밪�� �ʰ����� �ʴ��� Ȯ��
                auto Parent = Components[ParentIndex];
                if (Parent->GetChildren().size() < MaxChildrenPerNode)
                {
                    Parent->AddChild(NewNode);
                }
                else
                {
                    // �ڽ��� �̹� �ִ��� ���, �ٸ� �θ� ã��
                    bool FoundParent = false;
                    for (size_t j = 0; j < Components.size(); ++j)
                    {
                        if (Components[j]->GetChildren().size() < MaxChildrenPerNode)
                        {
                            Components[j]->AddChild(NewNode);
                            FoundParent = true;
                            break;
                        }
                    }

                    // ��� ��尡 �ִ� �ڽ� ���� ������ ���, ��Ʈ�� �߰�
                    if (!FoundParent)
                    {
                        RootNode->AddChild(NewNode);
                    }
                }

                Components.push_back(NewNode);
            }

            // ���� ���� �ʱ�ȭ
            for (auto& Component : Components)
            {
                Component->ResetFlags();
            }
        }

        // ���� ���� �ð�ȭ
        void PrintHierarchy(std::ostream& os = std::cout)
        {
            if (Components.empty())
            {
                os << "Empty hierarchy." << std::endl;
                return;
            }

            os << "Hierarchy Structure:" << std::endl;
            PrintNodeRecursive(Components[0], os, "", true);
        }

        // ����� ���� Ʈ������ ��ȭ üũ
        void CheckWorldTransformChanges()
        {
            for (auto& Component : Components)
            {
                Component->CheckWorldTransformChanged();
            }
        }

        // ���� ����� Ʈ������ ���� �׽�Ʈ
        void TestRandomTransformChange(std::ostream& os = std::cout)
        {
            if (Components.empty())
            {
                os << "Empty hierarchy. Create hierarchy first." << std::endl;
                return;
            }

            // ���� ��� ����
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> NodeDist(0, Components.size() - 1);
            int TargetNodeIndex = NodeDist(gen);
            auto TargetNode = Components[TargetNodeIndex];

            os << "Step 1: Initial hierarchy state" << std::endl;
            PrintHierarchy(os);
            os << std::endl;

            // ���� Ʈ������ ����
            Vector3 RandomPosition = Vector3(
                FRandom::RandF(-10.0f, 10.0f),
                FRandom::RandF(-10.0f, 10.0f),
                FRandom::RandF(-10.0f, 10.0f)
            );

            os << "Step 2: Changing local transform of Node " << TargetNode->GetID() << std::endl;
            TargetNode->SetLocalPosition(RandomPosition);
            os << "Local position changed to: ("
                << RandomPosition.x << ", "
                << RandomPosition.y << ", "
                << RandomPosition.z << ")" << std::endl;
            os << std::endl;

            // ��ȸ�Ǵ� USceneComponent�� ���������� ��������� ��� ������(�ǵ���� ���� ��)
            auto children = TargetNode->FindChildrenByType<USceneComponent>();
            if (children.size() > 0)
            {
                std::uniform_int_distribution<> ChildDist(0, children.size() - 1);
                int ChildIdx = ChildDist(gen);
                auto targetChild = std::dynamic_pointer_cast<FTestSceneComponent>(children[ChildIdx].lock());
            }
  
            CheckWorldTransformChanges();
            //CheckWorldTransformDirty();

            os << "Step 3: World transform propagation" << std::endl;
            PrintHierarchy(os);
            os << std::endl;

            // ����� ���� �ʱ�ȭ
            for (auto& Component : Components)
            {
                Component->ResetFlags();
            }
        }

    private:
        // ��������� ��� ���� ���� ��� (Ʈ�� ���� ǥ�� ����)
        void PrintNodeRecursive(const std::shared_ptr<FTestSceneComponent>& Node,
                                std::ostream& os,
                                const std::string& Prefix,
                                bool IsLast)
        {
            if (!Node)
                return;

            // ���� ����� ���� ���λ� (Ʈ�� ���� ���� ����)
            os << Prefix;

            // ������ �ڽ����� �ƴ����� ���� �ٸ� ���� ���� ���
            os << (IsLast ? "������ " : "������ ");

            // ��� ���� ���
            os << Node->ToString() << std::endl;

            // �ڽ� ��带 ���� �� ���λ� ���
            std::string NewPrefix = Prefix + (IsLast ? "    " : "��   ");

            auto Children = Node->GetChildren();
            for (size_t i = 0; i < Children.size(); ++i)
            {
                auto TestChild = std::dynamic_pointer_cast<FTestSceneComponent>(Children[i]);
                if (TestChild)
                {
                    // ������ �ڽ����� ���� ����
                    bool ChildIsLast = (i == Children.size() - 1);
                    PrintNodeRecursive(TestChild, os, NewPrefix, ChildIsLast);
                }
            }
        }
    };

    // �׽�Ʈ ���� �Լ�
    void RunTransformTest(std::ostream& os = std::cout, int NodeCount = 10, int MaxChildrenPerNode = 3)
    {
        os << "=== Scene Component Transform Hierarchy Test ===" << std::endl;
        os << "Creating a hierarchy with " << NodeCount << " nodes" << std::endl;
        os << "Maximum " << MaxChildrenPerNode << " children per node" << std::endl;
        os << std::endl;

        FSceneHierarchyTester Tester;
        Tester.CreateHierarchy(NodeCount, MaxChildrenPerNode);
        Tester.TestRandomTransformChange(os);

        os << "=== Test Completed ===" << std::endl;
    }

    // �ݺ� �׽�Ʈ ���� �Լ�
    void RunMultipleTests(int TestCount, std::ostream& os = std::cout, int NodeCount = 10, int MaxChildrenPerNode = 3)
    {
        for (int i = 0; i < TestCount; ++i)
        {
            os << "\n\n========= TEST RUN " << (i + 1) << " of " << TestCount << " =========" << std::endl;
            RunTransformTest(os, NodeCount, MaxChildrenPerNode);
        }
    }
}