// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nobject.h"
#include "nbytearray.h"

#include "driver/spi_master.h"

class NSPIBus;

class NSPIDevice : public NObject {
public:
    enum class SPIMode {
        Mode0 = 0,  // CPOL=0, CPHA=0 (rising edge, setup on leading)
        Mode1 = 1,  // CPOL=0, CPHA=1 (rising edge, setup on trailing)
        Mode2 = 2,  // CPOL=1, CPHA=0 (falling edge, setup on leading)
        Mode3 = 3   // CPOL=1, CPHA=1 (falling edge, setup on trailing)
    };

    enum class BitOrder {
        MSBFirst = 0,
        LSBFirst = 1
    };

    enum class TransferFlag {
        None = 0,
        HalfDuplex = 1 << 0,
        KeepCS = 1 << 1,
        NoDMA = 1 << 2
    };

    explicit NSPIDevice(NSPIBus* bus, int csPin,
                        int frequency = 1000000,
                        NObject* parent = nullptr);
    virtual ~NSPIDevice();

    void setFrequency(int freqHz);
    int frequency() const noexcept { return m_frequency; }

    void setMode(SPIMode mode);
    SPIMode mode() const noexcept { return m_mode; }

    void setBitOrder(BitOrder order);
    BitOrder bitOrder() const noexcept { return m_bitOrder; }

    void setCsPin(int pin);
    int csPin() const noexcept { return m_csPin; }

    void setQueueSize(int size);
    int queueSize() const noexcept { return m_queueSize; }

    void setDmaChannel(int channel);
    int dmaChannel() const noexcept { return m_dmaChannel; }
    void setUseDMA(bool enable);
    bool useDMA() const noexcept { return m_useDMA; }

    bool transfer(const uint8_t* tx, uint8_t* rx, size_t length);
    bool transfer(const NByteArray& txData, NByteArray& rxData);
    bool write(const uint8_t* data, size_t length);
    bool write(const NByteArray& data);
    bool read(uint8_t* data, size_t length);
    bool read(NByteArray& data, size_t length);
    bool writeRead(const uint8_t* tx, size_t txLen, uint8_t* rx, size_t rxLen);

    bool transferAsync(const uint8_t* tx, uint8_t* rx, size_t length);
    bool transferAsync(const NByteArray& txData, NByteArray& rxData);
    void waitForCompletion();
    bool isTransferComplete() const noexcept { return m_transferComplete; }
    bool isInitialized() const { return m_initialized; }

    class Transaction {
    public:
        explicit Transaction(NSPIDevice* device);
        ~Transaction();

        bool begin();
        void end();

    private:
        NSPIDevice* m_device;
        bool m_active;
    };

    NSignal<> transferCompleted;
    NSignal<> transferError;
    NSignal<int, int> bytesTransferred;  // tx, rx

    size_t totalTransfers() const noexcept { return m_totalTransfers; }
    size_t totalBytesTransferred() const noexcept { return m_totalBytes; }
    size_t errorCount() const noexcept { return m_errorCount; }
    void resetStats();

    void dumpStats() const;

protected:
    virtual void transferEvent(bool success);

private:
    bool initDevice();
    void deinitDevice();
    void prepareTransaction(spi_transaction_t& trans, const uint8_t* tx, uint8_t* rx, size_t length);
    static void transferCallback(spi_transaction_t* trans);

    NSPIBus* m_bus;
    spi_device_handle_t m_handle;

    int m_csPin;
    int m_frequency;
    SPIMode m_mode;
    BitOrder m_bitOrder;
    int m_queueSize;
    int m_dmaChannel;
    bool m_useDMA;

    bool m_initialized;
    bool m_transferComplete;
    volatile bool m_transferInProgress;

    size_t m_totalTransfers;
    size_t m_totalBytes;
    size_t m_errorCount;

    static constexpr int DEFAULT_QUEUE_SIZE = 1;
    static constexpr int DEFAULT_DMA_CHANNEL = SPI_DMA_CH_AUTO;
};
