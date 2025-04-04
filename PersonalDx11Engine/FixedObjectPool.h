#pragma once
#include <vector>
#include <memory>
#include <stdexcept>

/// <summary>
/// 고정크기의 재사용 객체 최적화 메모리 풀 템플릿
/// 최대 크기 고정
/// </summary>
template<typename T>
class TFixedObjectPool {
private:
    std::vector<std::unique_ptr<T>> Pool;      // 사용 가능한 오브젝트들
    std::vector<T*> ActiveObjects;             // 현재 사용 중인 오브젝트들
    size_t MaxSize;                            // 풀의 최대 크기

    // call new Object construct  
    template<typename... Args>
    T* CreateObject(Args&&... args) {
        return new T(std::forward<Args>(args)...);
    }

public:
    explicit TFixedObjectPool(size_t maxSize = 100) : MaxSize(maxSize) {
        Pool.reserve(maxSize);  // 초기 용량 예약
    }

    ~TFixedObjectPool() {
        Clear();
    }

    // 풀 초기화
    template<typename... Args>
    void Initialize(size_t initialSize, Args&&... args) {
        if (initialSize > MaxSize) {
            throw std::runtime_error("Initial size exceeds maximum pool size");
        }

        for (size_t i = 0; i < initialSize; ++i) {
            Pool.push_back(std::unique_ptr<T>(CreateObject(std::forward<Args>(args)...)));
        }
    }

    // 사용 가능한 오브젝트 획득 및 생성
    template<typename... Args>
    T* Acquire(Args&&... args) {
        T* obj = nullptr;

        if (!Pool.empty()) {
            // 사용 가능한 오브젝트가 있으면 재사용
            obj = Pool.back();
            Pool.pop_back();
        }
        else if (ActiveObjects.size() < MaxSize) {
         // 풀에 여유가 있으면 새로 생성
            obj = CreateObject(std::forward<Args>(args)...);
        }

        if (obj) {
            ActiveObjects.push_back(obj);
        }
        return obj;
    }

    // 오브젝트 반납
    void Release(T* obj) {
        auto it = std::find(ActiveObjects.begin(), ActiveObjects.end(), obj);
        if (it != ActiveObjects.end()) {
            ActiveObjects.erase(it);
            Pool.push_back(std::unique_ptr<T>(obj));
        }
    }

    // 모든 오브젝트 정리
    void Clear() {
        Pool.clear();
        ActiveObjects.clear();
    }

    // 현재 풀 상태 조회
    size_t GetActiveCount() const { return ActiveObjects.size(); }
    size_t GetPoolCount() const { return Pool.size(); }
    size_t GetMaxSize() const { return MaxSize; }
    bool IsFull() const { return (Pool.size() + ActiveObjects.size()) >= MaxSize; }
};
