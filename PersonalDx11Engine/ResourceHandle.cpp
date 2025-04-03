#include "ResourceHandle.h"
#include "ResourceManager.h"

// 내부적으로 타입을 지운 포인터를 얻는 메서드 (UResourceManager에서만 접근 가능)
IResource* FResourceHandle::GetRawResource() const {
    return UResourceManager::Get()->GetRawResource(Key);
}

FResourceHandle::FResourceHandle(const wchar_t* InPath) : Key(InPath)
{
}

FResourceHandle::FResourceHandle(const char* InPath) : Key(InPath)
{
}

bool FResourceHandle::IsLoaded() const
{
    return Key.IsValid() && UResourceManager::Get()->IsLoadedKey(Key.GetHash());
}

bool FResourceHandle::IsValid() const
{
    return Key.IsValid();
}

void FResourceHandle::Invalidate() { Key.Invalidate(); }
