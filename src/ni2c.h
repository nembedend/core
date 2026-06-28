// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nobject.h"
#include "nbytearray.h"
#include "nsignalbus.h"
#include "driver/i2c.h"

#include <vector>

class NI2C : public NObject {
public:
    enum class Mode { Master = 0, Slave = 1 };

    explicit NI2C(i2c_port_t port = I2C_NUM_0, NObject* parent = nullptr);
    virtual ~NI2C();

    bool begin(int sdaPin, int sclPin, uint32_t freq = 100000);
    void end();
    bool isInitialized() const { return m_initialized; }

    void setFrequency(uint32_t freq);
    uint32_t frequency() const { return m_freq; }

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    void setTimeOut(uint32_t ms);
    uint32_t timeOut() const { return m_timeoutMs; }

    void setPullupEnabled(bool enable) { m_pullupEnabled = enable; }
    bool pullupEnabled() const { return m_pullupEnabled; }

    std::vector<uint8_t> scan();
    bool isDevicePresent(uint8_t address);

    bool write(uint8_t address, const uint8_t* data, size_t size);
    bool write(uint8_t address, const NByteArray& data);
    bool writeByte(uint8_t address, uint8_t data);
    bool writeWord(uint8_t address, uint16_t data);

    bool read(uint8_t address, uint8_t* data, size_t size);
    NByteArray read(uint8_t address, size_t size);
    uint8_t readByte(uint8_t address);
    uint16_t readWord(uint8_t address);

    bool writeRead(uint8_t address,
                   const uint8_t* txData, size_t txSize,
                   uint8_t* rxData, size_t rxSize);
    NByteArray writeRead(uint8_t address, const NByteArray& txData, size_t rxSize);

    bool writeRegister(uint8_t address, uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t address, uint8_t reg);
    bool readRegister(uint8_t address, uint8_t reg, uint8_t* data, size_t size);

    NSignal<> error;
    NSignal<int> transferCompleted;
    NSignal<int> deviceDetected;

    uint32_t totalTransfers() const { return m_totalTransfers; }
    uint32_t errorCount() const { return m_errorCount; }
    void resetStats();

    void dumpInfo() const;

private:
    i2c_port_t m_port;
    Mode m_mode;
    uint32_t m_freq;
    uint32_t m_timeoutMs;
    bool m_pullupEnabled;
    bool m_initialized;

    int m_sdaPin;
    int m_sclPin;

    uint32_t m_totalTransfers;
    uint32_t m_errorCount;

    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 100;
};
