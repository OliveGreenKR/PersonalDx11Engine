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
    explicit FResourceHandle() : Key() {}
    explicit FResourceHandle(const wchar_t* InPath);
    explicit FResourceHandle(const char* InPath);

    //키 유효여부  + 로딩 여부 확인
    bool IsLoaded() const;
    //키 유효여부, 로드여부는 모름
    bool IsValid() const;
    //무효화
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