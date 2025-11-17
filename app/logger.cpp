#pragma once
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>
#include <deque>
#include <chrono>
#include <ctime>
#include <stdexcept>
#include "json.hpp"  // nlohmann::json

class AsyncJsonLogger
{
public:
    using json = nlohmann::json;

    /// base_path: "log/app_log" みたいなベース名
    /// max_bytes: ローテーションするファイルサイズ上限（例: 10*1024*1024）
    AsyncJsonLogger(const std::string& base_path,
                    std::size_t max_bytes)
        : base_path_(base_path)
        , max_bytes_(max_bytes)
    {
        open_new_file();                      // 最初のファイルを開く
        worker_ = std::thread(&AsyncJsonLogger::worker_loop, this);
    }

    ~AsyncJsonLogger()
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_one();

        if (worker_.joinable())
            worker_.join();

        if (ofs_.is_open()) {
            ofs_.flush();
            ofs_.close();
        }
    }

    // 任意の json を 1 行 1 JSON で書く
    void log(const json& j)
    {
        std::string line = j.dump();
        line.push_back('\n');

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            queue_.push_back(std::move(line));
        }
        cv_.notify_one();
    }

    // よくあるログ用のヘルパ
    void log_event(const std::string& level,
                   const std::string& message,
                   const json& extra = json::object())
    {
        json j = {
            {"level", level},
            {"msg",   message},
            {"time",  current_time_iso8601()}
        };

        for (auto it = extra.begin(); it != extra.end(); ++it) {
            j[it.key()] = it.value();     // 追加情報をマージ
        }

        log(j);
    }

private:
    // ==== 非同期スレッド側 ====
    void worker_loop()
    {
        for (;;) {
            std::string line;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [&]{
                    return stop_ || !queue_.empty();
                });

                if (stop_ && queue_.empty())
                    break;

                line = std::move(queue_.front());
                queue_.pop_front();
            }

            // ファイルサイズを見てローテーション
            if (current_size_ + line.size() > max_bytes_) {
                rotate_file();
            }

            ofs_ << line;
            current_size_ += line.size();

            // 必要に応じて flush（頻度は好みで調整）
            // ofs_.flush();
        }
    }

    void rotate_file()
    {
        if (ofs_.is_open()) {
            ofs_.flush();
            ofs_.close();
        }
        ++file_index_;
        open_new_file();
    }

    void open_new_file()
    {
        // ファイル名: base_path_ + "_00001.jsonl" みたいな感じ
        std::string filename = base_path_ + "_" + index_str(file_index_) + ".jsonl";

        ofs_.open(filename, std::ios::out | std::ios::app);
        if (!ofs_) {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
        current_size_ = static_cast<std::size_t>(ofs_.tellp());
        if (!ofs_) current_size_ = 0;
    }

    static std::string index_str(std::size_t idx)
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%05zu", idx);
        return std::string(buf);
    }

    static std::string current_time_iso8601()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        std::time_t t = system_clock::to_time_t(now);
        std::tm tm{};
    #if defined(_WIN32)
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
        return std::string(buf);
    }

private:
    // 設定
    std::string base_path_;
    std::size_t max_bytes_;

    // ファイル関連（workerスレッド専用）
    std::ofstream ofs_;
    std::size_t current_size_ = 0;
    std::size_t file_index_ = 0;   // 0,1,2,...

    // キュー & スレッド制御
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::deque<std::string> queue_;
    bool stop_ = false;
    std::thread worker_;
};



#include "AsyncJsonLogger.h"
#include <thread>

int main()
{
    // 10MBごとにローテーション
    AsyncJsonLogger logger("logs/app_log", 10 * 1024 * 1024);

    // 単発ログ
    logger.log_event("info", "program started");

    // 追加情報付き
    AsyncJsonLogger::json extra = {
        {"frame_id",  123},
        {"thread_id", (int)std::hash<std::thread::id>{}(std::this_thread::get_id())}
    };
    logger.log_event("debug", "processing frame", extra);

    // 複数スレッドからガンガン打ってもOK
    auto worker = [&](int id){
        for (int i = 0; i < 1000; ++i) {
            AsyncJsonLogger::json ex = {
                {"worker", id},
                {"i",      i}
            };
            logger.log_event("info", "worker loop", ex);
        }
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    t1.join();
    t2.join();

    // destructor で自動的にスレッド停止 & ファイルクローズ
}