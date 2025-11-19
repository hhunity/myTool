#pragma once

#include <cstdint>
#include <vector>
#include <atomic>
#include <chrono>

enum class PixelType
{
    U8,
    U16,
};

class LineStore
{
public:
    using i64 = std::int64_t;

    LineStore(int srcWidth, int roiX, int roiW,
              i64 capacityLines, int warmupMax, PixelType pt,
              bool circular = false);
    ~LineStore();

    void Dispose() noexcept;

    // ---- 構成情報 ----
    int  SourceWidth()    const noexcept;
    int  RoiX()           const noexcept;
    int  Width()          const noexcept;
    i64  CapacityLines()  const noexcept;
    int  WarmupMax()      const noexcept;
    PixelType PixelT()    const noexcept;
    int  ElemSizeBytes()  const noexcept;
    int  RowBytes()       const noexcept;
    int  SourceRowBytes() const noexcept;

    // ---- 状態 ----
    i64 HeadTotal()   const noexcept; // これまでに Push した総行数
    i64 StoredLines() const noexcept; // 現在バッファ内に存在する行数

    // ---- Commit（ウォームアップ完了）----
    void Commit();

    // ---- 書き込み ----
    bool PushBlock(const void* src, int rows, int srcStrideBytes);
    bool PushBlock(const void* src, int rows, int srcStrideBytes,
                   std::chrono::system_clock::time_point acquiredUtc);
    bool PushBlock(const void* src, int rows, int srcStrideBytes,
                   double acquiredUtcSec);

    // ---- 読み出し（時刻つき）----
    bool TryGetLatestWindowPtr(int winW, int winH, int x0,
                               const void*& ptr, int& strideBytes, double& timeSecAtTop) const noexcept;

    bool TryGetWindowPtr(i64 startRow, int winW, int winH, int x0,
                         const void*& ptr, int& strideBytes, double& timeSecAtTop) const noexcept;

    // ---- 読み出し（時刻不要）----
    bool TryGetLatestWindowPtr(int winW, int winH, int x0,
                               const void*& ptr, int& strideBytes) const noexcept;

    bool TryGetWindowPtr(i64 startRow, int winW, int winH, int x0,
                         const void*& ptr, int& strideBytes) const noexcept;

    // ---- util ----
    static double NowUnixSec();
    static double ToUnixSec(std::chrono::system_clock::time_point tp);

private:
    struct TimeSeg
    {
        i64   Start; // 論理行インデックス
        double T;    // Unix sec
    };

    static int  clamp(int v, int lo, int hi) noexcept;
    void        check_not_disposed() const;

    void   AddSeg(i64 startLogical, double t);
    double RowTimeSec(i64 rowAbs) const noexcept;

    void   PushWarmup(const void* src, int rows, int srcStrideBytes, double timeSec);
    bool   PushLinear(const void* src, int rows, int srcStrideBytes, double timeSec);

private:
    // 設定
    int       sourceWidth_;
    int       roiX_;
    int       width_;
    i64       capacityLines_;
    int       warmupMax_;
    PixelType pixelType_;
    int       elemSizeBytes_;

    // 状態
    std::atomic<bool> committed_;     // Commit 済みか
    int               warmupCount_;   // ウォームアップで埋まっている行数

    std::uint8_t* buf_;               // 実データ
    i64           writeIndex_;        // Commit 後: 絶対行インデックス（0,1,2,...）

    std::atomic<i64> headTotal_;      // Push された総行数（ウォームアップ含む）
    std::atomic<i64> storedLines_;    // 現在バッファ内に存在する行数（最大 capacityLines）
    std::atomic<bool> disposed_;

    // ウォームアップ用の per-line 時刻
    std::vector<double> warmupTimes_;

    // セグメント（時間情報）
    std::vector<TimeSeg> segs_;
    std::atomic<int>     segCount_;

    double warmupLastTimeSec_;
    i64    commitBase_;               // 絶対行 -> 論理行のオフセット

    bool circular_;                   // true ならリングバッファ動作
};