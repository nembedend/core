// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nstring.h"
#include "nbytearray.h"
#include <cctype>
#include <cstring>

const char NString::Null = '\0';

char* NString::allocate(size_t size)
{
    if (size == 0)
        return nullptr;
    return static_cast<char*>(malloc(size + 1));
}

void NString::deallocate(char* ptr)
{
    free(ptr);
}

void NString::init(const char* str, size_t len)
{
    m_isNull = (str == nullptr);
    m_len = len;

    if (len < N_STRING_INLINE_CAP) {
        m_isShort = true;
        m_data = m_short.data;
        m_cap = N_STRING_INLINE_CAP - 1;

        if (str && len > 0) {
            memcpy(m_short.data, str, len);
        }
        m_short.data[len] = '\0';
        m_short.size = len;
        m_short.capacity = m_cap;
    } else {
        m_isShort = false;
        m_data = allocate(len);
        m_cap = len;

        if (str && len > 0) {
            memcpy(m_data, str, len);
        }
        if (m_data) {
            m_data[len] = '\0';
            m_long.data = m_data;
            m_long.size = len;
            m_long.capacity = m_cap;
        }
    }
}

NString::NString() noexcept
    : m_isShort(true)
    , m_isNull(false)
    , m_len(0)
    , m_cap(N_STRING_INLINE_CAP - 1)
    , m_data(m_short.data)
{
    m_short.data[0] = '\0';
    m_short.size = 0;
    m_short.capacity = m_cap;
}

NString::NString(const char* str)
{
    if (!str) {
        init(nullptr, 0);
        m_isNull = true;
    } else {
        init(str, strlen(str));
    }
}

NString::NString(const char* str, size_type len)
{
    if (!str || len == 0) {
        init(nullptr, 0);
        m_isNull = (str == nullptr);
    } else {
        init(str, len);
    }
}

NString::NString(const NString& other)
    : m_isShort(other.m_isShort)
    , m_isNull(other.m_isNull)
    , m_len(other.m_len)
    , m_cap(other.m_cap)
{
    if (m_isShort) {
        memcpy(m_short.data, other.m_short.data, m_len + 1);
        m_data = m_short.data;
    } else {
        m_data = allocate(m_cap);
        if (m_data) {
            memcpy(m_data, other.m_data, m_len + 1);
            m_long.data = m_data;
            m_long.size = m_len;
            m_long.capacity = m_cap;
        }
    }
}

NString::NString(NString&& other) noexcept
    : m_isShort(other.m_isShort)
    , m_isNull(other.m_isNull)
    , m_len(other.m_len)
    , m_cap(other.m_cap)
{
    if (m_isShort) {
        memcpy(m_short.data, other.m_short.data, m_len + 1);
        m_data = m_short.data;
    } else {
        m_data = other.m_data;
        m_long.data = m_data;
        m_long.size = m_len;
        m_long.capacity = m_cap;

        other.m_data = nullptr;
        other.m_len = 0;
        other.m_cap = 0;
        other.m_isNull = true;
    }
}

NString::NString(size_type count, char ch)
{
    if (count == 0) {
        init(nullptr, 0);
        return;
    }

    init(nullptr, count);
    for (size_type i = 0; i < count; ++i) {
        m_data[i] = ch;
    }
    m_data[count] = '\0';
}

NString::NString(const NByteArray& ba)
{
    init(reinterpret_cast<const char*>(ba.data()), ba.size());
}

NString& NString::operator=(const NString& other)
{
    if (this != &other) {
        NString copy(other);
        // Simple swap for embedded (no std::swap)
        char* tmpData = copy.m_data;
        size_t tmpLen = copy.m_len;
        size_t tmpCap = copy.m_cap;
        bool tmpIsShort = copy.m_isShort;
        bool tmpIsNull = copy.m_isNull;

        copy.m_data = m_data;
        copy.m_len = m_len;
        copy.m_cap = m_cap;
        copy.m_isShort = m_isShort;
        copy.m_isNull = m_isNull;

        m_data = tmpData;
        m_len = tmpLen;
        m_cap = tmpCap;
        m_isShort = tmpIsShort;
        m_isNull = tmpIsNull;
    }
    return *this;
}

NString& NString::operator=(NString&& other) noexcept
{
    if (this != &other) {
        if (!m_isShort && m_data) {
            deallocate(m_data);
        }

        m_isShort = other.m_isShort;
        m_isNull = other.m_isNull;
        m_len = other.m_len;
        m_cap = other.m_cap;

        if (m_isShort) {
            memcpy(m_short.data, other.m_short.data, m_len + 1);
            m_data = m_short.data;
        } else {
            m_data = other.m_data;
            m_long.data = m_data;
            m_long.size = m_len;
            m_long.capacity = m_cap;

            other.m_data = nullptr;
            other.m_len = 0;
            other.m_cap = 0;
            other.m_isNull = true;
        }
    }
    return *this;
}

NString& NString::operator=(const char* str)
{
    if (!str) {
        clear();
        m_isNull = true;
    } else {
        NString temp(str);
        // Simple swap
        char* tmpData = temp.m_data;
        size_t tmpLen = temp.m_len;
        size_t tmpCap = temp.m_cap;
        bool tmpIsShort = temp.m_isShort;
        bool tmpIsNull = temp.m_isNull;

        temp.m_data = m_data;
        temp.m_len = m_len;
        temp.m_cap = m_cap;
        temp.m_isShort = m_isShort;
        temp.m_isNull = m_isNull;

        m_data = tmpData;
        m_len = tmpLen;
        m_cap = tmpCap;
        m_isShort = tmpIsShort;
        m_isNull = tmpIsNull;
    }
    return *this;
}

void NString::clear()
{
    if (!m_isShort && m_data) {
        deallocate(m_data);
    }

    m_isShort = true;
    m_isNull = false;
    m_len = 0;
    m_cap = N_STRING_INLINE_CAP - 1;
    m_data = m_short.data;
    m_short.data[0] = '\0';
    m_short.size = 0;
    m_short.capacity = m_cap;
}

void NString::reserve(size_type capacity)
{
    if (capacity <= m_cap)
        return;

    char* newData = allocate(capacity);
    if (newData) {
        if (m_data && m_len > 0) {
            memcpy(newData, m_data, m_len);
        }
        newData[m_len] = '\0';

        if (!m_isShort && m_data) {
            deallocate(m_data);
        }

        m_data = newData;
        m_cap = capacity;
        m_isShort = false;
        m_long.data = m_data;
        m_long.size = m_len;
        m_long.capacity = m_cap;
    }
}

void NString::squeeze()
{
    if (m_isShort)
        return;

    if (m_len < N_STRING_INLINE_CAP) {

        NShortString shortStr;
        memcpy(shortStr.data, m_data, m_len);
        shortStr.data[m_len] = '\0';
        shortStr.size = m_len;
        shortStr.capacity = N_STRING_INLINE_CAP - 1;

        deallocate(m_data);

        m_isShort = true;
        m_short = shortStr;
        m_data = m_short.data;
        m_cap = shortStr.capacity;
    } else if (m_cap > m_len + 16) {
        reserve(m_len);
    }
}

const char* NString::c_str() const noexcept
{
    return m_data ? m_data : &Null;
}

const char* NString::data() const noexcept
{
    return m_data ? m_data : &Null;
}

char* NString::data()
{
    detach();
    return m_data;
}

char NString::at(int position) const
{
    if (position < 0 || position >= static_cast<int>(m_len))
        return '\0';
    return m_data[position];
}

char NString::operator[](int position) const
{
    return at(position);
}

char& NString::operator[](int position)
{
    detach();
    static char dummy = '\0';
    if (position < 0 || position >= static_cast<int>(m_len))
        return dummy;
    return m_data[position];
}

NString NString::mid(int position, int len) const
{
    if (position < 0)
        position = 0;
    if (position >= static_cast<int>(m_len))
        return NString();

    if (len < 0 || position + len > static_cast<int>(m_len)) {
        len = m_len - position;
    }

    return NString(m_data + position, len);
}

NString NString::left(int len) const
{
    if (len <= 0)
        return NString();
    if (len >= static_cast<int>(m_len))
        return *this;
    return NString(m_data, len);
}

NString NString::right(int len) const
{
    if (len <= 0)
        return NString();
    if (len >= static_cast<int>(m_len))
        return *this;
    return NString(m_data + m_len - len, len);
}

NString NString::chopped(int len) const
{
    if (len <= 0)
        return *this;
    if (len >= static_cast<int>(m_len))
        return NString();
    return left(m_len - len);
}

NString NString::trimmed() const
{
    if (isEmpty())
        return NString();

    const char* start = m_data;
    const char* end = m_data + m_len - 1;

    while (start <= end && isspace(static_cast<unsigned char>(*start)))
        ++start;
    while (end >= start && isspace(static_cast<unsigned char>(*end)))
        --end;

    if (start > end)
        return NString();
    return NString(start, end - start + 1);
}

NString NString::simplified() const
{
    NString result;
    bool inSpace = false;

    for (size_type i = 0; i < m_len; ++i) {
        if (isspace(static_cast<unsigned char>(m_data[i]))) {
            if (!inSpace && !result.isEmpty()) {
                result.append(' ');
                inSpace = true;
            }
        } else {
            result.append(m_data[i]);
            inSpace = false;
        }
    }

    return result;
}

NString& NString::append(const NString& str)
{
    if (str.isEmpty())
        return *this;

    reserve(m_len + str.m_len);
    memcpy(m_data + m_len, str.m_data, str.m_len);
    m_len += str.m_len;
    m_data[m_len] = '\0';

    if (m_isShort) {
        m_short.size = m_len;
    } else {
        m_long.size = m_len;
    }

    return *this;
}

NString& NString::append(const char* str)
{
    if (!str)
        return *this;
    return append(NString(str));
}

NString& NString::append(char ch)
{
    reserve(m_len + 1);
    m_data[m_len++] = ch;
    m_data[m_len] = '\0';

    if (m_isShort) {
        m_short.size = m_len;
    } else {
        m_long.size = m_len;
    }

    return *this;
}

NString& NString::append(size_type count, char ch)
{
    reserve(m_len + count);
    memset(m_data + m_len, ch, count);
    m_len += count;
    m_data[m_len] = '\0';

    if (m_isShort) {
        m_short.size = m_len;
    } else {
        m_long.size = m_len;
    }

    return *this;
}

NString& NString::prepend(const NString& str)
{
    if (str.isEmpty())
        return *this;

    NString newStr(str);
    newStr.append(*this);
    // Simple swap
    char* tmpData = newStr.m_data;
    size_t tmpLen = newStr.m_len;
    size_t tmpCap = newStr.m_cap;
    bool tmpIsShort = newStr.m_isShort;
    bool tmpIsNull = newStr.m_isNull;

    newStr.m_data = m_data;
    newStr.m_len = m_len;
    newStr.m_cap = m_cap;
    newStr.m_isShort = m_isShort;
    newStr.m_isNull = m_isNull;

    m_data = tmpData;
    m_len = tmpLen;
    m_cap = tmpCap;
    m_isShort = tmpIsShort;
    m_isNull = tmpIsNull;

    return *this;
}

NString& NString::insert(int position, const NString& str)
{
    if (position < 0)
        position = 0;
    if (position > static_cast<int>(m_len))
        position = m_len;
    if (str.isEmpty())
        return *this;

    reserve(m_len + str.m_len);

    memmove(m_data + position + str.m_len,
        m_data + position,
        m_len - position);

    memcpy(m_data + position, str.m_data, str.m_len);
    m_len += str.m_len;
    m_data[m_len] = '\0';

    if (m_isShort) {
        m_short.size = m_len;
    } else {
        m_long.size = m_len;
    }

    return *this;
}

NString& NString::remove(int position, int len)
{
    if (position < 0 || position >= static_cast<int>(m_len) || len <= 0)
        return *this;

    if (position + len > static_cast<int>(m_len)) {
        len = m_len - position;
    }

    memmove(m_data + position, m_data + position + len, m_len - position - len);
    m_len -= len;
    m_data[m_len] = '\0';

    if (m_isShort) {
        m_short.size = m_len;
    } else {
        m_long.size = m_len;
    }

    return *this;
}

void NString::replace(int position, int len, const NString& after)
{
    remove(position, len);
    insert(position, after);
}

NString& NString::replace(const NString& before, const NString& after, CaseSensitivity cs)
{
    if (before.isEmpty())
        return *this;

    int pos = 0;
    while ((pos = indexOf(before, pos, cs)) != -1) {
        replace(pos, before.length(), after);
        pos += after.length();
    }
    return *this;
}

void NString::truncate(int position)
{
    if (position < 0)
        return;
    if (position >= static_cast<int>(m_len))
        return;

    m_data[position] = '\0';
    m_len = position;

    if (m_isShort) {
        m_short.size = m_len;
    } else {
        m_long.size = m_len;
    }
}

void NString::chop(int len)
{
    if (len <= 0)
        return;
    if (len >= static_cast<int>(m_len)) {
        clear();
    } else {
        truncate(m_len - len);
    }
}

void NString::resize(size_type size)
{
    if (size == m_len)
        return;

    reserve(size);
    m_len = size;
    m_data[m_len] = '\0';

    if (m_isShort) {
        m_short.size = m_len;
    } else {
        m_long.size = m_len;
    }
}

NString& NString::fill(char ch, size_type size)
{
    if (size == static_cast<size_type>(-1)) {
        size = m_len;
    }

    resize(size);
    memset(m_data, ch, m_len);
    return *this;
}

NString& NString::operator+=(const NString& str)
{
    return append(str);
}

NString& NString::operator+=(const char* str)
{
    return append(str);
}

NString& NString::operator+=(char ch)
{
    return append(ch);
}

int NString::compare(const NString& other, CaseSensitivity cs) const
{
    int result;
    if (cs == CaseSensitive) {
        result = strcmp(c_str(), other.c_str());
    } else {
        result = strcasecmp(c_str(), other.c_str());
    }

    if (result < 0)
        return -1;
    if (result > 0)
        return 1;
    return 0;
}

int NString::compare(const char* other, CaseSensitivity cs) const
{
    if (!other)
        return 1;
    return compare(NString(other), cs);
}

bool NString::operator==(const NString& other) const
{
    if (m_len != other.m_len)
        return false;
    return memcmp(m_data, other.m_data, m_len) == 0;
}

bool NString::operator!=(const NString& other) const
{
    return !(*this == other);
}

bool NString::operator<(const NString& other) const
{
    return compare(other, CaseSensitive) < 0;
}

bool NString::operator<=(const NString& other) const
{
    return compare(other, CaseSensitive) <= 0;
}

bool NString::operator>(const NString& other) const
{
    return compare(other, CaseSensitive) > 0;
}

bool NString::operator>=(const NString& other) const
{
    return compare(other, CaseSensitive) >= 0;
}

NString NString::toLower() const
{
    NString result(*this);
    for (size_type i = 0; i < result.m_len; ++i) {
        result.m_data[i] = tolower(static_cast<unsigned char>(result.m_data[i]));
    }
    return result;
}

NString NString::toUpper() const
{
    NString result(*this);
    for (size_type i = 0; i < result.m_len; ++i) {
        result.m_data[i] = toupper(static_cast<unsigned char>(result.m_data[i]));
    }
    return result;
}

NString NString::toCaseFolded() const
{
    return toLower();
}

int NString::indexOf(const NString& str, int from, CaseSensitivity cs) const
{
    if (str.isEmpty() || from >= static_cast<int>(m_len))
        return -1;
    if (from < 0)
        from = 0;

    const char* needle = str.c_str();
    size_t needleLen = str.length();

    for (int i = from; i <= static_cast<int>(m_len - needleLen); ++i) {
        if (cs == CaseSensitive) {
            if (memcmp(m_data + i, needle, needleLen) == 0) {
                return i;
            }
        } else {
            if (strncasecmp(m_data + i, needle, needleLen) == 0) {
                return i;
            }
        }
    }

    return -1;
}

int NString::lastIndexOf(const NString& str, int from, CaseSensitivity cs) const
{
    if (str.isEmpty())
        return -1;

    if (from < 0 || from >= static_cast<int>(m_len)) {
        from = m_len - 1;
    }

    const char* needle = str.c_str();
    size_t needleLen = str.length();

    for (int i = from; i >= 0; --i) {
        if (i + needleLen > m_len)
            continue;

        if (cs == CaseSensitive) {
            if (memcmp(m_data + i, needle, needleLen) == 0) {
                return i;
            }
        } else {
            if (strncasecmp(m_data + i, needle, needleLen) == 0) {
                return i;
            }
        }
    }

    return -1;
}

bool NString::contains(const NString& str, CaseSensitivity cs) const
{
    return indexOf(str, 0, cs) != -1;
}

bool NString::startsWith(const NString& str, CaseSensitivity cs) const
{
    if (str.length() > m_len)
        return false;

    if (cs == CaseSensitive) {
        return memcmp(m_data, str.c_str(), str.length()) == 0;
    } else {
        return strncasecmp(m_data, str.c_str(), str.length()) == 0;
    }
}

bool NString::endsWith(const NString& str, CaseSensitivity cs) const
{
    if (str.length() > m_len)
        return false;

    const char* start = m_data + m_len - str.length();

    if (cs == CaseSensitive) {
        return memcmp(start, str.c_str(), str.length()) == 0;
    } else {
        return strncasecmp(start, str.c_str(), str.length()) == 0;
    }
}

NString NString::number(int n, int base)
{
    char buf[64];
    char* ptr = buf + sizeof(buf) - 1;
    *ptr = '\0';

    bool negative = false;
    unsigned int num;

    if (base == 10 && n < 0) {
        negative = true;
        num = -n;
    } else {
        num = n;
    }

    do {
        *--ptr = "0123456789abcdef"[num % base];
        num /= base;
    } while (num);

    if (negative) {
        *--ptr = '-';
    }

    return NString(ptr);
}

NString NString::number(unsigned int n, int base)
{
    char buf[64];
    char* ptr = buf + sizeof(buf) - 1;
    *ptr = '\0';

    do {
        *--ptr = "0123456789abcdef"[n % base];
        n /= base;
    } while (n);

    return NString(ptr);
}

NString NString::number(long n, int base)
{
    char buf[64];
    char* ptr = buf + sizeof(buf) - 1;
    *ptr = '\0';

    bool negative = false;
    unsigned long num;

    if (base == 10 && n < 0) {
        negative = true;
        num = -n;
    } else {
        num = n;
    }

    do {
        *--ptr = "0123456789abcdef"[num % base];
        num /= base;
    } while (num);

    if (negative) {
        *--ptr = '-';
    }

    return NString(ptr);
}

NString NString::number(unsigned long n, int base)
{
    char buf[64];
    char* ptr = buf + sizeof(buf) - 1;
    *ptr = '\0';

    do {
        *--ptr = "0123456789abcdef"[n % base];
        n /= base;
    } while (n);

    return NString(ptr);
}

NString NString::number(long long n, int base)
{
    char buf[64];
    char* ptr = buf + sizeof(buf) - 1;
    *ptr = '\0';

    bool negative = false;
    unsigned long long num;

    if (base == 10 && n < 0) {
        negative = true;
        num = -n;
    } else {
        num = n;
    }

    do {
        *--ptr = "0123456789abcdef"[num % base];
        num /= base;
    } while (num);

    if (negative) {
        *--ptr = '-';
    }

    return NString(ptr);
}

NString NString::number(unsigned long long n, int base)
{
    char buf[64];
    char* ptr = buf + sizeof(buf) - 1;
    *ptr = '\0';

    do {
        *--ptr = "0123456789abcdef"[n % base];
        n /= base;
    } while (n);

    return NString(ptr);
}

NString NString::number(double n, char f, int prec)
{
    char buf[64];
    if (f == 'g') {
        snprintf(buf, sizeof(buf), "%.*g", prec, n);
    } else {
        snprintf(buf, sizeof(buf), "%.*f", prec, n);
    }
    return NString(buf);
}

NString NString::number(float n, char f, int prec)
{
    char buf[64];
    if (f == 'g') {
        snprintf(buf, sizeof(buf), "%.*g", prec, n);
    } else {
        snprintf(buf, sizeof(buf), "%.*f", prec, n);
    }
    return NString(buf);
}

int NString::toInt(bool* ok, int base) const
{
    char* endptr;
    long val = strtol(m_data, &endptr, base);

    if (ok) {
        *ok = (endptr != m_data && *endptr == '\0');
    }

    return static_cast<int>(val);
}

unsigned int NString::toUInt(bool* ok, int base) const
{
    char* endptr;
    unsigned long val = strtoul(m_data, &endptr, base);

    if (ok) {
        *ok = (endptr != m_data && *endptr == '\0');
    }

    return static_cast<unsigned int>(val);
}

long NString::toLong(bool* ok, int base) const
{
    char* endptr;
    long val = strtol(m_data, &endptr, base);

    if (ok) {
        *ok = (endptr != m_data && *endptr == '\0');
    }

    return val;
}

unsigned long NString::toULong(bool* ok, int base) const
{
    char* endptr;
    unsigned long val = strtoul(m_data, &endptr, base);

    if (ok) {
        *ok = (endptr != m_data && *endptr == '\0');
    }

    return val;
}

long long NString::toLongLong(bool* ok, int base) const
{
    char* endptr;
    long long val = strtoll(m_data, &endptr, base);

    if (ok) {
        *ok = (endptr != m_data && *endptr == '\0');
    }

    return val;
}

unsigned long long NString::toULongLong(bool* ok, int base) const
{
    char* endptr;
    unsigned long long val = strtoull(m_data, &endptr, base);

    if (ok) {
        *ok = (endptr != m_data && *endptr == '\0');
    }

    return val;
}

float NString::toFloat(bool* ok) const
{
    char* endptr;
    float val = strtof(m_data, &endptr);

    if (ok) {
        *ok = (endptr != m_data && *endptr == '\0');
    }

    return val;
}

double NString::toDouble(bool* ok) const
{
    char* endptr;
    double val = strtod(m_data, &endptr);

    if (ok) {
        *ok = (endptr != m_data && *endptr == '\0');
    }

    return val;
}

NString NString::asprintf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    NString result = vasprintf(format, ap);
    va_end(ap);
    return result;
}

NString NString::vasprintf(const char* format, va_list ap)
{
    va_list ap_copy;
    va_copy(ap_copy, ap);

    int size = vsnprintf(nullptr, 0, format, ap_copy);
    va_end(ap_copy);

    if (size <= 0)
        return NString();

    NString result;
    result.resize(size);
    vsnprintf(result.data(), size + 1, format, ap);

    return result;
}

NByteArray NString::toLatin1() const
{
    return NByteArray(m_data, m_len);
}

NByteArray NString::toUtf8() const
{
    return NByteArray(m_data, m_len);
}

NByteArray NString::toLocal8Bit() const
{
    return NByteArray(m_data, m_len);
}

NString NString::fromLatin1(const char* str, int size)
{
    if (size < 0)
        size = strlen(str);
    return NString(str, size);
}

NString NString::fromUtf8(const char* str, int size)
{
    if (size < 0)
        size = strlen(str);
    return NString(str, size);
}

NString NString::fromLocal8Bit(const char* str, int size)
{
    if (size < 0)
        size = strlen(str);
    return NString(str, size);
}

void NString::detach()
{
    if (!m_isShort && m_data) {
        NString copy(*this);
        // Simple swap
        char* tmpData = copy.m_data;
        size_t tmpLen = copy.m_len;
        size_t tmpCap = copy.m_cap;
        bool tmpIsShort = copy.m_isShort;
        bool tmpIsNull = copy.m_isNull;

        copy.m_data = m_data;
        copy.m_len = m_len;
        copy.m_cap = m_cap;
        copy.m_isShort = m_isShort;
        copy.m_isNull = m_isNull;

        m_data = tmpData;
        m_len = tmpLen;
        m_cap = tmpCap;
        m_isShort = tmpIsShort;
        m_isNull = tmpIsNull;
    }
}

char* NString::detachIfNeeded()
{
    detach();
    return m_data;
}

NString operator+(const NString& a, const NString& b)
{
    NString result(a);
    result += b;
    return result;
}

NString operator+(const NString& a, const char* b)
{
    NString result(a);
    result += b;
    return result;
}

NString operator+(const char* a, const NString& b)
{
    NString result(a);
    result += b;
    return result;
}

bool operator==(const char* a, const NString& b)
{
    return b == a;
}

bool operator!=(const char* a, const NString& b)
{
    return b != a;
}

bool NString::isValidUtf8() const
{
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(m_data);
    size_t i = 0;

    while (i < m_len) {
        if (bytes[i] < 0x80) {
            i++;
        } else if ((bytes[i] & 0xE0) == 0xC0) {
            if (i + 1 >= m_len)
                return false;
            if ((bytes[i + 1] & 0xC0) != 0x80)
                return false;
            i += 2;
        } else if ((bytes[i] & 0xF0) == 0xE0) {
            if (i + 2 >= m_len)
                return false;
            if ((bytes[i + 1] & 0xC0) != 0x80)
                return false;
            if ((bytes[i + 2] & 0xC0) != 0x80)
                return false;
            i += 3;
        } else if ((bytes[i] & 0xF8) == 0xF0) {
            if (i + 3 >= m_len)
                return false;
            if ((bytes[i + 1] & 0xC0) != 0x80)
                return false;
            if ((bytes[i + 2] & 0xC0) != 0x80)
                return false;
            if ((bytes[i + 3] & 0xC0) != 0x80)
                return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

NString NString::toBase64() const
{
    static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyz"
                                       "0123456789+/";

    size_t output_len = ((m_len + 2) / 3) * 4;
    NString result;
    result.resize(output_len);

    for (size_t i = 0, j = 0; i < m_len; i += 3, j += 4) {
        uint32_t octet_a = i < m_len ? static_cast<unsigned char>(m_data[i]) : 0;
        uint32_t octet_b = i + 1 < m_len ? static_cast<unsigned char>(m_data[i + 1]) : 0;
        uint32_t octet_c = i + 2 < m_len ? static_cast<unsigned char>(m_data[i + 2]) : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        result.m_data[j] = base64_chars[(triple >> 18) & 0x3F];
        result.m_data[j + 1] = base64_chars[(triple >> 12) & 0x3F];
        result.m_data[j + 2] = base64_chars[(triple >> 6) & 0x3F];
        result.m_data[j + 3] = base64_chars[triple & 0x3F];
    }

    size_t padding = (3 - (m_len % 3)) % 3;
    for (size_t i = 0; i < padding; ++i) {
        result.m_data[output_len - 1 - i] = '=';
    }

    return result;
}
