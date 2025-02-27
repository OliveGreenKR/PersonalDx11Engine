#include "Renderer.h"
#include "Model.h"
#include "D3DShader.h"
#include "GameObject.h"
#include "Camera.h"

void URenderer::Initialize(HWND hWindow)
{
	bool result;
	result = RenderHardware->Initialize(hWindow);
	assert(result);
	result = CreateDefaultSamplerState();
	assert(result);
	
}

void URenderer::BindShader(UShader* InShader)
{
	InShader->Bind(RenderHardware->GetDeviceContext());
}

void URenderer::BeforeRender()
{
	RenderHardware->BeginScene();
}


void URenderer::EndRender()
{
	RenderHardware->EndScene();
}

void URenderer::RenderModel(UModel* InModel, UShader* InShader, ID3D11SamplerState* customSampler)
{
	if (!InModel || !InModel->IsInitialized()|| !InShader)
		return;

	//���� ����
	const FVertexDataContainer* vertexData = UModelBufferManager::Get()->GetVertexDataByHash(InModel->GetDataHash());
	
	//�� ����  �Է� ���̾ƿ� ���̴� ����
	ID3D11InputLayout** outLayout;
	const void* shaderBytecode;
	size_t bytecodeLength;
	InShader->GetShaderBytecode(&shaderBytecode, &bytecodeLength);
	vertexData->GetOrCreateInputLayout(GetDevice(), shaderBytecode, bytecodeLength);

	// ���� �Ŵ������� �ش� ���� ���� ���ҽ� ��������
	const FBufferResource* bufferResource = UModelBufferManager::Get()->GetBufferByHash(InModel->GetDataHash());
	if (!bufferResource || !bufferResource->GetVertexBuffer())
		return;

	// ���� ���� ����
	ID3D11Buffer* vertexBuffer = bufferResource->GetVertexBuffer();
	if (vertexBuffer == nullptr)
	{
		// Handle null buffer error
		return;
	}

	UINT stride = vertexData->GetStride();
	UINT offset = 0;

	GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	// ���÷� ������Ʈ ����
	ID3D11SamplerState* samplerState = customSampler ? customSampler : GetDefaultSamplerState();
	GetDeviceContext()->PSSetSamplers(0, 1, &samplerState);

	// �ε��� ���۰� �ִ� ��� �ε��� ���� ���� �� DrawIndexed ȣ��
	if (bufferResource->HasIndexBuffer()) {
		GetDeviceContext()->IASetIndexBuffer(
			bufferResource->GetIndexBuffer(),
			DXGI_FORMAT_R32_UINT,
			0
		);
		GetDeviceContext()->DrawIndexed(vertexData->GetIndexCount(), 0, 0);
	}
	// �ε��� ���۰� ���� ��� �Ϲ� Draw ȣ��
	else {
		GetDeviceContext()->Draw(vertexData->GetVertexCount(), 0);
	}
}

void URenderer::RenderGameObject(UCamera* InCamera,const UGameObject* InObject,  UShader* InShader, ID3D11SamplerState* customSampler)
{
	if (!InObject || !InShader)
		return;

	Matrix WorldMatrix = InObject->GetWorldMatrix();
	Matrix ViewMatrix = InCamera->GetViewMatrix();
	Matrix ProjectionMatrix = InCamera->GetProjectionMatrix();
	auto BufferData = FMatrixBufferData(WorldMatrix, ViewMatrix, ProjectionMatrix);

	InShader->BindMatrix(GetDeviceContext(), BufferData);

	RenderModel(InObject->GetModel(), InShader, customSampler);
}

void URenderer::RenderGameObject(UCamera* InCamera, const UGameObject* InObject, UShader* InShader, ID3D11ShaderResourceView* InTexture, ID3D11SamplerState* InCustomSampler)
{
	if (!InObject || !InShader)
		return;

	Vector4 Color;
	Color = InObject->bDebug ? InObject->GetDebugColor() : Color::White();
	FDebugBufferData DebugBufferData(Color);
	InShader->BindColor(GetDeviceContext(), DebugBufferData);
	InShader->BindTexture(GetDeviceContext(), InTexture, ETextureSlot::Albedo);
	RenderGameObject(InCamera, InObject, InShader, InCustomSampler);
}

bool URenderer::CreateDefaultSamplerState()
{
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // ���̸��Ͼ� ���͸�, �ε巯�� �ؽ�ó ǥ��
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // U��ǥ �ݺ�
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // V��ǥ �ݺ�  
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // W��ǥ �ݺ�
	samplerDesc.MinLOD = 0;                                // �ּ� LOD ����
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;               // �ִ� LOD ���� ����
	samplerDesc.MipLODBias = 0;                           // LOD ���� ���� ����
	samplerDesc.MaxAnisotropy = 1;                        // ���漺 ���͸� ��� ����
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;  // �� ���ø� ��� ����
	
	HRESULT result = GetDevice()->CreateSamplerState(&samplerDesc, &DefaultSamplerState);
	return SUCCEEDED(result);
}

void URenderer::Release()
{
	if (DefaultSamplerState)
	{
		DefaultSamplerState->Release();
		DefaultSamplerState = nullptr;
	}
	RenderHardware->Release();
}

