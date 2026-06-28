// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <cstdint>
#include "nobject.h"
#include "nbytearray.h"
#include "nsignalbus.h"

class NIODevice : public NObject {
public:
    enum OpenModeFlag {
        NotOpen     = 0x0000,
        ReadOnly    = 0x0001,
        WriteOnly   = 0x0002,
        ReadWrite   = ReadOnly | WriteOnly,
        Append      = 0x0004,
        Truncate    = 0x0008,
        Text        = 0x0010,
        Unbuffered  = 0x0020
    };
    using OpenMode = int;

    enum class DeviceError {
        NoError,
        ReadError,
        WriteError,
        FatalError,
        ResourceError,
        OpenError,
        ConnectError,
        PermissionError,
        TimeoutError
    };

    explicit NIODevice(NObject* parent = nullptr);
    virtual ~NIODevice();

    virtual bool open(OpenMode mode);
    virtual void close();
    bool isOpen() const { return m_isOpen; }
    bool isReadable() const { return m_isOpen && (m_openMode & ReadOnly); }
    bool isWritable() const { return m_isOpen && (m_openMode & WriteOnly); }
    OpenMode openMode() const { return m_openMode; }

    DeviceError error() const { return m_error; }
    void setError(DeviceError error) { m_error = error; }
    void clearError() { m_error = DeviceError::NoError; }
    NString errorString() const;

    virtual int64_t read(char* data, int64_t maxSize);
    virtual int64_t readLine(char* data, int64_t maxSize);
    virtual NByteArray read(int64_t maxSize);
    virtual NByteArray readAll();
    virtual bool atEnd() const { return bytesAvailable() == 0; }
    virtual int64_t bytesAvailable() const { return 0; }
    virtual int64_t bytesToWrite() const;

    virtual int64_t write(const char* data, int64_t size);
    virtual int64_t write(const NByteArray& data);
    virtual int64_t write(const char* str);
    int64_t write(char ch) { return write(&ch, 1); }

    virtual bool isSequential() const { return true; }
    virtual bool seek(int64_t pos);
    virtual int64_t pos() const { return m_pos; }
    virtual int64_t size() const { return 0; }
    virtual bool reset() { return seek(0); }

    virtual bool waitForReadyRead(int msecs);
    virtual bool waitForBytesWritten(int msecs);

    // Сигналы
    NSignal<> readyRead;
    NSignal<> bytesWritten;           // сигнал, а не int64_t
    NSignal<int64_t> bytesWrittenCount;
    NSignal<DeviceError> errorOccurred;
    NSignal<> aboutToClose;

    static NString tr(const char* sourceText);
    static NString tr(const char* sourceText, const char* disambiguation);

protected:
    virtual int64_t readData(char* data, int64_t maxSize) = 0;
    virtual int64_t writeData(const char* data, int64_t size) = 0;
    virtual void timerEvent(NTimerEvent* event) override;

    void setOpenMode(OpenMode mode) { m_openMode = mode; }
    void setErrorString(const NString& str) { m_errorString = str; }

private:
    bool m_isOpen;
    OpenMode m_openMode;
    DeviceError m_error;
    NString m_errorString;
    int64_t m_pos;
    int m_readTimerId;
    int m_writeTimerId;

    static constexpr int DEFAULT_TIMEOUT_MS = 30000;
};
