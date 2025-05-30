#include "ResourceManager.h"
#include "D3DShader.h"
#include "Texture.h"
#include "Debug.h"
#include "TypeCast.h"

UResourceManager::~UResourceManager()
{
    for (auto& rscCache : ResourceCache)
    {
        rscCache.second.Resource.reset();
    }
}

IResource* UResourceManager::GetRawResource(const FStringHash& InKey) const
{
    auto it = ResourceCache.find(InKey.GetHash());
    if (it != ResourceCache.end()) {
        it->second.LastAccessTick = CurrentTick;
        return it->second.Resource.get();
    }
    return nullptr;
}

bool UResourceManager::IsLoadedKey(const uint32_t InKey) const
{
    return ResourceCache.find(InKey) != ResourceCache.end();
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
    ResourceCache.clear();

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

// 미사용 리소스 언로드
void UResourceManager::UnloadUnusedResources(float TimeSinceLastUseSeconds)
{
    const float TimeThreshold = TimeSinceLastUseSeconds;

    //  캐시 정리
    auto rscIt = ResourceCache.begin();
    while (rscIt != ResourceCache.end())
    {
        if (CurrentTick - rscIt->second.LastAccessTick > TimeThreshold)
        {
            LOG("Rsc delted : [%ls]", rscIt->second.Resource->GetPath().c_str());
            rscIt = ResourceCache.erase(rscIt);

            /*enum class EResourceType
            {
                Shader,
                Texture,
                Material,
                Model,
                Max,
            };*/
        }
        else
        {
            ++rscIt;
        }
    }

}

// 디버깅용 리소스 통계
void UResourceManager::PrintResourceStats()
{

    std::uint32_t Textures = 0;
    std::uint32_t Shaders = 0;
    for (const auto& rsc : ResourceCache)
    {
        auto type = rsc.second.Resource->GetType();

        switch (type)
        {
            case EResourceType::Texture :
            {
                Textures++;
                break;
            }
            case EResourceType::Shader :
            {
                Shaders++;
                break;
            }
        }
    }
    LOG("- Total : %zu\n", ResourceCache.size());
    LOG("- Shaders : %zu\n", Shaders);
    LOG("- Textures : %zu\n", Textures);
}
