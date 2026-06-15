// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nobject.h"
#include "driver/spi_master.h"

class NSPIBus : public NObject {
public:
    enum class Host {
        SPI2 = SPI2_HOST
    };

    enum class DMAChannel {
        Disabled = 0,
        Auto = 1
    };

    explicit NSPIBus(Host host = Host::SPI2, NObject* parent = nullptr);
    virtual ~NSPIBus();

    bool begin(int mosiPin, int misoPin, int sclkPin,
               DMAChannel dma = DMAChannel::Auto);
    void end();
    bool isInitialized() const { return m_initialized; }

    void setMaxFrequency(uint32_t freq);
    uint32_t maxFrequency() const { return m_maxFreq; }
    void setDmaChannel(DMAChannel dma);
    DMAChannel dmaChannel() const { return m_dmaChannel; }

    spi_host_device_t host() const { return static_cast<spi_host_device_t>(m_host); }

    uint32_t totalTransfers() const { return m_totalTransfers; }
    uint32_t errorCount() const { return m_errorCount; }
    void resetStats();

    void dumpInfo() const;

private:
    Host m_host;
    uint32_t m_maxFreq;
    DMAChannel m_dmaChannel;
    bool m_initialized;
    uint32_t m_totalTransfers;
    uint32_t m_errorCount;

    int m_mosiPin;
    int m_misoPin;
    int m_sclkPin;
};
