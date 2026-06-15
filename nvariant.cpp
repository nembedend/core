// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nvariant.h"
#include "nobject.h"
#include <cmath>
#include <cstdlib>

NVariant::NVariant(const NString& value)
    : m_type(String)
{
    initString(value);
}

NVariant::NVariant(const char* value)
    : m_type(String)
{
    initString(NString(value ? value : ""));
}

NVariant::NVariant(const NByteArray& value)
    : m_type(ByteArray)
{
    initByteArray(value);
}

NVariant::NVariant(const NVariant& other)
    : m_type(other.m_type)
{
    copyData(other);
}

NVariant::NVariant(NVariant&& other) noexcept
    : m_type(other.m_type)
    , m_data(other.m_data)
{
    other.m_type = Invalid;
    other.m_data.ptr = nullptr;
}

NVariant::~NVariant()
{
    freeData();
}

NVariant& NVariant::operator=(const NVariant& other)
{
    if (this != &other) {
        freeData();
        m_type = other.m_type;
        copyData(other);
    }
    return *this;
}

NVariant& NVariant::operator=(NVariant&& other) noexcept
{
    if (this != &other) {
        freeData();
        m_type = other.m_type;
        m_data = other.m_data;
        other.m_type = Invalid;
        other.m_data.ptr = nullptr;
    }
    return *this;
}

void NVariant::copyData(const NVariant& other)
{
    switch (m_type) {
    case String:
        m_data.str = new NString(*other.m_data.str);
        break;
    case ByteArray:
        m_data.ba = new NByteArray(*other.m_data.ba);
        break;
    default:
        m_data = other.m_data;
        break;
    }
}

void NVariant::freeData()
{
    switch (m_type) {
    case String:
        delete m_data.str;
        break;
    case ByteArray:
        delete m_data.ba;
        break;
    default:
        break;
    }
    m_type = Invalid;
    m_data.ptr = nullptr;
}

void NVariant::initString(const NString& value)
{
    m_data.str = new NString(value);
}

void NVariant::initByteArray(const NByteArray& value)
{
    m_data.ba = new NByteArray(value);
}

void NVariant::clear()
{
    freeData();
}

bool NVariant::canConvert(Type targetType) const
{
    if (m_type == targetType)
        return true;

    switch (targetType) {
    case Bool:
        return (m_type == Int || m_type == UInt || m_type == Long || m_type == ULong || m_type == LongLong || m_type == ULongLong || m_type == Float || m_type == Double || m_type == String);
    case Int:
    case UInt:
    case Long:
    case ULong:
    case LongLong:
    case ULongLong:
        return (m_type == Int || m_type == UInt || m_type == Long || m_type == ULong || m_type == LongLong || m_type == ULongLong || m_type == Float || m_type == Double || m_type == Bool || m_type == String);
    case Float:
    case Double:
        return (m_type == Int || m_type == UInt || m_type == Long || m_type == ULong || m_type == LongLong || m_type == ULongLong || m_type == Float || m_type == Double || m_type == Bool || m_type == String);
    case String:
        return (m_type != Invalid && m_type != ObjectPtr && m_type != VoidPtr);
    default:
        return false;
    }
}

bool NVariant::toBool(bool* ok) const
{
    bool success = true;
    bool result = false;

    switch (m_type) {
    case Bool:
        result = m_data.b;
        break;
    case Int:
        result = (m_data.i != 0);
        break;
    case UInt:
        result = (m_data.ui != 0);
        break;
    case Long:
        result = (m_data.l != 0);
        break;
    case ULong:
        result = (m_data.ul != 0);
        break;
    case LongLong:
        result = (m_data.ll != 0);
        break;
    case ULongLong:
        result = (m_data.ull != 0);
        break;
    case Float:
        result = (m_data.f != 0.0f);
        break;
    case Double:
        result = (m_data.d != 0.0);
        break;
    case String:
        result = (m_data.str->toInt() != 0);
        break;
    default:
        success = false;
        result = false;
        break;
    }

    if (ok)
        *ok = success;
    return result;
}

int NVariant::toInt(bool* ok) const
{
    bool success = true;
    int result = 0;

    switch (m_type) {
    case Bool:
        result = m_data.b ? 1 : 0;
        break;
    case Int:
        result = m_data.i;
        break;
    case UInt:
        result = static_cast<int>(m_data.ui);
        break;
    case Long:
        result = static_cast<int>(m_data.l);
        break;
    case ULong:
        result = static_cast<int>(m_data.ul);
        break;
    case LongLong:
        result = static_cast<int>(m_data.ll);
        break;
    case ULongLong:
        result = static_cast<int>(m_data.ull);
        break;
    case Float:
        result = static_cast<int>(m_data.f);
        break;
    case Double:
        result = static_cast<int>(m_data.d);
        break;
    case String:
        result = m_data.str->toInt(&success);
        break;
    default:
        success = false;
        break;
    }

    if (ok)
        *ok = success;
    return result;
}

unsigned int NVariant::toUInt(bool* ok) const
{
    bool success = true;
    unsigned int result = 0;

    switch (m_type) {
    case Bool:
        result = m_data.b ? 1U : 0U;
        break;
    case Int:
        result = static_cast<unsigned int>(m_data.i);
        break;
    case UInt:
        result = m_data.ui;
        break;
    case Long:
        result = static_cast<unsigned int>(m_data.l);
        break;
    case ULong:
        result = static_cast<unsigned int>(m_data.ul);
        break;
    case LongLong:
        result = static_cast<unsigned int>(m_data.ll);
        break;
    case ULongLong:
        result = static_cast<unsigned int>(m_data.ull);
        break;
    case Float:
        result = static_cast<unsigned int>(m_data.f);
        break;
    case Double:
        result = static_cast<unsigned int>(m_data.d);
        break;
    case String:
        result = static_cast<unsigned int>(m_data.str->toUInt(&success));
        break;
    default:
        success = false;
        break;
    }

    if (ok)
        *ok = success;
    return result;
}

long NVariant::toLong(bool* ok) const
{
    bool success = true;
    long result = 0;

    switch (m_type) {
    case Bool:
        result = m_data.b ? 1L : 0L;
        break;
    case Int:
        result = static_cast<long>(m_data.i);
        break;
    case UInt:
        result = static_cast<long>(m_data.ui);
        break;
    case Long:
        result = m_data.l;
        break;
    case ULong:
        result = static_cast<long>(m_data.ul);
        break;
    case LongLong:
        result = static_cast<long>(m_data.ll);
        break;
    case ULongLong:
        result = static_cast<long>(m_data.ull);
        break;
    case Float:
        result = static_cast<long>(m_data.f);
        break;
    case Double:
        result = static_cast<long>(m_data.d);
        break;
    case String:
        result = m_data.str->toLong(&success);
        break;
    default:
        success = false;
        break;
    }

    if (ok)
        *ok = success;
    return result;
}

unsigned long NVariant::toULong(bool* ok) const
{
    bool success = true;
    unsigned long result = 0;

    switch (m_type) {
    case Bool:
        result = m_data.b ? 1UL : 0UL;
        break;
    case Int:
        result = static_cast<unsigned long>(m_data.i);
        break;
    case UInt:
        result = static_cast<unsigned long>(m_data.ui);
        break;
    case Long:
        result = static_cast<unsigned long>(m_data.l);
        break;
    case ULong:
        result = m_data.ul;
        break;
    case LongLong:
        result = static_cast<unsigned long>(m_data.ll);
        break;
    case ULongLong:
        result = static_cast<unsigned long>(m_data.ull);
        break;
    case Float:
        result = static_cast<unsigned long>(m_data.f);
        break;
    case Double:
        result = static_cast<unsigned long>(m_data.d);
        break;
    case String:
        result = m_data.str->toULong(&success);
        break;
    default:
        success = false;
        break;
    }

    if (ok)
        *ok = success;
    return result;
}

long long NVariant::toLongLong(bool* ok) const
{
    bool success = true;
    long long result = 0;

    switch (m_type) {
    case Bool:
        result = m_data.b ? 1LL : 0LL;
        break;
    case Int:
        result = static_cast<long long>(m_data.i);
        break;
    case UInt:
        result = static_cast<long long>(m_data.ui);
        break;
    case Long:
        result = static_cast<long long>(m_data.l);
        break;
    case ULong:
        result = static_cast<long long>(m_data.ul);
        break;
    case LongLong:
        result = m_data.ll;
        break;
    case ULongLong:
        result = static_cast<long long>(m_data.ull);
        break;
    case Float:
        result = static_cast<long long>(m_data.f);
        break;
    case Double:
        result = static_cast<long long>(m_data.d);
        break;
    case String:
        result = m_data.str->toLongLong(&success);
        break;
    default:
        success = false;
        break;
    }

    if (ok)
        *ok = success;
    return result;
}

unsigned long long NVariant::toULongLong(bool* ok) const
{
    bool success = true;
    unsigned long long result = 0;

    switch (m_type) {
    case Bool:
        result = m_data.b ? 1ULL : 0ULL;
        break;
    case Int:
        result = static_cast<unsigned long long>(m_data.i);
        break;
    case UInt:
        result = static_cast<unsigned long long>(m_data.ui);
        break;
    case Long:
        result = static_cast<unsigned long long>(m_data.l);
        break;
    case ULong:
        result = static_cast<unsigned long long>(m_data.ul);
        break;
    case LongLong:
        result = static_cast<unsigned long long>(m_data.ll);
        break;
    case ULongLong:
        result = m_data.ull;
        break;
    case Float:
        result = static_cast<unsigned long long>(m_data.f);
        break;
    case Double:
        result = static_cast<unsigned long long>(m_data.d);
        break;
    case String:
        result = m_data.str->toULongLong(&success);
        break;
    default:
        success = false;
        break;
    }

    if (ok)
        *ok = success;
    return result;
}

float NVariant::toFloat(bool* ok) const
{
    bool success = true;
    float result = 0.0f;

    switch (m_type) {
    case Bool:
        result = m_data.b ? 1.0f : 0.0f;
        break;
    case Int:
        result = static_cast<float>(m_data.i);
        break;
    case UInt:
        result = static_cast<float>(m_data.ui);
        break;
    case Long:
        result = static_cast<float>(m_data.l);
        break;
    case ULong:
        result = static_cast<float>(m_data.ul);
        break;
    case LongLong:
        result = static_cast<float>(m_data.ll);
        break;
    case ULongLong:
        result = static_cast<float>(m_data.ull);
        break;
    case Float:
        result = m_data.f;
        break;
    case Double:
        result = static_cast<float>(m_data.d);
        break;
    case String:
        result = m_data.str->toFloat(&success);
        break;
    default:
        success = false;
        break;
    }

    if (ok)
        *ok = success;
    return result;
}

double NVariant::toDouble(bool* ok) const
{
    bool success = true;
    double result = 0.0;

    switch (m_type) {
    case Bool:
        result = m_data.b ? 1.0 : 0.0;
        break;
    case Int:
        result = static_cast<double>(m_data.i);
        break;
    case UInt:
        result = static_cast<double>(m_data.ui);
        break;
    case Long:
        result = static_cast<double>(m_data.l);
        break;
    case ULong:
        result = static_cast<double>(m_data.ul);
        break;
    case LongLong:
        result = static_cast<double>(m_data.ll);
        break;
    case ULongLong:
        result = static_cast<double>(m_data.ull);
        break;
    case Float:
        result = static_cast<double>(m_data.f);
        break;
    case Double:
        result = m_data.d;
        break;
    case String:
        result = m_data.str->toDouble(&success);
        break;
    default:
        success = false;
        break;
    }

    if (ok)
        *ok = success;
    return result;
}

NString NVariant::toString() const
{
    char buffer[64];

    switch (m_type) {
    case Bool:
        return NString(m_data.b ? "true" : "false");
    case Int:
        snprintf(buffer, sizeof(buffer), "%d", m_data.i);
        return NString(buffer);
    case UInt:
        snprintf(buffer, sizeof(buffer), "%u", m_data.ui);
        return NString(buffer);
    case Long:
        snprintf(buffer, sizeof(buffer), "%ld", m_data.l);
        return NString(buffer);
    case ULong:
        snprintf(buffer, sizeof(buffer), "%lu", m_data.ul);
        return NString(buffer);
    case LongLong:
        snprintf(buffer, sizeof(buffer), "%lld", m_data.ll);
        return NString(buffer);
    case ULongLong:
        snprintf(buffer, sizeof(buffer), "%llu", m_data.ull);
        return NString(buffer);
    case Float:
        snprintf(buffer, sizeof(buffer), "%g", m_data.f);
        return NString(buffer);
    case Double:
        snprintf(buffer, sizeof(buffer), "%g", m_data.d);
        return NString(buffer);
    case String:
        return *m_data.str;
    case ByteArray:
        return NString(reinterpret_cast<const char*>(m_data.ba->data()), m_data.ba->size());
    case ObjectPtr:
        return NString("NObject(") + NString::number(reinterpret_cast<uintptr_t>(m_data.obj), 16) + NString(")");
    case VoidPtr:
        return NString("void*(") + NString::number(reinterpret_cast<uintptr_t>(m_data.ptr), 16) + NString(")");
    default:
        return NString();
    }
}

NByteArray NVariant::toByteArray() const
{
    switch (m_type) {
    case ByteArray:
        return *m_data.ba;
    case String:
        return NByteArray(m_data.str->c_str(), m_data.str->size());
    case Bool:
        return NByteArray(m_data.b ? "true" : "false", m_data.b ? 4 : 5);
    case Int:
    case UInt:
    case Long:
    case ULong:
    case LongLong:
    case ULongLong:
    case Float:
    case Double:
        return NByteArray(toString().c_str(), toString().size());
    default:
        return NByteArray();
    }
}

NObject* NVariant::toObject(bool* ok) const
{
    if (ok)
        *ok = (m_type == ObjectPtr);
    return (m_type == ObjectPtr) ? m_data.obj : nullptr;
}

void* NVariant::toVoidPtr(bool* ok) const
{
    if (ok)
        *ok = (m_type == VoidPtr || m_type == ObjectPtr);
    return (m_type == VoidPtr) ? m_data.ptr : (m_type == ObjectPtr) ? static_cast<void*>(m_data.obj)
                                                                    : nullptr;
}

void NVariant::setValue(bool value)
{
    freeData();
    m_type = Bool;
    m_data.b = value;
}

void NVariant::setValue(int value)
{
    freeData();
    m_type = Int;
    m_data.i = value;
}

void NVariant::setValue(unsigned int value)
{
    freeData();
    m_type = UInt;
    m_data.ui = value;
}

void NVariant::setValue(long value)
{
    freeData();
    m_type = Long;
    m_data.l = value;
}

void NVariant::setValue(unsigned long value)
{
    freeData();
    m_type = ULong;
    m_data.ul = value;
}

void NVariant::setValue(long long value)
{
    freeData();
    m_type = LongLong;
    m_data.ll = value;
}

void NVariant::setValue(unsigned long long value)
{
    freeData();
    m_type = ULongLong;
    m_data.ull = value;
}

void NVariant::setValue(float value)
{
    freeData();
    m_type = Float;
    m_data.f = value;
}

void NVariant::setValue(double value)
{
    freeData();
    m_type = Double;
    m_data.d = value;
}

void NVariant::setValue(const NString& value)
{
    freeData();
    m_type = String;
    initString(value);
}

void NVariant::setValue(const char* value)
{
    setValue(NString(value ? value : ""));
}

void NVariant::setValue(const NByteArray& value)
{
    freeData();
    m_type = ByteArray;
    initByteArray(value);
}

void NVariant::setValue(NObject* value)
{
    freeData();
    m_type = ObjectPtr;
    m_data.obj = value;
}

void NVariant::setValue(void* value)
{
    freeData();
    m_type = VoidPtr;
    m_data.ptr = value;
}

bool NVariant::operator==(const NVariant& other) const
{
    if (m_type != other.m_type)
        return false;

    switch (m_type) {
    case Invalid:
        return true;
    case Bool:
        return m_data.b == other.m_data.b;
    case Int:
        return m_data.i == other.m_data.i;
    case UInt:
        return m_data.ui == other.m_data.ui;
    case Long:
        return m_data.l == other.m_data.l;
    case ULong:
        return m_data.ul == other.m_data.ul;
    case LongLong:
        return m_data.ll == other.m_data.ll;
    case ULongLong:
        return m_data.ull == other.m_data.ull;
    case Float:
        return m_data.f == other.m_data.f;
    case Double:
        return m_data.d == other.m_data.d;
    case String:
        return *m_data.str == *other.m_data.str;
    case ByteArray:
        return *m_data.ba == *other.m_data.ba;
    case ObjectPtr:
        return m_data.obj == other.m_data.obj;
    case VoidPtr:
        return m_data.ptr == other.m_data.ptr;
    default:
        return false;
    }
}

bool NVariant::operator!=(const NVariant& other) const
{
    return !(*this == other);
}
