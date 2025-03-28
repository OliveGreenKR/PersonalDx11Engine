#pragma once
#include <type_traits>
#include "ResourceInterface.h"
#include "ResourceDefines.h"
#include "TypeCast.h"

template<typename T,
    typename = std::enable_if_t<std::is_base_of_v<IResource, T> || std::is_same_v<IResource, T>>>
class TResourceHandle {
private:
    FResourceKey Key;
    T* CachedResource = nullptr;  // 캐싱된 포인터

public:
    TResourceHandle(const FResourceKey& InKey) : Key(InKey) {}

    T* GetResource() {
        if (!CachedResource) {
            CachedResource = Engine::Cast<T>( UResourceManager::Get()->GetResource(Key) );
        }
        return CachedResource;
    }
    
    FResourceKey& GetKey() { return Key; }

    void Invalidate() {
        Key.InValidate();
        CachedResource = nullptr;  
    }

    operator bool() const { return Key.IsValid(); }
};

