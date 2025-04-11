#include "Object.h"

UObject::~UObject()
{
	// 소멸 시 소멸 콜백 실행
	for (auto& callback : DeletionCallbacks)

		if (callback.first)
		{
			callback.second();
		}

}

void UObject::RegisterDeletionCallback(void* key, std::function<void()> callback)
{
	DeletionCallbacks[key] = callback;
}

void UObject::UnregisterDeletionCallback(void* key)
{
	DeletionCallbacks.erase(key);
}

