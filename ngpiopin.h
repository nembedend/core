// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nobject.h"
#include "nsignalbus.h"
#include "driver/gpio.h"

class NGpioPin : public NObject {
public:
    enum class Mode {
        Input = 0,
        Output = 1,
        InputOutput = 2,
        OpenDrain = 3
    };

    enum class Pull {
        NoPull = 0,
        PullUp = 1,
        PullDown = 2
    };

    enum class Interrupt {
        Disabled = 0,
        Rising = 1,
        Falling = 2,
        AnyEdge = 3,
        LowLevel = 4,
        HighLevel = 5
    };

    enum class DriveStrength {
        Default = 0,
        Weakest = 1,
        Weak = 2,
        Strong = 3,
        Strongest = 4
    };

    explicit NGpioPin(gpio_num_t pin, NObject* parent = nullptr);
    virtual ~NGpioPin();

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    void setPull(Pull pull);
    Pull pull() const { return m_pull; }

    void setInterrupt(Interrupt type);
    Interrupt interrupt() const { return m_interrupt; }

    void setDriveStrength(DriveStrength strength);
    DriveStrength driveStrength() const { return m_driveStrength; }

    void setDebounceTime(uint32_t ms);
    uint32_t debounceTime() const { return m_debounceMs; }

    gpio_num_t pin() const { return m_pin; }
    bool isInitialized() const { return m_initialized; }

    void write(bool value);
    bool read() const;
    bool readDebounced();
    void toggle();

    // Сигналы
    NSignal<bool> valueChanged;
    NSignal<> risingEdge;
    NSignal<> fallingEdge;
    NSignal<> buttonPressed;
    NSignal<> buttonReleased;
    NSignal<> longPressed;

    void setLongPressTime(uint32_t ms);
    uint32_t longPressTime() const { return m_longPressMs; }

    void setRepeatInterval(uint32_t ms);
    void setRepeatEnabled(bool enabled);

    bool setupPWM(uint32_t freq = 1000, uint8_t resolution = 8);
    void setPWM(uint8_t dutyCycle);
    void stopPWM();
    bool isPWMEnabled() const { return m_pwmEnabled; }

    uint32_t interruptCount() const { return m_interruptCount; }
    void resetStats();
    void dumpInfo() const;

protected:
    virtual void interruptHandler();
    virtual void valueChangedEvent(bool oldValue, bool newValue);

private:
    bool applyConfiguration();
    bool configureInterrupt();
    static void IRAM_ATTR isrHandler(void* arg);
    void processInterrupt();
    void checkLongPress();
    void checkRepeat();

    gpio_num_t m_pin;
    Mode m_mode;
    Pull m_pull;
    Interrupt m_interrupt;
    DriveStrength m_driveStrength;
    bool m_initialized;

    uint32_t m_debounceMs;
    uint32_t m_lastDebounceTime;
    bool m_lastStableState;

    uint32_t m_longPressMs;
    uint32_t m_repeatIntervalMs;
    bool m_repeatEnabled;
    uint32_t m_pressStartTime;
    bool m_longPressTriggered;
    uint32_t m_lastRepeatTime;
    TimerHandle_t m_repeatTimer;

    bool m_pwmEnabled;
    uint8_t m_pwmChannel;
    uint32_t m_pwmFreq;
    uint8_t m_pwmResolution;

    uint32_t m_interruptCount;
    uint32_t m_lastInterruptTime;
    uint32_t m_totalPinToggles;

    QueueHandle_t m_eventQueue;

    static constexpr size_t EVENT_QUEUE_SIZE = 16;
    static constexpr size_t ISR_TASK_STACK = 2048;
    static constexpr UBaseType_t ISR_TASK_PRIORITY = 3;
};
