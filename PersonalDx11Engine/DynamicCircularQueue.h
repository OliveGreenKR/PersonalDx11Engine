#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename T>
class TCircularQueue {
public:
    // ... (기존 typedefs 및 private 멤버 변수는 동일) ...
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

private:
    std::vector<T> _buffer;
    size_type _head;
    size_type _tail;
    size_type _size;

    void Reallocate(size_type new_capacity) {
        if (new_capacity < _size) {
            new_capacity = _size;
        }
        if (new_capacity == _buffer.capacity()) {
            return;
        }

        std::vector<T> new_buffer(new_capacity);
        for (size_type i = 0; i < _size; ++i) {
            new_buffer[i] = std::move(_buffer[(_head + i) % _buffer.capacity()]);
        }
        _buffer = std::move(new_buffer);
        _head = 0;
        _tail = _size;
    }

public:
    explicit TCircularQueue(size_type initial_capacity = 8) :
        _buffer(initial_capacity == 0 ? 1 : initial_capacity),
        _head(0),
        _tail(0),
        _size(0) {
    }

    TCircularQueue(const TCircularQueue& other) :
        _buffer(other._size),
        _head(0),
        _tail(other._size),
        _size(other._size) {
        for (size_type i = 0; i < _size; ++i) {
            _buffer[i] = other._buffer[(other._head + i) % other._buffer.capacity()];
        }
    }

    TCircularQueue(TCircularQueue&& other) noexcept :
        _buffer(std::move(other._buffer)),
        _head(other._head),
        _tail(other._tail),
        _size(other._size) {
        other._head = 0;
        other._tail = 0;
        other._size = 0;
    }

    TCircularQueue& operator=(const TCircularQueue& other) {
        if (this != &other) {
            TCircularQueue temp(other);
            *this = std::move(temp);
        }
        return *this;
    }

    TCircularQueue& operator=(TCircularQueue&& other) noexcept {
        if (this != &other) {
            _buffer = std::move(other._buffer);
            _head = other._head;
            _tail = other._tail;
            _size = other._size;

            other._head = 0;
            other._tail = 0;
            other._size = 0;
        }
        return *this;
    }

    reference Front() {
        if (Empty()) {
            throw std::out_of_range("front() called on empty queue");
        }
        return _buffer[_head];
    }

    const_reference Front() const {
        if (Empty()) {
            throw std::out_of_range("front() called on empty queue");
        }
        return _buffer[_head];
    }

    reference Back() {
        if (Empty()) {
            throw std::out_of_range("back() called on empty queue");
        }
        return _buffer[(_tail == 0 ? _buffer.capacity() : _tail) - 1];
    }

    const_reference Back() const {
        if (Empty()) {
            throw std::out_of_range("back() called on empty queue");
        }
        return _buffer[(_tail == 0 ? _buffer.capacity() : _tail) - 1];
    }

    void Push(const value_type& value) {
        if (_size == _buffer.capacity()) {
            Reallocate(_buffer.capacity() == 0 ? 8 : _buffer.capacity() * 2);
        }
        _buffer[_tail] = value;
        _tail = (_tail + 1) % _buffer.capacity();
        _size++;
    }

    void Push(value_type&& value) {
        if (_size == _buffer.capacity()) {
            Reallocate(_buffer.capacity() == 0 ? 8 : _buffer.capacity() * 2);
        }
        _buffer[_tail] = std::move(value);
        _tail = (_tail + 1) % _buffer.capacity();
        _size++;
    }

    template <typename... Args>
    void Emplace(Args&&... args) {
        if (_size == _buffer.capacity()) {
            Reallocate(_buffer.capacity() == 0 ? 8 : _buffer.capacity() * 2);
        }
        _buffer[_tail] = T(std::forward<Args>(args)...);
        _tail = (_tail + 1) % _buffer.capacity();
        _size++;
    }

    void Pop() {
        if (Empty()) {
            throw std::out_of_range("pop() called on empty queue");
        }
        _head = (_head + 1) % _buffer.capacity();
        _size--;
    }

    void Clear() noexcept {
        _head = 0;
        _tail = 0;
        _size = 0;
    }

    bool Empty() const noexcept {
        return _size == 0;
    }

    size_type Size() const noexcept {
        return _size;
    }

    size_type Capacity() const noexcept {
        return _buffer.capacity();
    }

    void Reserve(size_type new_capacity) {
        if (new_capacity < _size) {
            new_capacity = _size;
        }
        if (new_capacity > _buffer.capacity()) {
            Reallocate(new_capacity);
        }
    }

#pragma region Iterator
    class iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;

        private:
            std::vector<T>* _vec_ptr;
            size_type _vec_capacity;
            size_type _current_idx;
            size_type _elements_visited; // 방문한 요소의 개수
            size_type _queue_active_size; // 이터레이터가 순회해야 할 실제 큐의 크기 (고정)

        public:
            // begin() 또는 end()에서 호출될 생성자
            iterator(std::vector<T>* vec_ptr, size_type initial_idx, size_type vec_cap, size_type queue_active_size) :
                _vec_ptr(vec_ptr),
                _vec_capacity(vec_cap),
                _current_idx(initial_idx),
                _elements_visited(0),
                _queue_active_size(queue_active_size) {
            }

            // end() 이터레이터의 기본 생성자 (Nullptr 대신 큐의 tail 인덱스를 가리키도록 변경)
            // Note: C++ standard library end iterators often don't hold a pointer to the container.
            // For circular queues, it's often represented by the _tail index and _size == queue_active_size.
            // We'll adjust the comparison operators and begin/end methods to reflect this.
            iterator() :
                _vec_ptr(nullptr), // Sentinel value for end iterator when queue is empty or iteration is complete
                _vec_capacity(0),
                _current_idx(0),
                _elements_visited(0),
                _queue_active_size(0) {
            }

            // Dereferencing operators
            reference operator*() const {
                if (_vec_ptr == nullptr || _elements_visited >= _queue_active_size) {
                    throw std::out_of_range("Dereferencing invalid iterator: beyond end or default constructed");
                }
                return (*_vec_ptr)[_current_idx];
            }

            pointer operator->() const {
                if (_vec_ptr == nullptr || _elements_visited >= _queue_active_size) {
                    throw std::out_of_range("Dereferencing invalid iterator: beyond end or default constructed");
                }
                return &(*_vec_ptr)[_current_idx];
            }

            // Prefix increment
            iterator& operator++() {
                if (_elements_visited < _queue_active_size) {
                    _current_idx = (_current_idx + 1) % _vec_capacity;
                    _elements_visited++;
                }
                // When iteration is complete, simulate the end state for comparison
                if (_elements_visited == _queue_active_size) {
                    _vec_ptr = nullptr; // Sentinel value to indicate end
                }
                return *this;
            }

            // Postfix increment
            iterator operator++(int) {
                iterator temp = *this;
                ++(*this);
                return temp;
            }

            // Comparison operators
            bool operator==(const iterator& other) const {
                // Special case: both are "end" iterators (or default constructed)
                if (_vec_ptr == nullptr && other._vec_ptr == nullptr) {
                    return true;
                }
                // If one is end and the other isn't, they are not equal
                if (_vec_ptr == nullptr || other._vec_ptr == nullptr) {
                    return false;
                }
                // For valid iterators, compare based on container, current index, and elements visited
                // For end iterators, they represent the state where _elements_visited == _queue_active_size
                return _vec_ptr == other._vec_ptr &&
                    _current_idx == other._current_idx &&
                    _elements_visited == other._elements_visited &&
                    _queue_active_size == other._queue_active_size;
            }

            bool operator!=(const iterator& other) const {
                return !(*this == other);
            }
    };

    // const_iterator는 iterator와 거의 동일하며, 포인터와 참조 타입만 const로 변경
    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

    private:
        const std::vector<T>* _vec_ptr;
        size_type _vec_capacity;
        size_type _current_idx;
        size_type _elements_visited;
        size_type _queue_active_size;

    public:
        const_iterator(const std::vector<T>* vec_ptr, size_type initial_idx, size_type vec_cap, size_type queue_active_size) :
            _vec_ptr(vec_ptr),
            _vec_capacity(vec_cap),
            _current_idx(initial_idx),
            _elements_visited(0),
            _queue_active_size(queue_active_size) {
        }

        const_iterator() :
            _vec_ptr(nullptr),
            _vec_capacity(0),
            _current_idx(0),
            _elements_visited(0),
            _queue_active_size(0) {
        }

        const_reference operator*() const {
            if (_vec_ptr == nullptr || _elements_visited >= _queue_active_size) {
                throw std::out_of_range("Dereferencing invalid const_iterator: beyond end or default constructed");
            }
            return (*_vec_ptr)[_current_idx];
        }

        const_pointer operator->() const {
            if (_vec_ptr == nullptr || _elements_visited >= _queue_active_size) {
                throw std::out_of_range("Dereferencing invalid const_iterator: beyond end or default constructed");
            }
            return &(*_vec_ptr)[_current_idx];
        }

        const_iterator& operator++() {
            if (_elements_visited < _queue_active_size) {
                _current_idx = (_current_idx + 1) % _vec_capacity;
                _elements_visited++;
            }
            if (_elements_visited == _queue_active_size) {
                _vec_ptr = nullptr;
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const const_iterator& other) const {
            if (_vec_ptr == nullptr && other._vec_ptr == nullptr) {
                return true;
            }
            if (_vec_ptr == nullptr || other._vec_ptr == nullptr) {
                return false;
            }
            return _vec_ptr == other._vec_ptr &&
                _current_idx == other._current_idx &&
                _elements_visited == other._elements_visited &&
                _queue_active_size == other._queue_active_size;
        }

        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }
    };
#pragma endregion

    // begin()과 end() 메서드도 큐의 상태를 정확히 반영하도록 수정
    iterator begin() {
        // 큐가 비어있으면 end()를 반환
        if (_size == 0) {
            return end();
        }
        return iterator(&_buffer, _head, _buffer.capacity(), _size);
    }

    const_iterator begin() const {
        if (_size == 0) {
            return end();
        }
        return const_iterator(&_buffer, _head, _buffer.capacity(), _size);
    }

    const_iterator cbegin() const {
        return const_iterator(&_buffer, _head, _buffer.capacity(), _size);
    }

    iterator end() {
        // end 이터레이터는 _size 만큼 방문한 후의 상태를 나타내므로
        // _elements_visited == _queue_active_size 이고 _vec_ptr = nullptr 인 상태
        return iterator();
    }

    const_iterator end() const {
        return const_iterator();
    }

    const_iterator cend() const {
        return const_iterator();
    }
};