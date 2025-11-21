#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>
#include <deque>
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <nlohmann/json.hpp>

// =======================
// 環境ヘッダ用の構造体
// =======================
struct EnvironmentHeader
{
    std::string app_name;
    std::string app_version;
    std::string host_name;
    std::string model_name;
    std::string model_version;
    // 必要に応じて増やしてOK
};

// （使ってもいいけど、今回の実装では直接は使ってない）
inline void to_json(nlohmann::json& j, const EnvironmentHeader& env)
{
    j = nlohmann::json{
        {"app_name",      env.app_name},
        {"app_version",   env.app_version},
        {"host_name",     env.host_name},
        {"model_name",    env.model_name},
        {"model_version", env.model_version}
    };
}

class AsyncJsonLogger
{
public:
    using json = nlohmann::ordered_json;

    /// base_path: "log/app_log" みたいなベース名
    /// max_bytes: ローテーションするファイルサイズ上限（例: 10*1024*1024）
    /// env_header : 全ログ共通の環境ヘッダ
    AsyncJsonLogger(const std::string& base_path,
                    std::size_t max_bytes,
                    const EnvironmentHeader& env_header = {})
        : base_path_(base_path)
        , max_bytes_(max_bytes)
        , env_header_json_(make_env_json(env_header))
    {
        open_new_file();
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

    // ==== Run 関連 API =======================================

    // run_id をセットして、runヘッダを1行書く
    // run_meta には「この実験のパラメータ」などを入れておく
    void start_run(const std::string& run_id, const json& run_meta = json::object())
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            current_run_id_ = run_id;
        }

        json j;
        j["type"]   = "run_start";
        j["time"]   = current_time_iso8601();
        j["run_id"] = run_id;
        j["env"]    = env_header_json_;
        j["meta"]   = run_meta;

        enqueue_line(j.dump());
    }

void init_file_index_from_existing()
{
    namespace fs = std::filesystem;

    // base_path_ が空のときはとりあえず 0 で開始
    if (base_path_.empty()) {
        file_index_ = 0;
        return;
    }

    // 連番ファイルを探す: base_path_ + "_00000.jsonl", "_00001.jsonl", ...
    std::size_t idx = 0;
    for (;; ++idx) {
        std::string name = base_path_ + "_" + index_str(idx) + ".jsonl";
        if (!fs::exists(name)) {
            break;  // ここが「最初の存在しない index」
        }
    }

    if (idx == 0) {
        // 1つもファイルがない
        file_index_ = 0;
        return;
    }

    // 最後に存在するファイル
    std::size_t last = idx - 1;
    std::string last_name = base_path_ + "_" + index_str(last) + ".jsonl";
    std::uintmax_t sz = 0;
    try {
        sz = fs::file_size(last_name);
    } catch (...) {
        // 取れなかったらとりあえず新しいファイルに逃がす
        file_index_ = idx;
        return;
    }

    if (sz < max_bytes_) {
        // まだ余裕があるので、最後のファイルに追記開始
        file_index_ = last;
    } else {
        // もう一杯なので、新しい index から開始
        file_index_ = idx;
    }
}

    // run を切り替えたい時に使う（run_id を空にするだけ）
    void end_run()
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        current_run_id_.clear();
    }

    // env をあとから変えたい場合
    void set_environment(const EnvironmentHeader& env_header)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        env_header_json_ = make_env_json(env_header);
    }

    // ==== 通常ログ API =======================================

    // 任意の json を 1 行 1 JSON で書く
    // ※ この中で「type → time → run_id → env → その他」に並べ替える
    void log(json j)
    {
        // まず現在の run_id / env を snapshot
        std::string run_id_snapshot;
        json env_snapshot;
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            run_id_snapshot = current_run_id_;
            env_snapshot    = env_header_json_;
        }

        json out;

        // 1. type（あれば先に）
        if (j.contains("type")) {
            out["type"] = j["type"];
            j.erase("type");
        }

        // 2. time（あれば次に）
        if (j.contains("time")) {
            out["time"] = j["time"];
            j.erase("time");
        }

        // 3. run_id
        if (!run_id_snapshot.empty()) {
            out["run_id"] = run_id_snapshot;
        }

        // 4. env
        out["env"] = env_snapshot;

        // 5. 残りのフィールド（frame_id, elapsed_ms など）
        for (auto& item : j.items()) {
            const auto& key = item.key();
            if (key == "run_id" || key == "env") {
                // ユーザー側で入れてても無視して logger 側の値を優先
                continue;
            }
            out[key] = item.value();
        }

        enqueue_line(out.dump());
    }

    // よくあるログ用のヘルパ
    void log_event(const std::string& level,
                   const std::string& message,
                   const json& extra = json::object())
    {
        json j;
        j["type"]  = "event";
        j["time"]  = current_time_iso8601();
        j["level"] = level;
        j["msg"]   = message;

        for (auto it = extra.begin(); it != extra.end(); ++it) {
            j[it.key()] = it.value();     // 追加情報をマージ
        }

        log(std::move(j));
    }

private:
    // EnvironmentHeader から json を作るヘルパ
    static json make_env_json(const EnvironmentHeader& env)
    {
        json j;
        j["app_name"]      = env.app_name;
        j["app_version"]   = env.app_version;
        j["host_name"]     = env.host_name;
        j["model_name"]    = env.model_name;
        j["model_version"] = env.model_version;
        return j;
    }

    // ==== キュー投入共通処理 ====
    void enqueue_line(std::string line)
    {
        line.push_back('\n');
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            queue_.push_back(std::move(line));
        }
        cv_.notify_one();
    }

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

        // now（ナノ秒精度）
        auto now = system_clock::now();
        auto ns  = duration_cast<nanoseconds>(now.time_since_epoch()) % 1000000000LL;

        std::time_t t = system_clock::to_time_t(now);
        std::tm tm{};
    #if defined(_WIN32)
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif

        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);

        char final_buf[96];
        std::snprintf(final_buf, sizeof(final_buf),
                      "%s.%09lld",  // 9桁ゼロ埋め
                      buf,
                      static_cast<long long>(ns.count()));

        return std::string(final_buf);
    }

private:
    // 設定
    std::string base_path_;
    std::size_t max_bytes_;

    // 環境ヘッダ（全ログ共通）
    json env_header_json_;

    // 現在の run_id（空なら未設定）
    std::string current_run_id_;

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


// ===================================
// フレーム結果ログ用ヘルパ
// ===================================

void log_frame_result(AsyncJsonLogger& logger,
                      int frame_id,
                      const std::string& status,
                      double elapsed_ms,
                      bool defect,
                      double score)
{
    AsyncJsonLogger::json extra = {
        {"type",       "frame_result"},
        {"frame_id",   frame_id},
        {"status",     status},      // "ok" / "ng" / "error"
        {"elapsed_ms", elapsed_ms},
        {"defect",     defect},
        {"score",      score}
    };

    logger.log_event("info", "frame processed", extra);
}


// =======================
// 使用例
// =======================

int main()
{
    EnvironmentHeader env{
        .app_name      = "my_app",
        .app_version   = "1.0.0",
        .host_name     = "dev-machine-01",
        .model_name    = "dummy-model",
        .model_version = "v0.1"
    };

    // 10MBごとにローテーション
    AsyncJsonLogger logger("", 10 * 1024 * 1024, env);

    // ---- Run1: Sobel 関連の実験 ----
    AsyncJsonLogger::json run1_meta = {
        {"description", "sobel 閾値テスト"},
        {"sobel_ksize", 3},
        {"sobel_th",    128}
    };
    logger.start_run("run_2025_1118_01", run1_meta);

    logger.log_event("info", "program started");

    AsyncJsonLogger::json extra = {
        {"stage",    "sobel"},
        {"frame_id", 123}
    };
    logger.log_event("debug", "sobel start", extra);

    // …処理いろいろ…
    logger.log_event("debug", "sobel done", {
        {"stage",      "sobel"},
        {"frame_id",   123},
        {"elapsed_ms", 3.4}
    });

    logger.end_run();

    // ---- Run2: 別条件 ----
    AsyncJsonLogger::json run2_meta = {
        {"description", "fft テスト"},
        {"fft_size",    1024}
    };
    logger.start_run("run_2025_1118_02", run2_meta);

    logger.log_event("info", "fft experiment started", {
        {"stage", "fft"}
    });


    // …フレーム処理…



    bool defect = false;
    double score = 0.97; // 例えば判定スコア
    double elapsed_ms = 0;

    for(int i=0;i<1000;i++) {
        auto start = std::chrono::high_resolution_clock::now();
        log_frame_result(logger, i, "ok", elapsed_ms, defect, score*i);
        auto end   = std::chrono::high_resolution_clock::now();
        elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
    }
    // destructor で自動的にスレッド停止 & ファイルクローズ
}

// あなたのアプリ（C++）
//     └ async_json_logger → log/app_log_00001.jsonl
//            ↓   （ここで完結してる）
// Fluent Bit（別プロセス）
//     └ ファイルtail → 転送 → サーバ or DB → Grafana