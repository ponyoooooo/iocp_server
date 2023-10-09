#pragma once

#include <vector>
#include <algorithm> // for std::copy

class RingBuffer {
private:
    std::vector<char> m_Buffer;
    std::size_t m_StartIdx;
    std::size_t m_EndIdx;
    bool m_IsFull;

public:
    explicit RingBuffer(std::size_t size)
        : m_Buffer(size), m_StartIdx(0), m_EndIdx(0), m_IsFull(false) {}

    bool IsFull() const {
        return m_IsFull;
    }

    bool IsEmpty() const {
        return (!m_IsFull && (m_StartIdx == m_EndIdx));
    }

    std::size_t Capacity() const {
        return m_Buffer.size();
    }

    std::size_t Size() const {
        if (m_IsFull) {
            return m_Buffer.size();
        }
        if (m_EndIdx >= m_StartIdx) {
            return m_EndIdx - m_StartIdx;
        }
        return m_Buffer.size() + m_EndIdx - m_StartIdx;
    }

    bool Push(const char* data, std::size_t dataSize) {
        if (dataSize > m_Buffer.size() - Size()) {
            // 필요한 공간이 부족하면 버퍼 크기를 조정
            std::size_t newSize = m_Buffer.size() + dataSize - Size();
            std::vector<char> newBuffer(newSize);

            std::size_t oldSize = Size();
            if (!IsEmpty()) {
                if (m_StartIdx < m_EndIdx) {
                    std::copy(m_Buffer.begin() + m_StartIdx, m_Buffer.begin() + m_EndIdx, newBuffer.begin());
                }
                else {
                    std::copy(m_Buffer.begin() + m_StartIdx, m_Buffer.end(), newBuffer.begin());
                    std::copy(m_Buffer.begin(), m_Buffer.begin() + m_EndIdx, newBuffer.begin() + m_Buffer.size() - m_StartIdx);
                }
            }

            m_Buffer = std::move(newBuffer);
            m_EndIdx = oldSize;
        }

        std::size_t firstPart = std::min(dataSize, m_Buffer.size() - m_EndIdx);
        std::copy(data, data + firstPart, m_Buffer.begin() + m_EndIdx);
        m_EndIdx = (m_EndIdx + firstPart) % m_Buffer.size();

        if (firstPart < dataSize) {
            std::size_t secondPart = dataSize - firstPart;
            std::copy(data + firstPart, data + dataSize, m_Buffer.begin());
            m_EndIdx += secondPart;
        }

        m_IsFull = m_EndIdx == m_StartIdx;
        return true;
    }


    bool Pop(char* data, std::size_t dataSize) {
        if (IsEmpty() || dataSize > Size()) {
            return false;
        }

        std::size_t firstPart = std::min(dataSize, m_Buffer.size() - m_StartIdx);
        std::copy(m_Buffer.begin() + m_StartIdx, m_Buffer.begin() + m_StartIdx + firstPart, data);
        m_StartIdx = (m_StartIdx + firstPart) % m_Buffer.size();

        if (firstPart < dataSize) {
            std::size_t secondPart = dataSize - firstPart;
            std::copy(m_Buffer.begin(), m_Buffer.begin() + secondPart, data + firstPart);
            m_StartIdx += secondPart;
        }

        m_IsFull = false;
        return true;
    }
};