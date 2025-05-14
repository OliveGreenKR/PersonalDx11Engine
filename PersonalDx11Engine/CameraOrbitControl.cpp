#include "CameraOrbitControl.h"
#include "Camera.h"
#include "Debug.h"


Vector3 FCameraOrbit::GetTargetPos(UCamera* Camera)
{
    Vector3 Target = Vector3::Zero();
    if (Camera->bLookAtObject && Camera->GetCurrentLookAt().lock())
    {
        Target = Camera->GetCurrentLookAt().lock()->GetWorldTransform().Position;
    }
    return Target;
}


 void FCameraOrbit::UpdateOrbit(UCamera* Camera, float deltaYawRad, float deltaPitchRad)
 {
     if (!Camera) return;

     // 멤버 변수 로드 (XMFLOAT3 -> XMVECTOR)
     Vector3 vTargetWorldPos = GetTargetPos(Camera);
     XMVECTOR TargetPos = XMLoadFloat3(&vTargetWorldPos);


     // 현재 카메라 위치를 가져와 타겟 기준 상대 위치 벡터 계산
     Vector3 currentPos = Camera->GetWorldTransform().Position;
     XMVECTOR CurrentCamPos = XMLoadFloat3(&currentPos);

     XMVECTOR RelativePosition = XMVectorSubtract(CurrentCamPos, TargetPos);

     // 구체 반지름
     float OrbitRadius = XMVector3Length(RelativePosition).m128_f32[0];

     // 현재 상대 위치 벡터 길이 제곱 계산 (스칼라 값 추출)
     float RelativePositionLengthSq = XMVector3LengthSq(RelativePosition).m128_f32[0];

     // 현재 상대 위치 벡터가 너무 작으면 (타겟과 거의 같은 위치)
     if (RelativePositionLengthSq < KINDA_SMALL * KINDA_SMALL)
     {
         // 기본 상대 위치 설정 (예: Z축 기준 -OrbitRadius)
         RelativePosition = XMVectorSet(0.0f, 0.0f, -OrbitRadius, 0.0f);

         XMVECTOR vNewPos = XMVectorAdd(TargetPos, RelativePosition);
         Vector3 NewPos;
         XMStoreFloat3(&NewPos, vNewPos);
         Camera->SetPosition(NewPos);

         // LookAt 함수도 
         Camera->LookAt(vTargetWorldPos);
         return;
     }

     // --- 회전 계산 및 적용 ---

     // 월드 Up 벡터 (Y축)
     XMVECTOR UpVector = XMVector::XMUp();

     // 1. 수평 회전 (Yaw) - 월드 Up 벡터를 축으로 회전하는 쿼터니언 생성
     XMVECTOR YawRotation = XMQuaternionRotationAxis(UpVector, deltaYawRad);

     // 2. 수직 회전 (Pitch) - 현재 상대 위치 벡터와 월드 Up 벡터에 수직인 축을 축으로 회전
     XMVECTOR PitchAxis = XMVector3Cross(RelativePosition, UpVector);

     // Pitch 축이 유효한지 확인 (상대 위치가 월드 Up/Down과 평행하지 않은 경우)
     float PitchAxisLengthSq = XMVector3LengthSq(PitchAxis).m128_f32[0];

     // 새로운 상대 위치 벡터 계산 (Pitch 먼저 적용 후 Yaw 적용)
     // Pitch 축이 유효하면 Pitch 회전 적용
     if (PitchAxisLengthSq >= KINDA_SMALL * KINDA_SMALL)
     {
         PitchAxis = XMVector3Normalize(PitchAxis); // Pitch 축 정규화
         XMVECTOR PitchRotation = XMQuaternionRotationAxis(PitchAxis, deltaPitchRad);
         RelativePosition = XMVector3Rotate(RelativePosition, PitchRotation); // Pitch 적용
     }

     // Yaw 적용 (Pitch 적용 결과에 Yaw 적용)
     RelativePosition = XMVector3Rotate(RelativePosition, YawRotation);

     // --- 극점 제한 ---
     // 새로운 상대 위치 벡터가 Y축 극점(UpVector 또는 -UpVector)에 너무 가까워지는 것을 방지
     XMVECTOR NormalizedRelativePos = XMVector3Normalize(RelativePosition);
     XMVECTOR DotUpVector = XMVector3Dot(NormalizedRelativePos, UpVector);
     float dotUp = DotUpVector.m128_f32[0]; // 스칼라 값 추출

     const float minDotForPitchLimit = 0.99f; 
     // 상단 극점 제한 (dotUp > minDotForPitchLimit)
     if (dotUp > minDotForPitchLimit)
     {
         // Y 값을 제한값에 맞추고, XZ 평면 상의 벡터 길이를 재계산하여 반지름 유지
         float limitedY = OrbitRadius * minDotForPitchLimit;
         //현재 xz 성분 길이
         float currentHorizontalLengthSq = RelativePosition.m128_f32[0] * RelativePosition.m128_f32[0] + RelativePosition.m128_f32[2] * RelativePosition.m128_f32[2];
         //limty 에서의, 즉 최대의 xz성분 값
         float limitedHorizontalLengthSq = OrbitRadius * OrbitRadius - limitedY * limitedY;

         if (currentHorizontalLengthSq > KINDA_SMALLER) // XZ 평면에 투영된 길이가 0이 아니면 스케일 조정
         {
             float scale = std::sqrt(limitedHorizontalLengthSq / currentHorizontalLengthSq);

             //RelativePos = ('x,'z,limitY)
             RelativePosition = XMVectorSetX(
                 XMVectorSetZ(
                    XMVectorScale(RelativePosition, scale), RelativePosition.m128_f32[2] * scale), 
                    RelativePosition.m128_f32[0] * scale
             );

         }
         else // XZ 평면에 투영된 길이가 0이면 (이미 극점 근처), XZ 평면에서 작은 벡터 생성
         {
             // 특정 방향 설정
             float smallHorizontalRadius = std::sqrt(limitedHorizontalLengthSq);
             RelativePosition = XMVectorSet(smallHorizontalRadius, limitedY, 0.0f, 0.0f); 
         }
         RelativePosition = XMVectorSetY(RelativePosition, limitedY); // Y 컴포넌트 최종 설정
     }
     // 하단 극점 제한 (dotUp < -minDotForPitchLimit)
     else if (dotUp < -minDotForPitchLimit)
     {
         float limitedY = OrbitRadius * -minDotForPitchLimit;
         float currentHorizontalLengthSq = RelativePosition.m128_f32[0] * RelativePosition.m128_f32[0] + RelativePosition.m128_f32[2] * RelativePosition.m128_f32[2];
         float limitedHorizontalLengthSq = OrbitRadius * OrbitRadius - limitedY * limitedY;

         if (currentHorizontalLengthSq > KINDA_SMALLER)
         {
             float scale = std::sqrt(limitedHorizontalLengthSq / currentHorizontalLengthSq);
             RelativePosition = XMVectorSetX(
                 XMVectorSetZ(
                        XMVectorScale(RelativePosition, scale), 
                        RelativePosition.m128_f32[2] * scale
                 ), 
                 RelativePosition.m128_f32[0] * scale
             );
         }
         else
         {
             float smallHorizontalRadius = std::sqrt(limitedHorizontalLengthSq);
             RelativePosition = XMVectorSet(smallHorizontalRadius, limitedY, 0.0f, 0.0f); 
         }
         RelativePosition = XMVectorSetY(RelativePosition, limitedY); // Y 컴포넌트 최종 설정
     }

     // --- 카메라 위치 및 방향 업데이트 ---

     // 반지름 유지 (정규화 후 OrbitRadius 스케일)
     RelativePosition = XMVector3Normalize(RelativePosition);
     RelativePosition = XMVectorScale(RelativePosition, OrbitRadius);

     // 새로운 카메라 월드 위치 계산 및 설정
     XMVECTOR NewPos = XMVectorAdd(TargetPos, RelativePosition);
     Vector3 vNewPos;
     XMStoreFloat3(&vNewPos, NewPos); 
     Camera->SetPosition(vNewPos);

     // 카메라 방향 설정 (타겟 바라보기)
     // LookAt 함수도 XMFLOAT3를 받는다고 가정
     Camera->LookAt(vTargetWorldPos);
 }