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
#include "driver/uart.h"

class NSerialPort : public NIODevice {
public:
    enum class Parity {
        None = 0,
        Even = 1,
        Odd = 2
    };

    enum class StopBits {
        One = 1,
        OnePointFive = 2,
        Two = 3
    };

    enum class DataBits {
        Bits5 = 5,
        Bits6 = 6,
        Bits7 = 7,
        Bits8 = 8
    };

    enum class FlowControl {
        None = 0,
        Hardware = 1,
        Software = 2
    };

    explicit NSerialPort(uart_port_t port = UART_NUM_0, NObject* parent = nullptr);
    virtual ~NSerialPort();

    void setBaudRate(int baudRate);
    int baudRate() const { return m_baudRate; }

    void setDataBits(DataBits bits);
    DataBits dataBits() const { return m_dataBits; }

    void setParity(Parity parity);
    Parity parity() const { return m_parity; }

    void setStopBits(StopBits bits);
    StopBits stopBits() const { return m_stopBits; }

    void setFlowControl(FlowControl flow);
    FlowControl flowControl() const { return m_flowControl; }

    void setPins(int txPin, int rxPin, int rtsPin = -1, int ctsPin = -1);

    void setBufferSize(int rxBufferSize, int txBufferSize = 0);

    bool open(OpenMode mode) override;
    void close() override;

    int64_t readData(char* data, int64_t maxSize) override;
    int64_t writeData(const char* data, int64_t size) override;

    int64_t bytesAvailable() const override;
    int64_t bytesToWrite() const override;

    bool atEnd() const override;
    bool isSequential() const override { return true; }

    uint32_t rxBytesCount() const { return m_rxBytes; }
    uint32_t txBytesCount() const { return m_txBytes; }
    uint32_t overrunErrors() const { return m_overrunErrors; }
    uint32_t framingErrors() const { return m_framingErrors; }
    uint32_t parityErrors() const { return m_parityErrors; }
    void resetStats();

    void dumpInfo() const;

private:
    bool configure();
    void flushInput();
    void flushOutput();

    static void uartEventTask(void* arg);
    void processEvents();

    uart_port_t m_port;

    int m_baudRate;
    DataBits m_dataBits;
    Parity m_parity;
    StopBits m_stopBits;
    FlowControl m_flowControl;

    int m_txPin;
    int m_rxPin;
    int m_rtsPin;
    int m_ctsPin;

    int m_rxBufferSize;
    int m_txBufferSize;

    QueueHandle_t m_eventQueue;
    TaskHandle_t m_eventTask;
    volatile bool m_running;

    uint32_t m_rxBytes;
    uint32_t m_txBytes;
    uint32_t m_overrunErrors;
    uint32_t m_framingErrors;
    uint32_t m_parityErrors;

    uint32_t m_lastStatus;

    static constexpr int DEFAULT_RX_BUFFER = 1024;
    static constexpr int DEFAULT_TX_BUFFER = 512;
    static constexpr int EVENT_QUEUE_SIZE = 20;
    static constexpr int EVENT_TASK_STACK = 4096;
    static constexpr UBaseType_t EVENT_TASK_PRIORITY = 3;
};
