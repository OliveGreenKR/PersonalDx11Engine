#pragma once
#include <vector>
#include <cstddef>      
#include <stdexcept>
#include <cstring>      
#include <new>          // placement new 용
#include <type_traits>  

using byte = unsigned char;

class FFrameMemoryPool {
private:
    std::vector<byte*> buffers;
    byte* currentBuffer;
    size_t bufferSize;
    size_t usedBytes;
    size_t totalAllocated;

    byte* AllocateNewBuffer() {
        byte* newBuffer = new byte[bufferSize];
        buffers.push_back(newBuffer);
        totalAllocated += bufferSize;
        return newBuffer;
    }

public:
    explicit FFrameMemoryPool(size_t singleBufferSize = 1024 * 1024)
        : bufferSize(singleBufferSize), usedBytes(0), totalAllocated(0) {
        currentBuffer = AllocateNewBuffer();
    }

    // 소멸자에서만 완전 해제
    ~FFrameMemoryPool() {
        for (byte* buffer : buffers) {
            delete[] buffer;
        }
    }

    template<typename T = byte>
    void* AllocateVoid(size_t size = sizeof(T)) {
        size_t alignedSize = (size + alignof(T) - 1) & ~(alignof(T) - 1);
        if (usedBytes + alignedSize > bufferSize) {
            currentBuffer = AllocateNewBuffer();
            usedBytes = 0;
        }
        byte* ptr = currentBuffer + usedBytes;
        usedBytes += alignedSize;
        return ptr;
    }

    template<typename T>
    T* Allocate() {
        void* ptr = AllocateVoid<T>();
        return new(ptr) T(); // 기본 생성자 호출
    }

    template<typename T = void, typename... Args>
    T* Allocate(Args&&... args) {
        static_assert(std::is_constructible<T, Args...>::value,
                      "T must be constructible with the provided arguments");
        void* ptr = AllocateVoid<T>();
        return new(ptr) T(std::forward<Args>(args)...);
    }

    template<typename T>
    T* AllocateWithData(const T& data) {
        static_assert(std::is_copy_constructible<T>::value,
                      "T must be copy constructible for AllocateWithData");
        T* ptr = Allocate<T>();
        try {
            new(ptr) T(data);
            return ptr;
        }
        catch (...) {
            return nullptr;
        }
    }

    // 프레임 단위 초기화: 동적 해제 없이 재사용
    void Reset() {
        usedBytes = 0;
        if (!buffers.empty()) {
            currentBuffer = buffers[0]; // 첫 번째 버퍼로 되돌림
        }
    }

    // 명시적 전체 해제 (필요 시 호출)
    void ReleaseAll() {
        for (byte* buffer : buffers) {
            delete[] buffer;
        }
        buffers.clear();
        totalAllocated = 0;
        usedBytes = 0;
        currentBuffer = AllocateNewBuffer(); // 새 시작점 제공
    }

    size_t GetTotalAllocated() const { return totalAllocated; }
    size_t GetCurrentBufferUsed() const { return usedBytes; }
    size_t GetBufferCount() const { return buffers.size(); }
};