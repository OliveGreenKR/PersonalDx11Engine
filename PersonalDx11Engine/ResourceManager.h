#pragma once
#include "ResourceInterface.h"
#include "RenderHardwareInterface.h"
#include <unordered_map>
#include <string>
#include <memory>

struct FResourceData
{
    std::shared_ptr<IResource> Resource;
    size_t LastAccessTick = 0; // 마지막 접근 시간
};

// UResourceManager.h - 리소스 관리자
class UResourceManager
{
private:
    static constexpr size_t MAX_UNUSED_TIME = 300; // 5분 이상 미사용 리소스 해제

    bool bInitialized = false;
    size_t CurrentTick = 0;

    IRenderHardware* RenderHardware = nullptr;

    // 경로 기반 리소스 캐시
    std::unordered_map<std::wstring, FResourceData> TextureCache;
    std::unordered_map<std::wstring, FResourceData> ShaderCache;

    // 싱글톤 패턴
    UResourceManager() = default;
    ~UResourceManager() = default;

    // 복사/이동 방지
    UResourceManager(const UResourceManager&) = delete;
    UResourceManager& operator=(const UResourceManager&) = delete;
    UResourceManager(UResourceManager&&) = delete;
    UResourceManager& operator=(UResourceManager&&) = delete;

    // 리소스 접근 시간 업데이트
    void UpdateResourceAccessTime(const std::wstring& Key, std::unordered_map<std::wstring, FResourceData>& Cache)
    {
        auto it = Cache.find(Key);
        if (it != Cache.end())
        {
            it->second.LastAccessTick = CurrentTick;
        }
    }

public:
    static UResourceManager* Get()
    {
        static UResourceManager Instance;
        return &Instance;
    }

    void Initialize(IRenderHardware* InHardware)
    {
        assert(InHardware && "RenderHardware cannot be null");
        RenderHardware = InHardware;
        bInitialized = true;
    }

    void Shutdown()
    {
        // 리소스 해제
        TextureCache.clear();
        ShaderCache.clear();

        bInitialized = false;
    }

    // 시스템 틱 업데이트
    void Tick()
    {
        ++CurrentTick;
    }

    // 텍스처 로드
    std::shared_ptr<class UTexture2D> LoadTexture(const std::wstring& FilePath, bool bAsync = false);

    // 셰이더 로드
    std::shared_ptr<class UShader> LoadShader(const std::wstring& VSPath, const std::wstring& PSPath);

    // 미사용 리소스 언로드
    void UnloadUnusedResources(float TimeSinceLastUse = 60.0f)
    {
        const size_t TimeThreshold = static_cast<size_t>(TimeSinceLastUse * 60); // 초 단위 변환 가정

        // 텍스처 캐시 정리
        auto texIt = TextureCache.begin();
        while (texIt != TextureCache.end())
        {
            if (CurrentTick - texIt->second.LastAccessTick > TimeThreshold)
            {
                texIt = TextureCache.erase(texIt);
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
            }
            else
            {
                ++shaderIt;
            }
        }
    }

    // 디버깅용 리소스 통계
    void PrintResourceStats()
    {
        printf("Resource Statistics:\n");
        printf("- Textures: %zu\n", TextureCache.size());
        printf("- Shaders: %zu\n", ShaderCache.size());
    }
};