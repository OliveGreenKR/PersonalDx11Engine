#pragma once 
#include <array>
#include <bitset>
#include <memory>
#include <stdexcept>

template <typename T, size_t N>
class TFixedObjectPool {
private:
    // 풀 내 객체를 나타내는 구조체
    struct PoolEntry {
        bool bIsActive = false;              // 활성 상태 플래그
        std::shared_ptr<T> Object;           // 객체 소유권
        PoolEntry* Prev = nullptr;           // 이중 연결 리스트 이전 노드
        PoolEntry* Next = nullptr;           // 이중 연결 리스트 다음 노드

        template <typename... Args>
        PoolEntry(Args&&... args) : Object(std::make_shared<T>(std::forward<Args>(args)...)) {}
    };

    std::array<PoolEntry, N> Pool;          // 고정 크기 객체 풀
    std::bitset<N> InActiveFlags;          // 비활성 객체 추적 비트셋
    PoolEntry* ActiveHead = nullptr;       // 활성 리스트의 맨 앞 (가장 오래된 객체)
    PoolEntry* ActiveTail = nullptr;       // 활성 리스트의 맨 뒤
    size_t ActiveCount = 0;                // 활성 객체 수

private:
    // 이중 연결 리스트에 노드 추가 (활성화 시)
    void AddToActiveList(PoolEntry* entry) {
        entry->Prev = ActiveTail;
        entry->Next = nullptr;
        if (ActiveTail) {
            ActiveTail->Next = entry;
        }
        ActiveTail = entry;
        if (!ActiveHead) {
            ActiveHead = entry;
        }
    }

    // 이중 연결 리스트에서 노드 제거 (비활성화 시)
    void RemoveFromActiveList(PoolEntry* entry) {
        if (entry->Prev) {
            entry->Prev->Next = entry->Next;
        }
        else {
            ActiveHead = entry->Next;
        }
        if (entry->Next) {
            entry->Next->Prev = entry->Prev;
        }
        else {
            ActiveTail = entry->Prev;
        }
        entry->Prev = nullptr;
        entry->Next = nullptr;
    }

    size_t FindFirstSetBit()
    {
        if (!InActiveFlags.any())
        {
            return N; //비정상 인덱스 반환
        }
            
        // 워드 크기 계산 (일반적으로 unsigned long)
        constexpr size_t WORD_SIZE = sizeof(unsigned long) * CHAR_BIT;
        constexpr size_t WORD_COUNT = (N + WORD_SIZE - 1) / WORD_SIZE;

        for (size_t word_idx = 0; word_idx < WORD_COUNT; ++word_idx) {
            // 현재 워드의 비트 범위 계산
            size_t start_bit = word_idx * WORD_SIZE;
            size_t end_bit = std::min(start_bit + WORD_SIZE, N);

            // 현재 워드 추출
            unsigned long word = 0;
            for (size_t i = start_bit; i < end_bit; ++i) {
                if (InActiveFlags[i]) {
                    word |= (1UL << (i - start_bit));
                }
            }

            // 워드가 0이 아니면 최하위 1 비트 위치 찾기
            if (word != 0) {
                // 컴파일러별 내장 함수 사용
#if defined(__GNUC__) || defined(__clang__)
    // GCC/Clang
                return start_bit + __builtin_ctzl(word);
#elif defined(_MSC_VER)
    // MSVC
                unsigned long index;
                _BitScanForward(&index, word);
                return start_bit + index;
#else
    // 일반 구현 - 최하위 1 비트 찾기
                unsigned long temp = word;
                size_t bit_pos = 0;
                while (!(temp & 1)) {
                    temp >>= 1;
                    ++bit_pos;
                }
                return start_bit + bit_pos;
#endif
            }
        }

        throw std::runtime_error("Logic error: no bit found despite bs.any() being true");
    }

#pragma region std::Iterator
public:
    // 반복자 지원
    class Iterator {
    public:
        // STL 반복자 트레잇 정의
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator(PoolEntry* ptr = nullptr) : Current(ptr) {}

        // 역참조: weak_ptr 반환
        std::weak_ptr<T> operator*() const {
            return Current ? std::weak_ptr<T>(Current->Object) : std::weak_ptr<T>();
        }

        // 전위 증가 (다음 요소)
        Iterator& operator++() {
            if (Current) Current = Current->Next;
            return *this;
        }

        // 후위 증가
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        // 전위 감소 (이전 요소)
        Iterator& operator--() {
            if (Current) Current = Current->Prev;
            return *this;
        }

        // 후위 감소
        Iterator operator--(int) {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        // 비교 연산자
        bool operator==(const Iterator& other) const { return Current == other.Current; }
        bool operator!=(const Iterator& other) const { return Current != other.Current; }

    private:
        PoolEntry* Current;
    };

    // begin과 end 제공
    Iterator begin() { return Iterator(ActiveHead); }
    Iterator end() { return Iterator(nullptr); }

    // const 버전
    Iterator begin() const { return Iterator(const_cast<PoolEntry*>(ActiveHead)); }
#pragma endregion
public:
	// 생성자: 풀 초기화
	template <typename... Args>
	TFixedObjectPool(Args&&... args) {
		for (size_t i = 0; i < N; ++i) {
			Pool[i] = PoolEntry(std::forward<Args>(args)...);
		}
		InActiveFlags.set(); // 모든 객체를 비활성으로 초기화
	}

    // 비활성 객체를 찾아 활성화 후 반환
    template <typename... Args>
    std::weak_ptr<T> Acquire(Args&&... args) {
        if (ActiveCount >= N) {
            return std::weak_ptr<T>(); // 풀이 꽉찬 경우 빈 weak_ptr 반환
        }

        // 비활성 객체 찾기
        size_t index = FindFirstSetBit();
        if (index >= N) {
            return std::weak_ptr<T>(); // 비정상 상황 방지
        }

        PoolEntry& entry = Pool[index];
        entry.bIsActive = true;
        InActiveFlags.reset(index);
        ActiveCount++;

        // 활성 리스트에 추가
        AddToActiveList(&entry);

        return std::weak_ptr<T>(entry.Object);
    }
    //꽉찬 경우 가장 오래된 객체 초기화후 반환
    template <typename... Args>
    std::weak_ptr<T> AcquireForcely(Args&&... args) {
        if (ActiveCount < N) {
            // 풀이 꽉 차지 않았다면 Acquire와 동일하게 동작
            return Acquire(std::forward<Args>(args)...);
        }

        // 풀이 꽉 찬 경우: 가장 오래된 활성 객체를 재사용
        if (!ActiveHead) {
            return std::weak_ptr<T>(); // 비정상 상황 방지
        }

        PoolEntry* oldest = ActiveHead;
        RemoveFromActiveList(oldest); // 리스트에서 제거

        AddToActiveList(oldest); // 다시 리스트 끝에 추가 (최신 객체로 갱신)
        return std::weak_ptr<T>(oldest->Object);
    }

    // 객체를 풀에 반납
    void ReturnToPool(std::weak_ptr<T> weakObj) {
        if (auto sharedObj = weakObj.lock()) {
            // 풀 내 객체인지 확인하고 비활성화
            for (size_t i = 0; i < N; ++i) {
                if (Pool[i].Object == sharedObj) {
                    PoolEntry& entry = Pool[i];
                    if (!entry.bIsActive) {
                        return; // 이미 비활성 상태라면 무시
                    }
                    entry.bIsActive = false;
                    InActiveFlags.set(i);
                    ActiveCount--;
                    RemoveFromActiveList(&entry);
                    return;
                }
            }
        }
    }

    // 모든 활성 객체 비활성화
    void ClearAllActives() {
        PoolEntry* current = ActiveHead;
        while (current) {
            PoolEntry* Next = current->Next;
            current->bIsActive = false;
            InActiveFlags.set(current - Pool.data()); // 인덱스 계산
            current->Prev = nullptr;
            current->Next = nullptr;
            current = Next;
        }
        ActiveHead = nullptr;
        ActiveTail = nullptr;
        ActiveCount = 0;
    }

    // 활성 객체 수 반환
    size_t GetActiveCount() const {
        return ActiveCount;
    }

    // 풀 내 총 객체 수 반환 (활성 + 비활성)
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

    // 소멸자: 자원 정리 (필요 시 명시적 호출 가능)
    ~TFixedObjectPool() {
        ClearAllActives();
    }
};