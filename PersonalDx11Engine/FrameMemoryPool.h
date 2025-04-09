#pragma once
#include <cstddef>
#include <stdexcept>
#include <new>
#include <type_traits>
#include <cstring> // memset 사용을 위해 필요

using byte = unsigned char;

class FFrameMemoryPool {
private:
    byte* buffer;          // 단일 큰 버퍼
    size_t bufferSize;     // 버퍼의 총 크기
    size_t usedBytes;      // 현재 사용 중인 바이트 수

public:
    explicit FFrameMemoryPool(size_t size = 10 * 1024 * 1024) // 기본 10MB
        : bufferSize(size), usedBytes(0) {
        buffer = new byte[bufferSize];
        std::memset(buffer, 0, bufferSize); // 초기화
    }

    ~FFrameMemoryPool() {
        delete[] buffer;
    }

    // 복사 및 이동 생성자/할당 연산자 삭제
    FFrameMemoryPool(const FFrameMemoryPool&) = delete;
    FFrameMemoryPool& operator=(const FFrameMemoryPool&) = delete;
    FFrameMemoryPool(FFrameMemoryPool&&) = delete;
    FFrameMemoryPool& operator=(FFrameMemoryPool&&) = delete;

    template<typename T = byte>
    void* AllocateVoid(size_t size = sizeof(T)) {
        // 정렬 요구사항 처리
        size_t alignedSize = (size + alignof(T) - 1) & ~(alignof(T) - 1);

        // 남은 공간 확인
        if (usedBytes + alignedSize > bufferSize) {
             // 버퍼 공간 부족
            return nullptr;
        }

        byte* ptr = buffer + usedBytes;
        usedBytes += alignedSize;
        return ptr;
    }

    template<typename T>
     T* Allocate() {
        void* ptr = AllocateVoid<T>();
        return new(ptr) T(); // 기본 생성자 호출
    }

    template<typename T, typename... Args>
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
        void* ptr = AllocateVoid<T>();
        return new(ptr) T(data);
    }

    // 프레임 단위 초기화: 모든 메모리를 0으로 초기화하고 시작점으로 리셋
    void Reset() {
        //std::memset(buffer, 0, usedBytes); 
        std::memset(buffer, 0, bufferSize);
        usedBytes = 0;
    }

    // 현재 사용된 메모리 양 반환
    size_t GetUsedBytes() const {
        return usedBytes;
    }

    // 총 버퍼 크기 반환
    size_t GetBufferSize() const {
        return bufferSize;
    }
};