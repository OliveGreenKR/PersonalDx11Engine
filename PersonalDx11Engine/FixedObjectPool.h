#include <memory>
#include <vector>

/// <summary>
/// 고정크기의 재사용 객체 최적화 메모리 풀 템플릿
/// 단일 배열 + 비활성 인덱스 벡터로 캐시 효율성 개선
/// </summary>
template<typename T>
class TFixedObjectPool {
private:
    struct ObjectEntry {
        std::shared_ptr<T> object;
        bool isActive;
        ObjectEntry(std::shared_ptr<T> obj) : object(std::move(obj)), isActive(false) {}
    };

    std::vector<ObjectEntry> objects;     // 모든 객체와 상태 저장
    std::vector<size_t> inactiveIndices;  // 비활성 객체 인덱스 스택
    size_t MaxSize;

    template<typename... Args>
    std::shared_ptr<T> CreateObject(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

public:
    explicit TFixedObjectPool(size_t maxSize = 100) : MaxSize(maxSize) {
        objects.reserve(maxSize);
        inactiveIndices.reserve(maxSize); // 캐시 효율성을 위해 예약
    }

    ~TFixedObjectPool() = default;

    template<typename... Args>
    std::weak_ptr<T> Acquire(Args&&... args) {
        if (!inactiveIndices.empty()) {
            size_t index = inactiveIndices.back();
            inactiveIndices.pop_back();
            auto& entry = objects[index];
            entry.isActive = true;
            entry.object->~T();
            new (entry.object.get()) T(std::forward<Args>(args)...);
            return std::weak_ptr<T>(entry.object);
        }
        else if (objects.size() < MaxSize) {
            auto obj = CreateObject(std::forward<Args>(args)...);
            objects.emplace_back(obj);
            objects.back().isActive = true;
            return std::weak_ptr<T>(obj);
        }
        return std::weak_ptr<T>();
    }

    void ReturnToPool(std::weak_ptr<T> weakObj) {
        auto obj = weakObj.lock();
        if (!obj) return;

        for (size_t i = 0; i < objects.size(); ++i) {
            if (objects[i].object == obj && objects[i].isActive) {
                objects[i].isActive = false;
                inactiveIndices.push_back(i);
                break;
            }
        }
    }

    void ClearActives() {
        for (size_t i = 0; i < objects.size(); ++i) {
            if (objects[i].isActive) {
                objects[i].isActive = false;
                inactiveIndices.push_back(i);
            }
        }
    }

    std::vector<std::weak_ptr<T>> GetActiveObjects() const {
        std::vector<std::weak_ptr<T>> weakObjects;
        weakObjects.reserve(objects.size());
        for (const auto& entry : objects) {
            if (entry.isActive) {
                weakObjects.emplace_back(entry.object);
            }
        }
        return weakObjects;
    }

    size_t GetActiveCount() const { return objects.size() - inactiveIndices.size(); }
    size_t GetPoolCount() const { return inactiveIndices.size(); }
    size_t GetMaxSize() const { return MaxSize; }
    bool IsFull() const { return objects.size() >= MaxSize; }
};