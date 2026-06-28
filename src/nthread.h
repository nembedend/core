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
#include "freertos/task.h"

class NThread : public NObject {
public:
    enum Priority {
        Idle = 0,
        Lowest = 1,
        Low = 2,
        Normal = 3,
        High = 4,
        Highest = 5,
        RealTime = configMAX_PRIORITIES - 1
    };

    explicit NThread(NObject* parent = nullptr);
    explicit NThread(const char* name, NObject* parent = nullptr);
    virtual ~NThread();

    void setObjectName(const char* name);
    void setStackSize(size_t size);
    void setPriority(Priority priority);
    void setPriority(UBaseType_t priority);

    bool start();
    void quit();
    void requestInterruption();
    bool wait(unsigned long time = ULONG_MAX);

    bool isRunning() const { return m_running; }
    bool isFinished() const { return m_finished; }
    bool isInterruptionRequested() const { return m_interruptionRequested; }

    TaskHandle_t threadHandle() const { return m_handle; }

    static NThread* currentThread();
    static TaskHandle_t currentThreadId();
    static void msleep(unsigned long msecs);
    static void usleep(unsigned long usecs);
    static void yield();

    NSignal<> started;
    NSignal<> finished;

protected:
    virtual void run();

private:
    static void taskEntry(void* arg);
    void execute();

    char m_name[32];
    size_t m_stackSize;
    UBaseType_t m_priority;
    TaskHandle_t m_handle;
    volatile bool m_running;
    volatile bool m_finished;
    volatile bool m_interruptionRequested;
    volatile bool m_quitRequested;

    static constexpr size_t DEFAULT_STACK = 4096;
    static constexpr UBaseType_t DEFAULT_PRIORITY = Normal;
};
