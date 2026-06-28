// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nsignalbus.h"
#include "ndebug.h"

NSignalBus& NSignalBus::instance() noexcept
{
    static NSignalBus bus;
    return bus;
}

NSignalBus::NSignalBus() noexcept
    : m_mainThread(xTaskGetCurrentTaskHandle())
    , m_nextConnectionId(1)
    , m_totalEmits(0)
    , m_initialized(false)
    , m_running(false)
    , m_queue(nullptr)
{
}

NSignalBus::~NSignalBus()
{
    shutdown();
}

void NSignalBus::init()
{
    if (m_initialized.load())
        return;

    m_initialized = true;
    m_running = true;
    m_mainThread = xTaskGetCurrentTaskHandle();

    m_queue = xQueueCreate(QUEUE_SIZE, sizeof(std::function<void()>));
    if (m_queue) {
        xTaskCreate(processingTask, "sigbus", TASK_STACK_SIZE, this, TASK_PRIORITY, nullptr);
    }

    nDebug() << "NSignalBus initialized";
}

void NSignalBus::shutdown()
{
    if (!m_initialized.load())
        return;

    m_running = false;

    if (m_queue) {
        vQueueDelete(m_queue);
        m_queue = nullptr;
    }

    disconnectAll();
    m_initialized = false;

    nDebug() << "NSignalBus shutdown";
}

NMetaConnection NSignalBus::connectInternal(const void* signalId,
    std::function<void(const std::vector<std::any>&)> slot,
    bool queued)
{
    printf("[NSignalBus::connectInternal] signalId=%p, queued=%d\n", signalId, queued);
    if (!slot || !m_initialized.load()) {
        printf("[NSignalBus::connectInternal] slot invalid or not initialized\n");
        return NMetaConnection();
    }

    std::lock_guard<std::mutex> lock(m_slotsMutex);
    size_t id = m_nextConnectionId.fetch_add(1);

    SlotInfo info { id, slot, queued, xTaskGetCurrentTaskHandle() };
    m_slots[signalId].push_back(std::move(info));

    printf("[NSignalBus::connectInternal] id=%zu, now %zu slots for signalId=%p\n",
        id, m_slots[signalId].size(), signalId);

    return NMetaConnection(id);
}

NMetaConnection NSignalBus::connect(const void* signalId, std::function<void()> slot, bool queued)
{
    if (!slot)
        return NMetaConnection();

    std::function<void(const std::vector<std::any>&)> wrapper =
        [slot = std::move(slot)](const std::vector<std::any>&) {
            slot();
        };

    return connectInternal(signalId, wrapper, queued);
}

void NSignalBus::disconnect(const void* signalId) noexcept
{
    if (!signalId)
        return;

    std::lock_guard<std::mutex> lock(m_slotsMutex);
    m_slots.erase(signalId);
}

void NSignalBus::disconnect(const NMetaConnection& conn) noexcept
{
    if (!conn.isValid())
        return;

    std::lock_guard<std::mutex> lock(m_slotsMutex);

    for (auto& pair : m_slots) {
        auto& vec = pair.second;
        auto it = std::remove_if(vec.begin(), vec.end(),
            [&conn](const SlotInfo& info) {
                return info.id == conn.id();
            });
        if (it != vec.end()) {
            vec.erase(it, vec.end());
            break;
        }
    }
}

void NSignalBus::disconnectAll() noexcept
{
    std::lock_guard<std::mutex> lock(m_slotsMutex);
    m_slots.clear();
}

void NSignalBus::processQueued() noexcept
{
    if (!m_queue)
        return;

    std::function<void()> fn;
    while (xQueueReceive(m_queue, &fn, 0) == pdTRUE) {
        if (fn) {
            fn();
        }
    }
}

void NSignalBus::flush() noexcept
{
    while (hasQueued()) {
        processQueued();
        vTaskDelay(1);
    }
}

bool NSignalBus::hasQueued() const noexcept
{
    return m_queue ? (uxQueueMessagesWaiting(m_queue) > 0) : false;
}

bool NSignalBus::isSameThread() const noexcept
{
    return xTaskGetCurrentTaskHandle() == m_mainThread;
}

size_t NSignalBus::connectionCount(const void* signalId) const noexcept
{
    if (!signalId)
        return 0;

    std::lock_guard<std::mutex> lock(m_slotsMutex);
    auto it = m_slots.find(signalId);
    return (it == m_slots.end()) ? 0 : it->second.size();
}

size_t NSignalBus::queuedCount() const noexcept
{
    return m_queue ? uxQueueMessagesWaiting(m_queue) : 0;
}

void NSignalBus::resetStats() noexcept
{
    m_totalEmits.store(0);
}

void NSignalBus::dumpStats() const noexcept
{
    nDebug() << "=== NSignalBus Stats ===";
    nDebug() << "Initialized:" << m_initialized.load();
    nDebug() << "Running:" << m_running.load();
    nDebug() << "Main thread:" << (uint32_t)m_mainThread;
    nDebug() << "Next connection ID:" << m_nextConnectionId.load();
    nDebug() << "Total emits:" << m_totalEmits.load();
    nDebug() << "Queued items:" << queuedCount();

    std::lock_guard<std::mutex> lock(m_slotsMutex);
    nDebug() << "Active signals:" << m_slots.size();
}

void NSignalBus::processingTask(void* arg)
{
    NSignalBus* self = static_cast<NSignalBus*>(arg);
    self->runProcessingLoop();
    vTaskDelete(nullptr);
}

void NSignalBus::runProcessingLoop()
{
    while (m_running.load()) {
        processQueued();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
