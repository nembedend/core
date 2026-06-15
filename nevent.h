// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.


#pragma once

#include "nobject.h"

class NEvent {
public:
    enum Type {
        None = 0,
        Timer = 1,
        ChildAdded = 2,
        ChildRemoved = 3
    };

    explicit NEvent(Type type = None);
    virtual ~NEvent();

    Type type() const { return m_type; }
    bool isAccepted() const { return m_accepted; }
    void accept() { m_accepted = true; }
    void ignore() { m_accepted = false; }

protected:
    Type m_type;
    bool m_accepted;
};

class NTimerEvent : public NEvent {
public:
    explicit NTimerEvent(int timerId);
    int timerId() const { return m_timerId; }

private:
    int m_timerId;
};

class NChildEvent : public NEvent {
public:
    NChildEvent(Type type, NObject* child);
    NObject* child() const { return m_child; }

private:
    NObject* m_child;
};
