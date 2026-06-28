// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include <ndebug.h>
#include "gpioticker.h"

GPIOTicker::GPIOTicker() : NObject()
    , m_led(GPIO_NUM_6)
    , m_state(false)
{
    m_led.setMode(NGpioPin::Mode::Output);
}

void GPIOTicker::tick()
{
    nDebug() << "TICK!";

    m_led.write(m_state);
    m_state = !m_state;
}
