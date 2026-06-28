// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nspibus.h"
#include "ndebug.h"

NSPIBus::NSPIBus(Host host, NObject* parent)
    : NObject(parent)
    , m_host(host)
    , m_maxFreq(1000000)
    , m_dmaChannel(DMAChannel::Auto)
    , m_initialized(false)
    , m_totalTransfers(0)
    , m_errorCount(0)
    , m_mosiPin(-1)
    , m_misoPin(-1)
    , m_sclkPin(-1)
{
}

NSPIBus::~NSPIBus()
{
    end();
}

bool NSPIBus::begin(int mosiPin, int misoPin, int sclkPin, DMAChannel dma)
{
    if (m_initialized) {
        end();
    }

    m_mosiPin = mosiPin;
    m_misoPin = misoPin;
    m_sclkPin = sclkPin;
    m_dmaChannel = dma;

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = m_mosiPin;
    buscfg.miso_io_num = m_misoPin;
    buscfg.sclk_io_num = m_sclkPin;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 4096;

    // Используем SPI_DMA_CH_AUTO (1) для ESP32-C6
    int dmaChannelInt = (m_dmaChannel == DMAChannel::Disabled) ? 0 : SPI_DMA_CH_AUTO;

    esp_err_t err = spi_bus_initialize(static_cast<spi_host_device_t>(m_host),
        &buscfg, dmaChannelInt);
    if (err != ESP_OK) {
        nError() << "SPI bus: Failed to initialize host" << (int)m_host << " error:" << err;
        return false;
    }

    m_initialized = true;
    nDebug() << "SPI bus" << (int)m_host << "initialized";
    return true;
}

void NSPIBus::end()
{
    if (!m_initialized)
        return;

    esp_err_t err = spi_bus_free(static_cast<spi_host_device_t>(m_host));
    if (err != ESP_OK) {
        nWarning() << "SPI bus: Failed to free host" << (int)m_host;
    }
    m_initialized = false;
    nDebug() << "SPI bus" << (int)m_host << "deinitialized";
}

void NSPIBus::setMaxFrequency(uint32_t freq)
{
    m_maxFreq = freq;
}

void NSPIBus::setDmaChannel(DMAChannel dma)
{
    if (m_dmaChannel == dma)
        return;
    m_dmaChannel = dma;
    if (m_initialized) {
        nWarning() << "SPI bus: DMA channel change requires bus restart";
    }
}

void NSPIBus::resetStats()
{
    m_totalTransfers = 0;
    m_errorCount = 0;
}

void NSPIBus::dumpInfo() const
{
    nDebug() << "=== SPI Bus" << (int)m_host << "Info ===";
    nDebug() << "Initialized:" << m_initialized;
    nDebug() << "Max frequency:" << m_maxFreq << "Hz";
    nDebug() << "DMA channel:" << (m_dmaChannel == DMAChannel::Disabled ? "Disabled" : "Auto");
    nDebug() << "Pins: MOSI=" << m_mosiPin << ", MISO=" << m_misoPin << ", SCLK=" << m_sclkPin;
    nDebug() << "Total transfers:" << m_totalTransfers;
    nDebug() << "Errors:" << m_errorCount;
}
