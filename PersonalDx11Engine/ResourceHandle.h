#pragma once
#include "TypeCast.h"
#include "StringHash.h"
#include <memory>
#include "ResourceInterface.h"

class FResourceHandle
{
    friend class UResourceManager;
private:
    FStringHash Key;

    // 내부적으로 타입을 지운 포인터를 얻는 메서드 (UResourceManager에서만 접근 가능)
    IResource* GetRawResource() const;

public:
    explicit FResourceHandle(const FStringHash& InKey = FStringHash()) : Key(InKey) {}

    bool IsValid() const;
    void Invalidate();

    // 타입을 명시적으로 지정해 리소스를 얻음
    template<typename T>
    T* Get() const {
        IResource* Raw = GetRawResource();
        return Raw ? Engine::Cast<T>(Raw) : nullptr;
    }

    bool operator==(const FResourceHandle& Other) const { return Key == Other.Key; }
    bool operator!=(const FResourceHandle& Other) const { return Key != Other.Key; }
};