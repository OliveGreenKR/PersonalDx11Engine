#pragma once
#include <type_traits>
#include "ResourceInterface.h"
#include "ResourceDefines.h


template<typename ResourceType,
typename = std::enable_if_t<std::is_base_of_v<IResource, ResourceType> 
    || std::is_same_v<IResource, ResourceType>> >
class TResourceHandle {
private:
    FResourceKey Key;
    ResourceType* CachedResource = nullptr;  // 캐싱된 포인터

public:
    TResourceHandle(const FResourceKey& InKey) : Key(InKey) {}

    ResourceType* Get() {
        if (!CachedResource) {
            CachedResource = UResourceManager::Get()->GetResource<ResourceType>(Key);
        }
        return CachedResource;
    }

    void Invalidate() {
        CachedResource = nullptr;  // 리소스가 언로드될 때 호출
    }

    operator bool() const { return Key.IsValid(); }
};