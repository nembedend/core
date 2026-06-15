// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nstring.h"
#include <cstdint>
#include <cstring>

#ifndef N_COLOR_SWAP_BYTES
#define N_COLOR_SWAP_BYTES 0
#endif


class NColor {
public:
    static const NColor Invalid;
    static const NColor Transparent;

    static const NColor Black;
    static const NColor White;
    static const NColor Red;
    static const NColor Green;
    static const NColor Blue;
    static const NColor Cyan;
    static const NColor Magenta;
    static const NColor Yellow;
    static const NColor Gray;
    static const NColor DarkGray;
    static const NColor LightGray;

    NColor();
    NColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255);
    explicit NColor(uint32_t rgb);
    explicit NColor(uint16_t rgb565);
    explicit NColor(const NString& name);

    NColor(const NColor& other) = default;
    NColor& operator=(const NColor& other) = default;

    uint8_t red() const { return r; }
    uint8_t green() const { return g; }
    uint8_t blue() const { return b; }
    uint8_t alpha() const { return a; }

    void setRed(uint8_t red) { r = red; }
    void setGreen(uint8_t green) { g = green; }
    void setBlue(uint8_t blue) { b = blue; }
    void setAlpha(uint8_t alpha) { a = alpha; }

    void setRgb(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255);
    void setRgb(uint32_t rgb);

    uint32_t toRgb() const;
    uint32_t toArgb() const;
    uint32_t toRgba() const;

    uint16_t toRgb565() const;
    void fromRgb565(uint16_t rgb565);

    uint8_t toGray() const;

    static NColor fromRgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    static NColor fromRgba(uint32_t rgba);
    static NColor fromArgb(uint32_t argb);
    static NColor fromRgb565Value(uint16_t rgb565);
    static NColor fromHsv(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t alpha = 255);
    static NColor fromHsl(uint16_t hue, uint8_t saturation, uint8_t lightness, uint8_t alpha = 255);
    static NColor fromString(const NString& name);

    NColor lighter(int factor = 150) const;
    NColor darker(int factor = 150) const;
    NColor withAlpha(uint8_t alpha) const;

    bool isValid() const;
    bool isTransparent() const { return a == 0; }
    bool isOpaque() const { return a == 255; }

    NString name() const;
    NString toHexString(bool withAlpha = false) const;

    void toHsv(uint16_t& hue, uint8_t& saturation, uint8_t& value) const;
    void toHsl(uint16_t& hue, uint8_t& saturation, uint8_t& lightness) const;

    bool operator==(const NColor& other) const;
    bool operator!=(const NColor& other) const;

    static bool isValidColorName(const NString& name);

private:
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    void parseHex(const char* hex);
    void parseNamedColor(const NString& name);
};

inline NColor::NColor()
    : r(0), g(0), b(0), a(255)
{
}

inline NColor::NColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
    : r(red), g(green), b(blue), a(alpha)
{
}

inline NColor::NColor(uint32_t rgb)
    : r((rgb >> 16) & 0xFF)
    , g((rgb >> 8) & 0xFF)
    , b(rgb & 0xFF)
    , a(255)
{
}

inline void NColor::setRgb(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    r = red;
    g = green;
    b = blue;
    a = alpha;
}

inline void NColor::setRgb(uint32_t rgb)
{
    r = (rgb >> 16) & 0xFF;
    g = (rgb >> 8) & 0xFF;
    b = rgb & 0xFF;
}

inline uint32_t NColor::toRgb() const
{
    return (r << 16) | (g << 8) | b;
}

inline uint32_t NColor::toArgb() const
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

inline uint32_t NColor::toRgba() const
{
    return (r << 24) | (g << 16) | (b << 8) | a;
}

inline uint16_t NColor::toRgb565() const
{
    uint16_t c =
        ((r & 0xF8) << 8) |
        ((g & 0xFC) << 3) |
        ((b & 0xF8) >> 3);

#if N_COLOR_SWAP_BYTES
    c = (c >> 8) | (c << 8);
#endif

    return c;
}

inline void NColor::fromRgb565(uint16_t rgb565)
{
    r = ((rgb565 >> 11) & 0x1F) << 3;
    g = ((rgb565 >> 5) & 0x3F) << 2;
    b = ((rgb565 >> 0) & 0x1F) << 3;
    a = 255;
}

inline uint8_t NColor::toGray() const
{
    return (uint8_t)((r * 77 + g * 150 + b * 29) >> 8);
}

inline NColor NColor::fromRgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return NColor(r, g, b, a);
}

inline NColor NColor::fromRgba(uint32_t rgba)
{
    return NColor((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, rgba & 0xFF);
}

inline NColor NColor::fromArgb(uint32_t argb)
{
    return NColor((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, (argb >> 24) & 0xFF);
}

inline NColor NColor::fromRgb565Value(uint16_t rgb565)
{
    NColor c;
    c.fromRgb565(rgb565);
    return c;
}

inline NColor NColor::withAlpha(uint8_t alpha) const
{
    return NColor(r, g, b, alpha);
}

inline bool NColor::isValid() const
{
    return true;  // All uint8_t values are valid (0-255)
}

inline bool NColor::operator==(const NColor& other) const
{
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

inline bool NColor::operator!=(const NColor& other) const
{
    return !(*this == other);
}
