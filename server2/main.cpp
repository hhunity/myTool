// server.cpp
#include "httplib.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "mjpeg.h"

#if defined(_WIN32)
  #include <windows.h>
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
  #include <vector>
#elif defined(__linux__)
  #include <unistd.h>
  #include <limits.h>
#endif

extern LatestJpegStore g_jpeg;

static void add_cors(httplib::Response& res) {
  res.set_header("Access-Control-Allow-Origin", "*");
  res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
  res.set_header("Access-Control-Max-Age", "86400");
}

static std::filesystem::path get_executable_path() {
#if defined(_WIN32)
  wchar_t buf[MAX_PATH];
  DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
  return std::filesystem::path(buf, buf + len);

#elif defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::vector<char> buf(size);
  if (_NSGetExecutablePath(buf.data(), &size) != 0) {
    throw std::runtime_error("_NSGetExecutablePath failed");
  }
  return std::filesystem::weakly_canonical(std::filesystem::path(buf.data()));

#elif defined(__linux__)
  char buf[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len <= 0) throw std::runtime_error("readlink /proc/self/exe failed");
  buf[len] = '\0';
  return std::filesystem::path(buf);

#else
  // 最後の手段：cwd基準にする（必要ならここを別方式に）
  return std::filesystem::current_path();
#endif
}

int main() {
    httplib::Server svr;

    // CORS preflight（OPTIONS）対応：/api/ 配下をOKにする
    svr.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        res.status = 204;
    });

    // 例：最小API
    // svr.Get("/api/hello", [](const httplib::Request&, httplib::Response& res) {
    //     add_cors(res);
    //     res.set_content(R"({"message":"hello from cpp-httplib"})", "application/json");
    // });
    svr.Get("/api/hello", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"message":"hello from cpp-httplib"})", "application/json");
    });

    // MJPEG ストリーム
    svr.Get("/stream.mjpg", [](const httplib::Request& req, httplib::Response& res) {
        static constexpr const char* boundary = "frame";
        res.set_header("Cache-Control", "no-store");
        res.set_header("Pragma", "no-cache");
        res.set_header("Connection", "keep-alive");

        const std::string ctype =
            std::string("multipart/x-mixed-replace; boundary=") + boundary;

        res.set_content_provider(
            ctype,
            [&req](size_t /*offset*/, httplib::DataSink& sink) -> bool {
            uint64_t last_seq = 0;

            while (true) {
                // 切断チェック（環境により無い場合あり）
                if (req.is_connection_closed && req.is_connection_closed()) break;

                // 新フレームを待つ（1秒でタイムアウトしてループに戻る）
                auto [p, seq] = wait_next_jpeg(last_seq, std::chrono::milliseconds(1000));
                if (!p) {
                // timeout or stop：stopなら抜ける、timeoutなら継続
                // stop判定は g_jpeg.stop を見るのが確実
                std::lock_guard<std::mutex> lk(g_jpeg.mtx);
                if (g_jpeg.stop) break;
                continue;
                }

                const std::string& jpeg = *p;
                if (jpeg.empty()) { last_seq = seq; continue; }

                std::string header;
                header += "--";
                header += boundary;
                header += "\r\n";
                header += "Content-Type: image/jpeg\r\n";
                header += "Content-Length: " + std::to_string(jpeg.size()) + "\r\n\r\n";

                if (!sink.write(header.data(), header.size())) break;
                if (!sink.write(jpeg.data(), jpeg.size())) break;
                if (!sink.write("\r\n", 2)) break;

                last_seq = seq;
            }

            sink.done();
            return true;
            }
        );
    });

    svr.Get("/api/live/heartbeat", [](const httplib::Request&, httplib::Response& res) {
      uint64_t seq;
      int64_t last_epoch_ms;
      {
        std::lock_guard<std::mutex> lk(g_jpeg.mtx);
        seq = g_jpeg.seq;
        last_epoch_ms = g_jpeg.last_epoch_ms;
      }
      res.set_header("Cache-Control", "no-store");
      res.set_content(
        std::string("{\"seq\":") + std::to_string(seq) +
        ",\"last_epoch_ms\":" + std::to_string(last_epoch_ms) + "}",
        "application/json"
      );
    });

    auto exe_dir = get_executable_path().parent_path();
    auto public_dir = exe_dir / "public";

    std::cout << "exe_dir=" << exe_dir << "\n";
    std::cout << "public_dir=" << public_dir << "\n";

    // 静的ファイル配信（public/ を / にマウント）
    // 例: public/index.html が http://localhost:8080/ で開く
    if (!svr.set_mount_point("/", public_dir.string().c_str())) {
        std::cerr << "Failed to mount: " << public_dir << "\n";
        return 1;
    }

    std::thread player([&] { folder_player_thread( exe_dir / "frames", 30.0); });

    std::cout << "Listening on http://127.0.0.1:8080\n";
    svr.listen("127.0.0.1", 8080);
    return 0;
}