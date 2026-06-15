// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "ntimer.h"
#include "ndebug.h"
#include "neventloop.h"

int NTimer::s_nextTimerId = 1;

NTimer::NTimer(NObject* parent)
    : NObject(parent)
    , m_interval(0)
    , m_singleShot(false)
    , m_type(TimerType::PreciseTimer)
    , m_handle(nullptr)
    , m_timerId(0)
{
    printf("[NTimer] this=%p\n", this);
}

NTimer::NTimer(TimerType type, NObject* parent)
    : NObject(parent)
    , m_interval(0)
    , m_singleShot(false)
    , m_type(type)
    , m_handle(nullptr)
    , m_timerId(0)
{
    printf("[NTimer] this=%p\n", this);
}

NTimer::~NTimer()
{
    printf("[NTimer] this=%p\n", this);
    stop();
    deleteTimer();
}

void NTimer::start(int msec)
{
    printf("[NTimer] setInterval(%d)\n", msec);
    setInterval(msec);
    start();
}

void NTimer::start()
{
    printf("[NTimer] this=%p\n", this);
    printf("[NTimer] start() called, interval=%d\n", m_interval); // DEBUG

    if (m_interval <= 0) {
        nWarning() << "NTimer::start: Invalid interval" << m_interval;
        return;
    }

    if (!m_handle) {
        createTimer();
    }

    updateTimer();

    if (xTimerStart(m_handle, 0) != pdPASS) {
        nError() << "NTimer::start: Failed to start timer";
    }
}

void NTimer::stop()
{
    if (m_handle && xTimerIsTimerActive(m_handle)) {
        if (xTimerStop(m_handle, 0) != pdPASS) {
            nWarning() << "NTimer::stop: Failed to stop timer";
        }
    }
}

bool NTimer::isActive() const noexcept
{
    return m_handle && xTimerIsTimerActive(m_handle) == pdTRUE;
}

int NTimer::remainingTime() const noexcept
{
    if (!isActive())
        return -1;

    TickType_t remaining = xTimerGetExpiryTime(m_handle) - xTaskGetTickCount();
    return (int)(remaining * portTICK_PERIOD_MS);
}

void NTimer::setInterval(int msec)
{
    if (m_interval == msec)
        return;

    m_interval = msec;
    if (isActive()) {
        updateTimer();
    }
}

void NTimer::setSingleShot(bool singleShot)
{
    if (m_singleShot == singleShot)
        return;

    m_singleShot = singleShot;
    if (isActive()) {
        updateTimer();
    }
}

void NTimer::setTimerType(TimerType type)
{
    if (m_type == type)
        return;

    bool wasActive = isActive();
    if (wasActive) {
        stop();
    }

    m_type = type;

    if (wasActive) {
        start();
    }
}

void NTimer::singleShot(int msec, std::function<void()> function)
{
    if (!function)
        return;

    NTimer* timer = new NTimer();
    timer->setSingleShot(true);
    timer->setInterval(msec);

    timer->timeout.connect([timer, function = std::move(function)]() {
        function();
        timer->deleteLater();
    });

    timer->start();
}

void NTimer::timerCallback(TimerHandle_t xTimer)
{
    printf("[NTimer::timerCallback] entered\n");
    NTimer* timer = static_cast<NTimer*>(pvTimerGetTimerID(xTimer));
    if (!timer || timer->m_timerId == 0) {
        printf("[NTimer::timerCallback] invalid timer\n");
        return;
    }
    NTimerEvent* event = new NTimerEvent(timer->m_timerId);
    printf("[NTimer::timerCallback] about to call postEvent, receiver=%p\n", timer);
    NEventLoop::instance().postEvent(timer, event);
    printf("[NTimer::timerCallback] postEvent returned\n");
    if (timer->m_singleShot)
        timer->deleteLater();
}

void NTimer::createTimer()
{
    printf("[NTimer] createTimer() interval=%d\n", m_interval); // DEBUG
    if (m_handle)
        return;

    m_timerId = s_nextTimerId++;

    m_handle = xTimerCreate(
        "NTimer",
        pdMS_TO_TICKS(m_interval),
        pdTRUE, // Will be updated in updateTimer()
        this,
        timerCallback);

    if (!m_handle) {
        nError() << "NTimer: Failed to create FreeRTOS timer";
    } else {
        printf("[NTimer] xTimerCreate succeeded, handle=%p\n", m_handle);
    }
}

void NTimer::deleteTimer()
{
    if (m_handle) {
        if (xTimerIsTimerActive(m_handle)) {
            xTimerStop(m_handle, portMAX_DELAY);
        }
        xTimerDelete(m_handle, portMAX_DELAY);
        m_handle = nullptr;
    }
}

void NTimer::updateTimer()
{
    printf("[NTimer] updateTimer: entered, m_handle=%p, m_interval=%d\n", m_handle, m_interval);
    if (!m_handle) {
        printf("[NTimer] updateTimer: no handle, returning\n");
        return;
    }
    printf("[NTimer] updateTimer: calling xTimerChangePeriod\n");
    BaseType_t res = xTimerChangePeriod(m_handle, pdMS_TO_TICKS(m_interval), 0);
    printf("[NTimer] updateTimer: xTimerChangePeriod returned %d\n", res);
}

void NTimer::timerEvent(NTimerEvent* event)
{
    printf("[NTimer::timerEvent] entered, timerId=%d, m_timerId=%d\n", event->timerId(), m_timerId);
    if (event->timerId() == m_timerId) {
        printf("[NTimer::timerEvent] emitting timeout\n");
        timeout.emitSignal();
    }
}
