// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "ncolor.h"
#include "nrect.h"
#include "npoint.h"
#include "nsize.h"
#include "nbytearray.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

enum class NPixelFormat : uint8_t {
    Invalid = 0,
    RGB565 = 1,
    RGB888 = 2,
    ARGB8888 = 3,
    Gray8 = 4,
    Indexed8 = 5
};

class NImage {
public:
    NImage();
    NImage(int width, int height, NPixelFormat format = NPixelFormat::RGB565);
    NImage(const NSize& size, NPixelFormat format = NPixelFormat::RGB565);
    NImage(const NImage& other);
    NImage(NImage&& other) noexcept;
    ~NImage();

    NImage& operator=(const NImage& other);
    NImage& operator=(NImage&& other) noexcept;

    int width() const { return m_width; }
    int height() const { return m_height; }
    NSize size() const { return NSize(m_width, m_height); }
    NPixelFormat format() const { return m_format; }
    bool isValid() const { return m_data != nullptr && m_width > 0 && m_height > 0; }
    bool isNull() const { return m_data == nullptr; }

    uint8_t* bits() { return m_data; }
    const uint8_t* bits() const { return m_data; }
    uint8_t* scanLine(int y) { return m_data + y * m_bytesPerLine; }
    const uint8_t* scanLine(int y) const { return m_data + y * m_bytesPerLine; }

    int bytesPerLine() const { return m_bytesPerLine; }
    int bytesPerPixel() const;
    size_t sizeInBytes() const { return m_bytesPerLine * m_height; }

    NColor pixel(int x, int y) const;
    void setPixel(int x, int y, const NColor& color);
    void setPixel(int x, int y, uint16_t rgb565);

    void fill(const NColor& color);
    void fill(uint8_t value);
    void fillRect(const NRect& rect, const NColor& color);
    void fillRect(const NRect& rect, uint16_t rgb565);

    void drawPixel(int x, int y, const NColor& color);
    void drawLine(int x1, int y1, int x2, int y2, const NColor& color);
    void drawRect(const NRect& rect, const NColor& color);
    void drawRect(int x, int y, int w, int h, const NColor& color);
    void drawCircle(int cx, int cy, int radius, const NColor& color);

    NImage scaled(int newWidth, int newHeight) const;
    NImage rotated(int degrees) const;
    NImage mirrored(bool horizontal = true, bool vertical = false) const;
    NImage copy(const NRect& rect = NRect()) const;

    NImage convertedTo(NPixelFormat newFormat) const;
    void convertTo(NPixelFormat newFormat);

    static NImage fromRGB565(const uint8_t* data, int width, int height, int bytesPerLine = 0);
    static NImage fromARGB8888(const uint8_t* data, int width, int height, int bytesPerLine = 0);
    static NImage fromGray8(const uint8_t* data, int width, int height, int bytesPerLine = 0);

    void toRGB565(uint8_t* output, int bytesPerLine = 0) const;
    void toARGB8888(uint8_t* output, int bytesPerLine = 0) const;
    void toGray8(uint8_t* output, int bytesPerLine = 0) const;

    void blend(const NImage& source, const NPoint& position, uint8_t alpha = 255);
    void blend(const NImage& source, const NRect& sourceRect, const NPoint& destPosition);

    void adjustBrightness(int delta);
    void adjustContrast(int factor);
    void adjustGamma(float gamma);

    void detach();
    bool isDetached() const { return m_refCount && *m_refCount == 1; }

private:
    void allocate();
    void freeData();
    void copyData(const NImage& other);

    void drawLineLow(int x1, int y1, int x2, int y2, const NColor& color);
    void drawLineHigh(int x1, int y1, int x2, int y2, const NColor& color);

    uint8_t* m_data;
    int m_width;
    int m_height;
    int m_bytesPerLine;
    NPixelFormat m_format;
    int* m_refCount;

    static const int DEFAULT_ALIGNMENT = 4;
};
