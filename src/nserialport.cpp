// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nserialport.h"
#include "ndebug.h"

NSerialPort::NSerialPort(uart_port_t port, NObject* parent)
    : NIODevice(parent)
    , m_port(port)
    , m_baudRate(115200)
    , m_dataBits(DataBits::Bits8)
    , m_parity(Parity::None)
    , m_stopBits(StopBits::One)
    , m_flowControl(FlowControl::None)
    , m_txPin(-1)
    , m_rxPin(-1)
    , m_rtsPin(-1)
    , m_ctsPin(-1)
    , m_rxBufferSize(DEFAULT_RX_BUFFER)
    , m_txBufferSize(DEFAULT_TX_BUFFER)
    , m_eventQueue(nullptr)
    , m_eventTask(nullptr)
    , m_running(false)
    , m_rxBytes(0)
    , m_txBytes(0)
    , m_overrunErrors(0)
    , m_framingErrors(0)
    , m_parityErrors(0)
{
}

NSerialPort::~NSerialPort()
{
    close();
}

void NSerialPort::setBaudRate(int baudRate)
{
    if (m_baudRate == baudRate)
        return;
    m_baudRate = baudRate;
    if (isOpen())
        configure();
}

void NSerialPort::setDataBits(DataBits bits)
{
    if (m_dataBits == bits)
        return;
    m_dataBits = bits;
    if (isOpen())
        configure();
}

void NSerialPort::setParity(Parity parity)
{
    if (m_parity == parity)
        return;
    m_parity = parity;
    if (isOpen())
        configure();
}

void NSerialPort::setStopBits(StopBits bits)
{
    if (m_stopBits == bits)
        return;
    m_stopBits = bits;
    if (isOpen())
        configure();
}

void NSerialPort::setFlowControl(FlowControl flow)
{
    if (m_flowControl == flow)
        return;
    m_flowControl = flow;
    if (isOpen())
        configure();
}

void NSerialPort::setPins(int txPin, int rxPin, int rtsPin, int ctsPin)
{
    m_txPin = txPin;
    m_rxPin = rxPin;
    m_rtsPin = rtsPin;
    m_ctsPin = ctsPin;
    if (isOpen())
        configure();
}

void NSerialPort::setBufferSize(int rxBufferSize, int txBufferSize)
{
    m_rxBufferSize = rxBufferSize > 0 ? rxBufferSize : DEFAULT_RX_BUFFER;
    m_txBufferSize = txBufferSize > 0 ? txBufferSize : 0;
    if (isOpen()) {
        close();
        open(openMode());
    }
}

bool NSerialPort::open(OpenMode mode)
{
    if (isOpen())
        close();
    if (!configure()) {
        nError() << "Serial port: Failed to configure";
        return false;
    }

    esp_err_t err = uart_driver_install(m_port, m_rxBufferSize, m_txBufferSize,
        EVENT_QUEUE_SIZE, &m_eventQueue, 0);
    if (err != ESP_OK) {
        nError() << "Serial port: Failed to install driver";
        return false;
    }

    m_running = true;
    xTaskCreate(uartEventTask, "uart_evt", EVENT_TASK_STACK, this,
        EVENT_TASK_PRIORITY, &m_eventTask);

    return NIODevice::open(mode);
}

void NSerialPort::close()
{
    if (!isOpen())
        return;
    m_running = false;
    if (m_eventTask) {
        vTaskDelete(m_eventTask);
        m_eventTask = nullptr;
    }
    if (m_eventQueue) {
        vQueueDelete(m_eventQueue);
        m_eventQueue = nullptr;
    }
    uart_driver_delete(m_port);
    NIODevice::close();
}

bool NSerialPort::configure()
{
    uart_config_t config = {};
    config.baud_rate = m_baudRate;
    config.data_bits = (uart_word_length_t)(int)m_dataBits;
    config.parity = (uart_parity_t)(int)m_parity;
    config.stop_bits = (uart_stop_bits_t)(int)m_stopBits;
    config.flow_ctrl = (uart_hw_flowcontrol_t)(int)m_flowControl;
    config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t err = uart_param_config(m_port, &config);
    if (err != ESP_OK) {
        nError() << "Serial port: param config failed";
        return false;
    }

    int txPin = (m_txPin >= 0) ? m_txPin : UART_PIN_NO_CHANGE;
    int rxPin = (m_rxPin >= 0) ? m_rxPin : UART_PIN_NO_CHANGE;
    int rtsPin = (m_rtsPin >= 0) ? m_rtsPin : UART_PIN_NO_CHANGE;
    int ctsPin = (m_ctsPin >= 0) ? m_ctsPin : UART_PIN_NO_CHANGE;

    err = uart_set_pin(m_port, txPin, rxPin, rtsPin, ctsPin);
    if (err != ESP_OK) {
        nError() << "Serial port: set pins failed";
        return false;
    }

    nDebug() << "Serial port" << m_port << "configured:" << m_baudRate << "baud";
    return true;
}

void NSerialPort::flushInput()
{
    uart_flush_input(m_port);
}

void NSerialPort::flushOutput()
{
    uart_wait_tx_done(m_port, portMAX_DELAY);
}

int64_t NSerialPort::readData(char* data, int64_t maxSize)
{
    if (maxSize <= 0)
        return 0;
    int len = uart_read_bytes(m_port, (uint8_t*)data, maxSize, 0);
    if (len > 0)
        m_rxBytes += len;
    return len;
}

int64_t NSerialPort::writeData(const char* data, int64_t size)
{
    if (size <= 0)
        return 0;
    int written = uart_write_bytes(m_port, data, size);
    if (written > 0)
        m_txBytes += written;
    return written;
}

int64_t NSerialPort::bytesAvailable() const
{
    size_t len = 0;
    uart_get_buffered_data_len(m_port, &len);
    return len;
}

int64_t NSerialPort::bytesToWrite() const
{
    return 0;
}

bool NSerialPort::atEnd() const
{
    return bytesAvailable() == 0;
}

void NSerialPort::uartEventTask(void* arg)
{
    NSerialPort* self = static_cast<NSerialPort*>(arg);
    self->processEvents();
    vTaskDelete(nullptr);
}

void NSerialPort::processEvents()
{
    uart_event_t event;
    while (m_running) {
        if (xQueueReceive(m_eventQueue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
            case UART_DATA:
                readyRead.emitSignal();
                break;

            case UART_FIFO_OVF:
            case UART_BUFFER_FULL:
                uart_flush_input(m_port);
                xQueueReset(m_eventQueue);
                m_overrunErrors++;
                errorOccurred.emitSignal(NIODevice::DeviceError::ResourceError);
                nWarning() << "Serial port: FIFO overflow or buffer full";
                break;

            case UART_PARITY_ERR:
                m_parityErrors++;
                errorOccurred.emitSignal(NIODevice::DeviceError::ReadError);
                nWarning() << "Serial port: Parity error";
                break;

            case UART_FRAME_ERR:
                m_framingErrors++;
                errorOccurred.emitSignal(NIODevice::DeviceError::ReadError);
                nWarning() << "Serial port: Framing error";
                break;

            default:
                break;
            }
        }
    }
}

void NSerialPort::resetStats()
{
    m_rxBytes = 0;
    m_txBytes = 0;
    m_overrunErrors = 0;
    m_framingErrors = 0;
    m_parityErrors = 0;
}

void NSerialPort::dumpInfo() const
{
    nDebug() << "=== Serial Port" << m_port << "Info ===";
    nDebug() << "Open:" << isOpen();
    nDebug() << "Baud rate:" << m_baudRate;
    nDebug() << "Data bits:" << (int)m_dataBits;
    nDebug() << "Parity:" << (int)m_parity;
    nDebug() << "Stop bits:" << (int)m_stopBits;
    nDebug() << "Flow control:" << (int)m_flowControl;
    nDebug() << "RX buffer:" << m_rxBufferSize;
    nDebug() << "TX buffer:" << m_txBufferSize;
    nDebug() << "RX bytes:" << m_rxBytes;
    nDebug() << "TX bytes:" << m_txBytes;
    nDebug() << "Overrun errors:" << m_overrunErrors;
    nDebug() << "Framing errors:" << m_framingErrors;
    nDebug() << "Parity errors:" << m_parityErrors;
}
