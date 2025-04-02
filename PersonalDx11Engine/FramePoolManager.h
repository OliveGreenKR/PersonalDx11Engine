#pragma once
#include "FrameMemoryPool.h"

//매프레임 초기화되는  전역 메모리풀 제공
class UFramePoolManager
{
private:
    // 싱글톤 패턴
    UFramePoolManager() : FrameMemoryPool(4 * 1024 * 1024) {}; //초기 풀크기 4MB
    ~UFramePoolManager() = default;

    // 복사/이동 방지
    UFramePoolManager(const UFramePoolManager&) = delete;
    UFramePoolManager& operator=(const UFramePoolManager&) = delete;
    UFramePoolManager(UFramePoolManager&&) = delete;
    UFramePoolManager& operator=(UFramePoolManager&&) = delete;

private:
    FFrameMemoryPool FrameMemoryPool;

public:
    static UFramePoolManager* Get()
    {
        static UFramePoolManager* Instance =[]() 
            {
                UFramePoolManager* manager = new UFramePoolManager();
                return manager;
            }();

        return Instance;
    }

    void EndFrame()
    {
        FrameMemoryPool.Reset();
    }

    void* AllocateVoid(size_t Size)
    {
        return FrameMemoryPool.AllocateVoid(Size);
    }

    template<typename T>
    T* Allocate() 
    {
        return FrameMemoryPool.Allocate<T>();
    }

    template<typename T = void, typename... Args>
    T* Allocate(Args&&... args) 
    {
        return FrameMemoryPool.Allocate<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T* AllocateWithData(const T& data)
    {
        return FrameMemoryPool.AllocateWithData<T>(data);
    }

};