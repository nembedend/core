// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define N_BYTEARRAY_SSO_SIZE 32

class NByteArray {
public:
    using size_type = size_t;
    using value_type = uint8_t;
    using reference = uint8_t&;
    using const_reference = const uint8_t&;
    using pointer = uint8_t*;
    using const_pointer = const uint8_t*;

    using iterator = uint8_t*;
    using const_iterator = const uint8_t*;

    NByteArray() noexcept;
    explicit NByteArray(size_type size, uint8_t value = 0);
    NByteArray(const char* data, size_type size);
    NByteArray(const uint8_t* data, size_type size);
    NByteArray(const NByteArray& other);
    NByteArray(NByteArray&& other) noexcept;
    ~NByteArray();

    NByteArray& operator=(const NByteArray& other);
    NByteArray& operator=(NByteArray&& other) noexcept;
    NByteArray& operator=(const char* str);

    bool isEmpty() const noexcept { return m_size == 0; }
    bool isNull() const noexcept { return m_data == nullptr; }
    void clear();

    size_type size() const noexcept { return m_size; }
    size_type capacity() const noexcept { return m_capacity; }
    void reserve(size_type capacity);
    void squeeze();
    void resize(size_type size);

    const uint8_t* data() const noexcept;
    const uint8_t* constData() const noexcept { return data(); }
    uint8_t* data();

    uint8_t at(size_type position) const;
    uint8_t operator[](size_type position) const;
    uint8_t& operator[](size_type position);

    void append(const NByteArray& other);
    void append(const uint8_t* data, size_type size);
    void append(uint8_t value);
    void append(char value);

    void prepend(const NByteArray& other);
    void insert(size_type position, const NByteArray& other);
    void remove(size_type position, size_type len);
    void replace(size_type position, size_type len, const NByteArray& after);

    NByteArray& fill(uint8_t value, size_type size = -1);
    void truncate(size_type position);
    void chop(size_type len);

    NByteArray& operator+=(const NByteArray& other);
    NByteArray& operator+=(uint8_t value);
    NByteArray& operator+=(char value);

    bool operator==(const NByteArray& other) const;
    bool operator!=(const NByteArray& other) const;

    int compare(const NByteArray& other) const;
    int compare(const uint8_t* data, size_type size) const;

    int indexOf(uint8_t value, int from = 0) const;
    int indexOf(const NByteArray& needle, int from = 0) const;
    int lastIndexOf(uint8_t value, int from = -1) const;
    bool contains(const NByteArray& needle) const;
    bool startsWith(const NByteArray& prefix) const;
    bool endsWith(const NByteArray& suffix) const;

    NByteArray mid(size_type position, size_type len = -1) const;
    NByteArray left(size_type len) const;
    NByteArray right(size_type len) const;

    std::string toString() const;
    const char* toCharPtr() const;

    static NByteArray fromHex(const NByteArray& hexEncoded);
    NByteArray toHex() const;

    static NByteArray fromBase64(const NByteArray& base64Encoded);
    NByteArray toBase64() const;

    static NByteArray number(int n);
    static NByteArray number(unsigned int n);
    static NByteArray number(long n);
    static NByteArray number(unsigned long n);
    static NByteArray number(double n, int precision = 6);

    int toInt(bool* ok = nullptr) const;
    double toDouble(bool* ok = nullptr) const;

    iterator begin() noexcept { return data(); }
    iterator end() noexcept { return data() + m_size; }
    const_iterator begin() const noexcept { return data(); }
    const_iterator end() const noexcept { return data() + m_size; }
    const_iterator cbegin() const noexcept { return data(); }
    const_iterator cend() const noexcept { return data() + m_size; }

    void lock() const;
    void unlock() const;
    bool tryLock(TickType_t timeout = 0) const;

    static NByteArray fromRawData(const uint8_t* data, size_type size);
    static NByteArray fromRawData(const char* data, size_type size);

private:
    struct NSmallArray {
        uint8_t data[N_BYTEARRAY_SSO_SIZE];
        uint8_t size;
        uint8_t capacity;
    };

    struct NLargeArray {
        uint8_t* data;
        size_t size;
        size_t capacity;
    };

    union {
        NSmallArray m_small;
        NLargeArray m_large;
    };

    bool m_isSmall;
    bool m_isNull;
    size_t m_size;
    size_t m_capacity;
    uint8_t* m_data;
    mutable SemaphoreHandle_t m_mutex;

    void init(const uint8_t* data, size_t size);
    void initNull();
    void realloc(size_t newCapacity);
    void detach();
    void swap(NByteArray& other) noexcept;

    static uint8_t* allocate(size_t size);
    static void deallocate(uint8_t* ptr);
};

NByteArray operator+(const NByteArray& a, const NByteArray& b);
bool operator==(const char* a, const NByteArray& b);
