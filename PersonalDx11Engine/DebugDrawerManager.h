#pragma once
#include "PrimitiveComponent.h"
#include "Math.h"
#include "ResourceHandle.h"
#include <vector>
#include "FixedObjectPool.h"
#include <array>

constexpr uint16_t DEBUGDRASWER_POOL_SIZE = 256;

// 디버그 도형 관리 클래스
class UDebugDrawManager
{
private:
    // 복사 및 이동 방지
    UDebugDrawManager(const UDebugDrawManager&) = delete;
    UDebugDrawManager& operator=(const UDebugDrawManager&) = delete;
    UDebugDrawManager(UDebugDrawManager&&) = delete;
    UDebugDrawManager& operator=(UDebugDrawManager&&) = delete;
private:
    // 디버그 데이터 저장 구조체
    struct FDebugShape
    {
        FDebugShape();
        ~FDebugShape()
        {
            Primitive.reset();
        }
        std::unique_ptr<UPrimitiveComponent> Primitive;
        float RemainingTime = 0.0f;
        bool bPersistent = false;

        FDebugShape(const FDebugShape&) = delete; // 복사 금지
        FDebugShape& operator=(const FDebugShape&) = delete;

        FDebugShape(FDebugShape&&) noexcept = default;
        FDebugShape& operator=(FDebugShape&&) noexcept = default;

        // 재사용을 위한 초기화 메서드 
        void Reset() {
            RemainingTime = 0.0f;
            bPersistent = false;
            // 프리미티브 컴포넌트 상태 초기화
            if (Primitive) {
                Primitive->SetWorldPosition(Vector3::Zero);
                Primitive->SetWorldRotation(Quaternion::Identity);
                Primitive->SetWorldScale(Vector3::One);
                Primitive->SetColor(Vector4(1, 1, 1, 1));
            }
        }
        
        
    };

    // 싱글톤 구현
    UDebugDrawManager();

    ~UDebugDrawManager();

    //PrimitiveComp 풀
    std::unique_ptr<TFixedObjectPool<FDebugShape, DEBUGDRASWER_POOL_SIZE>> FixedPool;

    // 기본 재질/모델 핸들
    FResourceHandle SphereModelHandle_High;
    FResourceHandle SphereModelHandle_Mid;
    FResourceHandle SphereModelHandle_Low;
    FResourceHandle BoxModelHandle;

    FResourceHandle DebugMaterialHandle;

public:
    static UDebugDrawManager* Get()
    {
        static UDebugDrawManager* Instance = []() {
            UDebugDrawManager* manager = new UDebugDrawManager();
            manager->Initialize();
            return manager;
            }();
        return Instance;
    }

    void Render(class URenderer* InRenderer);
    void Tick(const float DeltaTime);

    void ClearActives()
    {
        FixedPool->ClearAllActives();
    }

    // 디버그 프리미티브 API
    void DrawLine(const Vector3& Start, const Vector3& End, const Vector4& Color,
                  float Thickness = 0.001f, float Duration = 0.1f, bool bPersist = false);

    void DrawSphere(const Vector3& Center, float Radius, const Quaternion& Rotation, 
                    const Vector4& Color, float Duration = 0.1f, bool bPersist = false);

    void DrawBox(const Vector3& Center, const Vector3& Extents, const Quaternion& Rotation,
                 const Vector4& Color, float Duration = 0.1f, bool bPersist = false);

    //void DrawText3D(const Vector3& Location, const std::string& Text,
    //                const Vector4& Color, float Scale = 1.0f, float Duration = 0.0f);

private:

    void Initialize();
    // 내부 유틸리티
    void SetupPrimitive(UPrimitiveComponent* TargetPrimitive,
                        const FResourceHandle& ModelHandle,
                        const Vector3& Position,
                        const Quaternion& Rotation,
                        const Vector3& Scale,
                        const Vector4& Color);
};