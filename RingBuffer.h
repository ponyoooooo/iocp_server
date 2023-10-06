#pragma once
#include <cstdint>
#include <vector>

#include "ClientInfo.h"

class RingBuffer {
private:
    char* buffer;
    size_t head = 0;
    size_t tail = 0;
    size_t capacity;

public:
    RingBuffer(size_t size) : capacity(size) {
        buffer = new char[size];
    }

    ~RingBuffer() {
        delete[] buffer;
    }

    bool push(const char* data, size_t size)
    {
        if (size > capacity - this->size()) {
            return false; // 버퍼에 충분한 공간이 없음
        }

        if (tail + size <= capacity) {
            memcpy(&buffer[tail], data, size);
            tail += size;
        }
        else {
            size_t part1Size = capacity - tail;
            memcpy(&buffer[tail], data, part1Size);
            size_t part2Size = size - part1Size;
            memcpy(&buffer[0], data + part1Size, part2Size);
            tail = part2Size;
        }

        return true;
    }

    std::vector<char> pop(size_t size) {
        if (size > this->size()) {
            return {}; // Not enough data
        }

        std::vector<char> result(size);
        if (head + size <= capacity) {
            memcpy(result.data(), buffer + head, size);
            head += size;
        }
        else {
            size_t part1Size = capacity - head;
            memcpy(result.data(), buffer + head, part1Size);
            size_t part2Size = size - part1Size;
            memcpy(result.data() + part1Size, buffer, part2Size);
            head = part2Size;
        }

        return result;
    }

    size_t size() const
    {
        if (tail >= head) {
            return tail - head;
        }
        else {
            return capacity + tail - head;
        }
    }

    char* data() const
    {
        return &buffer[head];
    }

    bool isFull() const
    {
        return size() == capacity;
    }

    bool isEmpty() const
    {
        return head == tail;
    }

    void clear()
    {
        head = 0;
        tail = 0;
    }
};

