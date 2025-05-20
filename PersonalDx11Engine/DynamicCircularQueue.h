#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename T>
class TCircularQueue {
public:
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
        size_type _elements_passed;
        size_type _queue_total_size;

    public:
        iterator(std::vector<T>* vec_ptr, size_type initial_idx, size_type vec_cap, size_type queue_size) :
            _vec_ptr(vec_ptr),
            _vec_capacity(vec_cap),
            _current_idx(initial_idx),
            _elements_passed(0),
            _queue_total_size(queue_size) {
        }

        iterator() :
            _vec_ptr(nullptr),
            _vec_capacity(0),
            _current_idx(0),
            _elements_passed(0),
            _queue_total_size(0) {
        }

        reference operator*() const {
            if (_vec_ptr == nullptr || _elements_passed >= _queue_total_size) {
                throw std::out_of_range("Dereferencing invalid iterator");
            }
            return (*_vec_ptr)[_current_idx];
        }

        pointer operator->() const {
            if (_vec_ptr == nullptr || _elements_passed >= _queue_total_size) {
                throw std::out_of_range("Dereferencing invalid iterator");
            }
            return &(*_vec_ptr)[_current_idx];
        }

        iterator& operator++() {
            if (_elements_passed < _queue_total_size) {
                _current_idx = (_current_idx + 1) % _vec_capacity;
                _elements_passed++;
            }
            if (_elements_passed == _queue_total_size) {
                _vec_ptr = nullptr;
            }
            return *this;
        }

        iterator operator++(int) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const iterator& other) const {
            if (_vec_ptr == nullptr && other._vec_ptr == nullptr) {
                return true;
            }
            return _vec_ptr == other._vec_ptr &&
                _current_idx == other._current_idx &&
                _elements_passed == other._elements_passed &&
                _queue_total_size == other._queue_total_size;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

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
        size_type _elements_passed;
        size_type _queue_total_size;

    public:
        const_iterator(const std::vector<T>* vec_ptr, size_type initial_idx, size_type vec_cap, size_type queue_size) :
            _vec_ptr(vec_ptr),
            _vec_capacity(vec_cap),
            _current_idx(initial_idx),
            _elements_passed(0),
            _queue_total_size(queue_size) {
        }

        const_iterator() :
            _vec_ptr(nullptr),
            _vec_capacity(0),
            _current_idx(0),
            _elements_passed(0),
            _queue_total_size(0) {
        }

        const_reference operator*() const {
            if (_vec_ptr == nullptr || _elements_passed >= _queue_total_size) {
                throw std::out_of_range("Dereferencing invalid const_iterator");
            }
            return (*_vec_ptr)[_current_idx];
        }

        const_pointer operator->() const {
            if (_vec_ptr == nullptr || _elements_passed >= _queue_total_size) {
                throw std::out_of_range("Dereferencing invalid const_iterator");
            }
            return &(*_vec_ptr)[_current_idx];
        }

        const_iterator& operator++() {
            if (_elements_passed < _queue_total_size) {
                _current_idx = (_current_idx + 1) % _vec_capacity;
                _elements_passed++;
            }
            if (_elements_passed == _queue_total_size) {
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
            return _vec_ptr == other._vec_ptr &&
                _current_idx == other._current_idx &&
                _elements_passed == other._elements_passed &&
                _queue_total_size == other._queue_total_size;
        }

        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }
    };
#pragma endregion
    iterator begin() {
        return iterator(&_buffer, _head, _buffer.capacity(), _size);
    }

    const_iterator begin() const {
        return const_iterator(&_buffer, _head, _buffer.capacity(), _size);
    }

    const_iterator cbegin() const {
        return const_iterator(&_buffer, _head, _buffer.capacity(), _size);
    }

    iterator end() {
        return iterator();
    }

    const_iterator end() const {
        return const_iterator();
    }

    const_iterator cend() const {
        return const_iterator();
    }
};