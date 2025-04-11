#pragma once
#include <functional>
#include <memory>

// 델리게이트 바인딩을 위한 인터페이스
class IBindable
{
public:
	virtual ~IBindable() = default;

	// 객체가 소멸될 때 호출할 콜백 등록
	virtual void RegisterDeletionCallback(void* key, std::function<void()> callback) = 0;

	// 콜백 해제
	virtual void UnregisterDeletionCallback(void* key) = 0;
};
 