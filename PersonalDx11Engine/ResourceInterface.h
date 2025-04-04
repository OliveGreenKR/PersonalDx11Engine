#pragma once
#include <string>

enum class EResourceType
{
    Shader,
    Texture,
    Material,
    Model,
    Max,
};
class IResource
{
public:
    virtual ~IResource() = default;
    virtual bool Load(class IRenderHardware* RenderHardware, const std::wstring& Path) = 0;
    virtual bool LoadAsync(class IRenderHardware* RenderHardware, const std::wstring& Path) = 0;
    virtual bool IsLoaded() const = 0;
    virtual void Release() = 0;
    virtual size_t GetMemorySize() const = 0;
    virtual EResourceType GetType() const = 0;
};