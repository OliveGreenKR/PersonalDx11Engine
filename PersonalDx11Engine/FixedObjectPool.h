#pragma once 
#include <array>
#include <bitset>
#include <memory>
#include <stdexcept>
#include <climits>
#include <utility>

/// <summary>
/// 고정크기 객체의 고정 크기 Pool. 
/// 객체의 'Pool 외부 공개 여부'외의 상태에는 관여하지 않음.
/// </summary>
template <typename T, size_t N>
class TFixedObjectPool {
private:
    static constexpr size_t INVALID_INDEX = N;

public:
    // 생성자
    template <typename... Args>
    TFixedObjectPool(Args&&... args) {
        for (size_t i = 0; i < N; ++i) {
            Objects[i] = std::move( T(std::forward<Args>(args)...)); // 직접 생성 후 전달
        }
        for (size_t i = 0; i < N; ++i) {
            PrevIndices[i] = INVALID_INDEX;
            NextIndices[i] = INVALID_INDEX;
        }
    }
public:
#pragma region WeakedObject
    // 외부 인터페이스를 위한 객체 래퍼 클래스 - 약한 참조
    class WeakedObject {
    private:
        TFixedObjectPool* Pool;                 // 소유자 풀
        size_t Index;                           // 객체 인덱스
        mutable bool Released;                  // 해제 여부 플래그

    public:
        // 생성자
        WeakedObject(TFixedObjectPool* pool, size_t index)
            : Pool(pool), Index(index), Released(false) {
        }

        // 무효한 객체 생성
        WeakedObject()
            : Pool(nullptr), Index(INVALID_INDEX), Released(true) {
        }


        //객체 반환 안함(약한 참조용 객체)
        ~WeakedObject() 
        {
            Pool = nullptr;
            Index = INVALID_INDEX;
        }

        // 객체를 풀에 반환(비활성화)
        void Release() {
            if (!Released && Pool && Index != INVALID_INDEX) {
                Pool->ReturnToPoolInternal(Index);
                Released = true;
            }
        }

        // 객체 포인터 얻기
        T* Get() const {
            return IsValid() ? &(Pool->Objects[Index]) : nullptr;
        }

        // 유효성 확인
        bool IsValid() const {
            return !(Released || !Pool || Index == INVALID_INDEX || !Pool->IsIndexActive(Index));
        }

        // 인덱스 접근 
        size_t GetIndex() const { return Index; }

    private:

        // 복사 방지
        //WeakedObject(const WeakedObject&) = delete;
        //WeakedObject& operator=(const WeakedObject&) = delete;

        friend class TFixedObjectPool;
    };
#pragma endregion
#pragma region Iterator
    // 이터레이터 정의
    class Iterator {
    private:
        TFixedObjectPool* Pool;
        size_t CurrentIndex;

    public:
        // STL 호환 트레이트
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = WeakedObject;
        using difference_type = std::ptrdiff_t;
        using pointer = void;  // WeakedObject는 참조 semantic
        using reference = WeakedObject;  // 참조 반환

        // 생성자
        Iterator(TFixedObjectPool* pool, size_t index)
            : Pool(pool), CurrentIndex(index) {
        }

        // 역참조 연산자: WeakedObject 반환
        WeakedObject operator*() const {
            if (!Pool || CurrentIndex == INVALID_INDEX) {
                return WeakedObject();  // 무효한 객체 반환
            }
            return Pool->CreateScopedObject(CurrentIndex);
        }

        // 전위 증가
        Iterator& operator++() {
            if (Pool && CurrentIndex != INVALID_INDEX) {
                CurrentIndex = Pool->NextIndices[CurrentIndex];
            }
            return *this;
        }

        // 후위 증가
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        // 전위 감소
        Iterator& operator--() {
            if (Pool && CurrentIndex != INVALID_INDEX) {
                CurrentIndex = Pool->PrevIndices[CurrentIndex];
            }
            return *this;
        }

        // 후위 감소
        Iterator operator--(int) {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        // 비교 연산자
        bool operator==(const Iterator& other) const {
            return Pool == other.Pool && CurrentIndex == other.CurrentIndex;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }
    };
#pragma endregion
private:
    // SoA(Structure of Arrays) 패턴 적용
    std::array<T, N> Objects;                    // 객체 배열
    std::bitset<N> ActiveFlags;                  // 활성 상태 플래그
    std::array<size_t, N> PrevIndices;           // 이전 노드 인덱스
    std::array<size_t, N> NextIndices;           // 다음 노드 인덱스

    size_t ActiveHead = INVALID_INDEX;           // 활성 리스트 시작 인덱스
    size_t ActiveTail = INVALID_INDEX;           // 활성 리스트 끝 인덱스
    size_t ActiveCount = 0;                      // 활성 객체 수

private:
    // 활성 리스트에 인덱스 추가
    void AddToActiveList(size_t index) {
        PrevIndices[index] = ActiveTail;
        NextIndices[index] = INVALID_INDEX;

        if (ActiveTail != INVALID_INDEX) {
            NextIndices[ActiveTail] = index;
        }

        ActiveTail = index;

        if (ActiveHead == INVALID_INDEX) {
            ActiveHead = index;
        }
    }

    // 활성 리스트에서 인덱스 제거
    void RemoveFromActiveList(size_t index) {
        size_t prev = PrevIndices[index];
        size_t next = NextIndices[index];

        if (prev != INVALID_INDEX) {
            NextIndices[prev] = next;
        }
        else {
            ActiveHead = next;
        }

        if (next != INVALID_INDEX) {
            PrevIndices[next] = prev;
        }
        else {
            ActiveTail = prev;
        }

        PrevIndices[index] = INVALID_INDEX;
        NextIndices[index] = INVALID_INDEX;
    }

    // 비활성 객체 인덱스 찾기
    size_t FindInactiveIndex() const {
        if (ActiveCount >= N) {
            return INVALID_INDEX;
        }

        for (size_t i = 0; i < N; ++i) {
            if (!ActiveFlags[i]) {
                return i;
            }
        }

        return INVALID_INDEX;
    }

    // 인덱스가 활성 상태인지 확인
    bool IsIndexActive(size_t index) const {
        return index < N && ActiveFlags[index];
    }

    // WeakedObject 생성 헬퍼 메소드
    WeakedObject CreateScopedObject(size_t index) {
        return WeakedObject(this, index);
    }

    // 내부 반환 메소드 - WeakedObject에서 호출
    void ReturnToPoolInternal(size_t index) {
        if (index >= N || !ActiveFlags[index]) {
            return; // 이미 비활성 상태
        }

        // 상태 업데이트
        ActiveFlags.reset(index);
        ActiveCount--;
        RemoveFromActiveList(index);
    }

public:
    // 비활성 객체를 찾아 활성화 후 WeakedObject 반환
    WeakedObject Acquire() {
        if (ActiveCount >= N) {
            return WeakedObject(); // 유효하지 않은 객체
        }

        // 비활성 객체 찾기
        size_t index = FindInactiveIndex();
        if (index == INVALID_INDEX) {
            return WeakedObject(); // 유효하지 않은 객체
        }

        ActiveFlags.set(index);
        ActiveCount++;

        // 활성 리스트에 추가
        AddToActiveList(index);

        return CreateScopedObject(index);
    }

    // 꽉찬 경우 가장 오래된 객체 초기화 후 WeakedObject 반환
    WeakedObject AcquireForcely() {
        if (ActiveCount < N) {
            return Acquire();
        }

        if (ActiveHead == INVALID_INDEX) {
            return WeakedObject();
        }

        size_t oldestIndex = ActiveHead;

        // 리스트 재정렬
        RemoveFromActiveList(oldestIndex);
        AddToActiveList(oldestIndex);

        return CreateScopedObject(oldestIndex);
    }

    // 이전과의 호환성을 위한 ReturnToPool 메소드
    // WeakedObject는 자동으로 반환되므로 이 메소드는 명시적 Release 호출 용도
    void ReturnToPool(WeakedObject& scopedObj) {
        scopedObj.Release();
    }

    // 모든 활성 객체 비활성화
    void ClearAllActives() {
        size_t current = ActiveHead;
        while (current != INVALID_INDEX) {
            size_t next = NextIndices[current];

            ActiveFlags.reset(current);
            PrevIndices[current] = INVALID_INDEX;
            NextIndices[current] = INVALID_INDEX;

            current = next;
        }

        ActiveHead = INVALID_INDEX;
        ActiveTail = INVALID_INDEX;
        ActiveCount = 0;
    }

    // 활성 객체 반복용 반복자
    Iterator begin() { return Iterator(this, ActiveHead); }
    Iterator end() { return Iterator(this, INVALID_INDEX); }

    // const 버전
    Iterator begin() const { return Iterator(const_cast<TFixedObjectPool*>(this), ActiveHead); }
    Iterator end() const { return Iterator(const_cast<TFixedObjectPool*>(this), INVALID_INDEX); }

    // 활성 객체 수 반환
    size_t GetActiveCount() const {
        return ActiveCount;
    }

    // 풀 내 총 객체 수 반환
    size_t GetPoolCount() const {
        return N;
    }

    // 풀의 최대 크기 반환
    size_t GetMaxSize() const {
        return N;
    }

    // 풀이 가득 찼는지 확인
    bool IsFull() const {
        return ActiveCount >= N;
    }

    // 소멸자: 자원 정리
    ~TFixedObjectPool() = default;
};