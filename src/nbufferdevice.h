// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "niodevice.h"
#include "nbytearray.h"
#include "nsignalbus.h"

class NBufferDevice : public NIODevice {
public:
    explicit NBufferDevice(NObject* parent = nullptr);
    explicit NBufferDevice(const NByteArray& data, NObject* parent = nullptr);
    virtual ~NBufferDevice() = default;

    void setBuffer(const NByteArray& data);
    void setBuffer(NByteArray&& data);
    NByteArray buffer() const;
    NByteArray& buffer();

    void clear();
    bool isEmpty() const { return m_buffer.isEmpty(); }
    int64_t size() const override;

    bool atEnd() const override;
    bool seek(int64_t pos) override;
    int64_t pos() const override { return m_pos; }
    int64_t bytesAvailable() const override;
    bool isSequential() const override { return false; }

    int64_t readData(char* data, int64_t maxSize) override;
    int64_t writeData(const char* data, int64_t size) override;

    NByteArray readAll() override;
    int64_t readLine(char* data, int64_t maxSize) override;

    void append(const NByteArray& data);
    void append(const char* data, int64_t size);
    void append(char ch);

    void insert(int64_t pos, const NByteArray& data);
    void remove(int64_t pos, int64_t len);
    void replace(int64_t pos, int64_t len, const NByteArray& data);

    NSignal<> bufferEmpty;
    NSignal<> bufferFull;
    NSignal<int64_t, int64_t> dataRangeChanged;

    void setMaxSize(int64_t maxSize);
    int64_t maxSize() const { return m_maxSize; }
    void setGrowable(bool growable) { m_growable = growable; }
    bool isGrowable() const { return m_growable; }

    int64_t bytesWritten() const { return m_totalBytesWritten; }
    int64_t bytesRead() const { return m_totalBytesRead; }
    void resetStats();

private:
    NByteArray m_buffer;
    int64_t m_pos;
    int64_t m_maxSize;
    bool m_growable;
    int64_t m_totalBytesWritten;
    int64_t m_totalBytesRead;

    void emitBufferEmptyIfNeeded();
    void emitBufferFullIfNeeded();
    void emitDataRangeChanged(int64_t start, int64_t end);
};
