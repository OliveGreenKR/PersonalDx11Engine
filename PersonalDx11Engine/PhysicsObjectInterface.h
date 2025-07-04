// PhysicsObjectInterface.h
#pragma once
#include "Math.h"
#include "PhysicsDataStructures.h"
#include "PhysicsDefine.h"

using PhysicsID = uint32_t;

/// <summary>
/// 물리 시스템 관리 대상 객체 인터페이스
/// 
/// 책임:
/// - 변경 빈도별 게임 상태 데이터 제공
/// - 물리 시뮬레이션 결과 수신 및 캐시
/// - 더티 플래그 기반 효율적 동기화 지원
/// - 물리 시스템과의 생명주기 연동
/// 
/// 설계 원칙:
/// - 게임 상태는 게임 컨텍스트 소유
/// - 물리 결과는 물리 시스템에서 전송받아 캐시
/// - 변경된 데이터만 선택적 동기화
/// </summary>
class IPhysicsObject
{
public:
    virtual ~IPhysicsObject() = default;

#pragma region Game State Data Access (Game → Physics)

    /// <summary>
    /// 높은 변경 빈도 데이터 획득 - Transform 관련
    /// 매 프레임 변경 가능성이 있는 데이터
    /// </summary>
    virtual FHighFrequencyData GetHighFrequencyData() const = 0;

    /// <summary>
    /// 중간 변경 빈도 데이터 획득 - 물리 타입 및 상태 제어
    /// 게임플레이 이벤트에 따라 변경되는 데이터
    /// </summary>
    virtual FMidFrequencyData GetMidFrequencyData() const = 0;

    /// <summary>
    /// 낮은 변경 빈도 데이터 획득 - 물리 속성
    /// 초기화 또는 특수 상황에서만 변경되는 데이터
    /// </summary>
    virtual FLowFrequencyData GetLowFrequencyData() const = 0;

#pragma endregion

#pragma region Physics Result Reception (Physics → Game)

    /// <summary>
    /// 물리 시뮬레이션 결과 수신
    /// 물리 시스템에서 계산된 최신 상태를 게임 캐시에 적용
    /// </summary>
    /// <param name="results">물리 시뮬레이션 결과 데이터</param>
    virtual void ReceivePhysicsResults(const FPhysicsToGameData& results) = 0;

#pragma endregion

#pragma region Dirty Flag Management

    /// <summary>
    /// 현재 더티 플래그 상태 조회
    /// 어떤 데이터 카테고리가 변경되었는지 확인
    /// </summary>
    /// <returns>현재 설정된 더티 플래그</returns>
    virtual FPhysicsDataDirtyFlags GetDirtyFlags() const = 0;

    /// <summary>
    /// 지정된 플래그를 클리어 상태로 설정
    /// 동기화 완료 후 해당 카테고리를 깨끗한 상태로 마킹
    /// </summary>
    /// <param name="flags">클리어할 플래그 카테고리</param>
    virtual void MarkDataClean(const FPhysicsDataDirtyFlags& flags) = 0;

#pragma endregion

#pragma region Physics System Lifecycle

    /// <summary>
    /// 물리 시스템 등록
    /// 물리 시뮬레이션 참여를 위한 초기 등록 과정
    /// </summary>
    virtual void RegisterPhysicsSystem() = 0;

    /// <summary>
    /// 물리 시스템 등록 해제
    /// 물리 시뮬레이션에서 제외 및 리소스 정리
    /// </summary>
    virtual void UnRegisterPhysicsSystem() = 0;

    /// <summary>
    /// 물리 틱 처리
    /// 게임플레이 로직과 물리 시스템 간 상호작용 처리
    /// 주의: 직접적인 물리 계산은 PhysicsSystem에서 배치 처리됨
    /// </summary>
    /// <param name="DeltaTime">프레임 시간</param>
    virtual void TickPhysics(const float DeltaTime) = 0;

#pragma endregion

#pragma region Physics System Integration

    /// <summary>
    /// 물리 시스템 내 고유 식별자 조회
    /// </summary>
    /// <returns>물리 시스템 할당 ID (0 = 무효)</returns>
    virtual PhysicsID GetPhysicsID() const = 0;

    /// <summary>
    /// 현재 물리 마스크 상태 조회
    /// 물리 시뮬레이션 참여 상태 및 설정 확인용
    /// </summary>
    /// <returns>현재 적용 중인 물리 마스크</returns>
    virtual FPhysicsMask GetPhysicsMask() const = 0;

#pragma endregion
};