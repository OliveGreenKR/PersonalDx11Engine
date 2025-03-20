#pragma once

// 렌더잡 - 렌더 상태 정의
enum class ERenderStateType
{
	Default, //Solid
	Wireframe,
};

// 기본 렌더 잡 인터페이스
struct FRenderJob
{
	ERenderStateType StateType;

	FRenderJob(ERenderStateType InStateType) : StateType(InStateType) {}
	virtual ~FRenderJob() = default;

	virtual void Execute(FRenderContext& Context) = 0;
};


// 메시 렌더링 잡 (와이어프레임, 솔리드 등)
struct FMeshRenderJob : public FRenderJob
{
	ID3D11Buffer* VertexBuffer;
	ID3D11Buffer* IndexBuffer;
	uint32_t VertexCount;
	uint32_t IndexCount;
	uint32_t Stride;
	uint32_t Offset;
	Matrix WorldMatrix;

	void Execute(FRenderContext& Context) override;
};

// 텍스처 렌더링 잡
struct FTexturedRenderJob : public FMeshRenderJob
{
	ID3D11ShaderResourceView* TextureSRV;
	Vector4 Color;

	void Execute(FRenderContext& Context) override
	{
		// 1. 버텍스 및 인덱스 버퍼 바인딩
		Context.BindVertexBuffer(VertexBuffer, Stride, Offset);
		Context.BindIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT);

		// 2. 쉐이더 상수 버퍼 업데이트
		FMatrixBufferData MatrixData(WorldMatrix, Context.GetViewMatrix(), Context.GetProjectionMatrix());
		Context.UpdateConstantBuffer(0, MatrixData);

		// 3. 텍스처 및 샘플러 바인딩
		Context.BindShaderResource(0, TextureSRV);
		Context.BindSampler(0, Context.GetDefaultSamplerState());

		// 4. 컬러 데이터 바인딩
		FColorBufferData ColorData(Color);
		Context.UpdateConstantBuffer(1, ColorData);

		// 5. 드로우 콜 실행
		Context.DrawIndexed(IndexCount, 0, 0);
	}
};
