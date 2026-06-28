// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "npoint.h"

NPoint::NPoint()
    : m_x(0)
    , m_y(0)
{
}

NPoint::NPoint(int x, int y)
    : m_x(x)
    , m_y(y)
{
}

void NPoint::setX(int x)
{
    m_x = x;
}

void NPoint::setY(int y)
{
    m_y = y;
}

void NPoint::set(int x, int y)
{
    m_x = x;
    m_y = y;
}

void NPoint::translate(int dx, int dy)
{
    m_x += dx;
    m_y += dy;
}

NPoint NPoint::transposed() const
{
    return NPoint(m_y, m_x);
}

int NPoint::manhattanLength() const
{
    return (m_x < 0 ? -m_x : m_x) + (m_y < 0 ? -m_y : m_y);
}

double NPoint::length() const
{
    return sqrt((double)(m_x * m_x + m_y * m_y));
}

int NPoint::dotProduct(const NPoint& other) const
{
    return m_x * other.m_x + m_y * other.m_y;
}

bool NPoint::operator==(const NPoint& other) const
{
    return m_x == other.m_x && m_y == other.m_y;
}

bool NPoint::operator!=(const NPoint& other) const
{
    return !(*this == other);
}

NPoint NPoint::operator-() const
{
    return NPoint(-m_x, -m_y);
}

NPoint NPoint::operator+(const NPoint& other) const
{
    return NPoint(m_x + other.m_x, m_y + other.m_y);
}

NPoint NPoint::operator-(const NPoint& other) const
{
    return NPoint(m_x - other.m_x, m_y - other.m_y);
}

NPoint NPoint::operator*(int factor) const
{
    return NPoint(m_x * factor, m_y * factor);
}

NPoint NPoint::operator/(int divisor) const
{
    return NPoint(m_x / divisor, m_y / divisor);
}

NPoint& NPoint::operator+=(const NPoint& other)
{
    m_x += other.m_x;
    m_y += other.m_y;
    return *this;
}

NPoint& NPoint::operator-=(const NPoint& other)
{
    m_x -= other.m_x;
    m_y -= other.m_y;
    return *this;
}

NPoint& NPoint::operator*=(int factor)
{
    m_x *= factor;
    m_y *= factor;
    return *this;
}

NPoint& NPoint::operator/=(int divisor)
{
    m_x /= divisor;
    m_y /= divisor;
    return *this;
}

int NPoint::dotProduct(const NPoint& p1, const NPoint& p2)
{
    return p1.m_x * p2.m_x + p1.m_y * p2.m_y;
}
