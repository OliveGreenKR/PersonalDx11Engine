#pragma once

// ������ - ���� ���� ����
enum class ERenderStateType
{
	Default, //Solid
	Wireframe,
};

// �⺻ ���� �� �������̽�
struct FRenderJob
{
	ERenderStateType StateType;

	FRenderJob(ERenderStateType InStateType) : StateType(InStateType) {}
	virtual ~FRenderJob() = default;

	virtual void Execute(FRenderContext& Context) = 0;
};


// �޽� ������ �� (���̾�������, �ָ��� ��)
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

// �ؽ�ó ������ ��
struct FTexturedRenderJob : public FMeshRenderJob
{
	ID3D11ShaderResourceView* TextureSRV;
	Vector4 Color;

	void Execute(FRenderContext& Context) override
	{
		// 1. ���ؽ� �� �ε��� ���� ���ε�
		Context.BindVertexBuffer(VertexBuffer, Stride, Offset);
		Context.BindIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT);

		// 2. ���̴� ��� ���� ������Ʈ
		FMatrixBufferData MatrixData(WorldMatrix, Context.GetViewMatrix(), Context.GetProjectionMatrix());
		Context.UpdateConstantBuffer(0, MatrixData);

		// 3. �ؽ�ó �� ���÷� ���ε�
		Context.BindShaderResource(0, TextureSRV);
		Context.BindSampler(0, Context.GetDefaultSamplerState());

		// 4. �÷� ������ ���ε�
		FColorBufferData ColorData(Color);
		Context.UpdateConstantBuffer(1, ColorData);

		// 5. ��ο� �� ����
		Context.DrawIndexed(IndexCount, 0, 0);
	}
};
