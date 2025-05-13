#pragma once
#include "Math.h"

class UCamera;
class UGameObject;


/**
 * 카메라 궤도 조작 유틸리티
 * 외부 카메라를 구면 좌표계 기반으로 조작하는 기능 제공
 */
class FCameraOrbit
{
public:
    //위도 조작
	void OrbitLatitude(UCamera* Camera, float Delta);
    //경도 조작
	void OrbitLongitude(UCamera* Camera, float Delta);
    //구체 반지름
	void OrbitDistance(UCamera* Camera, float Delta,
							  float MinDistance = 1.0f, float MaxDistance = 100.0f);

    bool Initialize(UCamera* Camera);

private:
    // 카메라와 대상 객체 사이의 현재 구면 좌표 계산 - [-PI/2,PI.2] , [-PI,PI]
    void CalculateSphericalCoordinates(UCamera* Camera, Vector3 Target);

    // 구면 좌표를 기반으로 카메라 위치와 방향 업데이트
    static void UpdateCameraFromSpherical(UCamera* Camera, Vector3 Target,
                                          float Radius, float Latitude, float Longitude);
    Vector3 GetTargetPos(UCamera* Camera);

private:
    float LongitudeRad = 0.0f;
    float LatitudeRad = 0.0;
    float Radius;
};