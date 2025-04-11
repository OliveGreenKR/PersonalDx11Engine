#include "Object.h"

UObject::~UObject()
{
	// DeletionCallbacks 복사본 생성
	auto callbacksCopy = DeletionCallbacks;
	DeletionCallbacks.clear(); // 이후 등록 방지 및 원본 정리

	// 소멸 시 소멸 콜백 실행
	// 복사본 순회
	// callback의 Unbind에는 UnregisterCallback이 호출됨
	// 이로인해 map이 수정- 순회중에 구조가 변하지 않도록 복사본을 사용
	for (auto& callback : callbacksCopy)
	{
		if (callback.first)
		{
			callback.second();
		}
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

