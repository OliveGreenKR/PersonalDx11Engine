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
    // 테스트용 확장 씬 컴포넌트 클래스
    class FTestSceneComponent : public USceneComponent
    {
    private:
        int ID;
        Vector3 PrevWorldPosition; // 이전 월드 위치 저장
        bool bWorldTransformNeedUpdate = false;

    public:
        FTestSceneComponent(int InID) : ID(InID)
        {
            // 초기 월드 위치 저장
            PrevWorldPosition = GetWorldPosition();
        }

        // 월드 트랜스폼을 체크하고 변경 여부 업데이트
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
        // 디버깅 목적의 문자열 표현
        std::string ToString() const
        {
            std::stringstream ss;

            // 트랜스폼 변경 상태에 따른 ID 표시
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

    // 계층 구조 테스트 클래스
    class FSceneHierarchyTester
    {
    private:
        std::vector<std::shared_ptr<FTestSceneComponent>> Components;

    public:
        // 지정된 개수의 노드로 계층 구조 생성
        void CreateHierarchy(int NodeCount, int MaxChildrenPerNode = 3)
        {
            if (NodeCount <= 0)
                return;

            // 기존 컴포넌트 정리
            Components.clear();

            // 루트 노드 생성
            auto RootNode = UActorComponent::Create<FTestSceneComponent>(0);
            Components.push_back(RootNode);

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> ChildCountDist(0, MaxChildrenPerNode);
            std::uniform_int_distribution<> ParentSelectionDist(0, 0); // 처음에는 루트만 있음

            // 나머지 노드 생성 및 계층 구조 구성
            for (int i = 1; i < NodeCount; ++i)
            {
                auto NewNode = UActorComponent::Create<FTestSceneComponent>(i);

                // 0부터 i-1 사이의 임의의 부모 선택
                ParentSelectionDist = std::uniform_int_distribution<>(0, i - 1);
                int ParentIndex = ParentSelectionDist(gen);

                // 부모의 자식 노드 수가 최대값을 초과하지 않는지 확인
                auto Parent = Components[ParentIndex];
                if (Parent->GetChildren().size() < MaxChildrenPerNode)
                {
                    Parent->AddChild(NewNode);
                }
                else
                {
                    // 자식이 이미 최대인 경우, 다른 부모 찾기
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

                    // 모든 노드가 최대 자식 수에 도달한 경우, 루트에 추가
                    if (!FoundParent)
                    {
                        RootNode->AddChild(NewNode);
                    }
                }

                Components.push_back(NewNode);
            }

            // 계층 구조 초기화
            for (auto& Component : Components)
            {
                Component->ResetFlags();
            }
        }

        // 계층 구조 시각화
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

        // 노드의 월드 트랜스폼 변화 체크
        void CheckWorldTransformChanges()
        {
            for (auto& Component : Components)
            {
                Component->CheckWorldTransformChanged();
            }
        }

        // 랜덤 노드의 트랜스폼 변경 테스트
        void TestRandomTransformChange(std::ostream& os = std::cout)
        {
            if (Components.empty())
            {
                os << "Empty hierarchy. Create hierarchy first." << std::endl;
                return;
            }

            // 랜덤 노드 선택
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> NodeDist(0, Components.size() - 1);
            int TargetNodeIndex = NodeDist(gen);
            auto TargetNode = Components[TargetNodeIndex];

            os << "Step 1: Initial hierarchy state" << std::endl;
            PrintHierarchy(os);
            os << std::endl;

            // 랜덤 트랜스폼 변경
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

            // 조회되는 USceneComponent가 내부적으로 변경사항을 즉시 전파함(의도대로 동작 시)
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

            // 변경된 내용 초기화
            for (auto& Component : Components)
            {
                Component->ResetFlags();
            }
        }

    private:
        // 재귀적으로 노드 계층 구조 출력 (트리 가지 표시 포함)
        void PrintNodeRecursive(const std::shared_ptr<FTestSceneComponent>& Node,
                                std::ostream& os,
                                const std::string& Prefix,
                                bool IsLast)
        {
            if (!Node)
                return;

            // 현재 노드의 라인 접두사 (트리 가지 문자 포함)
            os << Prefix;

            // 마지막 자식인지 아닌지에 따라 다른 가지 문자 사용
            os << (IsLast ? "└── " : "├── ");

            // 노드 정보 출력
            os << Node->ToString() << std::endl;

            // 자식 노드를 위한 새 접두사 계산
            std::string NewPrefix = Prefix + (IsLast ? "    " : "│   ");

            auto Children = Node->GetChildren();
            for (size_t i = 0; i < Children.size(); ++i)
            {
                auto TestChild = std::dynamic_pointer_cast<FTestSceneComponent>(Children[i]);
                if (TestChild)
                {
                    // 마지막 자식인지 여부 전달
                    bool ChildIsLast = (i == Children.size() - 1);
                    PrintNodeRecursive(TestChild, os, NewPrefix, ChildIsLast);
                }
            }
        }
    };

    // 테스트 실행 함수
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

    // 반복 테스트 실행 함수
    void RunMultipleTests(int TestCount, std::ostream& os = std::cout, int NodeCount = 10, int MaxChildrenPerNode = 3)
    {
        for (int i = 0; i < TestCount; ++i)
        {
            os << "\n\n========= TEST RUN " << (i + 1) << " of " << TestCount << " =========" << std::endl;
            RunTransformTest(os, NodeCount, MaxChildrenPerNode);
        }
    }
}