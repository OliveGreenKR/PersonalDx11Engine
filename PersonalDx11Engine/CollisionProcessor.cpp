#include "CollisionProcessor.h"
#include "Transform.h"
#include <algorithm>
#include "DynamicAABBTree.h"
#include "PhysicsSystem.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventCalculator.h"
#include "CollisionPositionalCorrectionCalculator.h"
#include "Debug.h"
#include "ConfigReadManager.h"
#include "PhysicsStateInternalInterface.h"
#include "CollisionShapeInternalInterface.h"
#include "PhysicsDefine.h"
#include "CollisionDefines.h"

#pragma region Configuration and Initialization

void FCollisionProcessor::LoadConfigFromIni()
{
    UConfigReadManager::Get()->GetValue("CCDVelocityThreshold", CCDVelocityThreshold);
    UConfigReadManager::Get()->GetValue("InitialCollisionCapacity", InitialCollisonCapacity);
    UConfigReadManager::Get()->GetValue("MaxConstraintIterations", MaxConstraintIterations);
    UConfigReadManager::Get()->GetValue("FatBoundsExtentRatio", FatBoundsExtentRatio);
}

FCollisionProcessor::~FCollisionProcessor()
{
    Release();
}

void FCollisionProcessor::Initialize()
{
    // 설정 로드
    LoadConfigFromIni();

    // 하부 시스템 초기화
    if (!Detector)
    {
        Detector = std::make_unique<FCollisionDetector>();
    }

    if (!ResponseCalculator)
    {
        ResponseCalculator = std::make_unique<FCollisionResponseCalculator>();
    }

    if (!PositionCorrectionCalculator)
    {
        PositionCorrectionCalculator = std::make_unique<FCollisionPositionalCorrectionCalculator>();
    }

    if (!EventCalculator)
    {
        EventCalculator = std::make_unique<FCollisionEventCalculator>();
    }

    // AABB 트리 초기화 (PhysicsID 기반)
    if (!CollisionTree)
    {
        CollisionTree = std::make_unique<FDynamicAABBTree>(InitialCollisonCapacity);
    }

    // 충돌 쌍 컨테이너 예약
    if (ActiveCollisionPairs.bucket_count() < InitialCollisonCapacity)
    {
        ActiveCollisionPairs.reserve(InitialCollisonCapacity);
    }
}

void FCollisionProcessor::Release()
{
    // 등록된 모든 컴포넌트 해제
    UnRegisterAll();

    // 하부 시스템 해제
    Detector.reset();
    ResponseCalculator.reset();
    PositionCorrectionCalculator.reset();
    EventCalculator.reset();
    CollisionTree.reset();

    // 컨테이너 정리
    ActiveCollisionPairs.clear();
}

void FCollisionProcessor::UnRegisterAll()
{
    if (CollisionTree)
    {
        CollisionTree->Clear();
    }
    ActiveCollisionPairs.clear();
}

#pragma endregion
