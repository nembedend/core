// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <algorithm>
#include <string>

#define N_STRING_INLINE_CAP 48      // Small string optimization (48 bytes)
#define N_STRING_MAX_SIZE   2048    // Max string length

class NByteArray;

class NString {
public:
    using size_type = size_t;
    using value_type = char;
    using const_reference = const char&;
    using const_pointer = const char*;
    using const_iterator = const char*;

    enum CaseSensitivity {
        CaseSensitive,
        CaseInsensitive
    };

    static const char Null;
    static const NString& null();

    NString() noexcept;
    NString(const char* str);
    NString(const char* str, size_type len);
    NString(const NString& other);
    NString(NString&& other) noexcept;
    explicit NString(size_type count, char ch);
    explicit NString(const NByteArray& ba);
    ~NString() = default;

    NString& operator=(const NString& other);
    NString& operator=(NString&& other) noexcept;
    NString& operator=(const char* str);

    bool isEmpty() const noexcept { return m_len == 0; }
    bool isNull() const noexcept { return m_data == nullptr; }
    bool isNullEmpty() const noexcept { return isEmpty() || isNull(); }
    void clear();

    size_type size() const noexcept { return m_len; }
    size_type length() const noexcept { return m_len; }
    size_type capacity() const noexcept { return m_cap; }
    void reserve(size_type capacity);
    void squeeze();

    const char* c_str() const noexcept;
    const char* data() const noexcept;
    const char* constData() const noexcept { return data(); }
    char* data();

    char at(int position) const;
    char operator[](int position) const;
    char& operator[](int position);

    NString mid(int position, int len = -1) const;
    NString left(int len) const;
    NString right(int len) const;
    NString chopped(int len) const;
    NString trimmed() const;
    NString simplified() const;

    NString& append(const NString& str);
    NString& append(const char* str);
    NString& append(char ch);
    NString& append(size_type count, char ch);

    NString& prepend(const NString& str);
    NString& insert(int position, const NString& str);
    NString& remove(int position, int len);
    void replace(int position, int len, const NString& after);
    NString& replace(const NString& before, const NString& after, CaseSensitivity cs = CaseSensitive);

    NString& fill(char ch, size_type size = -1);
    void truncate(int position);
    void chop(int len);
    void resize(size_type size);

    NString& operator+=(const NString& str);
    NString& operator+=(const char* str);
    NString& operator+=(char ch);

    int compare(const NString& other, CaseSensitivity cs = CaseSensitive) const;
    int compare(const char* other, CaseSensitivity cs = CaseSensitive) const;

    bool operator==(const NString& other) const;
    bool operator!=(const NString& other) const;
    bool operator<(const NString& other) const;
    bool operator<=(const NString& other) const;
    bool operator>(const NString& other) const;
    bool operator>=(const NString& other) const;

    NString toLower() const;
    NString toUpper() const;
    NString toCaseFolded() const;

    int indexOf(const NString& str, int from = 0, CaseSensitivity cs = CaseSensitive) const;
    int lastIndexOf(const NString& str, int from = -1, CaseSensitivity cs = CaseSensitive) const;
    bool contains(const NString& str, CaseSensitivity cs = CaseSensitive) const;
    bool startsWith(const NString& str, CaseSensitivity cs = CaseSensitive) const;
    bool endsWith(const NString& str, CaseSensitivity cs = CaseSensitive) const;

    static NString number(int n, int base = 10);
    static NString number(unsigned int n, int base = 10);
    static NString number(long n, int base = 10);
    static NString number(unsigned long n, int base = 10);
    static NString number(long long n, int base = 10);
    static NString number(unsigned long long n, int base = 10);
    static NString number(double n, char f = 'g', int prec = 6);
    static NString number(float n, char f = 'g', int prec = 6);

    int toInt(bool* ok = nullptr, int base = 10) const;
    unsigned int toUInt(bool* ok = nullptr, int base = 10) const;
    long toLong(bool* ok = nullptr, int base = 10) const;
    unsigned long toULong(bool* ok = nullptr, int base = 10) const;
    long long toLongLong(bool* ok = nullptr, int base = 10) const;
    unsigned long long toULongLong(bool* ok = nullptr, int base = 10) const;
    float toFloat(bool* ok = nullptr) const;
    double toDouble(bool* ok = nullptr) const;

    static NString asprintf(const char* format, ...) __attribute__((format(printf, 1, 2)));
    static NString vasprintf(const char* format, va_list ap);

    NByteArray toLatin1() const;
    NByteArray toUtf8() const;
    NByteArray toLocal8Bit() const;

    static NString fromLatin1(const char* str, int size = -1);
    static NString fromUtf8(const char* str, int size = -1);
    static NString fromLocal8Bit(const char* str, int size = -1);

    const_iterator begin() const noexcept { return m_data ? m_data : &Null; }
    const_iterator end() const noexcept { return m_data ? m_data + m_len : &Null; }

    bool isValidUtf8() const;
    NString toBase64() const;

private:
    // Small String Optimization
    struct NShortString {
        char data[N_STRING_INLINE_CAP];
        uint8_t size;
        uint8_t capacity;
    };

    struct NLongString {
        char* data;
        size_t size;
        size_t capacity;
    };

    union {
        NShortString m_short;
        NLongString m_long;
    };

    bool m_isShort : 1;
    bool m_isNull : 1;
    size_t m_len;
    size_t m_cap;
    char* m_data;

    void init(const char* str, size_t len);
    void realloc(size_t newCapacity);
    void detach();
    char* detachIfNeeded();

    static char* allocate(size_t size);
    static void deallocate(char* ptr);
};

NString operator+(const NString& a, const NString& b);
NString operator+(const NString& a, const char* b);
NString operator+(const char* a, const NString& b);

bool operator==(const char* a, const NString& b);
bool operator!=(const char* a, const NString& b);

namespace std {
template<>
struct hash<NString> {
    size_t operator()(const NString& s) const noexcept {
        return std::hash<std::string>()(s.c_str());
    }
};
}
