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
    void UpdateOrbit(UCamera* Camera, float deltaYawRad, float deltaPitchRad);
    

private:
    Vector3 GetTargetPos(UCamera* Camera);

};