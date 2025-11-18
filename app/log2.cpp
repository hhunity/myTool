// ログレベルは適当でOK
enum class LogLevel { Debug, Info, Warn, Error };

using LogCallback = std::function<void(LogLevel, std::string_view)>;

class MyWorker
{
public:
    // ログコールバックの登録
    void set_log_callback(LogCallback cb)
    {
        log_cb_ = std::move(cb);
    }

    void do_work(int value)
    {
        log(LogLevel::Info, "do_work start, value={}", value);

        if (value < 0)
        {
            log(LogLevel::Warn, "value is negative: {}", value);
        }

        // …処理 …

        log(LogLevel::Info, "do_work end");
    }

private:
    LogCallback log_cb_;

    template <typename... Args>
    void log(LogLevel level, std::string_view fmt, Args&&... args)
    {
        if (!log_cb_) return; // コールバック未設定なら何もしない

        // ここでメッセージを組み立てる（fmtlibでもostringstreamでも）
        // 簡易版として fmt::format を想定
        std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
        log_cb_(level, msg);
    }
};