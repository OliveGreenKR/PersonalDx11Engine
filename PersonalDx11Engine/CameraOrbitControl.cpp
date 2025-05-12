#include "CameraOrbitControl.h"
#include "Camera.h"

void FCameraOrbit::OrbitLatitude(UCamera* Camera, Vector3 Target, float Delta)
{
    if (!Camera)
        return;

    // 현재 구면 좌표 계산
    float Radius, Latitude, Longitude;
    CalculateSphericalCoordinates(Camera, Target, Radius, Latitude, Longitude);

    // 위도 업데이트 (극점 제한)
    Latitude += Delta;
    Latitude = Math::Clamp(Latitude, -89.0f, 89.0f); // 극점에서의 짐벌락 방지

    // 카메라 위치 및 방향 업데이트
    UpdateCameraFromSpherical(Camera, Target, Radius, Latitude, Longitude);
}

void FCameraOrbit::OrbitLongitude(UCamera* Camera, Vector3 Target, float Delta)
{
    if (!Camera)
        return;

    // 현재 구면 좌표 계산
    float Radius, Latitude, Longitude;
    CalculateSphericalCoordinates(Camera, Target, Radius, Latitude, Longitude);

    // 경도 업데이트 (360도 순환)
    Longitude += Delta;
    // 360도 범위로 정규화 (0~360)
    Longitude = fmodf(Longitude, 360.0f);
    if (Longitude < 0.0f)
        Longitude += 360.0f;

    // 카메라 위치 및 방향 업데이트
    UpdateCameraFromSpherical(Camera, Target, Radius, Latitude, Longitude);
}

void FCameraOrbit::OrbitDistance(UCamera* Camera, Vector3 Target, float Delta,
                                 float MinDistance, float MaxDistance)
{
    if (!Camera)
        return;

    // 현재 구면 좌표 계산
    float Radius, Latitude, Longitude;
    CalculateSphericalCoordinates(Camera, Target, Radius, Latitude, Longitude);

    // 거리 업데이트 (min/max 제한)
    Radius += Delta;
    Radius = Math::Clamp(Radius, MinDistance, MaxDistance);

    // 카메라 위치 및 방향 업데이트
    UpdateCameraFromSpherical(Camera, Target, Radius, Latitude, Longitude);
}

void FCameraOrbit::CalculateSphericalCoordinates(UCamera* Camera, Vector3 Target,
                                                 float& OutRadius, float& OutLatitude, float& OutLongitude)
{
    // 타겟을 중심으로 한 카메라의 상대 위치 계산
    Vector3 CameraPosition = Camera->GetTransform().Position;
    Vector3 RelativePosition = CameraPosition - Target;

    // 구면 좌표 계산
    // 반지름(거리)
    OutRadius = RelativePosition.Length();

    // 위도(y축 기준 각도, -90~90)
    OutLatitude = Math::RadToDegree(asinf(RelativePosition.y / OutRadius));

    // 경도(xz평면에서의 각도, 0~360)
    OutLongitude = Math::RadToDegree(atan2f(RelativePosition.z, RelativePosition.x));
}

void FCameraOrbit::UpdateCameraFromSpherical(UCamera* Camera, Vector3 Target,
                                             float Radius, float Latitude, float Longitude)
{
    // 라디안 변환
    float LatRad = Math::DegreeToRad(Latitude);
    float LongRad = Math::DegreeToRad(Longitude);

    // 구면좌표를 데카르트 좌표로 변환
    float HorizontalRadius = Radius * cosf(LatRad); // xz평면 반지름
    Vector3 NewPosition;
    NewPosition.x = Target.x + HorizontalRadius * cosf(LongRad);
    NewPosition.y = Target.y + Radius * sinf(LatRad);
    NewPosition.z = Target.z + HorizontalRadius * sinf(LongRad);

    // 카메라 위치 설정
    Camera->SetPosition(NewPosition);

    // 타겟 바라보기
    Camera->LookAt(Target);
}