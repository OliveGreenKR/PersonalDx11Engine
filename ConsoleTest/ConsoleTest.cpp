#include <iostream>
#include "../PersonalDx11Engine/DynamicAABBTree.h"
#include "../PersonalDx11Engine/ElasticBody.h"
#include "../PersonalDx11Engine/Math.h"

int main() {
    FDynamicAABBTree tree;
    auto ElasticBody = UGameObject::Create<UElasticBody>();
    ElasticBody->SetPosition(Vector3{ 0,0,0 });
    ElasticBody->SetScale(0.5f * Vector3::One);
    ElasticBody->SetShapeBox();
    ElasticBody->SetActive(true);
    ElasticBody->bDebug = true;
    ElasticBody->PostInitialized();
    ElasticBody->SetMass(3.0f);
    ElasticBody->PostInitializedComponents();

    auto ElasticBody2 = UGameObject::Create<UElasticBody>();
    ElasticBody2->SetPosition(Vector3{ 2,0,0 });
    ElasticBody2->SetScale(1.5f * Vector3::One);
    ElasticBody2->SetShapeBox();
    ElasticBody2->SetActive(true);
    ElasticBody2->bDebug = true;
    ElasticBody2->PostInitialized();
    ElasticBody2->SetMass(3.0f);
    ElasticBody2->PostInitializedComponents();

    auto ElasticBody3 = UGameObject::Create<UElasticBody>();
    ElasticBody3->SetPosition(Vector3{ 0,0,-2 });
    ElasticBody3->SetScale(2.5f * Vector3::One);
    ElasticBody3->SetShapeBox();
    ElasticBody3->SetActive(true);
    ElasticBody3->bDebug = true;
    ElasticBody3->PostInitialized();
    ElasticBody3->SetMass(3.0f);
    ElasticBody3->PostInitializedComponents();

    // 트리 업데이트 및 쿼리 테스트
    tree.UpdateTree();
    tree.PrintTreeStructure();

    //FDynamicAABBTree::AABB queryBounds;
    //queryBounds.Min = Vector3(-1.0f, -1.0f, -1.0f);
    //queryBounds.Max = Vector3(1.0f, 1.0f, 1.0f);

    //tree.QueryOverlap(queryBounds, [](size_t id) {
    //    std::cout << "Overlap with node: " << id << std::endl;
    //                  });

    return 0;
}