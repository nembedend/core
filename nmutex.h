// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nobject.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class NMutex : public NObject {
public:
    enum class RecursionMode {
        NonRecursive,
        Recursive
    };

    explicit NMutex(RecursionMode mode = RecursionMode::NonRecursive, NObject* parent = nullptr);
    virtual ~NMutex();

    void lock();
    bool tryLock();
    bool tryLock(uint32_t timeoutMs);
    void unlock();

    bool isLocked() const;

    void lockInline() { lock(); }
    void unlockInline() { unlock(); }

private:
    SemaphoreHandle_t m_mutex;
    RecursionMode m_mode;
    mutable portMUX_TYPE m_spinlock;
    volatile bool m_locked;
};

class NMutexLocker {
public:
    explicit NMutexLocker(NMutex* mutex);
    ~NMutexLocker();

    void unlock();
    void relock();
    NMutex* mutex() const { return m_mutex; }

private:
    NMutex* m_mutex;
    bool m_locked;
};

class NRecursiveMutex : public NMutex {
public:
    explicit NRecursiveMutex(NObject* parent = nullptr);
};
