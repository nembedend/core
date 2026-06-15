// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <cstdarg>
#include <cstdio>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "nstring.h"

#define N_FUNCTION_INFO __FUNCTION__

enum class NDebugLevel {
    NoDebug = 0,
    Fatal = 1,
    Critical = 2,
    Error = 3,
    Warning = 4,
    Info = 5,
    Debug = 6,
    Verbose = 7
};

class NDebug {
public:
    static void init(NDebugLevel defaultLevel = NDebugLevel::Info);
    static void shutdown();

    static void setGlobalLevel(NDebugLevel level);
    static NDebugLevel globalLevel();

    static void setLevel(const char* tag, NDebugLevel level);
    static NDebugLevel level(const char* tag);

#define nDebug()    NDebug(NDebugLevel::Debug,   __FILE__, __FUNCTION__, __LINE__)
#define nInfo()     NDebug(NDebugLevel::Info,    __FILE__, __FUNCTION__, __LINE__)
#define nWarning()  NDebug(NDebugLevel::Warning, __FILE__, __FUNCTION__, __LINE__)
#define nError()    NDebug(NDebugLevel::Error,   __FILE__, __FUNCTION__, __LINE__)
#define nCritical() NDebug(NDebugLevel::Critical,__FILE__, __FUNCTION__, __LINE__)
#define nVerbose()  NDebug(NDebugLevel::Verbose, __FILE__, __FUNCTION__, __LINE__)

    template<typename T>
    NDebug& operator<<(const T& value) {
        m_stream.append(value);
        return *this;
    }

    NDebug& operator<<(const char* value) {
        m_stream.append(value);
        return *this;
    }

    NDebug& operator<<(const NString& value) {
        m_stream.append(value.c_str());
        return *this;
    }

    NDebug& operator<<(int value) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", value);
        m_stream.append(buf);
        return *this;
    }

    NDebug& operator<<(unsigned int value) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u", value);
        m_stream.append(buf);
        return *this;
    }

    NDebug& operator<<(float value) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%f", value);
        m_stream.append(buf);
        return *this;
    }

    NDebug& operator<<(void* value) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%p", value);
        m_stream.append(buf);
        return *this;
    }

    ~NDebug();

    static void info(const char* tag, const char* format, ...)  __attribute__((format(printf, 2, 3)));
    static void debug(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));
    static void warning(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));
    static void error(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));
    static void critical(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));
    static void verbose(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));

    static void hexDump(const char* tag, const uint8_t* data, size_t len, const char* title = nullptr);

    NDebug(NDebugLevel level, const char* file, const char* function, int line);

private:
    void flush();
    bool isEnabled() const;
    void writeToESP_LOG();

    NDebugLevel m_level;
    const char* m_file;
    const char* m_function;
    int m_line;
    NString m_stream;
    TickType_t m_timestamp;
    TaskHandle_t m_taskHandle;

    static NDebugLevel s_globalLevel;
    static std::unordered_map<NString, NDebugLevel> s_tagLevels;
    static std::mutex s_tagMutex;
    static QueueHandle_t s_logQueue;
    static bool s_initialized;

    static constexpr size_t LOG_QUEUE_SIZE = 32;
    static constexpr size_t LOG_MSG_MAX_SIZE = 256;
    static constexpr size_t LOG_TASK_STACK = 4096;
    static constexpr UBaseType_t LOG_TASK_PRIORITY = 2;

    struct LogMessage {
        NDebugLevel level;
        char tag[32];
        char function[64];
        int line;
        char text[LOG_MSG_MAX_SIZE];
    };

    static void logTask(void* arg);
    static void enqueueMessage(NDebugLevel level, const char* tag, const char* function, int line, const char* msg);
};

#define N_ASSERT(cond) \
do { \
        if (!(cond)) { \
            NDebug::error("ASSERT", "Assertion failed: %s at %s:%d", #cond, __FILE__, __LINE__); \
            while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); } \
    } \
} while(0)
