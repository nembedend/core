// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "ngpiopin.h"
#include "ndebug.h"
#include "neventloop.h"

#ifdef CONFIG_IDF_TARGET_ESP32C6
#include "driver/ledc.h"
#endif

NGpioPin::NGpioPin(gpio_num_t pin, NObject* parent)
    : NObject(parent)
    , m_pin(pin)
    , m_mode(Mode::Input)
    , m_pull(Pull::NoPull)
    , m_interrupt(Interrupt::Disabled)
    , m_driveStrength(DriveStrength::Default)
    , m_initialized(false)
    , m_debounceMs(0)
    , m_lastDebounceTime(0)
    , m_lastStableState(false)
    , m_longPressMs(1000)
    , m_repeatIntervalMs(100)
    , m_repeatEnabled(false)
    , m_pressStartTime(0)
    , m_longPressTriggered(false)
    , m_lastRepeatTime(0)
    , m_repeatTimer(nullptr)
    , m_pwmEnabled(false)
    , m_pwmChannel(0)
    , m_pwmFreq(1000)
    , m_pwmResolution(8)
    , m_interruptCount(0)
    , m_lastInterruptTime(0)
    , m_totalPinToggles(0)
    , m_eventQueue(nullptr)
{
    applyConfiguration();
    m_eventQueue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(uint32_t));
}

NGpioPin::~NGpioPin()
{
    setInterrupt(Interrupt::Disabled);
    if (m_pwmEnabled)
        stopPWM();
    if (m_repeatTimer)
        xTimerDelete(m_repeatTimer, 0);
    if (m_eventQueue)
        vQueueDelete(m_eventQueue);
}

void NGpioPin::setMode(Mode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    applyConfiguration();
}

void NGpioPin::setPull(Pull pull)
{
    if (m_pull == pull)
        return;
    m_pull = pull;
    applyConfiguration();
}

void NGpioPin::setInterrupt(Interrupt type)
{
    if (m_interrupt == type)
        return;
    m_interrupt = type;
    configureInterrupt();
}

void NGpioPin::setDriveStrength(DriveStrength strength)
{
    if (m_driveStrength == strength)
        return;
    m_driveStrength = strength;
    applyConfiguration();
}

void NGpioPin::setDebounceTime(uint32_t ms)
{
    m_debounceMs = ms;
}

void NGpioPin::setLongPressTime(uint32_t ms)
{
    m_longPressMs = ms;
}

void NGpioPin::setRepeatInterval(uint32_t ms)
{
    m_repeatIntervalMs = ms;
}

void NGpioPin::setRepeatEnabled(bool enabled)
{
    m_repeatEnabled = enabled;
}

void NGpioPin::write(bool value)
{
    if (!m_initialized)
        return;
    if (m_mode == Mode::Output || m_mode == Mode::InputOutput) {
        gpio_set_level(m_pin, value ? 1 : 0);
        m_totalPinToggles++;
    }
}

bool NGpioPin::read() const
{
    if (!m_initialized)
        return false;
    return gpio_get_level(m_pin) == 1;
}

bool NGpioPin::readDebounced()
{
    if (m_debounceMs == 0)
        return read();
    uint32_t now = esp_timer_get_time() / 1000;
    bool current = read();
    if (current != m_lastStableState) {
        m_lastDebounceTime = now;
    }
    if ((now - m_lastDebounceTime) > m_debounceMs) {
        m_lastStableState = current;
    }
    return m_lastStableState;
}

void NGpioPin::toggle()
{
    write(!read());
}

bool NGpioPin::setupPWM(uint32_t freq, uint8_t resolution)
{
#ifdef CONFIG_IDF_TARGET_ESP32C6
    if (m_pwmEnabled)
        stopPWM();
    m_pwmFreq = freq;
    m_pwmResolution = resolution;

    ledc_timer_config_t timerConf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = (ledc_timer_bit_t)resolution,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = freq,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timerConf);

    ledc_channel_config_t channelConf = {
        .gpio_num = m_pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = (ledc_channel_t)m_pwmChannel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channelConf);
    m_pwmEnabled = true;
    return true;
#else
    return false;
#endif
}

void NGpioPin::setPWM(uint8_t dutyCycle)
{
    if (!m_pwmEnabled)
        return;
#ifdef CONFIG_IDF_TARGET_ESP32C6
    uint32_t maxDuty = (1 << m_pwmResolution) - 1;
    uint32_t duty = (dutyCycle * maxDuty) / 255;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)m_pwmChannel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)m_pwmChannel);
#endif
}

void NGpioPin::stopPWM()
{
    if (!m_pwmEnabled)
        return;
#ifdef CONFIG_IDF_TARGET_ESP32C6
    ledc_stop(LEDC_LOW_SPEED_MODE, (ledc_channel_t)m_pwmChannel, 0);
#endif
    m_pwmEnabled = false;
}

bool NGpioPin::applyConfiguration()
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << m_pin);
    switch (m_mode) {
    case Mode::Input:
        cfg.mode = GPIO_MODE_INPUT;
        break;
    case Mode::Output:
        cfg.mode = GPIO_MODE_OUTPUT;
        break;
    case Mode::InputOutput:
        cfg.mode = GPIO_MODE_INPUT_OUTPUT;
        break;
    case Mode::OpenDrain:
        cfg.mode = GPIO_MODE_INPUT_OUTPUT_OD;
        break;
    default:
        cfg.mode = GPIO_MODE_INPUT;
    }
    cfg.pull_up_en = (m_pull == Pull::PullUp) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = (m_pull == Pull::PullDown) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;

    esp_err_t err = gpio_config(&cfg);
    m_initialized = (err == ESP_OK);
    return m_initialized;
}

bool NGpioPin::configureInterrupt()
{
    if (m_interrupt == Interrupt::Disabled) {
        gpio_isr_handler_remove(m_pin);
        return true;
    }

    gpio_int_type_t t = GPIO_INTR_DISABLE;
    switch (m_interrupt) {
    case Interrupt::Rising:
        t = GPIO_INTR_POSEDGE;
        break;
    case Interrupt::Falling:
        t = GPIO_INTR_NEGEDGE;
        break;
    case Interrupt::AnyEdge:
        t = GPIO_INTR_ANYEDGE;
        break;
    case Interrupt::LowLevel:
        t = GPIO_INTR_LOW_LEVEL;
        break;
    case Interrupt::HighLevel:
        t = GPIO_INTR_HIGH_LEVEL;
        break;
    default:
        break;
    }
    gpio_set_intr_type(m_pin, t);
    gpio_install_isr_service(0);
    return (gpio_isr_handler_add(m_pin, isrHandler, this) == ESP_OK);
}

void IRAM_ATTR NGpioPin::isrHandler(void* arg)
{
    NGpioPin* self = static_cast<NGpioPin*>(arg);
    if (self && self->m_eventQueue) {
        uint32_t dummy = 1;
        xQueueSendFromISR(self->m_eventQueue, &dummy, nullptr);
    }
}

void NGpioPin::processInterrupt()
{
    m_interruptCount++;
    m_lastInterruptTime = esp_timer_get_time() / 1000;

    bool current = read();
    static uint32_t lastProcessTime = 0;
    uint32_t now = m_lastInterruptTime;
    if (m_debounceMs > 0 && (now - lastProcessTime) < m_debounceMs)
        return;
    lastProcessTime = now;

    bool old = m_lastStableState;
    m_lastStableState = current;

    if (old != current) {
        valueChanged.emitSignal(current);
        if (current == false) {
            buttonPressed.emitSignal();
            m_pressStartTime = now;
            m_longPressTriggered = false;
        } else {
            buttonReleased.emitSignal();
            m_pressStartTime = 0;
            if (m_repeatTimer) {
                xTimerStop(m_repeatTimer, 0);
                xTimerDelete(m_repeatTimer, 0);
                m_repeatTimer = nullptr;
            }
        }
        if (old == false && current == true)
            risingEdge.emitSignal();
        else if (old == true && current == false)
            fallingEdge.emitSignal();
        valueChangedEvent(old, current);
    }

    if (m_pressStartTime != 0 && !m_longPressTriggered && (now - m_pressStartTime) >= m_longPressMs) {
        m_longPressTriggered = true;
        longPressed.emitSignal();
    }
}

void NGpioPin::checkLongPress() { }
void NGpioPin::checkRepeat() { }
void NGpioPin::interruptHandler() { }
void NGpioPin::valueChangedEvent(bool oldValue, bool newValue)
{
    (void)oldValue;
    (void)newValue;
}

void NGpioPin::resetStats()
{
    m_interruptCount = 0;
    m_totalPinToggles = 0;
}

void NGpioPin::dumpInfo() const
{
    nDebug() << "=== GPIO Pin" << m_pin << "Info ===";
    nDebug() << "Mode:" << (int)m_mode;
    nDebug() << "Pull:" << (int)m_pull;
    nDebug() << "Interrupt:" << (int)m_interrupt;
    nDebug() << "Debounce:" << m_debounceMs << "ms";
    nDebug() << "Value:" << (read() ? "HIGH" : "LOW");
    nDebug() << "Interrupt count:" << m_interruptCount;
}
