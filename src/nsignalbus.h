// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <any>
#include <atomic>
#include <mutex>
#include <type_traits>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nmetaconnection.h"

#ifdef emit
#undef emit
#endif
#define emit

namespace NPrivate {

enum class NConnectionType {
    Auto = 0,
    Direct = 1,
    Queued = 2
};

template<typename T>
struct NFunctionTraits;

template<typename R, typename... Args>
struct NFunctionTraits<R(Args...)> {
    static constexpr size_t ArgCount = sizeof...(Args);
};

template<typename R, typename... Args>
struct NFunctionTraits<R(*)(Args...)> : NFunctionTraits<R(Args...)> {};

template<typename T, typename R, typename... Args>
struct NFunctionTraits<R(T::*)(Args...)> : NFunctionTraits<R(Args...)> {};

template<typename T, typename R, typename... Args>
struct NFunctionTraits<R(T::*)(Args...) const> : NFunctionTraits<R(Args...)> {};

template<typename Functor>
struct NFunctionTraits : NFunctionTraits<decltype(&Functor::operator())> {};

template<typename... SlotArgs>
class NSlotCaller {
public:
    using SlotFunction = std::function<void(SlotArgs...)>;

    NSlotCaller(SlotFunction&& slot) : m_slot(std::move(slot)) {}

    void call(const std::vector<std::any>& args) const {
        if (m_slot && args.size() == sizeof...(SlotArgs)) {
            callImpl(args, std::index_sequence_for<SlotArgs...>{});
        }
    }

    bool isValid() const { return static_cast<bool>(m_slot); }

private:
    template<size_t... Is>
    void callImpl(const std::vector<std::any>& args, std::index_sequence<Is...>) const {
        m_slot(std::any_cast<SlotArgs>(args[Is])...);
    }

    SlotFunction m_slot;
};

}

class NSignalBus {
public:
    static NSignalBus& instance() noexcept;

    void init();
    void shutdown();
    bool isInitialized() const noexcept { return m_initialized; }

    template<typename... Args, typename Slot>
    NMetaConnection connect(const void* signalId, Slot&& slot,
                            NPrivate::NConnectionType type = NPrivate::NConnectionType::Auto) {
        using SlotType = std::decay_t<Slot>;
        using Traits = NPrivate::NFunctionTraits<SlotType>;
        static_assert(Traits::ArgCount == sizeof...(Args),
                      "Slot argument count does not match signal arguments");

        bool queued = (type == NPrivate::NConnectionType::Queued) ||
                      (type == NPrivate::NConnectionType::Auto && !isSameThread());

        NPrivate::NSlotCaller<Args...> caller([slot = std::forward<Slot>(slot)](Args... args) {
            if constexpr (std::is_invocable_v<SlotType, Args...>) {
                slot(args...);
            }
        });

        std::function<void(const std::vector<std::any>&)> wrapper =
            [caller = std::move(caller)](const std::vector<std::any>& args) {
                caller.call(args);
            };

        return connectInternal(signalId, wrapper, queued);
    }

    NMetaConnection connect(const void* signalId, std::function<void()> slot, bool queued = false);

    void disconnect(const void* signalId) noexcept;
    void disconnect(const NMetaConnection& conn) noexcept;
    void disconnectAll() noexcept;

    template<typename... Args>
    void emitSignal(const void* signalId, const Args&... args) {
        std::vector<std::any> anyArgs{args...};
        std::vector<SlotInfo> slotsCopy;
        {
            std::lock_guard<std::mutex> lock(m_slotsMutex);
            auto it = m_slots.find(signalId);
            if (it == m_slots.end()) return;
            slotsCopy = it->second;
        }
        for (const auto& info : slotsCopy) {
            if (info.slot) {
                info.slot(anyArgs);
            }
        }
        m_totalEmits.fetch_add(1);
    }

    template<typename... Args>
    void emitSignalQueued(const void* signalId, const Args&... args) {
        emitSignal(signalId, args...);
    }

    void processQueued() noexcept;
    void flush() noexcept;
    bool hasQueued() const noexcept;

    bool isSameThread() const noexcept;
    TaskHandle_t getThread() const noexcept { return m_mainThread; }

    size_t connectionCount(const void* signalId) const noexcept;
    size_t queuedCount() const noexcept;
    size_t totalEmits() const noexcept { return m_totalEmits.load(); }
    void resetStats() noexcept;
    void dumpStats() const noexcept;

private:
    NSignalBus() noexcept;
    ~NSignalBus();
    NSignalBus(const NSignalBus&) = delete;
    NSignalBus& operator=(const NSignalBus&) = delete;

    struct SlotInfo {
        size_t id;
        std::function<void(const std::vector<std::any>&)> slot;
        bool queued;
        TaskHandle_t threadAffinity;
    };

    NMetaConnection connectInternal(const void* signalId,
                                    std::function<void(const std::vector<std::any>&)> slot,
                                    bool queued);

    static void processingTask(void* arg);
    void runProcessingLoop();

    mutable std::unordered_map<const void*, std::vector<SlotInfo>> m_slots;
    mutable std::mutex m_slotsMutex;

    mutable TaskHandle_t m_mainThread;
    mutable std::atomic<size_t> m_nextConnectionId;
    mutable std::atomic<size_t> m_totalEmits;

    std::atomic<bool> m_initialized;
    std::atomic<bool> m_running;

    QueueHandle_t m_queue;
    static constexpr size_t QUEUE_SIZE = 16;
    static constexpr size_t TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t TASK_PRIORITY = 3;
};
