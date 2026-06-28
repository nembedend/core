// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "neventloop.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "ncoreapplication.h"
#include "ndebug.h"
#include "nobject.h"

NEventLoop& NEventLoop::instance()
{
    static NEventLoop loop;
    return loop;
}

NEventLoop::~NEventLoop()
{
    shutdown();
}

void NEventLoop::init()
{
    if (m_wakeQueue)
        return;

    m_wakeQueue = xQueueCreate(WAKE_QUEUE_SIZE, sizeof(uint32_t));
    if (!m_wakeQueue) {
        nError() << "NEventLoop: Failed to create wake queue";
        return;
    }

    m_running = false;
    m_quitRequested = false;
    m_maxEventsPerIteration = DEFAULT_MAX_EVENTS;
    m_budgetUs = DEFAULT_BUDGET_US;
    m_lowPowerMode = false;
    m_watchdogInterval = 0;
    m_lastWatchdogFeed = 0;
    m_processedEvents = 0;
    m_droppedEvents = 0;
    m_loopCount = 0;
    m_maxLoopTimeUs = 0;
    m_totalLoopTimeUs = 0;

    nDebug() << "NEventLoop initialized";
}

void NEventLoop::shutdown()
{
    if (m_loopTask) {
        quit();
        vTaskDelete(m_loopTask);
        m_loopTask = nullptr;
    }

    if (m_wakeQueue) {
        vQueueDelete(m_wakeQueue);
        m_wakeQueue = nullptr;
    }

    cleanupEvents();

    nDebug() << "NEventLoop shutdown";
}

void NEventLoop::exec()
{
    printf("[NEventLoop::exec] STARTED\n");
    if (m_running) {
        printf("NEventLoop already running");
        return;
    }
    m_running = true;
    m_quitRequested = false;
    m_returnCode = 0;
    m_loopTask = xTaskGetCurrentTaskHandle();
    printf("NEventLoop started with budget = %li us\n", m_budgetUs);
    runLoop();
    m_running = false;
    nDebug() << "NEventLoop finished, processed:" << m_processedEvents
             << " events, avg loop:" << averageLoopTimeUs() << "us";
}

void NEventLoop::quit()
{
    if (!m_running)
        return;

    m_quitRequested = true;
    wakeUp();
}

void NEventLoop::exit(int returnCode)
{
    m_returnCode = returnCode;
    quit();
}

void NEventLoop::runLoop()
{
    while (!m_quitRequested && m_running) {
        processQueuedEvents(m_maxEventsPerIteration);
        vTaskDelay(1);
    }
}

void NEventLoop::processEvents(int maxTimeMs)
{
    if (maxTimeMs <= 0) {
        processQueuedEvents(m_maxEventsPerIteration);
        processTimers();
        return;
    }

    TickType_t startTicks = xTaskGetTickCount();
    TickType_t maxTicks = pdMS_TO_TICKS(maxTimeMs);

    while ((xTaskGetTickCount() - startTicks) < maxTicks) {
        if (!hasPendingEvents())
            break;
        processQueuedEvents(1);
        processTimers();
    }
}

void NEventLoop::processEvents(ProcessEventsFlags flags, int maxTimeMs)
{
    (void)flags;
    processEvents(maxTimeMs);
}

bool NEventLoop::hasPendingEvents() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return !m_eventQueue.empty();
}

void NEventLoop::wakeUp()
{
    if (m_wakeQueue) {
        uint32_t dummy = 0;
        xQueueSend(m_wakeQueue, &dummy, 0);
    }
}

void NEventLoop::postEvent(NEvent* event, int priority)
{
    if (!event)
        return;

    QueuedEvent queued;
    queued.event = event;
    queued.receiver = nullptr;
    queued.priority = priority;
    queued.enqueueTime = xTaskGetTickCount();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_eventQueue.push_back(queued);
    }

    wakeUp();
}

void NEventLoop::postEvent(NObject* receiver, NEvent* event, int priority)
{
    if (!receiver || !event) {
        return;
    }

    QueuedEvent queued;
    queued.event = event;
    queued.receiver = receiver;
    queued.priority = priority;
    queued.enqueueTime = xTaskGetTickCount();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_eventQueue.push_back(queued);
    }

    wakeUp();
}

void NEventLoop::sendEvent(NObject* receiver, NEvent* event)
{
    if (receiver && event) {
        deliverEvent(receiver, event);
    }
    delete event;
}

void NEventLoop::processQueuedEvents(int maxCount)
{
    if (maxCount <= 0)
        return;

    std::vector<QueuedEvent> events;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        int count = std::min((int)m_eventQueue.size(), maxCount);
        if (count == 0)
            return;
        events.reserve(count);
        for (int i = 0; i < count; ++i) {
            events.push_back(std::move(m_eventQueue.front()));
            m_eventQueue.pop_front();
        }
    }

    for (auto& queued : events) {
        if (queued.receiver) {
            queued.receiver->event(queued.event);
            delete queued.event;
        } else if (queued.event) {
            delete queued.event;
        }
        m_processedEvents++;
    }
}

void NEventLoop::processTimers()
{
    // Timers are handled by FreeRTOS software timers
}

void NEventLoop::processIdle()
{
    // Idle processing
}

void NEventLoop::deliverEvent(NObject* receiver, NEvent* event)
{
    if (!receiver || !event)
        return;
    receiver->event(event);
}

void NEventLoop::registerTimer(int timerId, int intervalMs, bool singleShot, NObject* obj)
{
    if (timerId <= 0 || intervalMs <= 0 || !obj)
        return;

    TimerHandle_t handle = xTimerCreate(
        "EventLoopTimer",
        pdMS_TO_TICKS(intervalMs),
        singleShot ? pdFALSE : pdTRUE,
        reinterpret_cast<void*>(static_cast<uintptr_t>(timerId)),
        timerCallback);

    if (handle) {
        std::lock_guard<std::mutex> lock(m_timerMutex);
        m_timers.push_back({ timerId, handle, obj, singleShot });
        xTimerStart(handle, 0);
    }
}

void NEventLoop::unregisterTimer(int timerId)
{
    std::lock_guard<std::mutex> lock(m_timerMutex);

    auto it = std::find_if(m_timers.begin(), m_timers.end(),
        [timerId](const RegisteredTimer& timer) {
            return timer.id == timerId;
        });

    if (it != m_timers.end()) {
        if (it->handle) {
            xTimerStop(it->handle, 0);
            xTimerDelete(it->handle, 0);
        }
        m_timers.erase(it);
    }
}

void NEventLoop::timerCallback(TimerHandle_t xTimer)
{
    int timerId = static_cast<int>(reinterpret_cast<uintptr_t>(pvTimerGetTimerID(xTimer)));

    auto& loop = instance();

    std::lock_guard<std::mutex> lock(loop.m_timerMutex);

    auto it = std::find_if(loop.m_timers.begin(), loop.m_timers.end(),
        [timerId](const RegisteredTimer& timer) {
            return timer.id == timerId;
        });

    if (it != loop.m_timers.end() && it->receiver) {
        NTimerEvent* event = new NTimerEvent(timerId);
        loop.postEvent(it->receiver, event, 0);
    }
}

void NEventLoop::cleanupEvents()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);

    for (auto& queued : m_eventQueue) {
        delete queued.event;
        m_droppedEvents++;
    }
    m_eventQueue.clear();
}

void NEventLoop::setBudgetUs(int32_t us)
{
    m_budgetUs = (us > 0) ? us : DEFAULT_BUDGET_US;
    nDebug() << "Event loop budget set to" << m_budgetUs << "us";
}

void NEventLoop::setMaxEventsPerIteration(int count)
{
    m_maxEventsPerIteration = (count > 0) ? count : DEFAULT_MAX_EVENTS;
}

void NEventLoop::setLowPowerMode(bool enabled)
{
    m_lowPowerMode = enabled;
    nDebug() << "Low power mode:" << enabled;
}

void NEventLoop::setWatchdogFeedInterval(int ms)
{
    m_watchdogInterval = ms;
    if (ms > 0) {
        m_lastWatchdogFeed = xTaskGetTickCount();
    }
    nDebug() << "Watchdog feed interval:" << ms << "ms";
}

float NEventLoop::averageLoopTimeUs() const
{
    if (m_loopCount == 0)
        return 0.0f;
    return static_cast<float>(m_totalLoopTimeUs) / m_loopCount;
}

void NEventLoop::resetStats()
{
    m_processedEvents = 0;
    m_droppedEvents = 0;
    m_loopCount = 0;
    m_maxLoopTimeUs = 0;
    m_totalLoopTimeUs = 0;
}

void NEventLoop::dumpStats() const
{
    nDebug() << "=== NEventLoop Stats ===";
    nDebug() << "Running:" << m_running.load();
    nDebug() << "Processed events:" << m_processedEvents;
    nDebug() << "Dropped events:" << m_droppedEvents;
    nDebug() << "Loop count:" << m_loopCount;
    nDebug() << "Avg loop time:" << averageLoopTimeUs() << "us";
    nDebug() << "Max loop time:" << m_maxLoopTimeUs << "us";
    nDebug() << "Budget:" << m_budgetUs << "us";
    nDebug() << "Max events/iter:" << m_maxEventsPerIteration;
    nDebug() << "Low power mode:" << m_lowPowerMode;

    std::lock_guard<std::mutex> lock(m_queueMutex);
    nDebug() << "Queued events:" << m_eventQueue.size();
}

NEventLoopWrapper::NEventLoopWrapper(NObject* parent)
    : m_loop(NEventLoop::instance())
{
    (void)parent;
}

NEventLoopWrapper::~NEventLoopWrapper()
{
    if (isRunning()) {
        exit(0);
    }
}

int NEventLoopWrapper::exec(NEventLoop::ProcessEventsFlags flags)
{
    (void)flags;
    m_loop.exec();
    return m_loop.returnCode();
}

void NEventLoopWrapper::exit(int returnCode)
{
    m_loop.exit(returnCode);
}

void NEventLoopWrapper::processEvents(NEventLoop::ProcessEventsFlags flags, int maxTimeMs)
{
    (void)flags;
    m_loop.processEvents(maxTimeMs);
}

void NEventLoopWrapper::wakeUp()
{
    m_loop.wakeUp();
}
