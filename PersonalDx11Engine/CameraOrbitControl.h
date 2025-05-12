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
	static void OrbitLatitude(UCamera* Camera, Vector3 Target, float Delta);
    //경도 조작
	static void OrbitLongitude(UCamera* Camera, Vector3 Target, float Delta);
    //구체 반지름
	static void OrbitDistance(UCamera* Camera, Vector3 Target, float Delta,
							  float MinDistance = 1.0f, float MaxDistance = 100.0f);

private:
    // 카메라와 대상 객체 사이의 현재 구면 좌표 계산
    static void CalculateSphericalCoordinates(UCamera* Camera, Vector3 Target,
                                              float& OutRadius, float& OutLatitude, float& OutLongitude);

    // 구면 좌표를 기반으로 카메라 위치와 방향 업데이트
    static void UpdateCameraFromSpherical(UCamera* Camera, Vector3 Target,
                                          float Radius, float Latitude, float Longitude);
};