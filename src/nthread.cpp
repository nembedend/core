// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nthread.h"
#include "ndebug.h"

NThread::NThread(NObject* parent)
    : NObject(parent)
    , m_stackSize(DEFAULT_STACK)
    , m_priority(DEFAULT_PRIORITY)
    , m_handle(nullptr)
    , m_running(false)
    , m_finished(false)
    , m_interruptionRequested(false)
    , m_quitRequested(false)
{
    m_name[0] = '\0';
}

NThread::NThread(const char* name, NObject* parent)
    : NObject(parent)
    , m_stackSize(DEFAULT_STACK)
    , m_priority(DEFAULT_PRIORITY)
    , m_handle(nullptr)
    , m_running(false)
    , m_finished(false)
    , m_interruptionRequested(false)
    , m_quitRequested(false)
{
    if (name) {
        strncpy(m_name, name, sizeof(m_name) - 1);
        m_name[sizeof(m_name) - 1] = '\0';
    } else {
        m_name[0] = '\0';
    }
}

NThread::~NThread()
{
    quit();
    if (m_running && m_handle) {
        wait(1000);
        if (m_running) {
            vTaskDelete(m_handle);
            m_handle = nullptr;
            m_running = false;
            m_finished = true;
        }
    }
}

void NThread::setObjectName(const char* name)
{
    NObject::setObjectName(name);
    if (name) {
        strncpy(m_name, name, sizeof(m_name) - 1);
        m_name[sizeof(m_name) - 1] = '\0';
    } else {
        m_name[0] = '\0';
    }
}

void NThread::setStackSize(size_t size)
{
    if (!m_running)
        m_stackSize = size;
    else
        nWarning() << "NThread: cannot change stack size while running";
}

void NThread::setPriority(Priority priority)
{
    setPriority(static_cast<UBaseType_t>(priority));
}

void NThread::setPriority(UBaseType_t priority)
{
    if (!m_running) {
        m_priority = priority;
    } else if (m_handle) {
        vTaskPrioritySet(m_handle, priority);
        m_priority = priority;
    }
}

bool NThread::start()
{
    if (m_running) {
        nWarning() << "NThread: already running";
        return false;
    }

    m_finished = false;
    m_interruptionRequested = false;
    m_quitRequested = false;

    BaseType_t result = xTaskCreate(
        taskEntry,
        m_name[0] ? m_name : "NThread",
        m_stackSize,
        this,
        m_priority,
        &m_handle);

    if (result != pdPASS) {
        nError() << "NThread: failed to create task";
        return false;
    }

    // Wait a little for the task to start
    while (!m_running) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return true;
}

void NThread::quit()
{
    m_quitRequested = true;
    requestInterruption();
}

void NThread::requestInterruption()
{
    m_interruptionRequested = true;
}

bool NThread::wait(unsigned long time)
{
    if (!m_running || m_finished)
        return true;

    TickType_t ticks = (time == ULONG_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(time);
    while (!m_finished && (ticks > 0)) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (ticks != portMAX_DELAY) {
            ticks -= pdMS_TO_TICKS(10);
        }
    }

    return m_finished;
}

NThread* NThread::currentThread()
{
    // TODO
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();
    if (!handle)
        return nullptr;

    return nullptr;
}

TaskHandle_t NThread::currentThreadId()
{
    return xTaskGetCurrentTaskHandle();
}

void NThread::msleep(unsigned long msecs)
{
    vTaskDelay(pdMS_TO_TICKS(msecs));
}

void NThread::usleep(unsigned long usecs)
{
    vTaskDelay(pdMS_TO_TICKS(usecs / 1000));
    if (usecs % 1000) {
        esp_rom_delay_us(usecs % 1000);
    }
}

void NThread::yield()
{
    taskYIELD();
}

void NThread::taskEntry(void* arg)
{
    NThread* self = static_cast<NThread*>(arg);
    self->execute();
    vTaskDelete(nullptr);
}

void NThread::execute()
{
    m_running = true;
    m_finished = false;
    started.emitSignal();

    run();

    m_running = false;
    m_finished = true;
    finished.emitSignal();
}

void NThread::run()
{
    while (!m_interruptionRequested && !m_quitRequested) {
        msleep(10);
    }
}
