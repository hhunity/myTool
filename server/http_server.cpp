#include <vector>
#include "civetweb.h"
#include <queue>
#include <mutex>
#include <string>
#include <optional>
#include <atomic>
#include "http_server.hpp"

// --------------------------
// サーバ開始
// --------------------------
bool CommandServer::start(const std::string& address, int port)
{
    if (running_) return false;

    std::string port_str = std::to_string(port);

    const char* options[] = {
        "listening_ports", port_str.c_str(),
        "num_threads", "2",
        nullptr
    };

    struct mg_callbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));

    //ctx が HTTP サーバーオブジェクト
    //これだけでスレッドが生成されて HTTP サーバーが動く
    ctx_ = mg_start(&callbacks, this, options);
    if (!ctx_) return false;

    // "/command" にハンドラ設定
    mg_set_request_handler(ctx_, "/command", CommandServer::handler, this);
    mg_set_request_handler(ctx_, "/status", CommandServer::statusHandler, this);
    
    running_ = true;
    return true;
}

// --------------------------
// サーバ停止
// --------------------------
void CommandServer::stop()
{
    if (!running_) return;
    mg_stop(ctx_);
    running_ = false;
}

// --------------------------
// コマンド取得（何も無ければ std::nullopt）
// --------------------------
std::optional<CommandServer::Command> CommandServer::popCommand()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty())
        return std::nullopt;

    Command cmd = queue_.front();
    queue_.pop();
    return cmd;
}

std::optional<CommandServer::Command> CommandServer::popCommandBlocking()
{
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] { return !queue_.empty(); });

    Command cmd = queue_.front();
    queue_.pop();
    return cmd;
}

// --------------------------
// CivetWeb ハンドラ（静的）
// --------------------------
int CommandServer::handler(struct mg_connection* conn, void* data)
{
    auto* self = static_cast<CommandServer*>(data);

    //リクエスト情報を取得
    //Civetweb の mg_connection から、
    //HTTP リクエストの情報（メソッド、パス、クエリなど）を丸ごと取ってくる。
    //mg_request_info の構造体には：
    //request_method → “GET”
    //request_uri → “/command”
    //query_string → “start=1”
    //etc.
    //が入っている。
    const mg_request_info* ri = mg_get_request_info(conn);

    // GET/POST どちらでも受け取る
    std::string cmd_name;
    if (ri->query_string) {
        // GET のパラメータ
        //query_string には："start=1&mode=xxx"が入る。ない場合はNULL
        const char* qs = ri->query_string;
        //“start” の値を入れるためのバッファ。
        //•	初期化しておく（{0}）。
        char buf[64] = {0};
        //mg_get_var(data, data_len, key, dst, dst_len)
        //data → "start=1"（query_string）
        //key → "start"
        //dst → start_buf
        //結果 → "1" が入る
        mg_get_var(qs, strlen(qs), "cmd", buf, sizeof(buf));
        cmd_name = buf;
    }
    else if (ri->request_method &&
                std::string(ri->request_method) == "POST")
    {
        // POST ボディ取得
        char body[256] = {0};
        int len = mg_read(conn, body, sizeof(body));
        if (len > 0) {
            // 形式: cmd=Start
            char buf[64] = {0};
            mg_get_var(body, len, "cmd", buf, sizeof(buf));
            cmd_name = buf;
        }
    }

    // コマンドを enum に変換
    Command cmd = self->toCommand(cmd_name);

    // キューへ追加
    self->pushCommand(cmd);

    // レスポンス
    mg_printf(conn,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "OK: received cmd=%s\n",
        cmd_name.c_str()
    );

    return 200;
}

// コマンドを文字列に変換（補助関数）
static const char* commandToString(CommandServer::Command cmd)
{
    switch(cmd){
        case CommandServer::Command::Start: return "Start";
        case CommandServer::Command::Update: return "Update";
        case CommandServer::Command::Stop: return "Stop";
        default: return "Unknown";
    }
}

int CommandServer::statusHandler(struct mg_connection* conn, void* data)
{
    auto* self = static_cast<CommandServer*>(data);
    std::vector<std::string> items;
    {
        std::lock_guard<std::mutex> lock(self->mtx_);
        std::queue<CommandServer::Command> copy = self->queue_;
        while(!copy.empty()) {
            items.push_back(commandToString(copy.front()));
            copy.pop();
        }
    }

    mg_printf(conn,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n\r\n"
        "{ \"queue_size\": %zu, \"items\": [", items.size());

    for(size_t i = 0; i < items.size(); ++i){
        mg_printf(conn, "\"%s\"%s", items[i].c_str(), (i + 1 < items.size() ? "," : ""));
    }

    mg_printf(conn, "] }\n");

    return 200;
}
// --------------------------
// 文字列 → enum 変換
// --------------------------
CommandServer::Command CommandServer::toCommand(const std::string& s)
{
    if (s == "Start")  return Command::Start;
    if (s == "Update") return Command::Update;
    if (s == "Stop")   return Command::Stop;
    return Command::Unknown;
}

// コマンドをプッシュ（HTTP スレッド側から呼ばれる）
void CommandServer::pushCommand(CommandServer::Command cmd)
{
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.push(cmd);
    cv_.notify_one();
}