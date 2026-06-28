// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <cstdint>
#include <vector>

#include "nbytearray.h"
#include "nstring.h"
#include "nvariant.h"

class NDataStream
{
public:
    enum ByteOrder
    {
        LittleEndian,
        BigEndian
    };

    NDataStream();
    NDataStream(const NByteArray& data);

    void setBuffer(const NByteArray& data);
    NByteArray buffer() const;

    void clear();

    void reset();
    size_t position() const;
    size_t size() const;
    bool atEnd() const;

    void setByteOrder(ByteOrder order);
    ByteOrder byteOrder() const;

    NDataStream& operator<<(int32_t v);
    NDataStream& operator<<(uint32_t v);
    NDataStream& operator<<(float v);
    NDataStream& operator<<(bool v);
    NDataStream& operator<<(const NString& v);
    NDataStream& operator<<(const NByteArray& v);
    NDataStream& operator<<(const NVariant& v);

    NDataStream& operator>>(int32_t& v);
    NDataStream& operator>>(uint32_t& v);
    NDataStream& operator>>(float& v);
    NDataStream& operator>>(bool& v);
    NDataStream& operator>>(NString& v);
    NDataStream& operator>>(NByteArray& v);
    NDataStream& operator>>(NVariant& v);

private:
    std::vector<uint8_t> m_buffer;
    size_t m_pos = 0;
    ByteOrder m_order = LittleEndian;

    void writeRaw(const void* data, size_t size);
    void readRaw(void* data, size_t size);

    template<typename T>
    void writePod(const T& v);

    template<typename T>
    void readPod(T& v);
};
