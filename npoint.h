// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <cmath>
#include <cstdint>

class NPoint {
public:
    NPoint();
    NPoint(int x, int y);
    NPoint(const NPoint& other) = default;

    int x() const { return m_x; }
    int y() const { return m_y; }

    void setX(int x);
    void setY(int y);
    void set(int x, int y);

    void translate(int dx, int dy);
    NPoint transposed() const;

    int manhattanLength() const;
    double length() const;
    int dotProduct(const NPoint& other) const;

    bool operator==(const NPoint& other) const;
    bool operator!=(const NPoint& other) const;

    NPoint operator-() const;
    NPoint operator+(const NPoint& other) const;
    NPoint operator-(const NPoint& other) const;
    NPoint operator*(int factor) const;
    NPoint operator/(int divisor) const;

    NPoint& operator+=(const NPoint& other);
    NPoint& operator-=(const NPoint& other);
    NPoint& operator*=(int factor);
    NPoint& operator/=(int divisor);

    static int dotProduct(const NPoint& p1, const NPoint& p2);

private:
    int m_x;
    int m_y;
};
