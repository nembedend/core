// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "niodevice.h"
#include "ndebug.h"
#include "neventloop.h"

NIODevice::NIODevice(NObject* parent)
    : NObject(parent)
    , m_isOpen(false)
    , m_openMode(NotOpen)
    , m_error(DeviceError::NoError)
    , m_pos(0)
    , m_readTimerId(0)
    , m_writeTimerId(0)
{
}

NIODevice::~NIODevice()
{
    close();
}

bool NIODevice::open(OpenMode mode)
{
    if (m_isOpen)
        close();
    m_openMode = mode;
    m_isOpen = true;
    m_pos = 0;
    m_error = DeviceError::NoError;
    m_errorString.clear();
    return true;
}

void NIODevice::close()
{
    if (!m_isOpen)
        return;
    aboutToClose.emitSignal();
    m_isOpen = false;
    m_openMode = NotOpen;
}

NString NIODevice::errorString() const
{
    if (!m_errorString.isEmpty())
        return m_errorString;
    switch (m_error) {
    case DeviceError::NoError:
        return NString("No error");
    case DeviceError::ReadError:
        return NString("Read error");
    case DeviceError::WriteError:
        return NString("Write error");
    case DeviceError::FatalError:
        return NString("Fatal error");
    case DeviceError::ResourceError:
        return NString("Resource error");
    case DeviceError::OpenError:
        return NString("Open error");
    case DeviceError::ConnectError:
        return NString("Connect error");
    case DeviceError::PermissionError:
        return NString("Permission error");
    case DeviceError::TimeoutError:
        return NString("Timeout error");
    default:
        return NString("Unknown error");
    }
}

int64_t NIODevice::read(char* data, int64_t maxSize)
{
    if (!m_isOpen || !(m_openMode & ReadOnly)) {
        m_error = DeviceError::ReadError;
        m_errorString = "Device not open for reading";
        return -1;
    }
    if (maxSize <= 0)
        return 0;
    int64_t bytesRead = readData(data, maxSize);
    if (bytesRead > 0)
        m_pos += bytesRead;
    else if (bytesRead < 0)
        m_error = DeviceError::ReadError;
    return bytesRead;
}

int64_t NIODevice::readLine(char* data, int64_t maxSize)
{
    if (!m_isOpen || !(m_openMode & ReadOnly))
        return -1;
    if (maxSize <= 0)
        return 0;
    int64_t bytesRead = 0;
    char* ptr = data;
    while (bytesRead < maxSize - 1) {
        int64_t chunk = readData(ptr, 1);
        if (chunk <= 0)
            break;
        bytesRead += chunk;
        ptr += chunk;
        if (*(ptr - 1) == '\n')
            break;
    }
    *ptr = '\0';
    m_pos += bytesRead;
    return bytesRead;
}

NByteArray NIODevice::read(int64_t maxSize)
{
    if (maxSize <= 0)
        return NByteArray();
    NByteArray result;
    result.resize(maxSize);
    int64_t bytesRead = read((char*)result.data(), maxSize);
    if (bytesRead <= 0)
        return NByteArray();
    result.resize(bytesRead);
    return result;
}

NByteArray NIODevice::readAll()
{
    NByteArray result;
    int64_t available = bytesAvailable();
    if (available <= 0)
        return result;
    result.resize(available);
    int64_t bytesRead = read((char*)result.data(), available);
    if (bytesRead < available)
        result.resize(bytesRead);
    return result;
}

int64_t NIODevice::bytesToWrite() const { return 0; }

int64_t NIODevice::write(const char* data, int64_t size)
{
    if (!m_isOpen || !(m_openMode & WriteOnly)) {
        m_error = DeviceError::WriteError;
        m_errorString = "Device not open for writing";
        return -1;
    }
    if (size <= 0)
        return 0;
    int64_t written = writeData(data, size);
    if (written > 0) {
        m_pos += written;
        bytesWrittenCount.emitSignal(written);
        bytesWritten.emitSignal();
    } else if (written < 0) {
        m_error = DeviceError::WriteError;
    }
    return written;
}

int64_t NIODevice::write(const NByteArray& data)
{
    return write((const char*)data.data(), data.size());
}

int64_t NIODevice::write(const char* str)
{
    if (!str)
        return 0;
    return write(str, strlen(str));
}

bool NIODevice::seek(int64_t pos)
{
    if (!m_isOpen || isSequential())
        return false;
    if (pos < 0)
        return false;
    m_pos = pos;
    return true;
}

bool NIODevice::waitForReadyRead(int msecs)
{
    if (!m_isOpen)
        return false;
    volatile bool ready = false;
    NMetaConnection conn = readyRead.connect([&ready]() { ready = true; });
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout = pdMS_TO_TICKS(msecs);
    while (!ready && (msecs < 0 || (xTaskGetTickCount() - start) < timeout)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    conn.disconnect();
    return ready;
}

bool NIODevice::waitForBytesWritten(int msecs)
{
    if (!m_isOpen)
        return false;
    volatile bool written = false;
    NMetaConnection conn = bytesWritten.connect([&written]() { written = true; });
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout = pdMS_TO_TICKS(msecs);
    while (!written && (msecs < 0 || (xTaskGetTickCount() - start) < timeout)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    conn.disconnect();
    return written;
}

void NIODevice::timerEvent(NTimerEvent* event)
{
    if (event->timerId() == m_readTimerId) {
        killTimer(m_readTimerId);
        m_readTimerId = 0;
        m_error = DeviceError::TimeoutError;
        errorOccurred.emitSignal(m_error);
    } else if (event->timerId() == m_writeTimerId) {
        killTimer(m_writeTimerId);
        m_writeTimerId = 0;
        m_error = DeviceError::TimeoutError;
        errorOccurred.emitSignal(m_error);
    }
}

NString NIODevice::tr(const char* sourceText)
{
    return NString(sourceText);
}

NString NIODevice::tr(const char* sourceText, const char* disambiguation)
{
    (void)disambiguation;
    return NString(sourceText);
}
