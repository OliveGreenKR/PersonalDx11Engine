#include "PrimitiveComponent.h"
#include "ModelBufferManager.h"
#include "SceneManager.h"
#include "RenderDataTexture.h"
#include "ResourceManager.h"
#include "Model.h"
#include "Texture.h"
#include "VertexShader.h"
#include "Camera.h"
#include "Debug.h"

void UPrimitiveComponent::SetModel(const std::shared_ptr<UModel>& InModel)
{
    //TODO
    // 모델데이터도 리소스로 변경후 키값으로 변화 감지.
    Model = InModel;
}

void UPrimitiveComponent::SetColor(const Vector4& InColor) 
{
    if (Color == InColor)
    {
        return;
    }
    Color = InColor;
}

void UPrimitiveComponent::SetTexture(const FResourceHandle& InHandle)
{
    if (!InHandle.IsValid() || TextureHandle == InHandle )
    {
        return; 
    }
    TextureHandle = InHandle;
}
