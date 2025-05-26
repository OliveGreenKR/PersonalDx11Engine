#pragma once
#include <cstddef>
#include <stdexcept>
#include <new>
#include <type_traits>
#include <cstring>
#include <vector>
#include <functional>

using byte = unsigned char;

class FArenaMemoryPool {
private:
    byte* Buffer;          // 단일 큰 버퍼
    size_t BufferSize;     // 버퍼의 총 크기
    size_t UsedBytes;      // 현재 사용 중인 바이트 수

    // 소멸자 호출을 위한 정보 저장 구조체
    struct DestructorInfo {
        void* Ptr;                            // 객체 주소
        std::function<void(void*)> Destroyer; // 소멸자 함수
    };

    // 할당된 객체들의 소멸자 정보
    std::vector<DestructorInfo> AllocatedObjects;

    // 타입별 소멸자 함수 생성 헬퍼 템플릿
    template<typename T>
    static void DestroyObject(void* ptr) {
        if (ptr) {
            static_cast<T*>(ptr)->~T();
        }
    }

public:
    explicit FArenaMemoryPool(size_t size = 10 * 1024 * 1024) // 기본 10MB
        : BufferSize(size), UsedBytes(0) {
        Buffer = new byte[BufferSize];
        std::memset(Buffer, 0, BufferSize); // 초기화
    }

    ~FArenaMemoryPool() {
        // 모든 객체의 소멸자 호출 (역순)
        for (auto it = AllocatedObjects.rbegin(); it != AllocatedObjects.rend(); ++it) {
            it->Destroyer(it->Ptr);
        }
        delete[] Buffer;
    }

    // 복사 및 이동 생성자/할당 연산자 삭제
    FArenaMemoryPool(const FArenaMemoryPool&) = delete;
    FArenaMemoryPool& operator=(const FArenaMemoryPool&) = delete;
    FArenaMemoryPool(FArenaMemoryPool&&) = delete;
    FArenaMemoryPool& operator=(FArenaMemoryPool&&) = delete;

    void Initialize(size_t byteBufferSize)
    {
        //기존 버퍼 클리어 및 삭제
        if (Buffer)
        {
            Reset();
            delete[] Buffer;
        }

        BufferSize = byteBufferSize;
        UsedBytes = 0;
        Buffer = new byte[BufferSize];
        std::memset(Buffer, 0, BufferSize); // 초기화
    }

    template<typename T = byte>
    void* AllocateVoid(size_t size = sizeof(T)) {
        // 정렬 요구사항 처리
        size_t alignedSize = (size + alignof(T) - 1) & ~(alignof(T) - 1);

        // 남은 공간 확인
        if (UsedBytes + alignedSize > BufferSize) {
            throw std::bad_alloc(); // 버퍼 공간 부족
        }

        byte* ptr = Buffer + UsedBytes;
        UsedBytes += alignedSize;
        return ptr;
    }

    template<typename T>
    T* Allocate() {
        void* ptr = AllocateVoid<T>();
        T* obj = new(ptr) T(); // 기본 생성자 호출

        // 소멸자 정보 저장
        AllocatedObjects.push_back({ obj, &DestroyObject<T> });
        return obj;
    }

    template<typename T, typename... Args>
    T* Allocate(Args&&... args) {
        static_assert(std::is_constructible<T, Args...>::value,
                      "T must be constructible with the provided arguments");
        void* ptr = AllocateVoid<T>();
        T* obj = new(ptr) T(std::forward<Args>(args)...);

        // 소멸자 정보 저장
        AllocatedObjects.push_back({ obj, &DestroyObject<T> });
        return obj;
    }

    template<typename T>
    T* AllocateWithData(const T& data) {
        static_assert(std::is_copy_constructible<T>::value,
                      "T must be copy constructible for AllocateWithData");
        void* ptr = AllocateVoid<T>();
        T* obj = new(ptr) T(data);

        // 소멸자 정보 저장
        AllocatedObjects.push_back({ obj, &DestroyObject<T> });
        return obj;
    }

    // 프레임 단위 초기화: 소멸자 호출 후 메모리를 0으로 초기화하고 시작점으로 리셋
    void Reset() {
        // 역순으로 소멸자 호출 (생성 순서의 반대)
        for (auto it = AllocatedObjects.rbegin(); it != AllocatedObjects.rend(); ++it) {
            it->Destroyer(it->Ptr);
        }

        AllocatedObjects.clear();
        std::memset(Buffer, 0, UsedBytes); // 사용한 부분만 초기화
        UsedBytes = 0;
    }

    // 현재 사용된 메모리 양 반환
    size_t GetUsedBytes() const {
        return UsedBytes;
    }

    // 총 버퍼 크기 반환
    size_t GetBufferSize() const {
        return BufferSize;
    }

    // 할당된 객체 수 반환
    size_t GetObjectCount() const {
        return AllocatedObjects.size();
    }
};