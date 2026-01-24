// server.cpp
#include <iostream>
#include <string>
#include "httplib.h"
#include <filesystem>

#if defined(_WIN32)
  #include <windows.h>
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
  #include <vector>
#elif defined(__linux__)
  #include <unistd.h>
  #include <limits.h>
#endif

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

    std::cout << "Listening on http://127.0.0.1:8080\n";
    svr.listen("127.0.0.1", 8080);
    return 0;
}