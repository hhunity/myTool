#pragma once

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/LogMacros.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>
#include <memory>
#include <string>

class Logger
{
public:
    // 一度だけ呼ぶ初期化
    static void Init(const std::string& file_name = "app.log")
    {
        // Backend 起動（1回だけでOK）
        quill::Backend::start();

        // コンソール用 sink
        auto console_sink =
            quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console_sink");

        // ファイル用 sink
        auto file_sink =
            quill::Frontend::create_or_get_sink<quill::FileSink>("file_sink", file_name);

        // ロガー作成：複数 sink をまとめて渡す
        std::vector<std::shared_ptr<quill::Sink>> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        s_logger = quill::Frontend::create_or_get_logger("app_logger", sinks);

        // ログレベル（必要に応じて変更）
        s_logger->set_log_level(quill::LogLevel::Debug);
    }

    static quill::Logger* Get()
    {
        return s_logger;
    }

private:
    inline static quill::Logger* s_logger = nullptr;
};

// 便利マクロ
#define LOG_INFO(fmt, ...)  LOG_INFO(Logger::Get(),  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG_WARNING(Logger::Get(), fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG_ERROR(Logger::Get(),  fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG_DEBUG(Logger::Get(),  fmt, ##__VA_ARGS__)