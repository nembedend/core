// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nrect.h"
#include <algorithm>

NRect::NRect()
    : m_x(0)
    , m_y(0)
    , m_width(0)
    , m_height(0)
{
}

NRect::NRect(int x, int y, int width, int height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
{
}

NRect::NRect(const NPoint& topLeft, const NPoint& bottomRight)
    : m_x(topLeft.x())
    , m_y(topLeft.y())
    , m_width(bottomRight.x() - topLeft.x() + 1)
    , m_height(bottomRight.y() - topLeft.y() + 1)
{
}

NRect::NRect(const NPoint& topLeft, const NSize& size)
    : m_x(topLeft.x())
    , m_y(topLeft.y())
    , m_width(size.width())
    , m_height(size.height())
{
}

void NRect::setLeft(int left)
{
    int right = this->right();
    m_x = left;
    m_width = right - left + 1;
    if (m_width < 0) {
        m_width = 0;
    }
}

void NRect::setTop(int top)
{
    int bottom = this->bottom();
    m_y = top;
    m_height = bottom - top + 1;
    if (m_height < 0) {
        m_height = 0;
    }
}

void NRect::setRight(int right)
{
    m_width = right - m_x + 1;
    if (m_width < 0) {
        m_width = 0;
    }
}

void NRect::setBottom(int bottom)
{
    m_height = bottom - m_y + 1;
    if (m_height < 0) {
        m_height = 0;
    }
}

void NRect::setTopLeft(const NPoint& p)
{
    setLeft(p.x());
    setTop(p.y());
}

void NRect::setTopRight(const NPoint& p)
{
    setRight(p.x());
    setTop(p.y());
}

void NRect::setBottomLeft(const NPoint& p)
{
    setLeft(p.x());
    setBottom(p.y());
}

void NRect::setBottomRight(const NPoint& p)
{
    setRight(p.x());
    setBottom(p.y());
}

void NRect::setRect(int x, int y, int width, int height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void NRect::setSize(const NSize& size)
{
    m_width = size.width();
    m_height = size.height();
}

bool NRect::isEmpty() const
{
    return m_width <= 0 || m_height <= 0;
}

bool NRect::isValid() const
{
    return m_width > 0 && m_height > 0;
}

bool NRect::isNull() const
{
    return m_width == 0 && m_height == 0;
}

void NRect::moveTo(int x, int y)
{
    m_x = x;
    m_y = y;
}

void NRect::moveTo(const NPoint& p)
{
    m_x = p.x();
    m_y = p.y();
}

void NRect::moveLeft(int x)
{
    m_x = x;
}

void NRect::moveTop(int y)
{
    m_y = y;
}

void NRect::moveRight(int x)
{
    m_x = x - m_width + 1;
}

void NRect::moveBottom(int y)
{
    m_y = y - m_height + 1;
}

void NRect::moveCenter(const NPoint& p)
{
    m_x = p.x() - m_width / 2;
    m_y = p.y() - m_height / 2;
}

void NRect::translate(int dx, int dy)
{
    m_x += dx;
    m_y += dy;
}

void NRect::translate(const NPoint& offset)
{
    m_x += offset.x();
    m_y += offset.y();
}

void NRect::adjust(int dx1, int dy1, int dx2, int dy2)
{
    m_x += dx1;
    m_y += dy1;
    m_width += dx2 - dx1;
    m_height += dy2 - dy1;

    if (m_width < 0) {
        m_width = 0;
    }
    if (m_height < 0) {
        m_height = 0;
    }
}

bool NRect::contains(int x, int y) const
{
    return x >= m_x && x < m_x + m_width && y >= m_y && y < m_y + m_height;
}

bool NRect::contains(const NPoint& p) const
{
    return contains(p.x(), p.y());
}

bool NRect::contains(const NRect& rect) const
{
    return rect.m_x >= m_x && rect.m_y >= m_y && rect.m_x + rect.m_width <= m_x + m_width && rect.m_y + rect.m_height <= m_y + m_height;
}

bool NRect::intersects(const NRect& rect) const
{
    return !(rect.m_x > m_x + m_width || rect.m_x + rect.m_width < m_x || rect.m_y > m_y + m_height || rect.m_y + rect.m_height < m_y);
}

NRect NRect::intersected(const NRect& rect) const
{
    int x1 = std::max(m_x, rect.m_x);
    int y1 = std::max(m_y, rect.m_y);
    int x2 = std::min(m_x + m_width, rect.m_x + rect.m_width);
    int y2 = std::min(m_y + m_height, rect.m_y + rect.m_height);

    if (x2 <= x1 || y2 <= y1) {
        return NRect();
    }

    return NRect(x1, y1, x2 - x1, y2 - y1);
}

NRect NRect::united(const NRect& rect) const
{
    if (isEmpty()) {
        return rect;
    }
    if (rect.isEmpty()) {
        return *this;
    }

    int x1 = std::min(m_x, rect.m_x);
    int y1 = std::min(m_y, rect.m_y);
    int x2 = std::max(m_x + m_width, rect.m_x + rect.m_width);
    int y2 = std::max(m_y + m_height, rect.m_y + rect.m_height);

    return NRect(x1, y1, x2 - x1, y2 - y1);
}

bool NRect::operator==(const NRect& other) const
{
    return m_x == other.m_x && m_y == other.m_y && m_width == other.m_width && m_height == other.m_height;
}

bool NRect::operator!=(const NRect& other) const
{
    return !(*this == other);
}

NRect NRect::normalized(const NRect& rect)
{
    if (rect.m_width >= 0 && rect.m_height >= 0) {
        return rect;
    }

    int x = rect.m_x;
    int y = rect.m_y;
    int w = rect.m_width;
    int h = rect.m_height;

    if (w < 0) {
        x += w;
        w = -w;
    }
    if (h < 0) {
        y += h;
        h = -h;
    }

    return NRect(x, y, w, h);
}
