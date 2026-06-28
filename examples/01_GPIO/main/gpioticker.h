// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#ifndef GPIOTICKER_H
#define GPIOTICKER_H

#include "ngpiopin.h"

class GPIOTicker : public NObject
{
public:
    GPIOTicker();
    void tick();

private:
    NGpioPin m_led;
    bool m_state;
};

#endif // GPIOTICKER_H
