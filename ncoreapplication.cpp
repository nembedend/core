// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "ncoreapplication.h"
#include "esp_rom_sys.h" // TODO add wrapper to other soc
#include "ndebug.h"
#include "nevent.h"
#include "neventloop.h"
#include "ntranslator.h"

NCoreApplication* NCoreApplication::m_instancePtr = nullptr;

NCoreApplication::NCoreApplication()
    : NObject(nullptr)
{
    m_instancePtr = this;
    m_appName = NString("Nembedend Core");
    m_appVersion = NString("0.0.0");
    m_orgName = NString("nembedend");
    m_orgDomain = NString("core");
}

NCoreApplication::~NCoreApplication()
{
    flushEvents();
    for (auto* t : m_translators) {
        delete t;
    }
    m_translators.clear();
    m_instancePtr = nullptr;
}

NCoreApplication& NCoreApplication::instance()
{
    static NCoreApplication app;
    return app;
}

void NCoreApplication::init()
{
    NEventLoop::instance().init();
    NSignalBus::instance().init();
    NDebug::init();
    executeStartupRunnables();
    running = true;
    quitting = false;
    m_returnCode = 0;
    nDebug() << "NCoreApplication initialized:" << m_appName.c_str();
}

void NCoreApplication::exec()
{
    if (!running) {
        nDebug() << "Application not initialized, call init() first";
        return;
    }
    printf("[NCoreApplication::exec] calling NEventLoop::exec\n");
    NEventLoop::instance().exec();
    printf("[NCoreApplication::exec] returned\n");
    processEvents();
    executeShutdownRunnables();
}

void NCoreApplication::quit()
{
    if (quitting)
        return;
    nDebug() << "Quit requested";
    quitting = true;
    m_aboutToQuit.emitSignal();
    NEventLoop::instance().quit();
}

void NCoreApplication::exit(int returnCode)
{
    m_returnCode = returnCode;
    m_finished.emitSignal(returnCode);
    quit();
}

void NCoreApplication::processEvents(int maxTimeMs)
{
    NEventLoop::instance().processEvents(maxTimeMs);
}

void NCoreApplication::flushEvents()
{
    while (hasPendingEvents()) {
        processEvents(10);
    }
}

bool NCoreApplication::hasPendingEvents() const
{
    return NEventLoop::instance().hasPendingEvents();
}

void NCoreApplication::installTranslator(NTranslator* translator)
{
    if (!translator)
        return;
    m_translators.insert(m_translators.begin(), translator);
    nDebug() << "Translator installed";
}

void NCoreApplication::removeTranslator(NTranslator* translator)
{
    auto it = std::find(m_translators.begin(), m_translators.end(), translator);
    if (it != m_translators.end()) {
        m_translators.erase(it);
        nDebug() << "Translator removed";
    }
}

NString NCoreApplication::translate(const char* context,
    const char* sourceText,
    const char* disambiguation,
    int n) const
{
    for (auto* translator : m_translators) {
        NString translated = translator->translate(context, sourceText, disambiguation, n);
        if (!translated.isEmpty()) {
            return translated;
        }
    }
    return NString(sourceText);
}

void NCoreApplication::postEvent(NEvent* event)
{
    if (!event)
        return;
    NEventLoop::instance().postEvent(event);
}

void NCoreApplication::sendEvent(NEvent* event)
{
    if (!event)
        return;
    notify(nullptr, event);
    delete event;
}

void NCoreApplication::removePostedEvents(NObject* receiver)
{
    (void)receiver;
    // TODO
}

void NCoreApplication::addStartupRunnable(Runnable fn)
{
    if (fn)
        m_startupRunnables.push_back(fn);
}

void NCoreApplication::addShutdownRunnable(Runnable fn)
{
    if (fn)
        m_shutdownRunnables.push_back(fn);
}

void NCoreApplication::msleep(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void NCoreApplication::usleep(uint32_t us)
{
    esp_rom_delay_us(us);
}

void NCoreApplication::event(NEvent* event)
{
    if (!event)
        return;
    switch (event->type()) {
    case NEvent::Timer:
        timerEvent(static_cast<NTimerEvent*>(event));
        break;
    default:
        break;
    }
}

void NCoreApplication::timerEvent(NTimerEvent* event)
{
    (void)event;
}

bool NCoreApplication::notify(NObject* receiver, NEvent* event)
{
    if (!receiver || !event)
        return false;
    receiver->event(event);
    return true;
}

void NCoreApplication::executeStartupRunnables()
{
    for (auto& fn : m_startupRunnables) {
        if (fn)
            fn();
    }
}

void NCoreApplication::executeShutdownRunnables()
{
    for (auto& fn : m_shutdownRunnables) {
        if (fn)
            fn();
    }
}
