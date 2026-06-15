// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "npoint.h"
#include "nsize.h"

class NRect {
public:
    NRect();
    NRect(int x, int y, int width, int height);
    NRect(const NPoint& topLeft, const NPoint& bottomRight);
    NRect(const NPoint& topLeft, const NSize& size);
    NRect(const NRect& other) = default;

    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    int left() const { return m_x; }
    int top() const { return m_y; }
    int right() const { return m_x + m_width - 1; }
    int bottom() const { return m_y + m_height - 1; }

    NPoint topLeft() const { return NPoint(left(), top()); }
    NPoint topRight() const { return NPoint(right(), top()); }
    NPoint bottomLeft() const { return NPoint(left(), bottom()); }
    NPoint bottomRight() const { return NPoint(right(), bottom()); }
    NPoint center() const { return NPoint(centerX(), centerY()); }

    NSize size() const { return NSize(m_width, m_height); }

    int centerX() const { return m_x + m_width / 2; }
    int centerY() const { return m_y + m_height / 2; }

    void setX(int x) { m_x = x; }
    void setY(int y) { m_y = y; }
    void setWidth(int width) { m_width = width; }
    void setHeight(int height) { m_height = height; }

    void setLeft(int left);
    void setTop(int top);
    void setRight(int right);
    void setBottom(int bottom);

    void setTopLeft(const NPoint& p);
    void setTopRight(const NPoint& p);
    void setBottomLeft(const NPoint& p);
    void setBottomRight(const NPoint& p);

    void setRect(int x, int y, int width, int height);
    void setSize(const NSize& size);

    bool isEmpty() const;
    bool isValid() const;
    bool isNull() const;

    void moveTo(int x, int y);
    void moveTo(const NPoint& p);
    void moveLeft(int x);
    void moveTop(int y);
    void moveRight(int x);
    void moveBottom(int y);
    void moveCenter(const NPoint& p);

    void translate(int dx, int dy);
    void translate(const NPoint& offset);

    void adjust(int dx1, int dy1, int dx2, int dy2);

    bool contains(int x, int y) const;
    bool contains(const NPoint& p) const;
    bool contains(const NRect& rect) const;

    bool intersects(const NRect& rect) const;
    NRect intersected(const NRect& rect) const;
    NRect united(const NRect& rect) const;

    bool operator==(const NRect& other) const;
    bool operator!=(const NRect& other) const;

    static NRect normalized(const NRect& rect);

private:
    int m_x;
    int m_y;
    int m_width;
    int m_height;
};
