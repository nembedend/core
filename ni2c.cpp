// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "ni2c.h"
#include "ndebug.h"

static NString hexString(uint8_t addr)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%02X", addr);
    return NString(buf);
}

NI2C::NI2C(i2c_port_t port, NObject* parent)
    : NObject(parent)
    , m_port(port)
    , m_mode(Mode::Master)
    , m_freq(100000)
    , m_timeoutMs(DEFAULT_TIMEOUT_MS)
    , m_pullupEnabled(true)
    , m_initialized(false)
    , m_sdaPin(-1)
    , m_sclPin(-1)
    , m_totalTransfers(0)
    , m_errorCount(0)
{
}

NI2C::~NI2C()
{
    end();
}

bool NI2C::begin(int sdaPin, int sclPin, uint32_t freq)
{
    if (m_initialized)
        end();

    m_sdaPin = sdaPin;
    m_sclPin = sclPin;
    m_freq = freq;

    i2c_config_t config = {};
    config.mode = (m_mode == Mode::Master) ? I2C_MODE_MASTER : I2C_MODE_SLAVE;
    config.sda_io_num = (gpio_num_t)sdaPin;
    config.scl_io_num = (gpio_num_t)sclPin;
    config.sda_pullup_en = m_pullupEnabled ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    config.scl_pullup_en = m_pullupEnabled ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;

    if (m_mode == Mode::Master) {
        config.master.clk_speed = m_freq;
    } else {
        config.slave.addr_10bit_en = 0;
        config.slave.slave_addr = 0x55;
    }

    esp_err_t err = i2c_param_config(m_port, &config);
    if (err != ESP_OK) {
        nError() << "I2C: Failed to configure parameters";
        return false;
    }

    err = i2c_driver_install(m_port, config.mode, 0, 0, 0);
    if (err != ESP_OK) {
        nError() << "I2C: Failed to install driver";
        return false;
    }

    m_initialized = true;
    nDebug() << "I2C" << m_port << "initialized as master, freq:" << m_freq << "Hz";
    return true;
}

void NI2C::end()
{
    if (!m_initialized)
        return;
    i2c_driver_delete(m_port);
    m_initialized = false;
    nDebug() << "I2C" << m_port << "deinitialized";
}

void NI2C::setFrequency(uint32_t freq)
{
    if (m_freq == freq)
        return;
    m_freq = freq;
    if (m_initialized && m_mode == Mode::Master) {

        bool wasOpen = m_initialized;
        int sda = m_sdaPin;
        int scl = m_sclPin;
        end();
        if (wasOpen && sda != -1 && scl != -1) {
            begin(sda, scl, m_freq);
        }
    }
}

void NI2C::setMode(Mode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    if (m_initialized) {
        end();
        if (m_sdaPin != -1 && m_sclPin != -1) {
            begin(m_sdaPin, m_sclPin, m_freq);
        }
    }
}

void NI2C::setTimeOut(uint32_t ms)
{
    m_timeoutMs = ms;
}

std::vector<uint8_t> NI2C::scan()
{
    std::vector<uint8_t> devices;
    if (!m_initialized || m_mode != Mode::Master) {
        nWarning() << "I2C: Not initialized or not in master mode";
        return devices;
    }

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t err = i2c_master_cmd_begin(m_port, cmd, pdMS_TO_TICKS(m_timeoutMs));
        i2c_cmd_link_delete(cmd);

        if (err == ESP_OK) {
            devices.push_back(addr);
            deviceDetected.emitSignal(addr);
            nDebug() << "I2C: Device found at address 0x" << hexString(addr);
        }
    }
    return devices;
}

bool NI2C::isDevicePresent(uint8_t address)
{
    if (!m_initialized || m_mode != Mode::Master)
        return false;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(m_port, cmd, pdMS_TO_TICKS(m_timeoutMs));
    i2c_cmd_link_delete(cmd);

    return (err == ESP_OK);
}

bool NI2C::write(uint8_t address, const uint8_t* data, size_t size)
{
    if (!m_initialized || !data || size == 0)
        return false;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, const_cast<uint8_t*>(data), size, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(m_port, cmd, pdMS_TO_TICKS(m_timeoutMs));
    i2c_cmd_link_delete(cmd);

    m_totalTransfers++;
    if (err != ESP_OK) {
        m_errorCount++;
        error.emitSignal();
        nError() << "I2C: Write failed to 0x" << hexString(address) << ", error:" << err;
        return false;
    }
    transferCompleted.emitSignal(size);
    return true;
}

bool NI2C::write(uint8_t address, const NByteArray& data)
{
    return write(address, data.data(), data.size());
}

bool NI2C::writeByte(uint8_t address, uint8_t data)
{
    return write(address, &data, 1);
}

bool NI2C::writeWord(uint8_t address, uint16_t data)
{
    uint8_t buf[2] = { (uint8_t)(data >> 8), (uint8_t)(data & 0xFF) };
    return write(address, buf, 2);
}

bool NI2C::read(uint8_t address, uint8_t* data, size_t size)
{
    if (!m_initialized || !data || size == 0)
        return false;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(m_port, cmd, pdMS_TO_TICKS(m_timeoutMs));
    i2c_cmd_link_delete(cmd);

    m_totalTransfers++;
    if (err != ESP_OK) {
        m_errorCount++;
        error.emitSignal();
        nError() << "I2C: Read failed from 0x" << hexString(address) << ", error:" << err;
        return false;
    }
    transferCompleted.emitSignal(size);
    return true;
}

NByteArray NI2C::read(uint8_t address, size_t size)
{
    NByteArray result;
    if (size == 0)
        return result;
    result.resize(size);
    if (read(address, result.data(), size)) {
        return result;
    }
    return NByteArray();
}

uint8_t NI2C::readByte(uint8_t address)
{
    uint8_t value = 0;
    read(address, &value, 1);
    return value;
}

uint16_t NI2C::readWord(uint8_t address)
{
    uint8_t buf[2] = { 0, 0 };
    if (read(address, buf, 2)) {
        return (buf[0] << 8) | buf[1];
    }
    return 0;
}

bool NI2C::writeRead(uint8_t address,
    const uint8_t* txData, size_t txSize,
    uint8_t* rxData, size_t rxSize)
{
    if (!m_initialized)
        return false;
    if ((txData && txSize == 0) && (rxData && rxSize == 0))
        return false;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);

    if (txData && txSize > 0) {
        i2c_master_write(cmd, const_cast<uint8_t*>(txData), txSize, true);
    }

    if (rxData && rxSize > 0) {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, rxData, rxSize, I2C_MASTER_LAST_NACK);
    }

    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(m_port, cmd, pdMS_TO_TICKS(m_timeoutMs));
    i2c_cmd_link_delete(cmd);

    m_totalTransfers++;
    if (err != ESP_OK) {
        m_errorCount++;
        error.emitSignal();
        nError() << "I2C: Write-read failed to 0x" << hexString(address);
        return false;
    }
    transferCompleted.emitSignal(txSize + rxSize);
    return true;
}

NByteArray NI2C::writeRead(uint8_t address, const NByteArray& txData, size_t rxSize)
{
    NByteArray result;
    if (rxSize == 0)
        return result;
    result.resize(rxSize);
    if (writeRead(address, txData.data(), txData.size(), result.data(), rxSize)) {
        return result;
    }
    return NByteArray();
}

bool NI2C::writeRegister(uint8_t address, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = { reg, value };
    return write(address, data, 2);
}

uint8_t NI2C::readRegister(uint8_t address, uint8_t reg)
{
    uint8_t value = 0;
    writeRead(address, &reg, 1, &value, 1);
    return value;
}

bool NI2C::readRegister(uint8_t address, uint8_t reg, uint8_t* data, size_t size)
{
    if (!data || size == 0)
        return false;
    return writeRead(address, &reg, 1, data, size);
}

void NI2C::resetStats()
{
    m_totalTransfers = 0;
    m_errorCount = 0;
}

void NI2C::dumpInfo() const
{
    nDebug() << "=== I2C" << m_port << "Info ===";
    nDebug() << "Initialized:" << m_initialized;
    nDebug() << "Mode:" << (m_mode == Mode::Master ? "Master" : "Slave");
    nDebug() << "Frequency:" << m_freq << "Hz";
    nDebug() << "Timeout:" << m_timeoutMs << "ms";
    nDebug() << "Pullup enabled:" << m_pullupEnabled;
    nDebug() << "Total transfers:" << m_totalTransfers;
    nDebug() << "Errors:" << m_errorCount;
}
