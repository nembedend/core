// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nstring.h"
#include "nbytearray.h"
#include <cstdint>
#include <cstring>

class NObject;

class NVariant {
public:
    enum Type {
        Invalid = 0,
        Bool,
        Int,
        UInt,
        Long,
        ULong,
        LongLong,
        ULongLong,
        Float,
        Double,
        String,
        ByteArray,
        ObjectPtr,
        VoidPtr
    };

    NVariant();
    NVariant(bool value);
    NVariant(int value);
    NVariant(unsigned int value);
    NVariant(long value);
    NVariant(unsigned long value);
    NVariant(long long value);
    NVariant(unsigned long long value);
    NVariant(float value);
    NVariant(double value);
    NVariant(const NString& value);
    NVariant(const char* value);
    NVariant(const NByteArray& value);
    NVariant(NObject* value);
    NVariant(void* value);

    NVariant(const NVariant& other);
    NVariant(NVariant&& other) noexcept;
    ~NVariant();

    NVariant& operator=(const NVariant& other);
    NVariant& operator=(NVariant&& other) noexcept;

    Type type() const { return m_type; }
    bool isValid() const { return m_type != Invalid; }
    bool isNull() const { return m_type == Invalid; }

    bool canConvert(Type targetType) const;
    bool canConvertBool() const { return canConvert(Bool); }
    bool canConvertInt() const { return canConvert(Int); }
    bool canConvertFloat() const { return canConvert(Float); }
    bool canConvertString() const { return canConvert(String); }

    void clear();

    bool toBool(bool* ok = nullptr) const;
    int toInt(bool* ok = nullptr) const;
    unsigned int toUInt(bool* ok = nullptr) const;
    long toLong(bool* ok = nullptr) const;
    unsigned long toULong(bool* ok = nullptr) const;
    long long toLongLong(bool* ok = nullptr) const;
    unsigned long long toULongLong(bool* ok = nullptr) const;
    float toFloat(bool* ok = nullptr) const;
    double toDouble(bool* ok = nullptr) const;
    NString toString() const;
    NByteArray toByteArray() const;
    NObject* toObject(bool* ok = nullptr) const;
    void* toVoidPtr(bool* ok = nullptr) const;

    void setValue(bool value);
    void setValue(int value);
    void setValue(unsigned int value);
    void setValue(long value);
    void setValue(unsigned long value);
    void setValue(long long value);
    void setValue(unsigned long long value);
    void setValue(float value);
    void setValue(double value);
    void setValue(const NString& value);
    void setValue(const char* value);
    void setValue(const NByteArray& value);
    void setValue(NObject* value);
    void setValue(void* value);

    bool operator==(const NVariant& other) const;
    bool operator!=(const NVariant& other) const;

    static NVariant fromValue(bool value) { return NVariant(value); }
    static NVariant fromValue(int value) { return NVariant(value); }
    static NVariant fromValue(const NString& value) { return NVariant(value); }
    static NVariant fromValue(NObject* value) { return NVariant(value); }

private:
    union Data {
        bool b;
        int i;
        unsigned int ui;
        long l;
        unsigned long ul;
        long long ll;
        unsigned long long ull;
        float f;
        double d;
        NString* str;
        NByteArray* ba;
        NObject* obj;
        void* ptr;

        Data() : ptr(nullptr) {}
        ~Data() {}
    };

    Type m_type;
    Data m_data;

    void copyData(const NVariant& other);
    void freeData();
    void initString(const NString& value);
    void initByteArray(const NByteArray& value);
};

inline NVariant::NVariant()
    : m_type(Invalid)
{
    m_data.ptr = nullptr;
}

inline NVariant::NVariant(bool value)
    : m_type(Bool)
{
    m_data.b = value;
}

inline NVariant::NVariant(int value)
    : m_type(Int)
{
    m_data.i = value;
}

inline NVariant::NVariant(unsigned int value)
    : m_type(UInt)
{
    m_data.ui = value;
}

inline NVariant::NVariant(long value)
    : m_type(Long)
{
    m_data.l = value;
}

inline NVariant::NVariant(unsigned long value)
    : m_type(ULong)
{
    m_data.ul = value;
}

inline NVariant::NVariant(long long value)
    : m_type(LongLong)
{
    m_data.ll = value;
}

inline NVariant::NVariant(unsigned long long value)
    : m_type(ULongLong)
{
    m_data.ull = value;
}

inline NVariant::NVariant(float value)
    : m_type(Float)
{
    m_data.f = value;
}

inline NVariant::NVariant(double value)
    : m_type(Double)
{
    m_data.d = value;
}

inline NVariant::NVariant(NObject* value)
    : m_type(ObjectPtr)
{
    m_data.obj = value;
}

inline NVariant::NVariant(void* value)
    : m_type(VoidPtr)
{
    m_data.ptr = value;
}
