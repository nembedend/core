// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nbytearray.h"
#include <cctype>
#include <cstdio>

uint8_t* NByteArray::allocate(size_t size)
{
    if (size == 0)
        return nullptr;
    return static_cast<uint8_t*>(malloc(size));
}

void NByteArray::deallocate(uint8_t* ptr)
{
    free(ptr);
}

void NByteArray::swap(NByteArray& other) noexcept
{
    bool tmpIsSmall = m_isSmall;
    bool tmpIsNull = m_isNull;
    size_t tmpSize = m_size;
    size_t tmpCapacity = m_capacity;
    uint8_t* tmpData = m_data;
    SemaphoreHandle_t tmpMutex = m_mutex;

    NSmallArray tmpSmall = m_small;
    NLargeArray tmpLarge = m_large;

    m_isSmall = other.m_isSmall;
    m_isNull = other.m_isNull;
    m_size = other.m_size;
    m_capacity = other.m_capacity;
    m_data = other.m_data;
    m_mutex = other.m_mutex;

    if (m_isSmall) {
        m_small = other.m_small;
    } else {
        m_large = other.m_large;
    }

    other.m_isSmall = tmpIsSmall;
    other.m_isNull = tmpIsNull;
    other.m_size = tmpSize;
    other.m_capacity = tmpCapacity;
    other.m_data = tmpData;
    other.m_mutex = tmpMutex;

    if (other.m_isSmall) {
        other.m_small = tmpSmall;
    } else {
        other.m_large = tmpLarge;
    }
}

void NByteArray::init(const uint8_t* data, size_t size)
{
    m_isNull = (data == nullptr);
    m_size = size;

    m_mutex = xSemaphoreCreateMutex();

    if (size < N_BYTEARRAY_SSO_SIZE) {
        m_isSmall = true;
        m_data = m_small.data;
        m_capacity = N_BYTEARRAY_SSO_SIZE - 1;

        if (data && size > 0) {
            memcpy(m_small.data, data, size);
        }
        m_small.data[size] = '\0';
        m_small.size = size;
        m_small.capacity = m_capacity;
    } else {
        m_isSmall = false;
        m_data = allocate(size);
        m_capacity = size;

        if (data && size > 0 && m_data) {
            memcpy(m_data, data, size);
        }
        if (m_data) {
            m_data[size] = '\0';
            m_large.data = m_data;
            m_large.size = size;
            m_large.capacity = m_capacity;
        }
    }
}

void NByteArray::initNull()
{
    m_isSmall = true;
    m_isNull = true;
    m_size = 0;
    m_capacity = N_BYTEARRAY_SSO_SIZE - 1;
    m_data = m_small.data;
    m_small.data[0] = '\0';
    m_small.size = 0;
    m_small.capacity = m_capacity;

    m_mutex = xSemaphoreCreateMutex();
}

NByteArray::NByteArray() noexcept
{
    init(nullptr, 0);
}

NByteArray::NByteArray(size_type size, uint8_t value)
{
    if (size == 0) {
        init(nullptr, 0);
        return;
    }

    init(nullptr, size);
    if (m_data && size > 0) {
        memset(m_data, value, size);
        m_data[size] = '\0';
    }
}

NByteArray::NByteArray(const char* data, size_type size)
{
    if (!data || size == 0) {
        init(nullptr, 0);
        if (!data)
            m_isNull = true;
    } else {
        init(reinterpret_cast<const uint8_t*>(data), size);
    }
}

NByteArray::NByteArray(const uint8_t* data, size_type size)
{
    if (!data || size == 0) {
        init(nullptr, 0);
        if (!data)
            m_isNull = true;
    } else {
        init(data, size);
    }
}

NByteArray::NByteArray(const NByteArray& other)
    : m_isSmall(other.m_isSmall)
    , m_isNull(other.m_isNull)
    , m_size(other.m_size)
    , m_capacity(other.m_capacity)
{
    m_mutex = xSemaphoreCreateMutex();

    if (m_isSmall) {
        memcpy(m_small.data, other.m_small.data, m_size + 1);
        m_data = m_small.data;
    } else {
        m_data = allocate(m_capacity);
        if (m_data) {
            memcpy(m_data, other.m_data, m_size + 1);
            m_large.data = m_data;
            m_large.size = m_size;
            m_large.capacity = m_capacity;
        }
    }
}

NByteArray::NByteArray(NByteArray&& other) noexcept
    : m_isSmall(other.m_isSmall)
    , m_isNull(other.m_isNull)
    , m_size(other.m_size)
    , m_capacity(other.m_capacity)
    , m_mutex(other.m_mutex)
{
    if (m_isSmall) {
        memcpy(m_small.data, other.m_small.data, m_size + 1);
        m_data = m_small.data;
    } else {
        m_data = other.m_data;
        m_large.data = m_data;
        m_large.size = m_size;
        m_large.capacity = m_capacity;

        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
        other.m_isNull = true;
    }

    other.m_mutex = nullptr;
}

NByteArray::~NByteArray()
{
    if (!m_isSmall && m_data) {
        deallocate(m_data);
    }

    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
    }
}

NByteArray& NByteArray::operator=(const NByteArray& other)
{
    if (this != &other) {
        NByteArray copy(other);
        swap(copy);
    }
    return *this;
}

NByteArray& NByteArray::operator=(NByteArray&& other) noexcept
{
    if (this != &other) {
        if (!m_isSmall && m_data) {
            deallocate(m_data);
        }

        if (m_mutex) {
            vSemaphoreDelete(m_mutex);
        }

        m_isSmall = other.m_isSmall;
        m_isNull = other.m_isNull;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_mutex = other.m_mutex;

        if (m_isSmall) {
            memcpy(m_small.data, other.m_small.data, m_size + 1);
            m_data = m_small.data;
        } else {
            m_data = other.m_data;
            m_large.data = m_data;
            m_large.size = m_size;
            m_large.capacity = m_capacity;
        }

        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
        other.m_isNull = true;
        other.m_mutex = nullptr;
    }
    return *this;
}

NByteArray& NByteArray::operator=(const char* str)
{
    if (!str) {
        clear();
        m_isNull = true;
    } else {
        NByteArray temp(reinterpret_cast<const uint8_t*>(str), strlen(str));
        swap(temp);
    }
    return *this;
}

void NByteArray::clear()
{
    if (!m_isSmall && m_data) {
        deallocate(m_data);
    }

    m_isSmall = true;
    m_isNull = false;
    m_size = 0;
    m_capacity = N_BYTEARRAY_SSO_SIZE - 1;
    m_data = m_small.data;
    m_small.data[0] = '\0';
    m_small.size = 0;
    m_small.capacity = m_capacity;
}

void NByteArray::reserve(size_type capacity)
{
    if (capacity <= m_capacity)
        return;

    uint8_t* newData = allocate(capacity);
    if (newData) {
        if (m_data && m_size > 0) {
            memcpy(newData, m_data, m_size);
        }
        newData[m_size] = '\0';

        if (!m_isSmall && m_data) {
            deallocate(m_data);
        }

        m_data = newData;
        m_capacity = capacity;
        m_isSmall = false;
        m_large.data = m_data;
        m_large.size = m_size;
        m_large.capacity = m_capacity;
    }
}

void NByteArray::squeeze()
{
    if (m_isSmall)
        return;

    if (m_size < N_BYTEARRAY_SSO_SIZE) {
        NSmallArray smallArr;
        memcpy(smallArr.data, m_data, m_size);
        smallArr.data[m_size] = '\0';
        smallArr.size = m_size;
        smallArr.capacity = N_BYTEARRAY_SSO_SIZE - 1;

        deallocate(m_data);

        m_isSmall = true;
        m_small = smallArr;
        m_data = m_small.data;
        m_capacity = smallArr.capacity;
    } else if (m_capacity > m_size + 16) {
        reserve(m_size);
    }
}

void NByteArray::resize(size_type size)
{
    if (size == m_size)
        return;

    reserve(size);
    m_size = size;

    if (m_data) {
        m_data[m_size] = '\0';
    }

    if (m_isSmall) {
        m_small.size = m_size;
    } else {
        m_large.size = m_size;
    }
}

const uint8_t* NByteArray::data() const noexcept
{
    static const uint8_t nullData = 0;
    return m_data ? m_data : &nullData;
}

uint8_t* NByteArray::data()
{
    detach();
    return m_data;
}

uint8_t NByteArray::at(size_type position) const
{
    if (position >= m_size)
        return 0;
    return m_data[position];
}

uint8_t NByteArray::operator[](size_type position) const
{
    return at(position);
}

uint8_t& NByteArray::operator[](size_type position)
{
    detach();
    static uint8_t dummy = 0;
    if (position >= m_size)
        return dummy;
    return m_data[position];
}

void NByteArray::append(const NByteArray& other)
{
    if (other.isEmpty())
        return;

    size_type newSize = m_size + other.m_size;
    reserve(newSize);

    memcpy(m_data + m_size, other.m_data, other.m_size);
    m_size = newSize;
    m_data[m_size] = '\0';

    if (m_isSmall) {
        m_small.size = m_size;
    } else {
        m_large.size = m_size;
    }
}

void NByteArray::append(const uint8_t* data, size_type size)
{
    if (!data || size == 0)
        return;

    size_type newSize = m_size + size;
    reserve(newSize);

    memcpy(m_data + m_size, data, size);
    m_size = newSize;
    m_data[m_size] = '\0';

    if (m_isSmall) {
        m_small.size = m_size;
    } else {
        m_large.size = m_size;
    }
}

void NByteArray::append(uint8_t value)
{
    append(&value, 1);
}

void NByteArray::append(char value)
{
    append(static_cast<uint8_t>(value));
}

void NByteArray::prepend(const NByteArray& other)
{
    if (other.isEmpty())
        return;

    NByteArray newArray(other);
    newArray.append(*this);
    swap(newArray);
}

void NByteArray::insert(size_type position, const NByteArray& other)
{
    if (position > m_size)
        position = m_size;
    if (other.isEmpty())
        return;

    size_type newSize = m_size + other.m_size;
    reserve(newSize);

    memmove(m_data + position + other.m_size,
        m_data + position,
        m_size - position);

    memcpy(m_data + position, other.m_data, other.m_size);
    m_size = newSize;
    m_data[m_size] = '\0';

    if (m_isSmall) {
        m_small.size = m_size;
    } else {
        m_large.size = m_size;
    }
}

void NByteArray::remove(size_type position, size_type len)
{
    if (position >= m_size || len == 0)
        return;

    if (position + len > m_size) {
        len = m_size - position;
    }

    memmove(m_data + position, m_data + position + len, m_size - position - len);
    m_size -= len;
    m_data[m_size] = '\0';

    if (m_isSmall) {
        m_small.size = m_size;
    } else {
        m_large.size = m_size;
    }
}

void NByteArray::replace(size_type position, size_type len, const NByteArray& after)
{
    remove(position, len);
    insert(position, after);
}

NByteArray& NByteArray::fill(uint8_t value, size_type size)
{
    if (size == static_cast<size_type>(-1)) {
        size = m_size;
    }

    resize(size);
    if (m_data) {
        memset(m_data, value, m_size);
    }

    return *this;
}

void NByteArray::truncate(size_type position)
{
    if (position >= m_size)
        return;

    m_size = position;
    m_data[m_size] = '\0';

    if (m_isSmall) {
        m_small.size = m_size;
    } else {
        m_large.size = m_size;
    }
}

void NByteArray::chop(size_type len)
{
    if (len >= m_size) {
        clear();
    } else if (len > 0) {
        truncate(m_size - len);
    }
}

NByteArray& NByteArray::operator+=(const NByteArray& other)
{
    append(other);
    return *this;
}

NByteArray& NByteArray::operator+=(uint8_t value)
{
    append(value);
    return *this;
}

NByteArray& NByteArray::operator+=(char value)
{
    append(value);
    return *this;
}

bool NByteArray::operator==(const NByteArray& other) const
{
    if (m_size != other.m_size)
        return false;
    if (m_size == 0)
        return true;
    return memcmp(m_data, other.m_data, m_size) == 0;
}

bool NByteArray::operator!=(const NByteArray& other) const
{
    return !(*this == other);
}

int NByteArray::compare(const NByteArray& other) const
{
    size_t minSize = (m_size < other.m_size) ? m_size : other.m_size;
    int result = memcmp(m_data, other.m_data, minSize);

    if (result != 0)
        return result;
    if (m_size < other.m_size)
        return -1;
    if (m_size > other.m_size)
        return 1;
    return 0;
}

int NByteArray::compare(const uint8_t* data, size_type size) const
{
    if (!data)
        return 1;

    size_t minSize = (m_size < size) ? m_size : size;
    int result = memcmp(m_data, data, minSize);

    if (result != 0)
        return result;
    if (m_size < size)
        return -1;
    if (m_size > size)
        return 1;
    return 0;
}

int NByteArray::indexOf(uint8_t value, int from) const
{
    if (from < 0)
        from = 0;
    if (from >= static_cast<int>(m_size))
        return -1;

    for (int i = from; i < static_cast<int>(m_size); ++i) {
        if (m_data[i] == value)
            return i;
    }
    return -1;
}

int NByteArray::indexOf(const NByteArray& needle, int from) const
{
    if (needle.isEmpty() || needle.m_size > m_size)
        return -1;
    if (from < 0)
        from = 0;
    if (from + needle.m_size > m_size)
        return -1;

    for (int i = from; i <= static_cast<int>(m_size - needle.m_size); ++i) {
        if (memcmp(m_data + i, needle.m_data, needle.m_size) == 0) {
            return i;
        }
    }
    return -1;
}

int NByteArray::lastIndexOf(uint8_t value, int from) const
{
    if (from < 0 || from >= static_cast<int>(m_size)) {
        from = m_size - 1;
    }

    for (int i = from; i >= 0; --i) {
        if (m_data[i] == value)
            return i;
    }
    return -1;
}

bool NByteArray::contains(const NByteArray& needle) const
{
    return indexOf(needle) != -1;
}

bool NByteArray::startsWith(const NByteArray& prefix) const
{
    if (prefix.m_size > m_size)
        return false;
    return memcmp(m_data, prefix.m_data, prefix.m_size) == 0;
}

bool NByteArray::endsWith(const NByteArray& suffix) const
{
    if (suffix.m_size > m_size)
        return false;
    return memcmp(m_data + m_size - suffix.m_size, suffix.m_data, suffix.m_size) == 0;
}

NByteArray NByteArray::mid(size_type position, size_type len) const
{
    if (position >= m_size)
        return NByteArray();

    if (len == static_cast<size_type>(-1) || position + len > m_size) {
        len = m_size - position;
    }

    return NByteArray(m_data + position, len);
}

NByteArray NByteArray::left(size_type len) const
{
    if (len >= m_size)
        return *this;
    return NByteArray(m_data, len);
}

NByteArray NByteArray::right(size_type len) const
{
    if (len >= m_size)
        return *this;
    return NByteArray(m_data + m_size - len, len);
}

std::string NByteArray::toString() const
{
    return std::string(reinterpret_cast<const char*>(data()), m_size);
}

const char* NByteArray::toCharPtr() const
{
    return reinterpret_cast<const char*>(data());
}

NByteArray NByteArray::fromHex(const NByteArray& hexEncoded)
{
    if (hexEncoded.isEmpty())
        return NByteArray();

    size_type outSize = hexEncoded.size() / 2;
    NByteArray result(outSize, 0);

    auto hexToNibble = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return 0;
    };

    for (size_type i = 0; i < outSize; ++i) {
        uint8_t high = hexToNibble(hexEncoded[i * 2]);
        uint8_t low = hexToNibble(hexEncoded[i * 2 + 1]);
        result[i] = (high << 4) | low;
    }

    return result;
}

NByteArray NByteArray::toHex() const
{
    static const char hexChars[] = "0123456789abcdef";

    NByteArray result(m_size * 2, 0);

    for (size_type i = 0; i < m_size; ++i) {
        result[i * 2] = hexChars[(m_data[i] >> 4) & 0x0F];
        result[i * 2 + 1] = hexChars[m_data[i] & 0x0F];
    }

    return result;
}

NByteArray NByteArray::fromBase64(const NByteArray& base64Encoded)
{
    static const int base64DecodeTable[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
    };

    size_type len = base64Encoded.size();
    if (len == 0)
        return NByteArray();

    size_type outLen = (len * 3) / 4;
    while (len > 0 && base64Encoded[len - 1] == '=') {
        outLen--;
        len--;
    }

    NByteArray result(outLen, 0);
    size_type outIdx = 0;
    uint32_t buffer = 0;
    int bits = 0;

    for (size_type i = 0; i < len; ++i) {
        uint8_t ch = base64Encoded[i];
        int value = base64DecodeTable[ch];

        if (value < 0)
            continue;

        buffer = (buffer << 6) | value;
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            result[outIdx++] = (buffer >> bits) & 0xFF;
        }
    }

    return result;
}

NByteArray NByteArray::toBase64() const
{
    static const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "abcdefghijklmnopqrstuvwxyz"
                                      "0123456789+/";

    size_type outLen = ((m_size + 2) / 3) * 4;
    NByteArray result(outLen, 0);

    for (size_type i = 0, j = 0; i < m_size; i += 3, j += 4) {
        uint32_t octet_a = i < m_size ? m_data[i] : 0;
        uint32_t octet_b = i + 1 < m_size ? m_data[i + 1] : 0;
        uint32_t octet_c = i + 2 < m_size ? m_data[i + 2] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        result[j] = base64Chars[(triple >> 18) & 0x3F];
        result[j + 1] = base64Chars[(triple >> 12) & 0x3F];
        result[j + 2] = base64Chars[(triple >> 6) & 0x3F];
        result[j + 3] = base64Chars[triple & 0x3F];
    }

    size_type padding = (3 - (m_size % 3)) % 3;
    for (size_type i = 0; i < padding; ++i) {
        result[outLen - 1 - i] = '=';
    }

    return result;
}

NByteArray NByteArray::number(int n)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", n);
    return NByteArray(buf, strlen(buf));
}

NByteArray NByteArray::number(unsigned int n)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", n);
    return NByteArray(buf, strlen(buf));
}

NByteArray NByteArray::number(long n)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", n);
    return NByteArray(buf, strlen(buf));
}

NByteArray NByteArray::number(unsigned long n)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", n);
    return NByteArray(buf, strlen(buf));
}

NByteArray NByteArray::number(double n, int precision)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*f", precision, n);
    return NByteArray(buf, strlen(buf));
}

int NByteArray::toInt(bool* ok) const
{
    char* endptr;
    long val = strtol(reinterpret_cast<const char*>(m_data), &endptr, 10);

    if (ok) {
        *ok = (endptr != reinterpret_cast<const char*>(m_data) && *endptr == '\0');
    }

    return static_cast<int>(val);
}

double NByteArray::toDouble(bool* ok) const
{
    char* endptr;
    double val = strtod(reinterpret_cast<const char*>(m_data), &endptr);

    if (ok) {
        *ok = (endptr != reinterpret_cast<const char*>(m_data) && *endptr == '\0');
    }

    return val;
}

void NByteArray::lock() const
{
    if (m_mutex) {
        xSemaphoreTake(m_mutex, portMAX_DELAY);
    }
}

void NByteArray::unlock() const
{
    if (m_mutex) {
        xSemaphoreGive(m_mutex);
    }
}

bool NByteArray::tryLock(TickType_t timeout) const
{
    if (m_mutex) {
        return xSemaphoreTake(m_mutex, timeout) == pdTRUE;
    }
    return false;
}

void NByteArray::detach()
{
    if (!m_isSmall && m_data) {
        NByteArray copy(*this);
        swap(copy);
    }
}

void NByteArray::realloc(size_t newCapacity)
{
    reserve(newCapacity);
}

NByteArray NByteArray::fromRawData(const uint8_t* data, size_type size)
{
    return NByteArray(data, size);
}

NByteArray NByteArray::fromRawData(const char* data, size_type size)
{
    return NByteArray(reinterpret_cast<const uint8_t*>(data), size);
}

NByteArray operator+(const NByteArray& a, const NByteArray& b)
{
    NByteArray result(a);
    result += b;
    return result;
}

bool operator==(const char* a, const NByteArray& b)
{
    if (!a)
        return b.isNull();
    return b == NByteArray(reinterpret_cast<const uint8_t*>(a), strlen(a));
}
