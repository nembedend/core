// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nobject.h"
#include "nsignalbus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <functional>

class NTimer : public NObject {
public:
    enum class TimerType {
        PreciseTimer,
        CoarseTimer,
        VeryCoarseTimer
    };

    explicit NTimer(NObject* parent = nullptr);
    explicit NTimer(TimerType type, NObject* parent = nullptr);
    virtual ~NTimer();

    void start(int msec);
    void start();
    void stop();

    bool isActive() const noexcept;
    bool isSingleShot() const noexcept { return m_singleShot; }
    int interval() const noexcept { return m_interval; }
    int remainingTime() const noexcept;
    int timerId() const noexcept { return m_timerId; }

    void setInterval(int msec);
    void setSingleShot(bool singleShot);
    void setTimerType(TimerType type);
    TimerType timerType() const noexcept { return m_type; }

    void setCallback(std::function<void()> cb) { m_callback = cb; }

    NSignal<> timeout;

    static void singleShot(int msec, std::function<void()> function);
    template<typename Object, typename Method>
    static void singleShot(int msec, Object* receiver, Method method) {
        singleShot(msec, [receiver, method]() {
            (receiver->*method)();
        });
    }

protected:
    void timerEvent(NTimerEvent* event) override;

private:
    static void timerCallback(TimerHandle_t xTimer);
    void createTimer();
    void deleteTimer();
    void updateTimer();

    int m_interval;
    bool m_singleShot;
    TimerType m_type;
    TimerHandle_t m_handle;
    int m_timerId;
    std::function<void()> m_callback;

    static int s_nextTimerId;
};
