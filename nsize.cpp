// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nsize.h"

NSize::NSize()
    : m_width(0)
    , m_height(0)
{
}

NSize::NSize(int width, int height)
    : m_width(width)
    , m_height(height)
{
}

void NSize::setWidth(int width)
{
    m_width = width;
}

void NSize::setHeight(int height)
{
    m_height = height;
}

void NSize::set(int width, int height)
{
    m_width = width;
    m_height = height;
}

void NSize::scale(int width, int height, NAspectRatioMode mode)
{
    if (mode == NAspectRatioMode::Ignore) {
        m_width = width;
        m_height = height;
        return;
    }

    if (m_width == 0 || m_height == 0) {
        return;
    }

    double aspect = (double)m_width / m_height;
    double targetAspect = (double)width / height;

    if (mode == NAspectRatioMode::Keep) {
        if (targetAspect < aspect) {
            m_width = width;
            m_height = (int)(width / aspect);
        } else {
            m_height = height;
            m_width = (int)(height * aspect);
        }
    } else if (mode == NAspectRatioMode::KeepByExpanding) {
        if (targetAspect > aspect) {
            m_width = width;
            m_height = (int)(width / aspect);
        } else {
            m_height = height;
            m_width = (int)(height * aspect);
        }
    }
}

void NSize::scale(const NSize& size, NAspectRatioMode mode)
{
    scale(size.width(), size.height(), mode);
}

void NSize::transpose()
{
    int temp = m_width;
    m_width = m_height;
    m_height = temp;
}

bool NSize::isEmpty() const
{
    return m_width <= 0 || m_height <= 0;
}

bool NSize::isValid() const
{
    return m_width > 0 && m_height > 0;
}

bool NSize::isNull() const
{
    return m_width == 0 && m_height == 0;
}

bool NSize::operator==(const NSize& other) const
{
    return m_width == other.m_width && m_height == other.m_height;
}

bool NSize::operator!=(const NSize& other) const
{
    return !(*this == other);
}

NSize NSize::operator+(const NSize& other) const
{
    return NSize(m_width + other.m_width, m_height + other.m_height);
}

NSize NSize::operator-(const NSize& other) const
{
    return NSize(m_width - other.m_width, m_height - other.m_height);
}

NSize NSize::operator*(int factor) const
{
    return NSize(m_width * factor, m_height * factor);
}

NSize NSize::operator/(int divisor) const
{
    return NSize(m_width / divisor, m_height / divisor);
}

NSize& NSize::operator+=(const NSize& other)
{
    m_width += other.m_width;
    m_height += other.m_height;
    return *this;
}

NSize& NSize::operator-=(const NSize& other)
{
    m_width -= other.m_width;
    m_height -= other.m_height;
    return *this;
}

NSize& NSize::operator*=(int factor)
{
    m_width *= factor;
    m_height *= factor;
    return *this;
}

NSize& NSize::operator/=(int divisor)
{
    m_width /= divisor;
    m_height /= divisor;
    return *this;
}

NSize NSize::bounded(const NSize& size, const NSize& maxSize)
{
    return NSize(
        size.width() > maxSize.width() ? maxSize.width() : size.width(),
        size.height() > maxSize.height() ? maxSize.height() : size.height());
}

NSize NSize::expanded(const NSize& size, const NSize& minSize)
{
    return NSize(
        size.width() < minSize.width() ? minSize.width() : size.width(),
        size.height() < minSize.height() ? minSize.height() : size.height());
}
