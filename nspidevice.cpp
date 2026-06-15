// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nspidevice.h"
#include "ndebug.h"
#include "nspibus.h"

NSPIDevice::NSPIDevice(NSPIBus* bus, int csPin, int frequency, NObject* parent)
    : NObject(parent)
    , m_bus(bus)
    , m_handle(nullptr)
    , m_csPin(csPin)
    , m_frequency(frequency)
    , m_mode(SPIMode::Mode0)
    , m_bitOrder(BitOrder::MSBFirst)
    , m_queueSize(DEFAULT_QUEUE_SIZE)
    , m_dmaChannel(DEFAULT_DMA_CHANNEL)
    , m_useDMA(true)
    , m_initialized(false)
    , m_transferComplete(true)
    , m_transferInProgress(false)
    , m_totalTransfers(0)
    , m_totalBytes(0)
    , m_errorCount(0)
{
    initDevice();
}

NSPIDevice::~NSPIDevice()
{
    deinitDevice();
}

void NSPIDevice::setFrequency(int freqHz)
{
    if (m_frequency == freqHz)
        return;
    m_frequency = freqHz;
    if (m_initialized) {
        deinitDevice();
        initDevice();
    }
}

void NSPIDevice::setMode(SPIMode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    if (m_initialized) {
        deinitDevice();
        initDevice();
    }
}

void NSPIDevice::setBitOrder(BitOrder order)
{
    if (m_bitOrder == order)
        return;
    m_bitOrder = order;
    if (m_initialized) {
        deinitDevice();
        initDevice();
    }
}

void NSPIDevice::setCsPin(int pin)
{
    if (m_csPin == pin)
        return;
    m_csPin = pin;
    if (m_initialized) {
        deinitDevice();
        initDevice();
    }
}

void NSPIDevice::setQueueSize(int size)
{
    if (size <= 0)
        return;
    m_queueSize = size;
    if (m_initialized) {
        deinitDevice();
        initDevice();
    }
}

void NSPIDevice::setDmaChannel(int channel)
{
    m_dmaChannel = channel;
}

void NSPIDevice::setUseDMA(bool enable)
{
    m_useDMA = enable;
}

bool NSPIDevice::initDevice()
{
    if (!m_bus) {
        nError() << "NSPIDevice: No SPI bus provided";
        return false;
    }

    spi_device_interface_config_t cfg = {};
    cfg.clock_speed_hz = m_frequency;
    cfg.mode = static_cast<int>(m_mode);
    cfg.spics_io_num = m_csPin;
    cfg.queue_size = m_queueSize;

    if (m_bitOrder == BitOrder::LSBFirst) {
        cfg.flags |= SPI_DEVICE_BIT_LSBFIRST;
    }

    cfg.flags |= SPI_DEVICE_HALFDUPLEX;

    esp_err_t err = spi_bus_add_device(m_bus->host(), &cfg, &m_handle);

    if (err != ESP_OK) {
        nError() << "NSPIDevice: Failed to add device, error:" << err;
        m_initialized = false;
        return false;
    }

    m_initialized = true;
    nDebug() << "NSPIDevice: Initialized on CS pin" << m_csPin
             << "freq:" << m_frequency << "Hz";

    return true;
}

void NSPIDevice::deinitDevice()
{
    if (!m_initialized)
        return;

    if (m_handle) {
        // Wait for pending transfers
        while (m_transferInProgress) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        spi_bus_remove_device(m_handle);
        m_handle = nullptr;
    }

    m_initialized = false;
    nDebug() << "NSPIDevice: Deinitialized";
}

void NSPIDevice::prepareTransaction(spi_transaction_t& trans,
    const uint8_t* tx,
    uint8_t* rx,
    size_t length)
{
    memset(&trans, 0, sizeof(trans));
    trans.length = length * 8;
    trans.tx_buffer = tx;
    trans.rx_buffer = rx;
    trans.user = this;
}

void NSPIDevice::transferCallback(spi_transaction_t* trans)
{
    NSPIDevice* device = static_cast<NSPIDevice*>(trans->user);

    if (device) {
        device->m_transferInProgress = false;
        device->m_transferComplete = true;

        device->transferCompleted.emitSignal();
        device->transferEvent(true);
        device->m_totalTransfers++;
    }
}

bool NSPIDevice::transfer(const uint8_t* tx, uint8_t* rx, size_t length)
{
    if (!m_initialized || !m_handle) {
        nError() << "NSPIDevice: Not initialized";
        return false;
    }

    if (length == 0)
        return true;

    spi_transaction_t trans;
    prepareTransaction(trans, tx, rx, length);

    m_transferInProgress = true;
    m_transferComplete = false;

    esp_err_t err = spi_device_transmit(m_handle, &trans);

    m_transferInProgress = false;
    m_transferComplete = true;

    if (err != ESP_OK) {
        m_errorCount++;
        nError() << "NSPIDevice: Transfer failed, error:" << err;
        return false;
    }

    m_totalTransfers++;
    m_totalBytes += length;
    bytesTransferred.emitSignal(length, length);

    return true;
}

bool NSPIDevice::transfer(const NByteArray& txData, NByteArray& rxData)
{
    if (txData.size() != rxData.size()) {
        nError() << "NSPIDevice: tx and rx sizes don't match";
        return false;
    }

    return transfer(txData.data(), rxData.data(), txData.size());
}

bool NSPIDevice::write(const uint8_t* data, size_t length)
{
    if (!m_initialized || !m_handle) {
        nError() << "NSPIDevice: Not initialized";
        return false;
    }

    if (length == 0)
        return true;

    spi_transaction_t trans;
    prepareTransaction(trans, data, nullptr, length);

    m_transferInProgress = true;
    m_transferComplete = false;

    esp_err_t err = spi_device_transmit(m_handle, &trans);

    m_transferInProgress = false;
    m_transferComplete = true;

    if (err != ESP_OK) {
        m_errorCount++;
        nError() << "NSPIDevice: Write failed, error:" << err;
        return false;
    }

    m_totalTransfers++;
    m_totalBytes += length;
    bytesTransferred.emitSignal(length, 0);

    return true;
}

bool NSPIDevice::write(const NByteArray& data)
{
    return write(data.data(), data.size());
}

bool NSPIDevice::read(uint8_t* data, size_t length)
{
    if (!m_initialized || !m_handle) {
        nError() << "NSPIDevice: Not initialized";
        return false;
    }

    if (length == 0)
        return true;

    uint8_t* dummy = new uint8_t[length]();

    spi_transaction_t trans;
    prepareTransaction(trans, dummy, data, length);

    m_transferInProgress = true;
    m_transferComplete = false;

    esp_err_t err = spi_device_transmit(m_handle, &trans);

    delete[] dummy;

    m_transferInProgress = false;
    m_transferComplete = true;

    if (err != ESP_OK) {
        m_errorCount++;
        nError() << "NSPIDevice: Read failed, error:" << err;
        return false;
    }

    m_totalTransfers++;
    m_totalBytes += length;
    bytesTransferred.emitSignal(0, length);

    return true;
}

bool NSPIDevice::read(NByteArray& data, size_t length)
{
    data.resize(length);
    return read(data.data(), length);
}

bool NSPIDevice::writeRead(const uint8_t* tx, size_t txLen, uint8_t* rx, size_t rxLen)
{
    if (txLen != rxLen) {
        nError() << "NSPIDevice: tx and rx lengths don't match";
        return false;
    }
    return transfer(tx, rx, txLen);
}

bool NSPIDevice::transferAsync(const uint8_t* tx, uint8_t* rx, size_t length)
{
    return transfer(tx, rx, length);
}

bool NSPIDevice::transferAsync(const NByteArray& txData, NByteArray& rxData)
{
    return transfer(txData, rxData);
}

void NSPIDevice::waitForCompletion()
{
    while (m_transferInProgress) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

NSPIDevice::Transaction::Transaction(NSPIDevice* device)
    : m_device(device)
    , m_active(false)
{
}

NSPIDevice::Transaction::~Transaction()
{
    if (m_active)
        end();
}

bool NSPIDevice::Transaction::begin()
{
    if (!m_device || !m_device->m_initialized)
        return false;
    m_device->waitForCompletion();
    m_active = true;
    return true;
}

void NSPIDevice::Transaction::end()
{
    if (!m_active)
        return;
    m_device->waitForCompletion();
    m_active = false;
}

void NSPIDevice::transferEvent(bool success)
{
    (void)success;
}

void NSPIDevice::resetStats()
{
    m_totalTransfers = 0;
    m_totalBytes = 0;
    m_errorCount = 0;
}

void NSPIDevice::dumpStats() const
{
    nDebug() << "=== NSPIDevice Stats ===";
    nDebug() << "CS Pin:" << m_csPin;
    nDebug() << "Frequency:" << m_frequency << "Hz";
    nDebug() << "Mode:" << static_cast<int>(m_mode);
    nDebug() << "Bit order:" << (m_bitOrder == BitOrder::MSBFirst ? "MSB" : "LSB");
    nDebug() << "Initialized:" << m_initialized;
    nDebug() << "Total transfers:" << m_totalTransfers;
    nDebug() << "Total bytes:" << m_totalBytes;
    nDebug() << "Errors:" << m_errorCount;
}
