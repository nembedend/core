// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <unordered_map>
#include <initializer_list>
#include "nlist.h"

template<typename Key, typename T>
class NMap {
public:
    using iterator = typename std::unordered_map<Key, T>::iterator;
    using const_iterator = typename std::unordered_map<Key, T>::const_iterator;

    NMap() = default;
    NMap(std::initializer_list<std::pair<const Key, T>> list) : m_data(list) {}

    void insert(const Key& key, const T& value) { m_data[key] = value; }
    void insert(const std::pair<Key, T>& pair) { m_data[pair.first] = pair.second; }
    void remove(const Key& key) { m_data.erase(key); }
    void clear() { m_data.clear(); }

    bool contains(const Key& key) const { return m_data.find(key) != m_data.end(); }
    int size() const { return m_data.size(); }
    bool isEmpty() const { return m_data.empty(); }

    T& operator[](const Key& key) { return m_data[key]; }
    const T& operator[](const Key& key) const { return m_data.at(key); }
    T value(const Key& key) const {
        auto it = m_data.find(key);
        return (it != m_data.end()) ? it->second : T();
    }
    T value(const Key& key, const T& defaultValue) const {
        auto it = m_data.find(key);
        return (it != m_data.end()) ? it->second : defaultValue;
    }

    T take(const Key& key) {
        T val = value(key);
        remove(key);
        return val;
    }

    Key key(const T& value) const {
        for (auto& pair : m_data) if (pair.second == value) return pair.first;
        return Key();
    }

    NList<Key> keys() const {
        NList<Key> result;
        for (auto& pair : m_data) result.append(pair.first);
        return result;
    }

    NList<T> values() const {
        NList<T> result;
        for (auto& pair : m_data) result.append(pair.second);
        return result;
    }

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    bool operator==(const NMap& other) const { return m_data == other.m_data; }
    bool operator!=(const NMap& other) const { return m_data != other.m_data; }

private:
    std::unordered_map<Key, T> m_data;
};
