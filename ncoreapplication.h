// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nobject.h"
#include "neventloop.h"
#include "nsignalbus.h"
#include "nstring.h"
#include <vector>

class NTranslator;

class NCoreApplication : public NObject
{
public:
    static NCoreApplication& instance();

    void init();
    void exec();
    void quit();
    void exit(int returnCode = 0);

    bool isRunning() const { return running; }
    int returnCode() const { return m_returnCode; }

    void processEvents(int maxTimeMs = 0);
    void flushEvents();
    bool hasPendingEvents() const;

    NString applicationName() const { return m_appName; }
    void setApplicationName(const NString& name) { m_appName = name; }

    NString applicationVersion() const { return m_appVersion; }
    void setApplicationVersion(const NString& version) { m_appVersion = version; }

    NString organizationName() const { return m_orgName; }
    void setOrganizationName(const NString& name) { m_orgName = name; }

    NString organizationDomain() const { return m_orgDomain; }
    void setOrganizationDomain(const NString& domain) { m_orgDomain = domain; }

    void installTranslator(NTranslator* translator);
    void removeTranslator(NTranslator* translator);
    NString translate(const char* context,
                      const char* sourceText,
                      const char* disambiguation = nullptr,
                      int n = -1) const;

    NSignal<>& aboutToQuit() { return m_aboutToQuit; }
    NSignal<int>& finished() { return m_finished; }

    void postEvent(NEvent* event);
    void sendEvent(NEvent* event);
    void removePostedEvents(NObject* receiver);

    using Runnable = std::function<void()>;
    void addStartupRunnable(Runnable fn);
    void addShutdownRunnable(Runnable fn);

    static void msleep(uint32_t ms);
    static void usleep(uint32_t us);

    static NCoreApplication* instancePtr() { return m_instancePtr; }
    static NEventLoop* eventLoop() { return &NEventLoop::instance(); }

protected:
    virtual void event(NEvent* event) override;
    virtual void timerEvent(NTimerEvent* event);
    virtual bool notify(NObject* receiver, NEvent* event);

private:
    friend class NGuiApplication; //TODO Fix me
    NCoreApplication();
    ~NCoreApplication();

    void executeStartupRunnables();
    void executeShutdownRunnables();

    bool running = false;
    bool quitting = false;
    int m_returnCode = 0;

    NString m_appName;
    NString m_appVersion;
    NString m_orgName;
    NString m_orgDomain;

    NSignal<> m_aboutToQuit;
    NSignal<int> m_finished;

    std::vector<NTranslator*> m_translators;
    std::vector<Runnable> m_startupRunnables;
    std::vector<Runnable> m_shutdownRunnables;

    static NCoreApplication* m_instancePtr;
};
