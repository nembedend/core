// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nobject.h"
#include "ndebug.h"
#include "nevent.h"
#include "neventloop.h"
#include "nvariant.h"
#include <algorithm>
#include <cstring>
#include <inttypes.h>

#define GUARD 0xDEADBEEF
uint32_t guard1 = GUARD;

uint32_t NObject::s_nextObjectId = 1;
NObject* NObject::s_deferredDeleteList = nullptr;
SemaphoreHandle_t NObject::s_deferredDeleteMutex = nullptr;

NObject::NObject(NObject* parent)
    : m_parent(nullptr)
    , m_nextDeferred(nullptr)
    , m_isDeleting(false)
    , m_deleteLater(false)
    , m_objectId(s_nextObjectId++)
    , m_thread(xTaskGetCurrentTaskHandle())
    , m_dynamicPropCount(0)
    , m_filterCount(0)
    , m_timerCount(0)

{
    memset(m_children, 0, sizeof(m_children));
    memset(m_objectName, 0, sizeof(m_objectName));
    memset(m_eventFilters, 0, sizeof(m_eventFilters));
    memset(m_timers, 0, sizeof(m_timers));

    for (int i = 0; i < MAX_DYNAMIC_PROPERTIES; ++i) {
        m_dynamicProps[i].name[0] = '\0';
        m_dynamicProps[i].value = NVariant();
    }

    m_mutex = xSemaphoreCreateMutex();
    if (!s_deferredDeleteMutex) {
        s_deferredDeleteMutex = xSemaphoreCreateMutex();
    }

    if (parent) {
        setParent(parent);
    }
}

NObject::~NObject()
{
    printf("GUARD=%08lx\n", (unsigned long)guard1);

    m_isDeleting = true;

    destroyed.emitSignal();

    while (m_filterCount > 0) {
        removeEventFilter(m_eventFilters[0]);
    }

    while (m_timerCount > 0) {
        killTimer(m_timers[0].id);
    }

    destroyChildren();
    detachFromParent();

    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
    }
}

void NObject::setParent(NObject* newParent)
{
    if (m_parent == newParent)
        return;

    lock();
    detachFromParent();
    attachToParent(newParent);
    unlock();

    parentChanged.emitSignal(this);
}

size_t NObject::childCount() const
{
    lock();
    size_t count = 0;
    for (size_t i = 0; i < MAX_CHILDREN; ++i) {
        if (m_children[i])
            ++count;
    }
    unlock();
    return count;
}

bool NObject::isChildOf(const NObject* parent) const
{
    if (!parent)
        return false;

    lock();
    for (size_t i = 0; i < MAX_CHILDREN; ++i) {
        if (m_children[i] == parent) {
            unlock();
            return true;
        }
    }
    unlock();
    return false;
}

bool NObject::isDescendantOf(const NObject* ancestor) const
{
    if (!ancestor)
        return false;

    const NObject* obj = this;
    while (obj) {
        if (obj == ancestor)
            return true;
        obj = obj->parent();
    }
    return false;
}

void NObject::setObjectName(const char* name)
{
    lock();
    strncpy(m_objectName, name ? name : "", MAX_OBJECT_NAME - 1);
    m_objectName[MAX_OBJECT_NAME - 1] = '\0';
    unlock();

    objectNameChanged.emitSignal(this);
}

bool NObject::inherits(const char* className) const
{
    const NObject* obj = this;
    while (obj) {
        if (strcmp(obj->className(), className) == 0)
            return true;
        break;
    }
    return false;
}

void NObject::setProperty(const char* name, const NVariant& value)
{
    if (!name)
        return;

    lock();

    for (int i = 0; i < m_dynamicPropCount; ++i) {
        if (strcmp(m_dynamicProps[i].name, name) == 0) {
            m_dynamicProps[i].value = value;
            unlock();
            propertyChanged(name, value);
            return;
        }
    }

    if (m_dynamicPropCount < MAX_DYNAMIC_PROPERTIES) {
        strncpy(m_dynamicProps[m_dynamicPropCount].name, name, 31);
        m_dynamicProps[m_dynamicPropCount].name[31] = '\0';
        m_dynamicProps[m_dynamicPropCount].value = value;
        m_dynamicPropCount++;
        unlock();
        propertyChanged(name, value);
    } else {
        unlock();
        nWarning() << "NObject: Max dynamic properties reached for" << objectName();
    }
}

NVariant NObject::property(const char* name) const
{
    if (!name)
        return NVariant();

    lock();
    for (int i = 0; i < m_dynamicPropCount; ++i) {
        if (strcmp(m_dynamicProps[i].name, name) == 0) {
            NVariant result = m_dynamicProps[i].value;
            unlock();
            return result;
        }
    }
    unlock();
    return NVariant();
}

std::vector<NString> NObject::dynamicPropertyNames() const
{
    std::vector<NString> names;
    lock();
    for (int i = 0; i < m_dynamicPropCount; ++i) {
        names.push_back(NString(m_dynamicProps[i].name));
    }
    unlock();
    return names;
}

bool NObject::hasProperty(const char* name) const
{
    if (!name)
        return false;

    lock();
    for (int i = 0; i < m_dynamicPropCount; ++i) {
        if (strcmp(m_dynamicProps[i].name, name) == 0) {
            unlock();
            return true;
        }
    }
    unlock();
    return false;
}

void NObject::removeProperty(const char* name)
{
    if (!name)
        return;

    lock();
    for (int i = 0; i < m_dynamicPropCount; ++i) {
        if (strcmp(m_dynamicProps[i].name, name) == 0) {
            for (int j = i; j < m_dynamicPropCount - 1; ++j) {
                m_dynamicProps[j] = m_dynamicProps[j + 1];
            }
            m_dynamicPropCount--;
            break;
        }
    }
    unlock();
}

void NObject::propertyChanged(const char* name, const NVariant& value)
{
    (void)name;
    (void)value;
}

void NObject::event(NEvent* event)
{
    if (!event)
        return;

    for (int i = 0; i < m_filterCount; ++i) {
        if (m_eventFilters[i] && m_eventFilters[i]->eventFilter(this, event)) {
            return;
        }
    }

    switch (event->type()) {
    case NEvent::Timer:
        timerEvent(static_cast<NTimerEvent*>(event));
        break;
    default:
        break;
    }
}

void NObject::timerEvent(NTimerEvent* event)
{
    (void)event;
}

bool NObject::eventFilter(NObject* watched, NEvent* event)
{
    (void)watched;
    (void)event;
    return false;
}

void NObject::installEventFilter(NObject* filter)
{
    if (!filter || m_filterCount >= 4)
        return;

    lock();
    m_eventFilters[m_filterCount++] = filter;
    unlock();
}

void NObject::removeEventFilter(NObject* filter)
{
    lock();
    for (int i = 0; i < m_filterCount; ++i) {
        if (m_eventFilters[i] == filter) {
            for (int j = i; j < m_filterCount - 1; ++j) {
                m_eventFilters[j] = m_eventFilters[j + 1];
            }
            m_filterCount--;
            break;
        }
    }
    unlock();
}

void NObject::postEvent(NObject* receiver, NEvent* event, int priority)
{
    (void)priority;
    if (receiver && event) {
        NEventLoop::instance().postEvent(receiver, event);
    }
}

void NObject::sendEvent(NObject* receiver, NEvent* event)
{
    if (receiver && event) {
        receiver->event(event);
        delete event;
    }
}

void NObject::removePostedEvents(NObject* receiver)
{
    (void)receiver;
}

void NObject::childEvent(NChildEvent* event)
{
    (void)event;
}

int NObject::startTimer(int intervalMs, bool singleShot)
{
    if (intervalMs <= 0 || m_timerCount >= 8)
        return 0;

    static int s_nextTimerId = 1000;
    int timerId = s_nextTimerId++;

    NEventLoop::instance().registerTimer(timerId, intervalMs, singleShot, this);
    registerTimer(timerId, nullptr, singleShot);

    return timerId;
}

void NObject::killTimer(int timerId)
{
    NEventLoop::instance().unregisterTimer(timerId);
    unregisterTimer(timerId);
}

void NObject::registerTimer(int timerId, TimerHandle_t handle, bool singleShot)
{
    if (m_timerCount < 8) {
        m_timers[m_timerCount].id = timerId;
        m_timers[m_timerCount].handle = handle;
        m_timers[m_timerCount].singleShot = singleShot;
        m_timerCount++;
    }
}

void NObject::unregisterTimer(int timerId)
{
    for (int i = 0; i < m_timerCount; ++i) {
        if (m_timers[i].id == timerId) {
            for (int j = i; j < m_timerCount - 1; ++j) {
                m_timers[j] = m_timers[j + 1];
            }
            m_timerCount--;
            break;
        }
    }
}

void NObject::timerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
}

void NObject::moveToThread(TaskHandle_t newThread)
{
    if (!newThread)
        return;
    lock();
    m_thread = newThread;
    unlock();
}

void NObject::deleteLater()
{
    if (m_deleteLater)
        return;
    m_deleteLater = true;

    if (s_deferredDeleteMutex) {
        xSemaphoreTake(s_deferredDeleteMutex, portMAX_DELAY);
    }
    m_nextDeferred = s_deferredDeleteList;
    s_deferredDeleteList = this;
    if (s_deferredDeleteMutex) {
        xSemaphoreGive(s_deferredDeleteMutex);
    }
}

void NObject::processDeferredDeletes()
{
    if (s_deferredDeleteMutex) {
        xSemaphoreTake(s_deferredDeleteMutex, portMAX_DELAY);
    }
    NObject* obj = s_deferredDeleteList;
    s_deferredDeleteList = nullptr;
    if (s_deferredDeleteMutex) {
        xSemaphoreGive(s_deferredDeleteMutex);
    }

    while (obj) {
        NObject* next = obj->m_nextDeferred;
        if (obj->m_deleteLater) {
            delete obj;
        }
        obj = next;
    }
}

void NObject::lock() const
{
    if (m_mutex) {
        xSemaphoreTake(m_mutex, portMAX_DELAY);
    }
}

void NObject::unlock() const
{
    if (m_mutex) {
        xSemaphoreGive(m_mutex);
    }
}

bool NObject::tryLock(TickType_t timeout) const
{
    if (m_mutex) {
        return xSemaphoreTake(m_mutex, timeout) == pdTRUE;
    }
    return false;
}

NString NObject::dumpObjectTree() const
{
    NString result;
    result.append(className());
    result.append(": ");
    result.append(objectName());
    result.append("\n");

    for (size_t i = 0; i < MAX_CHILDREN; ++i) {
        if (m_children[i]) {
            result.append("  ");
            result.append(m_children[i]->dumpObjectTree());
        }
    }
    return result;
}

NString NObject::dumpObjectInfo() const
{
    NString result;
    result.append("NObject: ");
    result.append(className());
    result.append(" (");
    result.append(objectName());
    result.append(")\n");
    result.append("  Object ID: ");
    result.append(NString::number(m_objectId));
    result.append("\n  Thread: ");
    result.append(NString::number((uint32_t)m_thread, 16));
    result.append("\n  Children: ");
    result.append(NString::number(childCount()));
    result.append("\n  Dynamic Properties: ");
    result.append(NString::number(m_dynamicPropCount));
    result.append("\n");
    return result;
}

void NObject::addChild(NObject* child)
{
    if (!child)
        return;

    for (size_t i = 0; i < MAX_CHILDREN; ++i) {
        if (m_children[i] == nullptr) {
            m_children[i] = child;
            return;
        }
    }
    nWarning() << "MAX_CHILDREN exceeded for" << className();
}

void NObject::removeChild(NObject* child)
{
    for (size_t i = 0; i < MAX_CHILDREN; ++i) {
        if (m_children[i] == child) {
            m_children[i] = nullptr;
            return;
        }
    }
}

void NObject::detachFromParent()
{
    if (m_parent) {
        m_parent->removeChild(this);
        m_parent = nullptr;
    }
}

void NObject::attachToParent(NObject* parent)
{
    if (!parent)
        return;
    m_parent = parent;
    parent->addChild(this);
}

void NObject::destroyChildren()
{
    for (size_t i = 0; i < MAX_CHILDREN; ++i) {
        if (m_children[i]) {
            m_children[i]->m_parent = nullptr;
            delete m_children[i];
            m_children[i] = nullptr;
        }
    }
}

NObject* NObject::findChildInternal(const char* name, FindOptions options) const
{
    lock();

    for (size_t i = 0; i < MAX_CHILDREN; ++i) {
        if (m_children[i]) {
            bool match = false;
            if (name == nullptr) {
                match = true;
            } else if (options & FindCaseSensitively) {
                match = strcmp(m_children[i]->objectName(), name) == 0;
            } else {
                match = strcasecmp(m_children[i]->objectName(), name) == 0;
            }

            if (match) {
                unlock();
                return m_children[i];
            }

            if (options & FindRecursively) {
                NObject* found = m_children[i]->findChildInternal(name, options);
                if (found) {
                    unlock();
                    return found;
                }
            }
        }
    }

    unlock();
    return nullptr;
}

std::vector<NObject*> NObject::findChildrenInternal(const char* name, FindOptions options) const
{
    std::vector<NObject*> results;
    lock();

    for (size_t i = 0; i < MAX_CHILDREN; ++i) {
        if (m_children[i]) {
            bool match = false;
            if (name == nullptr) {
                match = true;
            } else if (options & FindCaseSensitively) {
                match = strcmp(m_children[i]->objectName(), name) == 0;
            } else {
                match = strcasecmp(m_children[i]->objectName(), name) == 0;
            }

            if (match) {
                results.push_back(m_children[i]);
            }

            if (options & FindRecursively) {
                auto subResults = m_children[i]->findChildrenInternal(name, options);
                results.insert(results.end(), subResults.begin(), subResults.end());
            }
        }
    }

    unlock();
    return results;
}
