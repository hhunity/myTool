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

namespace fs = std::filesystem;

LatestJpegStore g_jpeg;

// 取り込み側：最新JPEGを更新（jpeg_bytes はバイナリ）
void set_latest_jpeg(std::string jpeg_bytes) {
  auto p = std::make_shared<std::string>(std::move(jpeg_bytes)); // ここで1回だけ確保
  const auto now_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
  {
    std::lock_guard<std::mutex> lk(g_jpeg.mtx);
    g_jpeg.jpeg = std::move(p); // 以後は共有参照
    g_jpeg.seq++;
    g_jpeg.last_epoch_ms = now_ms;
  }
  g_jpeg.cv.notify_all();
}

// 終了通知（サーバ停止時など）
void stop_streaming() {
  {
    std::lock_guard<std::mutex> lk(g_jpeg.mtx);
    g_jpeg.stop = true;
  }
  g_jpeg.cv.notify_all();
}

// 戻り値：{jpeg_ptr, new_seq}
// jpeg_ptr == nullptr の場合：タイムアウト or stop
std::pair<std::shared_ptr<const std::string>, uint64_t>
wait_next_jpeg(uint64_t last_seq, std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lk(g_jpeg.mtx);

  g_jpeg.cv.wait_for(lk, timeout, [&] {
    return g_jpeg.stop || g_jpeg.seq != last_seq;
  });

  if (g_jpeg.stop) return {nullptr, last_seq};
  if (g_jpeg.seq == last_seq) return {nullptr, last_seq}; // timeout

  return {g_jpeg.jpeg, g_jpeg.seq};
}

// ----------------------------
// フォルダ内JPEGを読み込むユーティリティ
// ----------------------------
bool has_jpeg_ext(const fs::path& p) {
  auto ext = p.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c){ return (char)std::tolower(c); });
  return (ext == ".jpg" || ext == ".jpeg");
}

std::vector<fs::path> list_jpegs_sorted(const fs::path& dir) {
  std::vector<fs::path> files;
  std::error_code ec;

  if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return files;

  for (const auto& ent : fs::directory_iterator(dir, ec)) {
    if (ec) break;
    if (!ent.is_regular_file(ec) || ec) continue;
    auto p = ent.path();
    if (has_jpeg_ext(p)) files.push_back(p);
  }

  // 連番前提ならファイル名ソートが一番素直
  std::sort(files.begin(), files.end(),
            [](const fs::path& a, const fs::path& b) {
              return a.filename().string() < b.filename().string();
            });

  return files;
}

std::string read_file_bytes(const fs::path& p) {
  std::ifstream ifs(p, std::ios::binary);
  if (!ifs) return {};
  ifs.seekg(0, std::ios::end);
  std::streamsize n = ifs.tellg();
  if (n <= 0) return {};
  ifs.seekg(0, std::ios::beg);

  std::string buf;
  buf.resize((size_t)n);
  if (!ifs.read(buf.data(), n)) return {};
  return buf;
}

// ----------------------------
// 連番JPEGを一定fpsで set_latest_jpeg するスレッド
// ----------------------------
void folder_player_thread(fs::path dir, double fps) {
  const auto files = list_jpegs_sorted(dir);
  if (files.empty()) {
    std::cerr << "No JPEG files found in: " << dir << "\n";
    return;
  }

  if (fps <= 0.0) fps = 10.0;
  const auto period = std::chrono::duration<double>(1.0 / fps);

  std::cout << "JPEG player: " << files.size() << " frames, fps=" << fps << "\n";

  size_t i = 0;
  while (true) {
    {
      std::lock_guard<std::mutex> lk(g_jpeg.mtx);
      if (g_jpeg.stop) break;
    }

    const fs::path& p = files[i];
    std::string jpeg = read_file_bytes(p);
    if (!jpeg.empty()) {
      set_latest_jpeg(std::move(jpeg));
    }

    i = (i + 1) % files.size();
    std::this_thread::sleep_for(period);
  }
}
