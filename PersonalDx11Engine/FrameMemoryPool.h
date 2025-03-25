#include <vector>
#include <cstddef>      // size_t 용
#include <stdexcept>
#include <cstring>      // std::memset 등 메모리 작업용
#include <new>          // placement new 용

using byte = unsigned char;

/// <summary>
/// Arena 방식의 메모리 풀. 매 프레임 리셋
/// </summary>
class FrameMemoryPool {
private:
    std::vector<byte*> buffers; // 할당된 메모리 블록들
    byte* currentBuffer;        // 현재 사용 중인 버퍼
    size_t bufferSize;                   // 각 버퍼의 크기
    size_t usedBytes;                    // 현재 버퍼에서 사용된 바이트 수
    size_t totalAllocated;               // 총 할당된 메모리 크기

    // 새 버퍼 할당
    byte* AllocateNewBuffer() {
        byte* newBuffer = new byte[bufferSize];
        buffers.push_back(newBuffer);
        totalAllocated += bufferSize;
        return newBuffer;
    }

public:
    explicit FrameMemoryPool(size_t singleBufferSize = 1024 * 1024) // 기본 1MB 버퍼
        : bufferSize(singleBufferSize), usedBytes(0), totalAllocated(0) {
        currentBuffer = AllocateNewBuffer();
    }

    ~FrameMemoryPool() {
        Clear();
    }

    // 메모리 할당 (정렬 고려)
    template<typename T = void>
    T* Allocate(size_t size = sizeof(T)) {
        size_t alignedSize = (size + alignof(T) - 1) & ~(alignof(T) - 1); // T의 정렬 요구사항 맞춤

        if (usedBytes + alignedSize > bufferSize) {
            currentBuffer = AllocateNewBuffer();
            usedBytes = 0;
        }

        byte* ptr = currentBuffer + usedBytes;
        usedBytes += alignedSize;

        return reinterpret_cast<T*>(ptr);
    }

    // 데이터와 함께 메모리 할당
    template<typename T>
    T* AllocateWithData(const T& data) {
        T* ptr = Allocate<T>();
        new (ptr) T(data); // 복사 생성
        return ptr;
    }

    // 프레임 끝에서 모든 메모리 초기화
    void Reset() {
        usedBytes = 0;
        if (!buffers.empty()) {
            currentBuffer = buffers[0];
        }
    }

    // 모든 메모리 해제
    void Clear() {
        for (byte* buffer : buffers) {
            delete[] buffer;
        }
        buffers.clear();
        totalAllocated = 0;
        usedBytes = 0;
        if (buffers.empty()) {
            currentBuffer = AllocateNewBuffer();
        }
        else {
            currentBuffer = buffers[0];
        }
    }

    // 상태 조회
    size_t GetTotalAllocated() const { return totalAllocated; }
    size_t GetCurrentBufferUsed() const { return usedBytes; }
    size_t GetBufferCount() const { return buffers.size(); }
};
