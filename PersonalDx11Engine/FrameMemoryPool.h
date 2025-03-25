#include <vector>
#include <cstddef>      // size_t 용
#include <stdexcept>
#include <cstring>      // std::memset 등 메모리 작업용
#include <new>          // placement new 용
#include <type_traits> // std::is_copy_constructible 용

using byte = unsigned char;

//Arena allocation방식의 메모리 풀
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

    ~FFrameMemoryPool() {
        Clear();
    }

    template<typename T = void>
    void* AllocateVoid(size_t size = sizeof(T)) {
        size_t alignedSize = (size + alignof(T) - 1) & ~(alignof(T) - 1);
        if (usedBytes + alignedSize > bufferSize) {
            currentBuffer = AllocateNewBuffer();
            usedBytes = 0;
        }
        byte* ptr = currentBuffer + usedBytes;
        usedBytes += alignedSize;

        return (ptr);
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
        // placement new에 가변 인자를 전달
        return new(ptr) T(std::forward<Args>(args)...);
    }

    // 복사 생성 가능한 경우에만 동작하도록 제한
    template<typename T>
    T* AllocateWithData(const T& data) {
        static_assert(std::is_copy_constructible<T>::value,
                      "T must be copy constructible for AllocateWithData");

        T* ptr = Allocate<T>();
        try {
            new (ptr) T(data); // 복사 생성 시도
            return ptr;
        }
        catch (...) {
         // 예외 발생 시 nullptr 반환
            return nullptr;
        }
    }

    void Reset() {
        usedBytes = 0;
        if (!buffers.empty()) {
            currentBuffer = buffers[0];
        }
    }

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

    size_t GetTotalAllocated() const { return totalAllocated; }
    size_t GetCurrentBufferUsed() const { return usedBytes; }
    size_t GetBufferCount() const { return buffers.size(); }
};