#include "ResourceHandle.h"
#include "ResourceManager.h"

// 내부적으로 타입을 지운 포인터를 얻는 메서드 (UResourceManager에서만 접근 가능)
IResource* FResourceHandle::GetRawResource() const {
    return UResourceManager::Get()->GetRawResource(Key);
}

bool FResourceHandle::IsValid() const
{
    return Key.IsValid() && UResourceManager::Get()->IsValidKey(Key.GetHash());
}

void FResourceHandle::Invalidate() { Key.Invalidate(); }
