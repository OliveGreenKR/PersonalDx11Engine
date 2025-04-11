#pragma once 
#include <array>
#include <bitset>
#include <memory>
#include <stdexcept>
#include <climits>

template <typename T, size_t N>
class TFixedObjectPool {
private:
    static constexpr size_t INVALID_INDEX = N;

public:
#pragma region ScopedObject
    // 외부 인터페이스를 위한 객체 래퍼 클래스
    class ScopedObject {
    private:
        TFixedObjectPool* Pool;      // 소유자 풀
        size_t Index;                // 객체 인덱스
        bool Released;               // 해제 여부 플래그

    public:
        // 생성자
        ScopedObject(TFixedObjectPool* pool, size_t index)
            : Pool(pool), Index(index), Released(false) {
        }

        // 무효한 객체 생성
        ScopedObject()
            : Pool(nullptr), Index(INVALID_INDEX), Released(true) {
        }

        // 이동 생성자
        ScopedObject(ScopedObject&& other) noexcept
            : Pool(other.Pool), Index(other.Index), Released(other.Released) {
            other.Released = true;  // 소유권 이전
        }

        // 이동 대입 연산자
        ScopedObject& operator=(ScopedObject&& other) noexcept {
            if (this != &other) {
                // 기존 객체 반환
                Release();

                // 새 객체 소유권 가져오기
                Pool = other.Pool;
                Index = other.Index;
                Released = other.Released;
                other.Released = true;  // 소유권 이전
            }
            return *this;
        }

        // 소멸자: RAII 패턴으로 자동 반환
        ~ScopedObject() {
            Release();
        }

        // 명시적 반환
        void Release() {
            if (!Released && Pool && Index != INVALID_INDEX) {
                Pool->ReturnToPoolInternal(Index);
                Released = true;
            }
        }


        // 객체 포인터 얻기
        T* Get() const {
            return IsValidAccess() ? &(Pool->Objects[Index]) : nullptr;
        }

        // 유효성 확인
        bool IsValid() const {
            return !Released && Pool && Index != INVALID_INDEX &&
                Pool->IsIndexActive(Index);
        }

        // 인덱스 접근 
        size_t GetIndex() const { return Index; }

    private:
        // 객체 접근 유효성 검사
        bool IsValidAccess() const {
            return !(Released || !Pool || Index == INVALID_INDEX || !Pool->IsIndexActive(Index));
        }

        // 복사 방지
        ScopedObject(const ScopedObject&) = delete;
        ScopedObject& operator=(const ScopedObject&) = delete;

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
        using value_type = ScopedObject;
        using difference_type = std::ptrdiff_t;
        using pointer = void;  // ScopedObject는 참조 semantic
        using reference = ScopedObject;  // 참조 반환

        // 생성자
        Iterator(TFixedObjectPool* pool, size_t index)
            : Pool(pool), CurrentIndex(index) {
        }

        // 역참조 연산자: ScopedObject 반환
        ScopedObject operator*() const {
            if (!Pool || CurrentIndex == INVALID_INDEX) {
                return ScopedObject();  // 무효한 객체 반환
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

    // ScopedObject 생성 헬퍼 메소드
    ScopedObject CreateScopedObject(size_t index) {
        return ScopedObject(this, index);
    }

    // 내부 반환 메소드 - ScopedObject에서 호출
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
    // 생성자: 풀 초기화
    TFixedObjectPool() {
        for (size_t i = 0; i < N; ++i) {
            PrevIndices[i] = INVALID_INDEX;
            NextIndices[i] = INVALID_INDEX;
        }
    }

    // 비활성 객체를 찾아 활성화 후 ScopedObject 반환
    template <typename... Args>
    ScopedObject Acquire(Args&&... args) {
        if (ActiveCount >= N) {
            return ScopedObject(); // 유효하지 않은 객체
        }

        // 비활성 객체 찾기
        size_t index = FindInactiveIndex();
        if (index == INVALID_INDEX) {
            return ScopedObject(); // 유효하지 않은 객체
        }

        ActiveFlags.set(index);
        ActiveCount++;

        // 활성 리스트에 추가
        AddToActiveList(index);

        return CreateScopedObject(index);
    }

    // 꽉찬 경우 가장 오래된 객체 초기화 후 ScopedObject 반환
    template <typename... Args>
    ScopedObject AcquireForcely(Args&&... args) {
        if (ActiveCount < N) {
            return Acquire(std::forward<Args>(args)...);
        }

        if (ActiveHead == INVALID_INDEX) {
            return ScopedObject();
        }

        size_t oldestIndex = ActiveHead;

        // 리스트 재정렬
        RemoveFromActiveList(oldestIndex);
        AddToActiveList(oldestIndex);

        return CreateScopedObject(oldestIndex);
    }

    // 이전과의 호환성을 위한 ReturnToPool 메소드
    // ScopedObject는 자동으로 반환되므로 이 메소드는 명시적 Release 호출 용도
    void ReturnToPool(ScopedObject& scopedObj) {
        scopedObj.Release();
    }

    // 모든 활성 객체 비활성화
    void ClearAllActives() {
        size_t current = ActiveHead;
        while (current != INVALID_INDEX) {
            size_t next = NextIndices[current];

            // 소멸자 호출
            Objects[current].~T();

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
    ~TFixedObjectPool() {
        ClearAllActives();
    }
};