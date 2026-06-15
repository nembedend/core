// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nbufferdevice.h"
#include "ndebug.h"

NBufferDevice::NBufferDevice(NObject* parent)
    : NIODevice(parent)
    , m_pos(0)
    , m_maxSize(-1)
    , m_growable(true)
    , m_totalBytesWritten(0)
    , m_totalBytesRead(0)
{
}

NBufferDevice::NBufferDevice(const NByteArray& data, NObject* parent)
    : NIODevice(parent)
    , m_buffer(data)
    , m_pos(0)
    , m_maxSize(-1)
    , m_growable(true)
    , m_totalBytesWritten(0)
    , m_totalBytesRead(0)
{
}

void NBufferDevice::setBuffer(const NByteArray& data)
{
    m_buffer = data;
    m_pos = 0;
    m_totalBytesWritten = 0;
    m_totalBytesRead = 0;
    emitDataRangeChanged(0, m_buffer.size());
    emitBufferEmptyIfNeeded();
    emitBufferFullIfNeeded();
}

void NBufferDevice::setBuffer(NByteArray&& data)
{
    m_buffer = std::move(data);
    m_pos = 0;
    m_totalBytesWritten = 0;
    m_totalBytesRead = 0;
    emitDataRangeChanged(0, m_buffer.size());
    emitBufferEmptyIfNeeded();
    emitBufferFullIfNeeded();
}

NByteArray NBufferDevice::buffer() const
{
    return m_buffer;
}

NByteArray& NBufferDevice::buffer()
{
    return m_buffer;
}

void NBufferDevice::clear()
{
    m_buffer.clear();
    m_pos = 0;
    emitBufferEmptyIfNeeded();
    emitDataRangeChanged(0, 0);
}

int64_t NBufferDevice::size() const
{
    return m_buffer.size();
}

bool NBufferDevice::atEnd() const
{
    return m_pos >= static_cast<int64_t>(m_buffer.size());
}

bool NBufferDevice::seek(int64_t pos)
{
    if (pos < 0)
        return false;

    if (pos > static_cast<int64_t>(m_buffer.size())) {
        if (!m_growable)
            return false;
        m_buffer.resize(pos);
    }

    m_pos = pos;
    return true;
}

int64_t NBufferDevice::bytesAvailable() const
{
    if (!isOpen())
        return 0;
    return m_buffer.size() - m_pos;
}

int64_t NBufferDevice::readData(char* data, int64_t maxSize)
{
    if (maxSize <= 0)
        return 0;

    int64_t available = bytesAvailable();
    if (available <= 0)
        return 0;

    int64_t toRead = (maxSize < available) ? maxSize : available;

    memcpy(data, m_buffer.data() + m_pos, toRead);
    m_pos += toRead;
    m_totalBytesRead += toRead;

    if (bytesAvailable() == 0) {
        emitBufferEmptyIfNeeded();
    }

    return toRead;
}

int64_t NBufferDevice::writeData(const char* data, int64_t size)
{
    if (!isWritable())
        return -1;
    if (size <= 0)
        return 0;

    if (m_maxSize > 0 && m_pos + size > m_maxSize) {
        if (!m_growable) {
            setError(DeviceError::ResourceError);
            setErrorString(NString("Buffer max size exceeded"));
            return -1;
        }
        size = m_maxSize - m_pos;
        if (size <= 0)
            return 0;
    }

    if (m_pos + size > static_cast<int64_t>(m_buffer.size())) {
        if (!m_growable) {
            size = m_buffer.size() - m_pos;
            if (size <= 0) {
                setError(DeviceError::ResourceError);
                return -1;
            }
        } else {
            m_buffer.resize(m_pos + size);
        }
    }

    memcpy(m_buffer.data() + m_pos, data, size);
    m_pos += size;
    m_totalBytesWritten += size;

    readyRead.emitSignal();
    emitDataRangeChanged(m_pos - size, m_pos);
    emitBufferFullIfNeeded();

    return size;
}

NByteArray NBufferDevice::readAll()
{
    NByteArray result = NIODevice::readAll();
    emitBufferEmptyIfNeeded();
    return result;
}

int64_t NBufferDevice::readLine(char* data, int64_t maxSize)
{
    if (maxSize <= 0)
        return 0;

    int64_t bytesRead = 0;
    char* ptr = data;
    int64_t available = bytesAvailable();

    while (bytesRead < maxSize - 1 && bytesRead < available) {
        char ch = static_cast<char>(m_buffer[m_pos + bytesRead]);
        *ptr++ = ch;
        bytesRead++;

        if (ch == '\n')
            break;
    }

    if (bytesRead > 0) {
        *ptr = '\0';
        m_pos += bytesRead;
        m_totalBytesRead += bytesRead;
    }

    return bytesRead;
}

void NBufferDevice::append(const NByteArray& data)
{
    if (!isWritable())
        return;

    int64_t oldSize = m_buffer.size();
    m_buffer.append(data);

    if (m_maxSize > 0 && m_buffer.size() > static_cast<size_t>(m_maxSize)) {
        if (!m_growable) {
            m_buffer.resize(m_maxSize);
            setError(DeviceError::ResourceError);
            setErrorString(NString("Buffer max size exceeded"));
            return;
        }
    }

    emitDataRangeChanged(oldSize, m_buffer.size());
    readyRead.emitSignal();
    emitBufferEmptyIfNeeded();
    emitBufferFullIfNeeded();
}

void NBufferDevice::append(const char* data, int64_t size)
{
    if (!isWritable())
        return;
    append(NByteArray(data, size));
}

void NBufferDevice::append(char ch)
{
    append(&ch, 1);
}

void NBufferDevice::insert(int64_t pos, const NByteArray& data)
{
    if (!isWritable())
        return;
    if (pos < 0)
        pos = 0;
    if (pos > static_cast<int64_t>(m_buffer.size()))
        pos = m_buffer.size();

    m_buffer.insert(pos, data);
    if (m_pos >= pos) {
        m_pos += data.size();
    }

    emitDataRangeChanged(pos, m_buffer.size());
    readyRead.emitSignal();
    emitBufferFullIfNeeded();
}

void NBufferDevice::remove(int64_t pos, int64_t len)
{
    if (!isWritable())
        return;
    if (pos < 0)
        pos = 0;
    if (len <= 0)
        return;

    if (pos + len > static_cast<int64_t>(m_buffer.size())) {
        len = m_buffer.size() - pos;
    }

    m_buffer.remove(pos, len);

    if (m_pos >= pos) {
        m_pos = (m_pos > pos + len) ? m_pos - len : pos;
    }

    emitDataRangeChanged(pos, m_buffer.size());
    emitBufferEmptyIfNeeded();
    emitBufferFullIfNeeded();
}

void NBufferDevice::replace(int64_t pos, int64_t len, const NByteArray& data)
{
    if (!isWritable())
        return;

    remove(pos, len);
    insert(pos, data);
}

void NBufferDevice::setMaxSize(int64_t maxSize)
{
    m_maxSize = maxSize;
    if (m_maxSize > 0 && static_cast<int64_t>(m_buffer.size()) > m_maxSize) {
        m_buffer.resize(m_maxSize);
        if (m_pos > m_maxSize) {
            m_pos = m_maxSize;
        }
        emitBufferFullIfNeeded();
    }
}

void NBufferDevice::resetStats()
{
    m_totalBytesWritten = 0;
    m_totalBytesRead = 0;
}

void NBufferDevice::emitBufferEmptyIfNeeded()
{
    if (m_buffer.isEmpty()) {
        bufferEmpty.emitSignal();
    }
}

void NBufferDevice::emitBufferFullIfNeeded()
{
    if (m_maxSize > 0 && static_cast<int64_t>(m_buffer.size()) >= m_maxSize) {
        bufferFull.emitSignal();
    }
}

void NBufferDevice::emitDataRangeChanged(int64_t start, int64_t end)
{
    dataRangeChanged.emitSignal(start, end);
}
