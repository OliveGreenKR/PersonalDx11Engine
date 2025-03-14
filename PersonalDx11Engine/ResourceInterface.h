#pragma once
class IResource
{
public:
    virtual ~IResource() = default;
    virtual bool IsLoaded() const = 0;
    virtual void Release() = 0;
    virtual size_t GetMemorySize() const = 0;
};