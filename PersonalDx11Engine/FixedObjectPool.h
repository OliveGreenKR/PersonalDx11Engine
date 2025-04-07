#pragma once
#include <vector>
#include <memory>
#include <stdexcept>
#include <new>

/// <summary>
/// 고정크기의 재사용 객체 최적화 메모리 풀 템플릿
/// 최대 크기 고정
/// </summary>
template<typename T>
class TFixedObjectPool {
private:
    std::vector<std::unique_ptr<T>> Pool;
    std::vector<std::unique_ptr<T>> ActiveObjects;
    size_t MaxSize;

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

    // 사용 가능한 오브젝트 획득 및 생성
    template<typename... Args>
    T* Acquire(Args&&... args) {
        T* obj = nullptr;

        if (!Pool.empty()) {
            // 풀에서 꺼내서 활성 목록으로 이동
            ActiveObjects.push_back(std::move(Pool.back()));
            Pool.pop_back();
            obj = ActiveObjects.back().get();
        }
        else if (ActiveObjects.size() < MaxSize) {
            // 새로 생성해서 활성 목록에 추가
            ActiveObjects.push_back(std::make_unique<T>(std::forward<Args>(args)...));
            obj = ActiveObjects.back().get();
        }

        return obj;
    }

    // 오브젝트 반납
    void ReturnToPool(T* obj) {
        if (!obj) return;

        // 활성 목록에서 해당 객체 찾기
        auto it = std::find_if(ActiveObjects.begin(), ActiveObjects.end(),
                               [obj](const std::unique_ptr<T>& ptr) { return ptr.get() == obj; });

        if (it != ActiveObjects.end()) {
            // 풀로 이동
            Pool.push_back(std::move(*it));
            // 활성 목록에서 제거 (소유권은 이미 이전됨)
            ActiveObjects.erase(it);
        }
    }

    //활성화 객체만 정리
    void ClearActives()
    {
        ActiveObjects.clear();
    }

    // 모든 오브젝트 정리
    void Clear() {
        Pool.clear();
        ActiveObjects.clear();
    }


    // 현재 풀 상태 조회
    const std::vector<std::unique_ptr<T>>& GetActiveObjects() const { return ActiveObjects; }
    size_t GetActiveCount() const { return ActiveObjects.size(); }
    size_t GetPoolCount() const { return Pool.size(); }
    size_t GetMaxSize() const { return MaxSize; }
    bool IsFull() const { return (Pool.size() + ActiveObjects.size()) >= MaxSize; }
};
