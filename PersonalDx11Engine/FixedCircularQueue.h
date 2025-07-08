#pragma once
// TFixedCircularQueue.h
#pragma once
#include <iterator>
#include <type_traits>
#include <cassert>

/// <summary>
/// 고정 크기 순환 큐 자료구조
/// 
/// 특징:
/// - 템플릿 기반 타입 확장 지원
/// - 연속된 메모리 공간 보장 (캐시 친화적)
/// - std::iterator 지원 (순환 순회)
/// - 고정 크기로 동적 할당 없음
/// </summary>
template<typename T, size_t Capacity>
class TFixedCircularQueue
{
    static_assert(Capacity > 0, "TFixedCircularQueue capacity must be greater than 0");

#pragma region Core Data Members

private:
    T Data[Capacity];           // 고정 크기 배열
    size_t Head = 0;           // 가장 오래된 원소 인덱스
    size_t Size = 0;           // 현재 저장된 원소 개수

#pragma endregion

#pragma region Iterator Implementation

public:
    /// <summary>
    /// 순환 순회를 위한 Iterator 클래스
    /// 가장 오래된 원소부터 가장 최근 원소까지 순회
    /// </summary>
    class Iterator
    {
    private:
        const TFixedCircularQueue* Queue;
        size_t ElementsTraversed;
        bool bIsEndIterator;

    public:
        // Iterator traits for std::iterator compatibility
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        // 일반 Iterator 생성자
        Iterator(const TFixedCircularQueue* InQueue, size_t InTraversed = 0)
            : Queue(InQueue), ElementsTraversed(InTraversed), bIsEndIterator(false) {
        }

        // End Iterator 생성자
        static Iterator CreateEndIterator(const TFixedCircularQueue* InQueue)
        {
            Iterator endIter(InQueue, InQueue->Size);
            endIter.bIsEndIterator = true;
            return endIter;
        }

        // 현재 실제 인덱스 계산
        size_t GetCurrentIndex() const
        {
            assert(Queue && !bIsEndIterator && ElementsTraversed < Queue->Size);
            return (Queue->Head + ElementsTraversed) % Capacity;
        }

        // Dereference operators
        reference operator*() const
        {
            assert(Queue && !bIsEndIterator && ElementsTraversed < Queue->Size && "Iterator out of bounds");
            size_t currentIndex = GetCurrentIndex();
            return const_cast<T&>(Queue->Data[currentIndex]);
        }

        pointer operator->() const
        {
            assert(Queue && !bIsEndIterator && ElementsTraversed < Queue->Size && "Iterator out of bounds");
            size_t currentIndex = GetCurrentIndex();
            return const_cast<T*>(&Queue->Data[currentIndex]);
        }

        // Pre-increment (순환 이동)
        Iterator& operator++()
        {
            assert(!bIsEndIterator && ElementsTraversed < Queue->Size && "Iterator increment beyond end");
            ++ElementsTraversed;

            // 마지막 원소를 넘어서면 end iterator가 됨
            if (ElementsTraversed >= Queue->Size)
            {
                bIsEndIterator = true;
            }
            return *this;
        }

        // Post-increment
        Iterator operator++(int)
        {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        // Comparison operators
        bool operator==(const Iterator& Other) const
        {
            if (Queue != Other.Queue) return false;

            // 둘 다 end iterator인 경우
            if (bIsEndIterator && Other.bIsEndIterator) return true;

            // 하나만 end iterator인 경우
            if (bIsEndIterator != Other.bIsEndIterator) return false;

            // 둘 다 일반 iterator인 경우
            return ElementsTraversed == Other.ElementsTraversed;
        }

        bool operator!=(const Iterator& Other) const
        {
            return !(*this == Other);
        }
    };

    using const_iterator = Iterator;

#pragma endregion

#pragma region Container Interface

public:
    /// <summary>
    /// 기본 생성자 - 모든 원소를 기본값으로 초기화
    /// </summary>
    TFixedCircularQueue()
    {
        for (size_t i = 0; i < Capacity; ++i)
        {
            Data[i] = T{};
        }
    }

    /// <summary>
    /// 초기값으로 모든 슬롯을 채우는 생성자
    /// </summary>
    explicit TFixedCircularQueue(const T& InitialValue)
    {
        for (size_t i = 0; i < Capacity; ++i)
        {
            Data[i] = InitialValue;
        }
        Size = Capacity;
    }

    /// <summary>
    /// 새 원소를 뒤에 추가 (용량 초과 시 실패)
    /// </summary>
    /// <param name="Item">추가할 원소</param>
    /// <returns>성공 시 true, 용량 초과 시 false</returns>
    bool PushBack(const T& Item)
    {
        if (Size >= Capacity)
        {
            return false;  // 용량 초과
        }

        size_t TailIndex = (Head + Size) % Capacity;
        Data[TailIndex] = Item;
        ++Size;
        return true;
    }

    /// <summary>
    /// 새 원소를 강제로 추가 (용량 초과 시 가장 오래된 원소 덮어씀)
    /// 동작 예시: [1,1,1,1] + PushForcely(2) → [2,1,1,1] (Head가 1로 이동)
    /// </summary>
    /// <param name="Item">추가할 원소</param>
    void PushForcely(const T& Item)
    {
        if (Size < Capacity)
        {
            // 아직 여유 공간이 있음 - 일반 PushBack과 동일
            size_t TailIndex = (Head + Size) % Capacity;
            Data[TailIndex] = Item;
            ++Size;
        }
        else
        {
            // 용량 초과 - 가장 오래된 위치에 새 값 쓰고 Head 이동
            Data[Head] = Item;
            Head = (Head + 1) % Capacity;
            // Size는 Capacity로 유지
        }
    }

    /// <summary>
    /// 가장 최근 원소 제거 및 반환
    /// </summary>
    /// <returns>제거된 원소 (큐가 비어있으면 기본값)</returns>
    T Pop()
    {
        if (Size == 0)
        {
            return T{};  // 빈 큐에서는 기본값 반환
        }

        --Size;
        size_t LastIndex = (Head + Size) % Capacity;
        T result = Data[LastIndex];
        Data[LastIndex] = T{};  // 명시적 초기화
        return result;
    }

    /// <summary>
    /// 인덱스 기반 원소 접근 (0 = 가장 오래된 원소)
    /// </summary>
    T& operator[](size_t Index)
    {
        assert(Index < Size && "Index out of bounds");
        size_t ActualIndex = (Head + Index) % Capacity;
        return Data[ActualIndex];
    }

    const T& operator[](size_t Index) const
    {
        assert(Index < Size && "Index out of bounds");
        size_t ActualIndex = (Head + Index) % Capacity;
        return Data[ActualIndex];
    }

#pragma endregion

#pragma region Iterator Support

public:
    /// <summary>
    /// 가장 오래된 원소를 가리키는 iterator 반환
    /// </summary>
    Iterator begin() const
    {
        return Iterator(this, 0);
    }

    /// <summary>
    /// 순회 종료 지점 iterator 반환
    /// </summary>
    Iterator end() const
    {
        return Iterator::CreateEndIterator(this);
    }

    /// <summary>
    /// const iterator 지원
    /// </summary>
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

#pragma endregion

#pragma region Capacity and State Queries

public:
    /// <summary>
    /// 현재 저장된 원소 개수
    /// </summary>
    size_t GetSize() const { return Size; }

    /// <summary>
    /// 최대 저장 가능 원소 개수
    /// </summary>
    constexpr size_t GetCapacity() const { return Capacity; }

    /// <summary>
    /// 큐가 비어있는지 확인
    /// </summary>
    bool IsEmpty() const { return Size == 0; }

    /// <summary>
    /// 큐가 가득 찼는지 확인
    /// </summary>
    bool IsFull() const { return Size == Capacity; }

    /// <summary>
    /// 가장 최근 원소 참조 (읽기 전용)
    /// </summary>
    const T& Front() const
    {
        assert(Size > 0 && "Cannot get front from empty queue");
        size_t LatestIndex = (Head + Size - 1) % Capacity;
        return Data[LatestIndex];
    }

#pragma endregion

#pragma region Utility Functions

public:
    /// <summary>
    /// 모든 원소를 기본값으로 초기화 및 크기 재설정
    /// </summary>
    void Clear()
    {
        for (size_t i = 0; i < Capacity; ++i)
        {
            Data[i] = T{};
        }
        Head = 0;
        Size = 0;
    }

    /// <summary>
    /// 특정 값으로 모든 슬롯 채우기
    /// </summary>
    void Fill(const T& Value)
    {
        for (size_t i = 0; i < Capacity; ++i)
        {
            Data[i] = Value;
        }
        Head = 0;
        Size = Capacity;
    }

#pragma endregion
};