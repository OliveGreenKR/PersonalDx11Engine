// © 2024 KRAFTON, Inc. ALL RIGHTS RESERERVED
#pragma once
#include "Model.h"
#include <vector>

std::shared_ptr<UModel> UModel::GetDefaultTriangle(ID3D11Device* InDevice)
{
	const static FVertexData triangle_vertices[] = {
	{ {  0.0f,  1.0f, 0.0f }, {  0.5f, 0.0f } }, // Top vertex (red)
	{ {  1.0f, -1.0f, 0.0f }, {  1.0f, 1.0f } }, // Bottom-right vertex (green)
	{ { -1.0f, -1.0f, 0.0f }, {  0.0f, 1.0f } }  // Bottom-left vertex (blue)
	};
	auto model = std::make_shared<UModel>();
	model->Initialize<FVertexData>(InDevice, triangle_vertices, 3);
	return model;
}


std::shared_ptr<UModel> UModel::GetDefaultCube(ID3D11Device* InDevice)
{
	const static FVertexData cube_vertices[] = {
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}, },
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, },
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, },
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, },
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, },
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, },
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}, },
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, },
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, },
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, },
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, },
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, },
		{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, },
		{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, },
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, },
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, },
		{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, },
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, },
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, },
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, },
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, },
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, },
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, },
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, },
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, },
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, },
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, },
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, },
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, },
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, },
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, },
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, },
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, },
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, },
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, },
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, },
	};
	auto model = std::make_shared<UModel>();
	model->Initialize<FVertexData>(InDevice, cube_vertices, 36);
	return model;
}

std::shared_ptr<UModel> UModel::GetDefaultSphere(ID3D11Device* InDevice) {
	uint32_t HorizontalSegments = 30;		// 가로 단계 수 (경도 분할)
	uint32_t VerticalSegments = 30;			// 세로 단계 수 (위도 분할)
	float Radius = 1.0f;					// 구체의 반지름

	// 최종 결과를 저장할 벡터
	// 각 격자 셀당 2개의 삼각형, 각 삼각형당 3개의 정점
	const size_t FinalVertexCount = HorizontalSegments * VerticalSegments * 6;
	std::vector<FVertexData> OrderedVertices;
	OrderedVertices.reserve(FinalVertexCount);

	// 각 단계별 각도 증분 계산
	const float PhiStep = XM_PI / static_cast<float>(VerticalSegments);    // 위도 증분
	const float ThetaStep = XM_2PI / static_cast<float>(HorizontalSegments); // 경도 증분

	// 각 격자 셀에 대해 순차적으로 삼각형 정점 생성
	for (uint32_t VerticalIndex = 0; VerticalIndex < VerticalSegments; ++VerticalIndex)
	{
		for (uint32_t HorizontalIndex = 0; HorizontalIndex < HorizontalSegments; ++HorizontalIndex)
		{
			// 현재 격자 셀의 네 꼭지점 생성
			FVertexData TopLeft, TopRight, BottomLeft, BottomRight;

			// 상단 좌측 정점 (TopLeft)
			float Phi = XM_PIDIV2 - (static_cast<float>(VerticalIndex) * PhiStep);
			float Theta = static_cast<float>(HorizontalIndex) * ThetaStep;
			float CosPhi = cosf(Phi);

			TopLeft.Position = XMFLOAT3(
				Radius * CosPhi * cosf(Theta),
				Radius * sinf(Phi),
				Radius * CosPhi * sinf(Theta)
			);
			TopLeft.TexCoord = XMFLOAT2(
				static_cast<float>(HorizontalIndex) / HorizontalSegments,
				1.0f - (static_cast<float>(VerticalIndex) / VerticalSegments)
			);

			// 상단 우측 정점 (TopRight)
			Theta = static_cast<float>(HorizontalIndex + 1) * ThetaStep;
			TopRight.Position = XMFLOAT3(
				Radius * CosPhi * cosf(Theta),
				Radius * sinf(Phi),
				Radius * CosPhi * sinf(Theta)
			);
			TopRight.TexCoord = XMFLOAT2(
				static_cast<float>(HorizontalIndex + 1) / HorizontalSegments,
				1.0f - (static_cast<float>(VerticalIndex) / VerticalSegments)
			);

			// 하단 좌측 정점 (BottomLeft)
			Phi = XM_PIDIV2 - (static_cast<float>(VerticalIndex + 1) * PhiStep);
			Theta = static_cast<float>(HorizontalIndex) * ThetaStep;
			CosPhi = cosf(Phi);

			BottomLeft.Position = XMFLOAT3(
				Radius * CosPhi * cosf(Theta),
				Radius * sinf(Phi),
				Radius * CosPhi * sinf(Theta)
			);
			BottomLeft.TexCoord = XMFLOAT2(
				static_cast<float>(HorizontalIndex) / HorizontalSegments,
				1.0f - (static_cast<float>(VerticalIndex + 1) / VerticalSegments)
			);

			// 하단 우측 정점 (BottomRight)
			Theta = static_cast<float>(HorizontalIndex + 1) * ThetaStep;
			BottomRight.Position = XMFLOAT3(
				Radius * CosPhi * cosf(Theta),
				Radius * sinf(Phi),
				Radius * CosPhi * sinf(Theta)
			);
			BottomRight.TexCoord = XMFLOAT2(
				static_cast<float>(HorizontalIndex + 1) / HorizontalSegments,
				1.0f - (static_cast<float>(VerticalIndex + 1) / VerticalSegments)
			);

			// 첫 번째 삼각형 추가 (시계 방향: TopLeft -> BottomLeft -> TopRight)
			OrderedVertices.push_back(TopLeft);
			OrderedVertices.push_back(BottomLeft);
			OrderedVertices.push_back(TopRight);

			// 두 번째 삼각형 추가 (시계 방향: TopRight -> BottomLeft -> BottomRight)
			OrderedVertices.push_back(TopRight);
			OrderedVertices.push_back(BottomLeft);
			OrderedVertices.push_back(BottomRight);
		}
	}


	auto model = std::make_shared<UModel>();
	model->Initialize<FVertexData>(InDevice, OrderedVertices.data(), OrderedVertices.size());
	return model;
}
	