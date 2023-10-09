#pragma once

#include <atomic>
#include <iostream>
#include <thread>

class SpinLock {
private:
    std::atomic<bool> m_Lock{ false };

public:
    void lock() {
        bool expected = false;
        while (!m_Lock.compare_exchange_strong(expected, true)) {
            expected = false;
        }
    }

    void unlock() {
        m_Lock.store(false);
    }
};