#include "DebugDrawManager.h"
#include "Camera.h"

void FDebugDrawManager::DrawArrow(const Vector3& Start, const Vector3& Direction, float Length, float Size, const Vector4& Color, float Duration)
{
	auto Arrow = std::make_unique<FDebugDrawArrow>(
		Start, Direction, Length, Size, Color, Duration);
	Elements.push_back(std::move(Arrow));
}

void FDebugDrawManager::Tick(const float DeltaTime)
{
	// ����� ��� ����
	Elements.erase(
		std::remove_if(
			Elements.begin(),
			Elements.end(),
			[](const auto& Element) { return Element->IsExpired(); }
		),
		Elements.end()
	);

	// ���� ��ҵ��� �ð� ������Ʈ
	for (auto& Element : Elements)
	{
		Element->CurrentTime += DeltaTime;
	}
}

void FDebugDrawManager::DrawAll(UCamera* Camera)
{
	for (auto& Element : Elements)
	{
		Element->Draw(Camera);
	}
}


//----------


// FDebugDrawArrow�� Draw ����
void FDebugDrawArrow::Draw(UCamera* Camera)
{
	if (!ImGui::GetCurrentContext())
		return;

	// ���� ��ǥ�� ��ũ�� ��ǥ�� ��ȯ
	XMVECTOR WorldStart = XMLoadFloat3(&Start);
	XMVECTOR vDirection = XMLoadFloat3(&Direction);
	vDirection = XMVector3Normalize(vDirection);

	XMVECTOR WorldEnd = WorldStart + vDirection * Length;

	// ViewProjection ��� ����
	Matrix ViewProj = Camera->GetViewMatrix() * Camera->GetProjectionMatrix();

	// ��ũ�� �������� ��ȯ
	XMVECTOR ProjStart = XMVector3Transform(WorldStart, ViewProj);
	XMVECTOR ProjEnd = XMVector3Transform(WorldEnd, ViewProj);

	// NDC ��ǥ�� ��ũ�� ��ǥ�� ��ȯ
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

	// Z���� �� ��� �ڿ� �ִ��� Ȯ��
	if (XMVectorGetZ(ProjStart) < 0 || XMVectorGetZ(ProjEnd) < 0 ||
		XMVectorGetW(ProjStart) < KINDA_SMALL || XMVectorGetW(ProjEnd) < KINDA_SMALL)
		return;

	// ȭ��ǥ �׸���
	ImDrawList* DrawList = ImGui::GetForegroundDrawList();
	//ImDrawList* DrawList = ImGui::GetBackgroundDrawList();


	// ���� ����
	DrawList->AddLine(
		ScreenStart,
		ScreenEnd,
		ImColor(Color.x, Color.y, Color.z, Color.w),
		Size
	);

	// ȭ��ǥ ��� ���
	ImVec2 Direction = ImVec2(
		ScreenEnd.x - ScreenStart.x,
		ScreenEnd.y - ScreenStart.y
	);
	float DirectionLength = sqrtf(Direction.x * Direction.x + Direction.y * Direction.y);

	if (DirectionLength < 1e-6f)
		return;

	// ���� ����ȭ
	Direction.x /= DirectionLength;
	Direction.y /= DirectionLength;

	// ȭ��ǥ ����� �� �� ���
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

	// ȭ��ǥ ��� �׸���
	DrawList->AddTriangleFilled(
		ScreenEnd,
		ArrowPoint1,
		ArrowPoint2,
		ImColor(Color.x, Color.y, Color.z, Color.w)
	);

}