#pragma once
#include "ResourceInterface.h"
#include "RenderHardwareInterface.h"
#include <unordered_map>
#include <string>
#include <memory>
#include "TypeCast.h"
#include "StringHash.h"

//외부 제공 리소스 핸들
class FResourceHandle;

struct FResourceData {
    std::unique_ptr<IResource> Resource;
    mutable float LastAccessTick = 0; // 마지막 접근 시간
};

// UResourceManager.h - 리소스 관리자
class UResourceManager {
private:
    static constexpr size_t MAX_UNUSED_TIME = 300; // 5분 이상 미사용 리소스 해제

    bool bInitialized = false;
    float CurrentTick = 0.0f;

    IRenderHardware* RenderHardware = nullptr;

    // 리소스 캐시 (해시값 -> 리소스 데이터)
    std::unordered_map<std::uint32_t, FResourceData> ResourceCache;

    // 싱글톤 패턴
    UResourceManager() = default;
    ~UResourceManager() = default;

    // 복사/이동 방지
    UResourceManager(const UResourceManager&) = delete;
    UResourceManager& operator=(const UResourceManager&) = delete;
    UResourceManager(UResourceManager&&) = delete;
    UResourceManager& operator=(UResourceManager&&) = delete;

    // 리소스 얻기
    IResource* GetRawResource(const FStringHash& InKey) const;

    //키 등록여부 확인
    bool IsValidKey(const uint32_t InKey) const;

    // FResourceHandle을 friend로 지정(리소스 get을 위해)
    friend class FResourceHandle;

public:
    static UResourceManager* Get() {
        static UResourceManager Instance;
        return &Instance;
    }

    void Initialize(IRenderHardware* InHardware);
    void Shutdown();
    void Tick(const float DeltaTime);

    // 리소스 로드
    template <typename T>
    FResourceHandle LoadResource(const std::wstring& FilePath, bool bAsync = false);

    // 미사용 리소스 언로드
    void UnloadUnusedResources(float TimeSinceLastUseSec = 60.0f);

    // 디버깅용 리소스 통계
    void PrintResourceStats();
};

template<typename T>
inline FResourceHandle UResourceManager::LoadResource(const std::wstring& FilePath, bool bAsync)
{
    assert(bInitialized && "ResourceManager not initialized");

    static_assert(std::is_base_of_v<IResource, T> || std::is_same_v<IResource, T>);

    FStringHash RscKey(FilePath.c_str());

    // 이미 로드된 텍스처인지 확인
    auto it = ResourceCache.find(RscKey.GetHash());
    if (it != ResourceCache.end())
    {
        // 접근 시간 업데이트
        it->second.LastAccessTick = CurrentTick;

        return FResourceHandle(RscKey);
    }

    // 새 리소스 객체 생성
    auto rscUniquePtr = std::make_unique<T>();

    // 텍스처 로드
    bool success = false;

    if (bAsync)
    {
        // 비동기 로드 시작
        success = rscUniquePtr->LoadAsync(RenderHardware, FilePath);
    }
    else
    {
        // 동기 로드
        success = rscUniquePtr->Load(RenderHardware, FilePath);
    }

    if (success)
    {
        // 캐시에 추가
        FResourceData ResourceData;
        ResourceData.Resource = std::move(rscUniquePtr);
        ResourceData.LastAccessTick = CurrentTick;
        ResourceCache[RscKey.GetHash()] = std::move(ResourceData);
        return FResourceHandle(RscKey);
    }

    RscKey.Invalidate();
    return FResourceHandle(RscKey);
}

class FResourceHandle
{
    friend class UResourceManager;
private:
    FStringHash Key;

    // 내부적으로 타입을 지운 포인터를 얻는 메서드 (UResourceManager에서만 접근 가능)
    IResource* GetRawResource() const {
        return UResourceManager::Get()->GetRawResource(Key);
    }

public:
    explicit FResourceHandle(const FStringHash & InKey = FStringHash()) : Key(InKey) {}

    bool IsValid() const 
    { 
        return Key.IsValid() && UResourceManager::Get()->IsValidKey(Key.GetHash()); 
    }
    void Invalidate() { Key.Invalidate(); }

    // 타입을 명시적으로 지정해 리소스를 얻음
    template<typename T>
    T* Get() const {
        IResource* Raw = GetRawResource();
        return Raw ? Engine::Cast<T>(Raw) : nullptr;
    }

    bool operator==(const FResourceHandle & Other) const { return Key == Other.Key; }
    bool operator!=(const FResourceHandle & Other) const { return Key != Other.Key; }
};