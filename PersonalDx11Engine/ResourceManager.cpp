#include "ResourceManager.h"
#include "D3DShader.h"
#include "Texture.h"

std::shared_ptr<UTexture2D> UResourceManager::LoadTexture(
    const std::wstring& FilePath,
    bool bAsync)
{
    assert(bInitialized && "ResourceManager not initialized");

    // 이미 로드된 텍스처인지 확인
    auto it = TextureCache.find(FilePath);
    if (it != TextureCache.end())
    {
        // 접근 시간 업데이트
        it->second.LastAccessTick = CurrentTick;

        // 캐시된 텍스처 반환
        return std::static_pointer_cast<UTexture2D>(it->second.Resource);
    }

    // 새 텍스처 객체 생성
    auto texture = std::make_shared<UTexture2D>();

    // 텍스처 로드
    bool success = false;
    if (bAsync)
    {
        // 비동기 로드 시작
        success = texture->LoadAsync(RenderHardware, FilePath);
    }
    else
    {
        // 동기 로드
        success = texture->Load(RenderHardware, FilePath);
    }

    if (success)
    {
        // 캐시에 추가
        FResourceData ResourceData;
        ResourceData.Resource = texture;
        ResourceData.LastAccessTick = CurrentTick;
        TextureCache[FilePath] = ResourceData;

        return texture;
    }

    return nullptr; // 로드 실패
}

std::shared_ptr<class UShader> UResourceManager::LoadShader(const std::wstring& VSPath, const std::wstring& PSPath)
{
    assert(bInitialized && "ResourceManager not initialized");

    // 셰이더 캐시 키 생성 (VS와 PS 경로 조합)
    std::wstring ShaderKey = VSPath + L"|" + PSPath;

    // 이미 로드된 셰이더인지 확인
    auto it = ShaderCache.find(ShaderKey);
    if (it != ShaderCache.end())
    {
        // 접근 시간 업데이트
        it->second.LastAccessTick = CurrentTick;
        return std::static_pointer_cast<UShader>(it->second.Resource);
    }

    // 새 셰이더 로드
    auto Shader = std::make_shared<UShader>();
    Shader->Load(RenderHardware->GetDevice(), VSPath.c_str(), PSPath.c_str());
    if (!Shader->IsLoaded())
    {
        return nullptr; // 로드 실패
    }

    // 캐시에 추가
    FResourceData ResourceData;
    ResourceData.Resource = Shader;
    ResourceData.LastAccessTick = CurrentTick;
    ShaderCache[ShaderKey] = ResourceData;

    return Shader;
}
