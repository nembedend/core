// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "ndebug.h"
#include <cstdio>
#include <cstring>
#include <ctime>

NDebugLevel NDebug::s_globalLevel = NDebugLevel::Info;
std::unordered_map<NString, NDebugLevel> NDebug::s_tagLevels;
std::mutex NDebug::s_tagMutex;
QueueHandle_t NDebug::s_logQueue = nullptr;
bool NDebug::s_initialized = false;

static esp_log_level_t toEspLevel(NDebugLevel level)
{
    switch (level) {
    case NDebugLevel::NoDebug:
        return ESP_LOG_NONE;
    case NDebugLevel::Fatal:
    case NDebugLevel::Critical:
    case NDebugLevel::Error:
        return ESP_LOG_ERROR;
    case NDebugLevel::Warning:
        return ESP_LOG_WARN;
    case NDebugLevel::Info:
        return ESP_LOG_INFO;
    case NDebugLevel::Debug:
        return ESP_LOG_DEBUG;
    case NDebugLevel::Verbose:
        return ESP_LOG_VERBOSE;
    default:
        return ESP_LOG_INFO;
    }
}

static const char* levelToStr(NDebugLevel level)
{
    switch (level) {
    case NDebugLevel::Verbose:
        return "VERBOSE";
    case NDebugLevel::Debug:
        return "DEBUG";
    case NDebugLevel::Info:
        return "INFO";
    case NDebugLevel::Warning:
        return "WARN";
    case NDebugLevel::Error:
        return "ERROR";
    case NDebugLevel::Critical:
        return "CRITICAL";
    case NDebugLevel::Fatal:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

void NDebug::init(NDebugLevel defaultLevel)
{
    if (s_initialized)
        return;

    s_globalLevel = defaultLevel;

    s_logQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(LogMessage));
    if (!s_logQueue) {
        ESP_LOGE("NDebug", "Failed to create log queue");
        return;
    }

    s_initialized = true;

    xTaskCreate(logTask, "log_task", LOG_TASK_STACK, nullptr, LOG_TASK_PRIORITY, nullptr);

    ESP_LOGI("NDebug", "Debug system initialized");
}

void NDebug::shutdown()
{
    if (!s_initialized)
        return;

    s_initialized = false;

    if (s_logQueue) {
        vQueueDelete(s_logQueue);
        s_logQueue = nullptr;
    }

    s_tagLevels.clear();
}

void NDebug::setGlobalLevel(NDebugLevel level)
{
    s_globalLevel = level;
    esp_log_level_set("*", toEspLevel(level));
}

NDebugLevel NDebug::globalLevel()
{
    return s_globalLevel;
}

void NDebug::setLevel(const char* tag, NDebugLevel level)
{
    if (!tag)
        return;

    std::lock_guard<std::mutex> lock(s_tagMutex);
    s_tagLevels[NString(tag)] = level;
    esp_log_level_set(tag, toEspLevel(level));
}

NDebugLevel NDebug::level(const char* tag)
{
    if (!tag)
        return s_globalLevel;

    std::lock_guard<std::mutex> lock(s_tagMutex);
    auto it = s_tagLevels.find(NString(tag));
    return it != s_tagLevels.end() ? it->second : s_globalLevel;
}

void NDebug::logTask(void* arg)
{
    (void)arg;
    LogMessage msg;

    while (true) {
        if (xQueueReceive(s_logQueue, &msg, portMAX_DELAY) == pdTRUE) {
            esp_log_level_t espLevel = toEspLevel(msg.level);
            const char* tag = msg.tag;

            esp_log_write(espLevel, tag, "[%s:%d] %s\n", msg.function, msg.line, msg.text);
        }
    }
}

void NDebug::enqueueMessage(NDebugLevel level, const char* tag, const char* function, int line, const char* msg)
{
    if (!s_initialized || !s_logQueue)
        return;

    LogMessage logMsg;
    logMsg.level = level;

    strncpy(logMsg.tag, tag ? tag : "APP", sizeof(logMsg.tag) - 1);
    logMsg.tag[sizeof(logMsg.tag) - 1] = '\0';

    strncpy(logMsg.function, function ? function : "", sizeof(logMsg.function) - 1);
    logMsg.function[sizeof(logMsg.function) - 1] = '\0';

    logMsg.line = line;

    strncpy(logMsg.text, msg, sizeof(logMsg.text) - 1);
    logMsg.text[sizeof(logMsg.text) - 1] = '\0';

    if (xQueueSend(s_logQueue, &logMsg, 0) != pdTRUE) {
        esp_log_write(toEspLevel(level), tag, "[%s:%d] %s\n", function, line, msg);
    }
}

void NDebug::info(const char* tag, const char* format, ...)
{
    if (s_globalLevel < NDebugLevel::Info)
        return;

    va_list args;
    va_start(args, format);
    char buffer[LOG_MSG_MAX_SIZE];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    enqueueMessage(NDebugLevel::Info, tag, "", 0, buffer);
}

void NDebug::debug(const char* tag, const char* format, ...)
{
    if (s_globalLevel < NDebugLevel::Debug)
        return;

    va_list args;
    va_start(args, format);
    char buffer[LOG_MSG_MAX_SIZE];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    enqueueMessage(NDebugLevel::Debug, tag, "", 0, buffer);
}

void NDebug::warning(const char* tag, const char* format, ...)
{
    if (s_globalLevel < NDebugLevel::Warning)
        return;

    va_list args;
    va_start(args, format);
    char buffer[LOG_MSG_MAX_SIZE];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    enqueueMessage(NDebugLevel::Warning, tag, "", 0, buffer);
}

void NDebug::error(const char* tag, const char* format, ...)
{
    if (s_globalLevel < NDebugLevel::Error)
        return;

    va_list args;
    va_start(args, format);
    char buffer[LOG_MSG_MAX_SIZE];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    enqueueMessage(NDebugLevel::Error, tag, "", 0, buffer);
}

void NDebug::critical(const char* tag, const char* format, ...)
{
    if (s_globalLevel < NDebugLevel::Critical)
        return;

    va_list args;
    va_start(args, format);
    char buffer[LOG_MSG_MAX_SIZE];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    enqueueMessage(NDebugLevel::Critical, tag, "", 0, buffer);
}

void NDebug::verbose(const char* tag, const char* format, ...)
{
    if (s_globalLevel < NDebugLevel::Verbose)
        return;

    va_list args;
    va_start(args, format);
    char buffer[LOG_MSG_MAX_SIZE];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    enqueueMessage(NDebugLevel::Verbose, tag, "", 0, buffer);
}

void NDebug::hexDump(const char* tag, const uint8_t* data, size_t len, const char* title)
{
    if (!data || len == 0)
        return;

    const size_t bytes_per_line = 16;
    char line[128];
    size_t offset = 0;

    if (title) {
        info(tag, "=== %s (len=%zu) ===", title, len);
    }

    while (offset < len) {
        size_t line_len = (len - offset) < bytes_per_line ? (len - offset) : bytes_per_line;
        char* ptr = line;
        ptr += snprintf(ptr, sizeof(line) - (ptr - line), "%04zu: ", offset);

        for (size_t i = 0; i < bytes_per_line; i++) {
            if (i < line_len) {
                ptr += snprintf(ptr, sizeof(line) - (ptr - line), "%02X ", data[offset + i]);
            } else {
                ptr += snprintf(ptr, sizeof(line) - (ptr - line), "   ");
            }
            if (i == 7)
                ptr += snprintf(ptr, sizeof(line) - (ptr - line), " ");
        }

        ptr += snprintf(ptr, sizeof(line) - (ptr - line), " |");

        for (size_t i = 0; i < line_len; i++) {
            char c = data[offset + i];
            ptr += snprintf(ptr, sizeof(line) - (ptr - line), "%c", (c >= 0x20 && c < 0x7F) ? c : '.');
        }

        info(tag, "%s", line);
        offset += line_len;
    }
}

NDebug::NDebug(NDebugLevel level, const char* file, const char* function, int line)
    : m_level(level)
    , m_file(file)
    , m_function(function)
    , m_line(line)
    , m_timestamp(esp_timer_get_time() / 1000)
    , m_taskHandle(xTaskGetCurrentTaskHandle())
{
}

NDebug::~NDebug()
{
    flush();
}

bool NDebug::isEnabled() const
{
    if (m_level > s_globalLevel)
        return false;

    const char* tag = m_file ? strrchr(m_file, '/') : nullptr;
    tag = tag ? tag + 1 : (m_file ? m_file : "");

    NDebugLevel tagLevel = level(tag);
    return m_level <= tagLevel;
}

void NDebug::writeToESP_LOG()
{
    const char* tag = m_file ? strrchr(m_file, '/') : nullptr;
    tag = tag ? tag + 1 : (m_file ? m_file : "APP");

    esp_log_level_t espLevel = toEspLevel(m_level);
    const char* levelStr = levelToStr(m_level);

    esp_log_write(espLevel, tag, "[%s] %s:%d: %s\n", levelStr, m_function, m_line, m_stream.c_str());
}

void NDebug::flush()
{
    if (!s_initialized || !isEnabled() || m_stream.isEmpty()) {
        return;
    }

    enqueueMessage(m_level, m_file ? strrchr(m_file, '/') ? strrchr(m_file, '/') + 1 : m_file : "APP",
        m_function, m_line, m_stream.c_str());
}
