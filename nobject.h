// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <cstddef>
#include <cstring>
#include <vector>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "nstring.h"
#include "nvariant.h"
#include "nsignalbus.h"
#include "nsignal.h"

#define MAX_CHILDREN 16
#define MAX_OBJECT_NAME 64
#define MAX_DYNAMIC_PROPERTIES 8

class NEvent;
class NTimerEvent;
class NChildEvent;


class NObject {
public:
    enum FindOptions {
        FindDirectChildrenOnly = 0x00,
        FindRecursively = 0x01,
        FindCaseSensitively = 0x02
    };

    explicit NObject(NObject* parent = nullptr);
    virtual ~NObject();

    NObject(const NObject&) = delete;
    NObject& operator=(const NObject&) = delete;

    NObject* parent() const noexcept { return m_parent; }
    void setParent(NObject* parent);

    size_t childCount() const;
    bool isChildOf(const NObject* parent) const;
    bool isDescendantOf(const NObject* ancestor) const;

    template<typename T>
    T* findChild(const char* name = nullptr,
                 FindOptions options = FindDirectChildrenOnly) const {
        NObject* result = findChildInternal(name, options);
        return static_cast<T*>(result);
    }

    template<typename T>
    std::vector<T*> findChildren(const char* name = nullptr,
                                  FindOptions options = FindRecursively) const {
        std::vector<NObject*> results = findChildrenInternal(name, options);
        std::vector<T*> typedResults;
        for (auto* obj : results) {
            T* typed = static_cast<T*>(obj);
            if (typed) typedResults.push_back(typed);
        }
        return typedResults;
    }

    void setObjectName(const char* name);
    const char* objectName() const { return m_objectName; }
    virtual const char* className() const { return "NObject"; }
    virtual bool inherits(const char* className) const;

    uint32_t objectId() const { return m_objectId; }

    void setProperty(const char* name, const NVariant& value);
    NVariant property(const char* name) const;
    std::vector<NString> dynamicPropertyNames() const;
    bool hasProperty(const char* name) const;
    void removeProperty(const char* name);

    virtual void event(NEvent* event);
    virtual void timerEvent(NTimerEvent* event);
    virtual bool eventFilter(NObject* watched, NEvent* event);

    void installEventFilter(NObject* filter);
    void removeEventFilter(NObject* filter);

    static void postEvent(NObject* receiver, NEvent* event, int priority = 0);
    static void sendEvent(NObject* receiver, NEvent* event);
    static void removePostedEvents(NObject* receiver);

    int startTimer(int intervalMs, bool singleShot = false);
    void killTimer(int timerId);

    TaskHandle_t thread() const { return m_thread; }
    void moveToThread(TaskHandle_t newThread);
    bool isInThread() const { return xTaskGetCurrentTaskHandle() == m_thread; }

    void deleteLater();
    bool isDeleting() const { return m_isDeleting; }
    static void processDeferredDeletes();

    NSignal<> destroyed;
    NSignal<NObject*> objectNameChanged;
    NSignal<NObject*> parentChanged;

    void lock() const;
    void unlock() const;
    bool tryLock(TickType_t timeout = 0) const;

    NString dumpObjectTree() const;
    NString dumpObjectInfo() const;

protected:
    virtual void childEvent(NChildEvent* event);
    virtual void propertyChanged(const char* name, const NVariant& value);

    void addChild(NObject* child);
    void removeChild(NObject* child);

private:
    struct DynamicProperty {
        char name[32];
        NVariant value;
    };

    void detachFromParent();
    void attachToParent(NObject* parent);
    void destroyChildren();

    NObject* findChildInternal(const char* name, FindOptions options) const;
    std::vector<NObject*> findChildrenInternal(const char* name, FindOptions options) const;

    void registerTimer(int timerId, TimerHandle_t handle, bool singleShot);
    void unregisterTimer(int timerId);
    static void timerCallback(TimerHandle_t xTimer);

    NObject* m_parent;
    NObject* m_children[MAX_CHILDREN];
    NObject* m_nextDeferred;
    char m_objectName[MAX_OBJECT_NAME];
    bool m_isDeleting;
    bool m_deleteLater;
    uint32_t m_objectId;
    TaskHandle_t m_thread;
    mutable SemaphoreHandle_t m_mutex;

    DynamicProperty m_dynamicProps[MAX_DYNAMIC_PROPERTIES];
    int m_dynamicPropCount;

    NObject* m_eventFilters[4];
    int m_filterCount;

    struct TimerInfo {
        int id;
        TimerHandle_t handle;
        bool singleShot;
    };
    TimerInfo m_timers[8];
    int m_timerCount;

    static uint32_t s_nextObjectId;
    static NObject* s_deferredDeleteList;
    static SemaphoreHandle_t s_deferredDeleteMutex;
};
