#include "CollisionProcessor.h"
#include "Transform.h"
#include <algorithm>
#include "DynamicAABBTree.h"
#include "PhysicsSystem.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventDispatcher.h"
#include "CollisionPositionalCorrectionCalculator.h"
#include "Debug.h"
#include "ConfigReadManager.h"
#include "PhysicsStateInternalInterface.h"
#include "CollisionShapeInternalInterface.h"
#include "PhysicsDefine.h"

#pragma region Lifecycle Management

FCollisionProcessor::~FCollisionProcessor()
{
    Release();
}

void FCollisionProcessor::Initialize()
{
    try
    {
        LoadConfigFromIni();

        // 하위 시스템 초기화
        Detector = std::make_unique<FCollisionDetector>();
        ResponseCalculator = std::make_unique<FCollisionResponseCalculator>();
        EventDispatcher = std::make_unique<FCollisionEventDispatcher>();
        PositionCorrectionCalculator = std::make_unique<FCollisionPositionalCorrectionCalculator>();

        // 공간 분할 트리 초기화
        CollisionTree = std::make_unique<FDynamicAABBTree>(InitialCollisonCapacity);

        // PhysicsSystem 인터페이스 획득
        auto* physicsSystem = UPhysicsSystem::Get();
        PhysicsStateInterface = physicsSystem;
        ShapeInterface = physicsSystem;  // UPhysicsSystem이 ICollisionShapeInternal 구현

        if (!PhysicsStateInterface || !ShapeInterface)
        {
            throw std::runtime_error("Failed to acquire PhysicsSystem interfaces");
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to initialize CollisionProcessor: %s", e.what());
        Release();
        throw;
    }
}

void FCollisionProcessor::Release()
{
    LOG_INFO("Releasing CollisionProcessor...");

    // 모든 등록 해제
    UnRegisterAll();

    // 하위 시스템 해제 (역순으로 해제)
    PositionCorrectionCalculator.reset();
    EventDispatcher.reset();
    ResponseCalculator.reset();
    Detector.reset();
    CollisionTree.reset();

    // 인터페이스 참조 해제
    PhysicsStateInterface = nullptr;
    ShapeInterface = nullptr;

    LOG_INFO("CollisionProcessor released successfully");
}

void FCollisionProcessor::LoadConfigFromIni()
{
    auto* configManager = UConfigReadManager::Get();
    if (!configManager)
    {
        LOG_WARNING("ConfigReadManager not available, using default values");
        return;
    }

    // 기본값 보존하면서 설정 로드
    float prevCCDThreshold = CCDVelocityThreshold;
    int prevCapacity = InitialCollisonCapacity;
    int prevMaxIterations = MaxConstraintIterations;
    float prevFatRatio = FatBoundsExtentRatio;

    configManager->GetValue("CCDVelocityThreshold", CCDVelocityThreshold);
    configManager->GetValue("InitialCollisionCapacity", InitialCollisonCapacity);
    configManager->GetValue("MaxConstraintIterations", MaxConstraintIterations);
    configManager->GetValue("FatBoundsExtentRatio", FatBoundsExtentRatio);

    // 설정값 유효성 검증
    if (CCDVelocityThreshold < 0.0f)
    {
        LOG_WARNING("Invalid CCDVelocityThreshold %.2f, using default %.2f",
                    CCDVelocityThreshold, prevCCDThreshold);
        CCDVelocityThreshold = prevCCDThreshold;
    }

    if (InitialCollisonCapacity <= 0)
    {
        LOG_WARNING("Invalid InitialCollisionCapacity %d, using default %d",
                    InitialCollisonCapacity, prevCapacity);
        InitialCollisonCapacity = prevCapacity;
    }

    if (MaxConstraintIterations <= 0 || MaxConstraintIterations > 100)
    {
        LOG_WARNING("Invalid MaxConstraintIterations %d, using default %d",
                    MaxConstraintIterations, prevMaxIterations);
        MaxConstraintIterations = prevMaxIterations;
    }

    if (FatBoundsExtentRatio < 0.0f || FatBoundsExtentRatio > 1.0f)
    {
        LOG_WARNING("Invalid FatBoundsExtentRatio %.3f, using default %.3f",
                    FatBoundsExtentRatio, prevFatRatio);
        FatBoundsExtentRatio = prevFatRatio;
    }

    LOG_INFO("Configuration loaded successfully:");
    LOG_INFO("- CCD Velocity Threshold: %.2f", CCDVelocityThreshold);
    LOG_INFO("- Initial Collision Capacity: %d", InitialCollisonCapacity);
    LOG_INFO("- Max Constraint Iterations: %d", MaxConstraintIterations);
    LOG_INFO("- Fat Bounds Extent Ratio: %.3f", FatBoundsExtentRatio);
}

void FCollisionProcessor::UnRegisterAll()
{
    if (!CollisionTree)
    {
        return;
    }

    LOG_INFO("Unregistering all collision objects...");
    LOG_INFO("- Active collision pairs: %zu", ActiveCollisionPairs.size());
    LOG_INFO("- Registered PhysicsIDs: %zu", PhysicsToTreeID.size());

    // 트리 정리
    CollisionTree->Clear();

    // 매핑 테이블 정리
    PhysicsToTreeID.clear();
    TreeToPhysicsID.clear();

    // 활성 충돌 쌍 정리
    ActiveCollisionPairs.clear();

    LOG_INFO("All collision registrations cleared successfully");
}

#pragma endregion

