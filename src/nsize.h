// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <cstdint>

enum class NAspectRatioMode {
    Ignore,
    Keep,
    KeepByExpanding
};
class NSize {
public:
    NSize();
    NSize(int width, int height);
    NSize(const NSize& other) = default;

    int width() const { return m_width; }
    int height() const { return m_height; }

    void setWidth(int width);
    void setHeight(int height);
    void set(int width, int height);

    void scale(int width, int height, NAspectRatioMode mode);
    void scale(const NSize& size, NAspectRatioMode mode);
    void transpose();

    bool isEmpty() const;
    bool isValid() const;
    bool isNull() const;

    bool operator==(const NSize& other) const;
    bool operator!=(const NSize& other) const;

    NSize operator+(const NSize& other) const;
    NSize operator-(const NSize& other) const;
    NSize operator*(int factor) const;
    NSize operator/(int divisor) const;

    NSize& operator+=(const NSize& other);
    NSize& operator-=(const NSize& other);
    NSize& operator*=(int factor);
    NSize& operator/=(int divisor);

    static NSize bounded(const NSize& size, const NSize& maxSize);
    static NSize expanded(const NSize& size, const NSize& minSize);

private:
    int m_width;
    int m_height;
};
