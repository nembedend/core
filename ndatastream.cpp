// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "ndatastream.h"
#include <cstring>

NDataStream::NDataStream() { }

NDataStream::NDataStream(const NByteArray& data)
{
    m_buffer.assign(data.data(), data.data() + data.size());
}

void NDataStream::setBuffer(const NByteArray& data)
{
    m_buffer.assign(data.data(), data.data() + data.size());
    m_pos = 0;
}

NByteArray NDataStream::buffer() const
{
    return NByteArray(
        reinterpret_cast<const char*>(m_buffer.data()),
        m_buffer.size());
}

void NDataStream::clear()
{
    m_buffer.clear();
    m_pos = 0;
}

void NDataStream::reset()
{
    m_pos = 0;
}

size_t NDataStream::position() const
{
    return m_pos;
}

size_t NDataStream::size() const
{
    return m_buffer.size();
}

bool NDataStream::atEnd() const
{
    return m_pos >= m_buffer.size();
}

void NDataStream::setByteOrder(ByteOrder order)
{
    m_order = order;
}

NDataStream::ByteOrder NDataStream::byteOrder() const
{
    return m_order;
}

void NDataStream::writeRaw(const void* data, size_t size)
{
    const uint8_t* p = (const uint8_t*)data;
    m_buffer.insert(m_buffer.end(), p, p + size);
}

void NDataStream::readRaw(void* data, size_t size)
{
    if (m_pos + size > m_buffer.size())
        return;

    memcpy(data, m_buffer.data() + m_pos, size);
    m_pos += size;
}

template <typename T>
void NDataStream::writePod(const T& v)
{
    writeRaw(&v, sizeof(T));
}

template <typename T>
void NDataStream::readPod(T& v)
{
    readRaw(&v, sizeof(T));
}

NDataStream& NDataStream::operator<<(int32_t v)
{
    writePod(v);
    return *this;
}

NDataStream& NDataStream::operator>>(int32_t& v)
{
    readPod(v);
    return *this;
}

NDataStream& NDataStream::operator<<(uint32_t v)
{
    writePod(v);
    return *this;
}

NDataStream& NDataStream::operator>>(uint32_t& v)
{
    readPod(v);
    return *this;
}

NDataStream& NDataStream::operator<<(float v)
{
    writePod(v);
    return *this;
}

NDataStream& NDataStream::operator>>(float& v)
{
    readPod(v);
    return *this;
}

NDataStream& NDataStream::operator<<(bool v)
{
    writeRaw(&v, sizeof(v));
    return *this;
}

NDataStream& NDataStream::operator>>(bool& v)
{
    readRaw(&v, sizeof(v));
    return *this;
}

NDataStream& NDataStream::operator<<(const NString& v)
{
    uint32_t size = v.size();
    (*this) << size;
    writeRaw(v.c_str(), size);
    return *this;
}

NDataStream& NDataStream::operator>>(NString& v)
{
    uint32_t size = 0;
    (*this) >> size;

    if (size == 0 || m_pos + size > m_buffer.size())
        return *this;

    v = NString((const char*)m_buffer.data() + m_pos, size);
    m_pos += size;

    return *this;
}

NDataStream& NDataStream::operator<<(const NByteArray& v)
{
    uint32_t size = v.size();
    (*this) << size;
    writeRaw(v.data(), size);
    return *this;
}

NDataStream& NDataStream::operator>>(NByteArray& v)
{
    uint32_t size = 0;
    (*this) >> size;

    if (size == 0 || m_pos + size > m_buffer.size())
        return *this;

    v = NByteArray(m_buffer.data() + m_pos, size);
    m_pos += size;

    return *this;
}

NDataStream& NDataStream::operator<<(const NVariant& v)
{
    int type = v.type();
    (*this) << static_cast<int32_t>(type);

    switch (type) {
    case NVariant::Int:
        (*this) << static_cast<int32_t>(v.toInt());
        break;
    case NVariant::Float:
        (*this) << v.toFloat();
        break;
    case NVariant::Bool:
        (*this) << v.toBool();
        break;
    case NVariant::String: {
        NString s = v.toString();
        (*this) << s;
        break;
    }
    case NVariant::ByteArray: {
        NByteArray ba = v.toByteArray();
        (*this) << ba;
        break;
    }
    default:
        break;
    }

    return *this;
}

NDataStream& NDataStream::operator>>(NVariant& v)
{
    int32_t type;
    (*this) >> type;

    switch (type) {
    case NVariant::Int: {
        int32_t x;
        (*this) >> x;
        v.setValue(x);
        break;
    }
    case NVariant::Float: {
        float x;
        (*this) >> x;
        v.setValue(x);
        break;
    }
    case NVariant::Bool: {
        bool x;
        (*this) >> x;
        v.setValue(x);
        break;
    }
    case NVariant::String: {
        NString s;
        (*this) >> s;
        v.setValue(s);
        break;
    }
    case NVariant::ByteArray: {
        NByteArray b;
        (*this) >> b;
        v.setValue(b);
        break;
    }
    default:
        v.clear();
        break;
    }

    return *this;
}
