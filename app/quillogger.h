#pragma once
#include <quill/Frontend.h>
#include <quill/Backend.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
class Logger
{
public:
    static void Init(const std::string& file_name = "app.log")
    {
        // Backend 起動
        quill::Backend::start();

        // Console sink
        auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

        std::vector<std::shared_ptr<quill::Sink>> sinks;
        sinks.push_back(console_sink);
        // sinks.push_back(file_sink);

        s_logger = quill::Frontend::create_or_get_logger("app_logger", sinks);
        s_logger->set_log_level(quill::LogLevel::Debug);
    }

    static quill::Logger* Get()
    {
        return s_logger;
    }

private:
    inline static quill::Logger* s_logger = nullptr;
};

#define LOGI(fmt, ...) QUILL_LOG_INFO(Logger::Get(), fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) QUILL_LOG_WARNING(Logger::Get(), fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) QUILL_LOG_ERROR(Logger::Get(), fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) QUILL_LOG_DEBUG(Logger::Get(), fmt, ##__VA_ARGS__)