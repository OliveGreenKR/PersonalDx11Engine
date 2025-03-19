#include "DebugDrawManager.h"
#include "Camera.h"

void UDebugDrawManager::DrawArrow(const Vector3& Start, const Vector3& Direction, float Length, float Size, const Vector4& Color, float Duration)
{
	auto Arrow = std::make_unique<FDebugDrawArrow>(
		Start, Direction, Length, Size, Color, Duration);
	Elements.push_back(std::move(Arrow));
}

void UDebugDrawManager::Tick(const float DeltaTime)
{
	// 만료된 요소 제거
	Elements.erase(
		std::remove_if(
			Elements.begin(),
			Elements.end(),
			[](const auto& Element) { return Element->IsExpired(); }
		),
		Elements.end()
	);

	// 남은 요소들의 시간 업데이트
	for (auto& Element : Elements)
	{
		Element->CurrentTime += DeltaTime;
	}
}

void UDebugDrawManager::DrawAll(UCamera* Camera)
{
	if (!Camera)
		return;
	for (auto& Element : Elements)
	{
		Element->Draw(Camera);
	}
}


//----------


// FDebugDrawArrow의 Draw 구현
void FDebugDrawArrow::Draw(UCamera* Camera)
{
	if (!ImGui::GetCurrentContext())
		return;

	// 월드 좌표를 스크린 좌표로 변환
	XMVECTOR WorldStart = XMLoadFloat3(&Start);
	XMVECTOR vDirection = XMLoadFloat3(&Direction);
	vDirection = XMVector3Normalize(vDirection);

	XMVECTOR WorldEnd = WorldStart + vDirection * Length;

	// ViewProjection 행렬 적용
	Matrix ViewProj = Camera->GetViewMatrix() * Camera->GetProjectionMatrix();

	// 스크린 공간으로 변환
	XMVECTOR ProjStart = XMVector3Transform(WorldStart, ViewProj);
	XMVECTOR ProjEnd = XMVector3Transform(WorldEnd, ViewProj);

	// NDC 좌표를 스크린 좌표로 변환
	float ScreenWidth = ImGui::GetIO().DisplaySize.x;
	float ScreenHeight = ImGui::GetIO().DisplaySize.y;

	ImVec2 ScreenStart(
		(XMVectorGetX(ProjStart) / XMVectorGetW(ProjStart) + 1.0f) * 0.5f * ScreenWidth,
		(1.0f - (XMVectorGetY(ProjStart) / XMVectorGetW(ProjStart) + 1.0f) * 0.5f) * ScreenHeight
	);

	ImVec2 ScreenEnd(
		(XMVectorGetX(ProjEnd) / XMVectorGetW(ProjEnd) + 1.0f) * 0.5f * ScreenWidth,
		(1.0f - (XMVectorGetY(ProjEnd) / XMVectorGetW(ProjEnd) + 1.0f) * 0.5f) * ScreenHeight
	);

	// Z값이 뷰 평면 뒤에 있는지 확인
	if (XMVectorGetZ(ProjStart) < 0 || XMVectorGetZ(ProjEnd) < 0 ||
		XMVectorGetW(ProjStart) < KINDA_SMALL || XMVectorGetW(ProjEnd) < KINDA_SMALL)
		return;

	// 화살표 그리기
	ImDrawList* DrawList = ImGui::GetForegroundDrawList();
	//ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

	// 메인 라인
	DrawList->AddLine(
		ScreenStart,
		ScreenEnd,
		ImColor(Color.x, Color.y, Color.z, Color.w),
		Size
	);

	// 화살표 헤드 계산
	ImVec2 Direction = ImVec2(
		ScreenEnd.x - ScreenStart.x,
		ScreenEnd.y - ScreenStart.y
	);
	float DirectionLength = sqrtf(Direction.x * Direction.x + Direction.y * Direction.y);

	if (DirectionLength < 1e-6f)
		return;

	// 방향 정규화
	Direction.x /= DirectionLength;
	Direction.y /= DirectionLength;

	// 화살표 헤드의 두 점 계산
	float ArrowSize = Size * 4.0f;
	ImVec2 Perpendicular(-Direction.y, Direction.x);

	ImVec2 ArrowPoint1(
		ScreenEnd.x - Direction.x * ArrowSize + Perpendicular.x * ArrowSize,
		ScreenEnd.y - Direction.y * ArrowSize + Perpendicular.y * ArrowSize
	);

	ImVec2 ArrowPoint2(
		ScreenEnd.x - Direction.x * ArrowSize - Perpendicular.x * ArrowSize,
		ScreenEnd.y - Direction.y * ArrowSize - Perpendicular.y * ArrowSize
	);

	// 화살표 헤드 그리기
	DrawList->AddTriangleFilled(
		ScreenEnd,
		ArrowPoint1,
		ArrowPoint2,
		ImColor(Color.x, Color.y, Color.z, Color.w)
	);

}