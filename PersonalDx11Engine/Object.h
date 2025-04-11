#pragma once
#include "BindableInterface.h"
#include <unordered_map>
#include <functional>

class UObject : public IBindable
{
private:
	std::unordered_map<void*, std::function<void()>> DeletionCallbacks; //for IBindable

public:
	virtual ~UObject();
	// Inherited via IBindable
	void RegisterDeletionCallback(void* key, std::function<void()> callback) override;
	void UnregisterDeletionCallback(void* key) override;
};