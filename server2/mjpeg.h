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

struct LatestJpegStore {
  std::mutex mtx;
  std::condition_variable cv;

  std::shared_ptr<const std::string> jpeg; // 最新JPEG（共有・不変）
  uint64_t seq = 0;                        // 更新番号
  bool stop = false;

  int64_t last_epoch_ms = 0; // Unix epoch ms
};

// 取り込み側：最新JPEGを更新（jpeg_bytes はバイナリ）
void set_latest_jpeg(std::string jpeg_bytes);

// 終了通知（サーバ停止時など）
void stop_streaming();

// 戻り値：{jpeg_ptr, new_seq}
std::pair<std::shared_ptr<const std::string>, uint64_t>
wait_next_jpeg(uint64_t last_seq, std::chrono::milliseconds timeout);

// ----------------------------
// フォルダ内JPEGを読み込むユーティリティ
// ----------------------------
bool has_jpeg_ext(const fs::path& p);
std::vector<fs::path> list_jpegs_sorted(const fs::path& dir);
std::string read_file_bytes(const fs::path& p);

// ----------------------------
// 連番JPEGを一定fpsで set_latest_jpeg するスレッド
// ----------------------------
void folder_player_thread(fs::path dir, double fps);