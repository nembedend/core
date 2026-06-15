// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nimage.h"
#include "ndebug.h"
#include <algorithm>
#include <cmath>

NImage::NImage()
    : m_data(nullptr)
    , m_width(0)
    , m_height(0)
    , m_bytesPerLine(0)
    , m_format(NPixelFormat::Invalid)
    , m_refCount(nullptr)
{
}

NImage::NImage(int width, int height, NPixelFormat format)
    : m_width(width)
    , m_height(height)
    , m_format(format)
    , m_refCount(nullptr)
{
    if (width > 0 && height > 0 && format != NPixelFormat::Invalid) {
        int bpp = bytesPerPixel();
        if (bpp > 0) {
            m_bytesPerLine = width * bpp;
            m_bytesPerLine = (m_bytesPerLine + DEFAULT_ALIGNMENT - 1) & ~(DEFAULT_ALIGNMENT - 1);
            allocate();
        }
    } else {
        m_data = nullptr;
        m_bytesPerLine = 0;
    }
}

NImage::NImage(const NSize& size, NPixelFormat format)
    : NImage(size.width(), size.height(), format)
{
}

NImage::NImage(const NImage& other)
    : m_data(other.m_data)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_bytesPerLine(other.m_bytesPerLine)
    , m_format(other.m_format)
    , m_refCount(other.m_refCount)
{
    if (m_refCount) {
        (*m_refCount)++;
    }
}

NImage::NImage(NImage&& other) noexcept
    : m_data(other.m_data)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_bytesPerLine(other.m_bytesPerLine)
    , m_format(other.m_format)
    , m_refCount(other.m_refCount)
{
    other.m_data = nullptr;
    other.m_width = 0;
    other.m_height = 0;
    other.m_bytesPerLine = 0;
    other.m_format = NPixelFormat::Invalid;
    other.m_refCount = nullptr;
}

NImage::~NImage()
{
    freeData();
}

NImage& NImage::operator=(const NImage& other)
{
    if (this != &other) {
        freeData();
        m_data = other.m_data;
        m_width = other.m_width;
        m_height = other.m_height;
        m_bytesPerLine = other.m_bytesPerLine;
        m_format = other.m_format;
        m_refCount = other.m_refCount;
        if (m_refCount) {
            (*m_refCount)++;
        }
    }
    return *this;
}

NImage& NImage::operator=(NImage&& other) noexcept
{
    if (this != &other) {
        freeData();
        m_data = other.m_data;
        m_width = other.m_width;
        m_height = other.m_height;
        m_bytesPerLine = other.m_bytesPerLine;
        m_format = other.m_format;
        m_refCount = other.m_refCount;
        other.m_data = nullptr;
        other.m_width = 0;
        other.m_height = 0;
        other.m_bytesPerLine = 0;
        other.m_format = NPixelFormat::Invalid;
        other.m_refCount = nullptr;
    }
    return *this;
}

int NImage::bytesPerPixel() const
{
    switch (m_format) {
    case NPixelFormat::RGB565:
        return 2;
    case NPixelFormat::Gray8:
    case NPixelFormat::Indexed8:
        return 1;
    case NPixelFormat::RGB888:
        return 3;
    case NPixelFormat::ARGB8888:
        return 4;
    default:
        return 0;
    }
}

void NImage::allocate()
{
    size_t size = m_bytesPerLine * m_height;
    if (size > 0) {
        m_data = (uint8_t*)malloc(size);
        if (m_data) {
            memset(m_data, 0, size);
            m_refCount = new int(1);
        }
    }
}

void NImage::freeData()
{
    if (m_refCount) {
        (*m_refCount)--;
        if (*m_refCount == 0) {
            if (m_data)
                free(m_data);
            delete m_refCount;
        }
        m_refCount = nullptr;
        m_data = nullptr;
    }
}

void NImage::detach()
{
    if (m_refCount && *m_refCount > 1) {
        NImage copy(*this);
        freeData();
        m_data = copy.m_data;
        m_width = copy.m_width;
        m_height = copy.m_height;
        m_bytesPerLine = copy.m_bytesPerLine;
        m_format = copy.m_format;
        m_refCount = copy.m_refCount;
        if (m_refCount)
            (*m_refCount)++;
        copy.m_refCount = nullptr;
        copy.m_data = nullptr;
    }
}

NColor NImage::pixel(int x, int y) const
{
    if (!m_data || x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return NColor();
    }
    const uint8_t* ptr = scanLine(y) + x * bytesPerPixel();
    switch (m_format) {
    case NPixelFormat::RGB565: {
        uint16_t rgb565 = *(uint16_t*)ptr;
        return NColor(rgb565);
    }
    case NPixelFormat::RGB888: {
        return NColor(ptr[0], ptr[1], ptr[2]);
    }
    case NPixelFormat::ARGB8888: {
        return NColor(ptr[1], ptr[2], ptr[3], ptr[0]);
    }
    case NPixelFormat::Gray8: {
        uint8_t g = ptr[0];
        return NColor(g, g, g);
    }
    default:
        return NColor();
    }
}

void NImage::setPixel(int x, int y, const NColor& color)
{
    if (!m_data || x < 0 || x >= m_width || y < 0 || y >= m_height)
        return;
    detach();
    uint8_t* ptr = scanLine(y) + x * bytesPerPixel();
    switch (m_format) {
    case NPixelFormat::RGB565: {
        uint16_t rgb565 = color.toRgb565();
        *(uint16_t*)ptr = rgb565;
        break;
    }
    case NPixelFormat::RGB888: {
        ptr[0] = color.red();
        ptr[1] = color.green();
        ptr[2] = color.blue();
        break;
    }
    case NPixelFormat::ARGB8888: {
        ptr[0] = color.alpha();
        ptr[1] = color.red();
        ptr[2] = color.green();
        ptr[3] = color.blue();
        break;
    }
    case NPixelFormat::Gray8: {
        ptr[0] = color.toGray();
        break;
    }
    default:
        break;
    }
}

void NImage::setPixel(int x, int y, uint16_t rgb565)
{
    setPixel(x, y, NColor(rgb565));
}

void NImage::fill(const NColor& color)
{
    if (!m_data)
        return;
    detach();
    fillRect(NRect(0, 0, m_width, m_height), color);
}

void NImage::fill(uint8_t value)
{
    if (!m_data)
        return;
    detach();
    memset(m_data, value, sizeInBytes());
}

void NImage::fillRect(const NRect& rect, const NColor& color)
{
    if (!m_data)
        return;
    NRect r = rect.intersected(NRect(0, 0, m_width, m_height));
    if (r.isEmpty())
        return;
    detach();
    int bpp = bytesPerPixel();
    for (int y = r.y(); y < r.y() + r.height(); ++y) {
        uint8_t* ptr = scanLine(y) + r.x() * bpp;
        for (int x = 0; x < r.width(); ++x) {
            switch (m_format) {
            case NPixelFormat::RGB565: {
                *(uint16_t*)ptr = color.toRgb565();
                ptr += 2;
                break;
            }
            case NPixelFormat::RGB888: {
                ptr[0] = color.red();
                ptr[1] = color.green();
                ptr[2] = color.blue();
                ptr += 3;
                break;
            }
            case NPixelFormat::ARGB8888: {
                ptr[0] = color.alpha();
                ptr[1] = color.red();
                ptr[2] = color.green();
                ptr[3] = color.blue();
                ptr += 4;
                break;
            }
            case NPixelFormat::Gray8: {
                ptr[0] = color.toGray();
                ptr += 1;
                break;
            }
            default:
                break;
            }
        }
    }
}

void NImage::fillRect(const NRect& rect, uint16_t rgb565)
{
    fillRect(rect, NColor(rgb565));
}

void NImage::drawPixel(int x, int y, const NColor& color)
{
    setPixel(x, y, color);
}

void NImage::drawLine(int x1, int y1, int x2, int y2, const NColor& color)
{
    if (abs(y2 - y1) < abs(x2 - x1)) {
        if (x1 > x2)
            drawLineLow(x2, y2, x1, y1, color);
        else
            drawLineLow(x1, y1, x2, y2, color);
    } else {
        if (y1 > y2)
            drawLineHigh(x2, y2, x1, y1, color);
        else
            drawLineHigh(x1, y1, x2, y2, color);
    }
}

void NImage::drawLineLow(int x1, int y1, int x2, int y2, const NColor& color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int yi = 1;
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int D = (2 * dy) - dx;
    int y = y1;
    for (int x = x1; x <= x2; x++) {
        drawPixel(x, y, color);
        if (D > 0) {
            y += yi;
            D += 2 * (dy - dx);
        } else {
            D += 2 * dy;
        }
    }
}

void NImage::drawLineHigh(int x1, int y1, int x2, int y2, const NColor& color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int xi = 1;
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }
    int D = (2 * dx) - dy;
    int x = x1;
    for (int y = y1; y <= y2; y++) {
        drawPixel(x, y, color);
        if (D > 0) {
            x += xi;
            D += 2 * (dx - dy);
        } else {
            D += 2 * dx;
        }
    }
}

void NImage::drawRect(const NRect& rect, const NColor& color)
{
    drawLine(rect.x(), rect.y(), rect.x() + rect.width() - 1, rect.y(), color);
    drawLine(rect.x(), rect.y() + rect.height() - 1, rect.x() + rect.width() - 1, rect.y() + rect.height() - 1, color);
    drawLine(rect.x(), rect.y(), rect.x(), rect.y() + rect.height() - 1, color);
    drawLine(rect.x() + rect.width() - 1, rect.y(), rect.x() + rect.width() - 1, rect.y() + rect.height() - 1, color);
}

void NImage::drawRect(int x, int y, int w, int h, const NColor& color)
{
    drawRect(NRect(x, y, w, h), color);
}

void NImage::drawCircle(int cx, int cy, int radius, const NColor& color)
{
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    auto drawPoints = [&](int x, int y) {
        drawPixel(cx + x, cy + y, color);
        drawPixel(cx - x, cy + y, color);
        drawPixel(cx + x, cy - y, color);
        drawPixel(cx - x, cy - y, color);
        drawPixel(cx + y, cy + x, color);
        drawPixel(cx - y, cy + x, color);
        drawPixel(cx + y, cy - x, color);
        drawPixel(cx - y, cy - x, color);
    };
    while (y >= x) {
        drawPoints(x, y);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

NImage NImage::scaled(int newWidth, int newHeight) const
{
    if (!isValid() || newWidth <= 0 || newHeight <= 0)
        return NImage();
    NImage result(newWidth, newHeight, m_format);
    float xRatio = (float)m_width / newWidth;
    float yRatio = (float)m_height / newHeight;
    for (int y = 0; y < newHeight; y++) {
        int srcY = (int)(y * yRatio);
        for (int x = 0; x < newWidth; x++) {
            int srcX = (int)(x * xRatio);
            result.setPixel(x, y, pixel(srcX, srcY));
        }
    }
    return result;
}

NImage NImage::rotated(int degrees) const
{
    degrees = degrees % 360;
    if (degrees < 0)
        degrees += 360;
    float rad = degrees * M_PI / 180.0f;
    float sinVal = fabs(sin(rad));
    float cosVal = fabs(cos(rad));
    int newWidth = (int)(m_width * cosVal + m_height * sinVal);
    int newHeight = (int)(m_width * sinVal + m_height * cosVal);
    NImage result(newWidth, newHeight, m_format);
    float cx = newWidth / 2.0f;
    float cy = newHeight / 2.0f;
    float srcCx = m_width / 2.0f;
    float srcCy = m_height / 2.0f;
    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {
            float dx = x - cx;
            float dy = y - cy;
            int srcX = (int)(dx * cosVal + dy * sinVal + srcCx);
            int srcY = (int)(dy * cosVal - dx * sinVal + srcCy);
            if (srcX >= 0 && srcX < m_width && srcY >= 0 && srcY < m_height) {
                result.setPixel(x, y, pixel(srcX, srcY));
            }
        }
    }
    return result;
}

NImage NImage::mirrored(bool horizontal, bool vertical) const
{
    if (!isValid())
        return NImage();
    NImage result(m_width, m_height, m_format);
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            int srcX = horizontal ? (m_width - 1 - x) : x;
            int srcY = vertical ? (m_height - 1 - y) : y;
            result.setPixel(x, y, pixel(srcX, srcY));
        }
    }
    return result;
}

NImage NImage::copy(const NRect& rect) const
{
    NRect r = rect.isValid() ? rect : NRect(0, 0, m_width, m_height);
    r = r.intersected(NRect(0, 0, m_width, m_height));
    if (r.isEmpty())
        return NImage();
    NImage result(r.width(), r.height(), m_format);
    for (int y = 0; y < r.height(); y++) {
        memcpy(result.scanLine(y), scanLine(r.y() + y) + r.x() * bytesPerPixel(),
            r.width() * bytesPerPixel());
    }
    return result;
}

NImage NImage::convertedTo(NPixelFormat newFormat) const
{
    if (!isValid() || newFormat == m_format)
        return *this;
    NImage result(m_width, m_height, newFormat);
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            result.setPixel(x, y, pixel(x, y));
        }
    }
    return result;
}

void NImage::convertTo(NPixelFormat newFormat)
{
    if (!isValid() || newFormat == m_format)
        return;
    *this = convertedTo(newFormat);
}

NImage NImage::fromRGB565(const uint8_t* data, int width, int height, int bytesPerLine)
{
    NImage result(width, height, NPixelFormat::RGB565);
    int srcStride = (bytesPerLine > 0) ? bytesPerLine : width * 2;
    for (int y = 0; y < height; y++) {
        memcpy(result.scanLine(y), data + y * srcStride, width * 2);
    }
    return result;
}

NImage NImage::fromARGB8888(const uint8_t* data, int width, int height, int bytesPerLine)
{
    NImage result(width, height, NPixelFormat::ARGB8888);
    int srcStride = (bytesPerLine > 0) ? bytesPerLine : width * 4;
    for (int y = 0; y < height; y++) {
        memcpy(result.scanLine(y), data + y * srcStride, width * 4);
    }
    return result;
}

NImage NImage::fromGray8(const uint8_t* data, int width, int height, int bytesPerLine)
{
    NImage result(width, height, NPixelFormat::Gray8);
    int srcStride = (bytesPerLine > 0) ? bytesPerLine : width;
    for (int y = 0; y < height; y++) {
        memcpy(result.scanLine(y), data + y * srcStride, width);
    }
    return result;
}

void NImage::toRGB565(uint8_t* output, int bytesPerLine) const
{
    if (!isValid() || !output)
        return;
    int dstStride = (bytesPerLine > 0) ? bytesPerLine : m_width * 2;
    for (int y = 0; y < m_height; y++) {
        uint16_t* dst = (uint16_t*)(output + y * dstStride);
        for (int x = 0; x < m_width; x++) {
            dst[x] = pixel(x, y).toRgb565();
        }
    }
}

void NImage::toARGB8888(uint8_t* output, int bytesPerLine) const
{
    if (!isValid() || !output)
        return;
    int dstStride = (bytesPerLine > 0) ? bytesPerLine : m_width * 4;
    for (int y = 0; y < m_height; y++) {
        uint32_t* dst = (uint32_t*)(output + y * dstStride);
        for (int x = 0; x < m_width; x++) {
            NColor c = pixel(x, y);
            dst[x] = c.toArgb();
        }
    }
}

void NImage::toGray8(uint8_t* output, int bytesPerLine) const
{
    if (!isValid() || !output)
        return;
    int dstStride = (bytesPerLine > 0) ? bytesPerLine : m_width;
    for (int y = 0; y < m_height; y++) {
        uint8_t* dst = output + y * dstStride;
        for (int x = 0; x < m_width; x++) {
            dst[x] = pixel(x, y).toGray();
        }
    }
}

void NImage::blend(const NImage& source, const NPoint& position, uint8_t alpha)
{
    if (!isValid() || !source.isValid())
        return;
    NRect srcRect(0, 0, source.width(), source.height());
    blend(source, srcRect, position);
}

void NImage::blend(const NImage& source, const NRect& sourceRect, const NPoint& destPosition)
{
    if (!isValid() || !source.isValid())
        return;
    detach();
    NRect srcRect = sourceRect.intersected(NRect(0, 0, source.width(), source.height()));
    NRect dstRect(destPosition.x(), destPosition.y(), srcRect.width(), srcRect.height());
    dstRect = dstRect.intersected(NRect(0, 0, m_width, m_height));
    if (srcRect.isEmpty() || dstRect.isEmpty())
        return;
    for (int y = 0; y < dstRect.height(); y++) {
        for (int x = 0; x < dstRect.width(); x++) {
            NColor srcColor = source.pixel(srcRect.x() + x, srcRect.y() + y);
            NColor dstColor = pixel(dstRect.x() + x, dstRect.y() + y);
            uint8_t srcAlpha = srcColor.alpha();
            uint8_t dstAlpha = dstColor.alpha();
            uint8_t finalAlpha = (srcAlpha * 255 + dstAlpha * (255 - srcAlpha)) / 255;
            if (finalAlpha > 0) {
                uint8_t r = (srcColor.red() * srcAlpha + dstColor.red() * (255 - srcAlpha)) / 255;
                uint8_t g = (srcColor.green() * srcAlpha + dstColor.green() * (255 - srcAlpha)) / 255;
                uint8_t b = (srcColor.blue() * srcAlpha + dstColor.blue() * (255 - srcAlpha)) / 255;
                setPixel(dstRect.x() + x, dstRect.y() + y, NColor(r, g, b, finalAlpha));
            }
        }
    }
}

void NImage::adjustBrightness(int delta)
{
    if (!isValid())
        return;
    detach();
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            NColor c = pixel(x, y);
            int r = c.red() + delta;
            int g = c.green() + delta;
            int b = c.blue() + delta;
            r = std::max(0, std::min(255, r));
            g = std::max(0, std::min(255, g));
            b = std::max(0, std::min(255, b));
            setPixel(x, y, NColor(r, g, b, c.alpha()));
        }
    }
}

void NImage::adjustContrast(int factor)
{
    if (!isValid())
        return;
    detach();
    float f = factor / 100.0f;
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            NColor c = pixel(x, y);
            int r = (int)((c.red() - 128) * f + 128);
            int g = (int)((c.green() - 128) * f + 128);
            int b = (int)((c.blue() - 128) * f + 128);
            r = std::max(0, std::min(255, r));
            g = std::max(0, std::min(255, g));
            b = std::max(0, std::min(255, b));
            setPixel(x, y, NColor(r, g, b, c.alpha()));
        }
    }
}

void NImage::adjustGamma(float gamma)
{
    if (!isValid() || gamma <= 0)
        return;
    detach();
    float invGamma = 1.0f / gamma;
    uint8_t gammaLUT[256];
    for (int i = 0; i < 256; i++) {
        gammaLUT[i] = (uint8_t)(powf(i / 255.0f, invGamma) * 255.0f);
    }
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            NColor c = pixel(x, y);
            setPixel(x, y, NColor(gammaLUT[c.red()], gammaLUT[c.green()], gammaLUT[c.blue()], c.alpha()));
        }
    }
}
