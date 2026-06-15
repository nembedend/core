// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <functional>
#include <vector>
#include <deque>
#include <atomic>
#include <mutex>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "nobject.h"
#include "nevent.h"

class NEventLoop {
public:
    enum ProcessEventsFlag {
        ProcessEventsAll             = 0x00,
        ProcessEventsExcludeUserInput = 0x01,
        ProcessEventsExcludeSocket   = 0x02,
        ProcessEventsWaitForMore     = 0x04
    };
    using ProcessEventsFlags = uint32_t;

    static NEventLoop& instance();

    void init();
    void shutdown();

    void exec();
    void quit();
    void exit(int returnCode = 0);
    bool isRunning() const { return m_running; }
    int returnCode() const { return m_returnCode; }

    void processEvents(int maxTimeMs = 0);
    void processEvents(ProcessEventsFlags flags, int maxTimeMs = 0);
    bool hasPendingEvents() const;
    void wakeUp();

    void postEvent(NEvent* event, int priority = 0);
    void postEvent(NObject* receiver, NEvent* event, int priority = 0);
    void sendEvent(NObject* receiver, NEvent* event);

    void registerTimer(int timerId, int intervalMs, bool singleShot, NObject* obj);
    void unregisterTimer(int timerId);

    void setBudgetUs(int32_t us);
    void setMaxEventsPerIteration(int count);
    void setLowPowerMode(bool enabled);
    void setWatchdogFeedInterval(int ms);

    uint32_t processedEventCount() const { return m_processedEvents; }
    uint32_t droppedEventCount() const { return m_droppedEvents; }
    float averageLoopTimeUs() const;
    void resetStats();

    void dumpStats() const;

    NSignal<> aboutToQuit;
    NSignal<> idle;
    NSignal<int> processEventsRequested;

private:
    NEventLoop() = default;
    ~NEventLoop();

    NEventLoop(const NEventLoop&) = delete;
    NEventLoop& operator=(const NEventLoop&) = delete;

    struct QueuedEvent {
        NEvent* event;
        NObject* receiver;
        int priority;
        TickType_t enqueueTime;

        bool operator<(const QueuedEvent& other) const {
            return priority < other.priority;
        }
    };

    struct RegisteredTimer {
        int id;
        TimerHandle_t handle;
        NObject* receiver;
        bool singleShot;
    };

    void runLoop();
    void processQueuedEvents(int maxCount);
    void processTimers();
    void processIdle();
    void deliverEvent(NObject* receiver, NEvent* event);
    void cleanupEvents();

    // FreeRTOS callbacks
    static void timerCallback(TimerHandle_t xTimer);
    static void eventTask(void* arg);

    std::atomic<bool> m_running;
    std::atomic<bool> m_quitRequested;
    std::atomic<int> m_returnCode;

    std::deque<QueuedEvent> m_eventQueue;
    mutable std::mutex m_queueMutex;

    std::vector<RegisteredTimer> m_timers;
    mutable std::mutex m_timerMutex;

    QueueHandle_t m_wakeQueue;
    TaskHandle_t m_loopTask;

    int32_t m_budgetUs;
    int m_maxEventsPerIteration;
    bool m_lowPowerMode;
    int m_watchdogInterval;
    TickType_t m_lastWatchdogFeed;

    uint32_t m_processedEvents;
    uint32_t m_droppedEvents;
    uint32_t m_loopCount;
    uint32_t m_maxLoopTimeUs;
    uint32_t m_totalLoopTimeUs;

    static constexpr size_t WAKE_QUEUE_SIZE = 1;
    static constexpr int DEFAULT_BUDGET_US = 5000;     // 5ms default
    static constexpr int DEFAULT_MAX_EVENTS = 10;

    // FreeRTOS parameters
    static constexpr UBaseType_t EVENT_TASK_PRIORITY = 5;
    static constexpr size_t EVENT_TASK_STACK = 4096;
};

class NEventLoopWrapper {
public:
    explicit NEventLoopWrapper(NObject* parent = nullptr);
    ~NEventLoopWrapper();

    int exec(NEventLoop::ProcessEventsFlags flags = NEventLoop::ProcessEventsAll);
    void exit(int returnCode = 0);
    bool isRunning() const { return NEventLoop::instance().isRunning(); }

    void processEvents(NEventLoop::ProcessEventsFlags flags = NEventLoop::ProcessEventsAll, int maxTimeMs = 0);
    void wakeUp();

    static NEventLoop* instance() { return &NEventLoop::instance(); }

private:
    NEventLoop& m_loop;
};
