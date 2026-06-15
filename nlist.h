// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <vector>
#include <algorithm>
#include <initializer_list>

template<typename T>
class NList {
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    NList() = default;
    NList(std::initializer_list<T> list) : m_data(list) {}
    explicit NList(int size) : m_data(size) {}
    NList(int size, const T& value) : m_data(size, value) {}

    void append(const T& value) { m_data.push_back(value); }
    void append(const NList<T>& other) {
        m_data.insert(m_data.end(), other.m_data.begin(), other.m_data.end());
    }
    void prepend(const T& value) { m_data.insert(m_data.begin(), value); }
    void insert(int i, const T& value) { m_data.insert(m_data.begin() + i, value); }
    void replace(int i, const T& value) { m_data[i] = value; }
    void removeAt(int i) { m_data.erase(m_data.begin() + i); }
    void removeFirst() { if (!isEmpty()) m_data.erase(m_data.begin()); }
    void removeLast() { if (!isEmpty()) m_data.pop_back(); }
    void clear() { m_data.clear(); }

    T takeAt(int i) { T val = m_data[i]; removeAt(i); return val; }
    T takeFirst() { T val = m_data.front(); removeFirst(); return val; }
    T takeLast() { T val = m_data.back(); removeLast(); return val; }

    bool contains(const T& value) const {
        return std::find(m_data.begin(), m_data.end(), value) != m_data.end();
    }
    int indexOf(const T& value, int from = 0) const {
        for (int i = from; i < size(); ++i) if (m_data[i] == value) return i;
        return -1;
    }
    int lastIndexOf(const T& value, int from = -1) const {
        int start = (from < 0) ? size() - 1 : from;
        for (int i = start; i >= 0; --i) if (m_data[i] == value) return i;
        return -1;
    }

    int size() const { return m_data.size(); }
    bool isEmpty() const { return m_data.empty(); }

    T& operator[](int i) { return m_data[i]; }
    const T& operator[](int i) const { return m_data[i]; }
    T at(int i) const { return m_data.at(i); }
    T value(int i) const { return (i >= 0 && i < size()) ? m_data[i] : T(); }

    NList<T> mid(int pos, int len = -1) const {
        int end = (len < 0 || pos + len > size()) ? size() : pos + len;
        return NList<T>(m_data.begin() + pos, m_data.begin() + end);
    }

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

private:
    std::vector<T> m_data;
    NList(typename std::vector<T>::const_iterator first, typename std::vector<T>::const_iterator last)
        : m_data(first, last) {}
};
