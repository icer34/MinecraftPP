#pragma once

#include <array>
#include <cstdlib>

template <typename T, size_t N> class RingBuffer
{
public:
    void push(T value)
    {
        _data[_head] = value;
        _head = (_head + 1) % N;
        _size = std::min(_size + 1, N);
    }

    size_t size() const { return _size; }
    size_t offset() const { return _size < N ? 0 : _head; } // index of the oldest value
    const T *data() const { return _data.data(); }

private:
    std::array<T, N> _data{};
    size_t _head = 0;
    size_t _size = 0;
};