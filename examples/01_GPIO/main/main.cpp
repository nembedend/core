// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include <ncoreapplication.h>
#include <ntimer.h>
#include <ndebug.h>

#include "gpioticker.h"


extern "C" void app_main(void)
{
    NCoreApplication::instance().init();

    NDebug::setGlobalLevel(NDebugLevel::Debug);

    GPIOTicker* ticker = new GPIOTicker();
    NTimer *tickTimer = new NTimer();

    connect(tickTimer, &NTimer::timeout, ticker, &GPIOTicker::tick);

    tickTimer->setInterval(1000);
    tickTimer->start();

    NCoreApplication::instance().exec();
}
