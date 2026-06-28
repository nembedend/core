// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nevent.h"
#include "nobject.h"

NEvent::NEvent(Type type)
    : m_type(type)
    , m_accepted(true)
{
}

NEvent::~NEvent()
{
}

NTimerEvent::NTimerEvent(int timerId)
    : NEvent(Timer)
    , m_timerId(timerId)
{
}

NChildEvent::NChildEvent(Type type, NObject* child)
    : NEvent(type)
    , m_child(child)
{
}
