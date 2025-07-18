#pragma once
#include "CollisionDefines.h"

/// <summary>
/// 물리 이벤트 배송 요청 인터페이스
/// 
/// 역할:
/// - 외부 시스템에서 생성한 이벤트를 PhysicsSystem Dispatcher에게 배송 요청
/// - 구현체(PhysicsSystem)가 내부적으로 이벤트 큐 관리 및 배송 시점 결정
/// - 요청자는 이벤트 생성 후 배송만 위임
/// 
/// 책임:
/// - 이벤트 데이터 접수 및 내부 큐 저장
/// - 배송 실패 시 안전한 처리
/// - 향후 이벤트 타입 확장 지원
/// </summary>
class IPhysicsEventDispatcher
{
public:
    virtual ~IPhysicsEventDispatcher() = default;

    /// <summary>
    /// 충돌 이벤트 배송 요청
    /// 외부에서 생성된 충돌 이벤트를 PhysicsSystem 이벤트 큐에 추가
    /// 실제 배송 시점과 방식은 PhysicsSystem이 내부적으로 결정
    /// </summary>
    /// <param name="Event">배송 요청할 충돌 이벤트</param>
    virtual void AddCollisionEvent(const FPhysicsCollisionEvent& Event) = 0;

    // 향후 확장하려면 이벤트 타입별 함수 추가 필요.
};