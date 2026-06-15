// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "ncolor.h"
#include <cmath>
#include <cstdlib>

const NColor NColor::Invalid(0, 0, 0, 0);
const NColor NColor::Transparent(0, 0, 0, 0);

const NColor NColor::Black(0, 0, 0);
const NColor NColor::White(255, 255, 255);
const NColor NColor::Red(255, 0, 0);
const NColor NColor::Green(0, 255, 0);
const NColor NColor::Blue(0, 0, 255);
const NColor NColor::Cyan(0, 255, 255);
const NColor NColor::Magenta(255, 0, 255);
const NColor NColor::Yellow(255, 255, 0);
const NColor NColor::Gray(128, 128, 128);
const NColor NColor::DarkGray(64, 64, 64);
const NColor NColor::LightGray(192, 192, 192);

NColor::NColor(const NString& name)
    : r(0)
    , g(0)
    , b(0)
    , a(255)
{
    if (name.isEmpty())
        return;

    if (name[0] == '#') {
        parseHex(name.c_str());
    } else {
        parseNamedColor(name);
    }
}

NColor::NColor(uint16_t rgb565)
    : r(0)
    , g(0)
    , b(0)
    , a(255)
{
    fromRgb565(rgb565);
}

void NColor::parseHex(const char* hex)
{
    if (!hex)
        return;

    if (hex[0] == '#')
        hex++;

    int len = 0;
    while (hex[len] != '\0')
        len++;

    auto nibble = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return 0;
    };

    if (len == 3) {
        // #RGB
        r = nibble(hex[0]) * 17;
        g = nibble(hex[1]) * 17;
        b = nibble(hex[2]) * 17;
        a = 255;
    } else if (len == 4) {
        // #RGBA
        r = nibble(hex[0]) * 17;
        g = nibble(hex[1]) * 17;
        b = nibble(hex[2]) * 17;
        a = nibble(hex[3]) * 17;
    } else if (len == 6) {
        // #RRGGBB
        r = (nibble(hex[0]) << 4) | nibble(hex[1]);
        g = (nibble(hex[2]) << 4) | nibble(hex[3]);
        b = (nibble(hex[4]) << 4) | nibble(hex[5]);
        a = 255;
    } else if (len == 8) {
        // #RRGGBBAA
        r = (nibble(hex[0]) << 4) | nibble(hex[1]);
        g = (nibble(hex[2]) << 4) | nibble(hex[3]);
        b = (nibble(hex[4]) << 4) | nibble(hex[5]);
        a = (nibble(hex[6]) << 4) | nibble(hex[7]);
    }
}

void NColor::parseNamedColor(const NString& name)
{
    NString s = name.toLower();

    if (s == "black") {
        r = 0;
        g = 0;
        b = 0;
        a = 255;
    } else if (s == "white") {
        r = 255;
        g = 255;
        b = 255;
        a = 255;
    } else if (s == "red") {
        r = 255;
        g = 0;
        b = 0;
        a = 255;
    } else if (s == "green") {
        r = 0;
        g = 255;
        b = 0;
        a = 255;
    } else if (s == "blue") {
        r = 0;
        g = 0;
        b = 255;
        a = 255;
    } else if (s == "cyan") {
        r = 0;
        g = 255;
        b = 255;
        a = 255;
    } else if (s == "magenta") {
        r = 255;
        g = 0;
        b = 255;
        a = 255;
    } else if (s == "yellow") {
        r = 255;
        g = 255;
        b = 0;
        a = 255;
    } else if (s == "gray" || s == "grey") {
        r = 128;
        g = 128;
        b = 128;
        a = 255;
    } else if (s == "darkgray" || s == "darkgrey") {
        r = 64;
        g = 64;
        b = 64;
        a = 255;
    } else if (s == "lightgray" || s == "lightgrey") {
        r = 192;
        g = 192;
        b = 192;
        a = 255;
    } else if (s == "transparent") {
        r = 0;
        g = 0;
        b = 0;
        a = 0;
    } else {
        r = 0;
        g = 0;
        b = 0;
        a = 255;
    }
}

NColor NColor::lighter(int factor) const
{
    if (factor <= 0)
        return *this;
    if (factor > 255)
        factor = 255;

    int newR = r + ((255 - r) * factor) / 255;
    int newG = g + ((255 - g) * factor) / 255;
    int newB = b + ((255 - b) * factor) / 255;

    return NColor((uint8_t)newR, (uint8_t)newG, (uint8_t)newB, a);
}

NColor NColor::darker(int factor) const
{
    if (factor <= 0)
        return *this;
    if (factor > 255)
        factor = 255;

    int newR = (r * factor) / 255;
    int newG = (g * factor) / 255;
    int newB = (b * factor) / 255;

    return NColor((uint8_t)newR, (uint8_t)newG, (uint8_t)newB, a);
}

void NColor::toHsv(uint16_t& hue, uint8_t& saturation, uint8_t& value) const
{
    uint8_t max = r;
    if (g > max)
        max = g;
    if (b > max)
        max = b;
    value = max;

    uint8_t min = r;
    if (g < min)
        min = g;
    if (b < min)
        min = b;

    uint8_t delta = max - min;

    if (delta == 0) {
        hue = 0;
        saturation = 0;
        return;
    }

    saturation = (delta * 255) / max;

    int h = 0;
    if (max == r) {
        h = (g - b) * 60 / delta;
    } else if (max == g) {
        h = 120 + (b - r) * 60 / delta;
    } else {
        h = 240 + (r - g) * 60 / delta;
    }

    if (h < 0)
        h += 360;
    hue = (uint16_t)h;
}

void NColor::toHsl(uint16_t& hue, uint8_t& saturation, uint8_t& lightness) const
{
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float max = rf;
    if (gf > max)
        max = gf;
    if (bf > max)
        max = bf;

    float min = rf;
    if (gf < min)
        min = gf;
    if (bf < min)
        min = bf;

    float l = (max + min) / 2.0f;
    lightness = (uint8_t)(l * 255);

    float s = 0;
    if (max != min) {
        if (l <= 0.5f) {
            s = (max - min) / (max + min);
        } else {
            s = (max - min) / (2.0f - max - min);
        }
    }
    saturation = (uint8_t)(s * 255);

    float h = 0;
    if (max == rf) {
        h = (gf - bf) / (max - min);
        if (gf < bf)
            h += 6.0f;
    } else if (max == gf) {
        h = (bf - rf) / (max - min) + 2.0f;
    } else {
        h = (rf - gf) / (max - min) + 4.0f;
    }
    h *= 60.0f;
    if (h < 0)
        h += 360.0f;

    hue = (uint16_t)h;
}

NColor NColor::fromHsv(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t alpha)
{
    uint8_t r, g, b;
    uint16_t h = hue % 360;

    if (saturation == 0) {
        r = g = b = value;
    } else {
        uint8_t region = h / 60;
        uint8_t remainder = h % 60;

        uint16_t p = value * (255 - saturation) / 255;
        uint16_t q = value * (255 - (saturation * remainder) / 60) / 255;
        uint16_t t = value * (255 - (saturation * (60 - remainder)) / 60) / 255;

        switch (region) {
        case 0:
            r = value;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = value;
            b = p;
            break;
        case 2:
            r = p;
            g = value;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = value;
            break;
        case 4:
            r = t;
            g = p;
            b = value;
            break;
        default:
            r = value;
            g = p;
            b = q;
            break;
        }
    }

    return NColor(r, g, b, alpha);
}

NColor NColor::fromHsl(uint16_t hue, uint8_t saturation, uint8_t lightness, uint8_t alpha)
{
    float h = hue % 360;
    float s = saturation / 255.0f;
    float l = lightness / 255.0f;

    float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
    float hp = h / 60.0f;
    float x = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    float r1, g1, b1;
    int region = (int)hp;

    switch (region) {
    case 0:
        r1 = c;
        g1 = x;
        b1 = 0;
        break;
    case 1:
        r1 = x;
        g1 = c;
        b1 = 0;
        break;
    case 2:
        r1 = 0;
        g1 = c;
        b1 = x;
        break;
    case 3:
        r1 = 0;
        g1 = x;
        b1 = c;
        break;
    case 4:
        r1 = x;
        g1 = 0;
        b1 = c;
        break;
    default:
        r1 = c;
        g1 = 0;
        b1 = x;
        break;
    }

    return NColor(
        (uint8_t)((r1 + m) * 255),
        (uint8_t)((g1 + m) * 255),
        (uint8_t)((b1 + m) * 255),
        alpha);
}

NString NColor::name() const
{
    char buf[16];
    if (a == 255) {
        snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
    } else {
        snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", r, g, b, a);
    }
    return NString(buf);
}

NString NColor::toHexString(bool withAlpha) const
{
    char buf[16];
    if (withAlpha) {
        snprintf(buf, sizeof(buf), "%02X%02X%02X%02X", r, g, b, a);
    } else {
        snprintf(buf, sizeof(buf), "%02X%02X%02X", r, g, b);
    }
    return NString(buf);
}

bool NColor::isValidColorName(const NString& name)
{
    if (name.isEmpty())
        return false;
    if (name[0] == '#')
        return true;

    NString s = name.toLower();
    return (s == "black" || s == "white" || s == "red" || s == "green" || s == "blue" || s == "cyan" || s == "magenta" || s == "yellow" || s == "gray" || s == "grey" || s == "darkgray" || s == "darkgrey" || s == "lightgray" || s == "lightgrey" || s == "transparent");
}

NColor NColor::fromString(const NString& name)
{
    return NColor(name);
}
