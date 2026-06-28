// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nmutex.h"
#include "ndebug.h"

NMutex::NMutex(RecursionMode mode, NObject* parent)
    : NObject(parent)
    , m_mode(mode)
    , m_locked(false)
{
    if (mode == RecursionMode::Recursive) {
        m_mutex = xSemaphoreCreateRecursiveMutex();
    } else {
        m_mutex = xSemaphoreCreateMutex();
    }

    if (m_mutex == nullptr) {
        nError() << "NMutex: Failed to create mutex";
    }
}

NMutex::~NMutex()
{
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
        m_mutex = nullptr;
    }
}

void NMutex::lock()
{
    if (!m_mutex)
        return;

    if (m_mode == RecursionMode::Recursive) {
        xSemaphoreTakeRecursive(m_mutex, portMAX_DELAY);
    } else {
        xSemaphoreTake(m_mutex, portMAX_DELAY);
    }

    m_locked = true;
}

bool NMutex::tryLock()
{
    if (!m_mutex)
        return false;

    BaseType_t result;
    if (m_mode == RecursionMode::Recursive) {
        result = xSemaphoreTakeRecursive(m_mutex, 0);
    } else {
        result = xSemaphoreTake(m_mutex, 0);
    }

    if (result == pdTRUE) {
        m_locked = true;
        return true;
    }
    return false;
}

bool NMutex::tryLock(uint32_t timeoutMs)
{
    if (!m_mutex)
        return false;

    TickType_t ticks = pdMS_TO_TICKS(timeoutMs);
    BaseType_t result;

    if (m_mode == RecursionMode::Recursive) {
        result = xSemaphoreTakeRecursive(m_mutex, ticks);
    } else {
        result = xSemaphoreTake(m_mutex, ticks);
    }

    if (result == pdTRUE) {
        m_locked = true;
        return true;
    }
    return false;
}

void NMutex::unlock()
{
    if (!m_mutex)
        return;

    if (m_mode == RecursionMode::Recursive) {
        xSemaphoreGiveRecursive(m_mutex);
    } else {
        xSemaphoreGive(m_mutex);
    }

    m_locked = false;
}

bool NMutex::isLocked() const
{
    if (!m_mutex)
        return false;

    if (m_mode == RecursionMode::Recursive) {
        return m_locked;
    } else {
        return m_locked;
    }
}

NMutexLocker::NMutexLocker(NMutex* mutex)
    : m_mutex(mutex)
    , m_locked(false)
{
    if (m_mutex) {
        m_mutex->lock();
        m_locked = true;
    }
}

NMutexLocker::~NMutexLocker()
{
    if (m_locked && m_mutex) {
        m_mutex->unlock();
    }
}

void NMutexLocker::unlock()
{
    if (m_locked && m_mutex) {
        m_mutex->unlock();
        m_locked = false;
    }
}

void NMutexLocker::relock()
{
    if (!m_locked && m_mutex) {
        m_mutex->lock();
        m_locked = true;
    }
}

NRecursiveMutex::NRecursiveMutex(NObject* parent)
    : NMutex(RecursionMode::Recursive, parent)
{
}
