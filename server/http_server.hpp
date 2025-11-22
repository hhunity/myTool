#pragma once
#include "civetweb.h"
#include <string>
#include <optional>
#include <queue>

class CommandServer
{
public:
    enum class Command {
        Start,
        Update,
        Stop,
        Unknown,
    };

    CommandServer() = default;

    // --------------------------
    // サーバ開始
    // --------------------------
    bool start(const std::string& address, int port);
    // --------------------------
    // サーバ停止
    // --------------------------
    void stop();

    // --------------------------
    // コマンド取得（何も無ければ std::nullopt）
    // --------------------------
    std::optional<Command> popCommand();
    std::optional<Command> popCommandBlocking();
private:
    mg_context* ctx_ = nullptr;
    std::atomic<bool> running_ = false;

    std::queue<Command> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    // --------------------------
    // CivetWeb ハンドラ（静的）
    // --------------------------
    static int handler(struct mg_connection* conn, void* data);
    static int statusHandler(struct mg_connection* conn, void* data);
    // --------------------------
    // 文字列 → enum 変換
    // --------------------------
    Command toCommand(const std::string& s);
    void pushCommand(CommandServer::Command cmd);
};