// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <functional>
#include "nsignalbus.h"

template<typename... Args>
class NSignal {
public:
    NSignal() = default;

    NMetaConnection connect(std::function<void(Args...)> slot,
                            NPrivate::NConnectionType type = NPrivate::NConnectionType::Auto) const {
        printf("[NSignal::connect] this=%p, bus init=%d\n", this, m_bus.isInitialized());
        if (m_bus.isInitialized() && slot) {
            auto conn = m_bus.template connect<Args...>(this, std::move(slot), type);
            printf("[NSignal::connect] got connection id=%zu\n", conn.id());
            return conn;
        }
        printf("[NSignal::connect] returning invalid\n");
        return NMetaConnection();
    }

    template<typename Object, typename Method>
    NMetaConnection connect(Object* receiver, Method method,
                            NPrivate::NConnectionType type = NPrivate::NConnectionType::Auto) const {
        return connect([receiver, method](Args... args) {
            (receiver->*method)(std::forward<Args>(args)...);
        }, type);
    }

    void emitSignal(Args... args) const {
        if (!m_blocked && m_bus.isInitialized()) {
            m_bus.emitSignal(this, std::forward<Args>(args)...);
        }
    }

    void emitQueued(Args... args) const {
        if (!m_blocked && m_bus.isInitialized()) {
            m_bus.emitSignalQueued(this, std::forward<Args>(args)...);
        }
    }

    void operator()(Args... args) const {
        emitSignal(std::forward<Args>(args)...);
    }

    void disconnect() const {
        if (m_bus.isInitialized()) {
            m_bus.disconnect(this);
        }
    }

    void disconnect(const NMetaConnection& connection) const {
        if (m_bus.isInitialized()) {
            m_bus.disconnect(connection);
        }
    }

    bool hasConnections() const {
        return m_bus.isInitialized() && m_bus.connectionCount(this) > 0;
    }

    size_t connectionCount() const {
        return m_bus.isInitialized() ? m_bus.connectionCount(this) : 0;
    }

    void block(bool block = true) { m_blocked = block; }
    bool isBlocked() const { return m_blocked; }

private:
    NSignalBus& m_bus = NSignalBus::instance();
    bool m_blocked = false;
};

#ifdef connect
#undef connect
#endif

template<typename Sender, typename... SignalArgs, typename Slot>
inline NMetaConnection connect(Sender* sender,
                               NSignal<SignalArgs...> Sender::*signal,
                               Slot&& slot) {
    return (sender->*signal).connect(std::forward<Slot>(slot));
}

template<typename Sender, typename... SignalArgs,
         typename Receiver, typename... SlotArgs>
inline NMetaConnection connect(Sender* sender,
                               NSignal<SignalArgs...> Sender::*signal,
                               Receiver* receiver,
                               void (Receiver::*slot)(SlotArgs...)) {
    static_assert(sizeof...(SignalArgs) == sizeof...(SlotArgs),
                  "Signal and slot argument counts do not match");
    return (sender->*signal).connect(receiver, slot);
}
