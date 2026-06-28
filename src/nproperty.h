// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <functional>
#include <vector>
#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "nsignal.h"

#define N_PROPERTY(type, name)                                      \
private:                                                            \
    type m_##name{};                                                \
    public:                                                             \
    type name() const { return m_##name; }                          \
    void set##name(const type& v)                                   \
{                                                                \
        if (m_##name == v) return;                                  \
        m_##name = v;                                               \
        name##Changed.Emit(m_##name);                               \
}                                                                \
    NSignal<type> name##Changed;

template<typename T>
class NProperty
{
public:
    using Callback = std::function<void(const T&)>;

    NProperty(T v = {})
        : value(v)
    {
        mutex = xSemaphoreCreateMutex();
    }

    ~NProperty()
    {
        if (mutex)
            vSemaphoreDelete(mutex);
    }

    void set(const T& v)
    {
        if (v == value)
            return;

        Lock();

        value = v;

        auto local = observers;

        Unlock();

        Notify(local);
    }

    T get() const
    {
        Lock();
        T v = value;
        Unlock();
        return v;
    }

    void Connect(Callback cb)
    {
        lock();
        observers.push_back(std::move(cb));
        unlock();
    }

private:
    T value;
    std::vector<Callback> observers;

    SemaphoreHandle_t mutex = nullptr;

    bool notifying = false;

private:
    void notify(const std::vector<Callback>& list)
    {
        if (notifying)
            return;

        notifying = true;

        for (auto& o : list)
        {
            if (o)
                o(value);
        }

        notifying = false;
    }

    void lock() const
    {
        if (mutex)
            xSemaphoreTake(mutex, portMAX_DELAY);
    }

    void unlock() const
    {
        if (mutex)
            xSemaphoreGive(mutex);
    }
};
