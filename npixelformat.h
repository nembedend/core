// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

enum class NPixelFormat
{
    Invalid = 0,

    RGB565,
    BGR565,
    RGB565_Swapped,
    BGR565_Swapped,

    RGB888,
    BGR888,
    ARGB8888
};
