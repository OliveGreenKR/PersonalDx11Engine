#include "ResourceManager.h"
#include "D3DShader.h"
#include "Texture.h"
#include "Debug.h"


// 리소스 접근 시간 업데이트
void UResourceManager::UpdateResourceAccessTime(const std::wstring& Key, std::unordered_map<std::wstring, FResourceData>& Cache)
{
    auto it = Cache.find(Key);
    if (it != Cache.end())
    {
        it->second.LastAccessTick = CurrentTick;
    }
}

void UResourceManager::Initialize(IRenderHardware* InHardware)
{
    assert(InHardware && "RenderHardware cannot be null");
    RenderHardware = InHardware;
    bInitialized = true;
}

void UResourceManager::Shutdown()
{
    // 리소스 해제
    TextureCache.clear();
    ShaderCache.clear();

    bInitialized = false;
}

void UResourceManager::Tick(const float DeltaTime)
{
    CurrentTick += DeltaTime;

    if (abs(CurrentTick / (MAX_UNUSED_TIME *0.5f) - 
            round(CurrentTick / (MAX_UNUSED_TIME * 0.5f))) < KINDA_SMALL)
    {
        UnloadUnusedResources(MAX_UNUSED_TIME);
    }
}

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
        LOG("Shader Load Failed");
        return nullptr; // 로드 실패
    }

    // 캐시에 추가
    FResourceData ResourceData;
    ResourceData.Resource = Shader;
    ResourceData.LastAccessTick = CurrentTick;
    ShaderCache[ShaderKey] = ResourceData;

    return Shader;
}

// 미사용 리소스 언로드
void UResourceManager::UnloadUnusedResources(float TimeSinceLastUseSeconds)
{
    const float TimeThreshold = TimeSinceLastUseSeconds;

    // 텍스처 캐시 정리
    auto texIt = TextureCache.begin();
    while (texIt != TextureCache.end())
    {
        if (CurrentTick - texIt->second.LastAccessTick > TimeThreshold)
        {
            texIt = TextureCache.erase(texIt);
            LOG("Texture deleted in ResourceManager");
        }
        else
        {
            ++texIt;
        }
    }

    // 셰이더 캐시 정리
    auto shaderIt = ShaderCache.begin();
    while (shaderIt != ShaderCache.end())
    {
        if (CurrentTick - shaderIt->second.LastAccessTick > TimeThreshold)
        {
            shaderIt = ShaderCache.erase(shaderIt);
            LOG("Shader deleted in ResourceManager");
        }
        else
        {
            ++shaderIt;
        }
    }
}

// 디버깅용 리소스 통계
void UResourceManager::PrintResourceStats()
{
    printf("Resource Statistics:\n");
    printf("- Textures: %zu\n", TextureCache.size());
    printf("- Shaders: %zu\n", ShaderCache.size());
}
