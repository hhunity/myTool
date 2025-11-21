
#include <iostream>

#ifdef _WIN32
#include <windows.h>
std::string get_hostname() {
    char buf[256];
    DWORD size = sizeof(buf);
    GetComputerNameA(buf, &size);
    return buf;
}
#else
#include <unistd.h>
std::string get_hostname() {
    char buf[256];
    gethostname(buf, sizeof(buf));
    return buf;
}
#endif

int main() {
    std::cout << get_hostname() << std::endl;
}

#include <Windows.h>
#include <iostream>

bool sleep_us_hrtimer(long long usec)
{
    // 1. タイマー作成（High-Resolution 指定）
    HANDLE hTimer = CreateWaitableTimerEx(
        nullptr,                       // セキュリティ属性
        nullptr,                       // 名前なし
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, // 高分解能
        TIMER_ALL_ACCESS
    );

    if (!hTimer) {
        std::cerr << "CreateWaitableTimerEx failed. GetLastError = "
                  << GetLastError() << std::endl;
        return false;
    }

    // 2. 相対時間を 100ns 単位で指定
    //    1us = 10 * 100ns なので usec * 10
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -usec * 10;  // ★★ マイナスが超重要 ★★

    // 3. 一回だけ鳴るワンショットタイマーとしてセット
    //    第3引数 period = 0 → 繰り返しなし
    if (!SetWaitableTimer(
        hTimer,
        &dueTime,
        0,
        nullptr,
        nullptr,
        FALSE))
    {
        std::cerr << "SetWaitableTimer failed. GetLastError = "
                  << GetLastError() << std::endl;
        CloseHandle(hTimer);
        return false;
    }

    // 4. 発火を待つ
    DWORD dw = WaitForSingleObject(hTimer, INFINITE);
    if (dw != WAIT_OBJECT_0) {
        std::cerr << "WaitForSingleObject failed. Ret = "
                  << dw << " GetLastError = " << GetLastError() << std::endl;
        CloseHandle(hTimer);
        return false;
    }

    CloseHandle(hTimer);
    return true;
}

int main()
{
    std::cout << "start\n";
    sleep_us_hrtimer(200);   // 200 usec のつもり
    std::cout << "end\n";
}

#include <Windows.h>

void sleep_us_hrtimer(long usec)
{
    HANDLE timer = CreateWaitableTimerEx(
        nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

    LARGE_INTEGER li;
    li.QuadPart = -usec * 10;  // 1us = 10 * 100ns

    SetWaitableTimerEx(
        timer, &li, 0, nullptr, nullptr, nullptr,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION);

    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}


#include <mutex>
#include <condition_variable>
#include <chrono>

class UseCounter {
public:
    // 処理を開始するときに呼ぶ
    void acquire() {
        std::lock_guard<std::mutex> lock(m_);
        ++count_;
    }

    // 処理が終わったら呼ぶ
    void release() {
        std::lock_guard<std::mutex> lock(m_);
        if (--count_ == 0) {
            cv_.notify_all();   // 0 になったら待ってるスレッドを起こす
        }
    }

    // 0 になるまで無制限に待つ
    void wait_zero() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&]{ return count_ == 0; });
    }

    // 0 になるまで「最大 duration だけ」待つ
    // 戻り値: true  = 0 になった
    //         false = タイムアウト（まだ 0 じゃない）
    template<class Rep, class Period>
    bool wait_zero_for(const std::chrono::duration<Rep, Period>& d) {
        std::unique_lock<std::mutex> lock(m_);
        return cv_.wait_for(lock, d, [&]{ return count_ == 0; });
    }

private:
    int count_ = 0;
    std::mutex m_;
    std::condition_variable cv_;
};
#include <mutex>
#include <condition_variable>

class UseCounter {
public:
    // 処理を開始するときに呼ぶ
    void acquire() {
        std::lock_guard<std::mutex> lock(m_);
        ++count_;
    }

    // 処理が終わったら呼ぶ
    void release() {
        std::lock_guard<std::mutex> lock(m_);
        if (--count_ == 0) {
            cv_.notify_all();   // 0 になったら待ってるスレッドを起こす
        }
    }

    // 別スレッドから「0 になるまで待つ」
    void wait_zero() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&]{ return count_ == 0; });
    }

private:
    int count_ = 0;
    std::mutex m_;
    std::condition_variable cv_;
};


void wait_usec(int usec)
{
    using namespace std::chrono;

    auto start = high_resolution_clock::now();
    auto end   = start + microseconds(usec);

    // 9割までは sleep_for で雑に進める
    auto coarse = end - microseconds(50); // 最後の50µsはスピン

    while (high_resolution_clock::now() < coarse) {
        std::this_thread::sleep_for(std::chrono::microseconds(5));
    }

    // 残りは busy-wait で正確に合わせる
    while (high_resolution_clock::now() < end) {}
}


#include <chrono>

class CallCounter {
public:
    CallCounter()
        : count_(0), started_(false)
    {}

    // 外部から明示的に開始する
    void start() {
        count_ = 0;
        started_ = true;
        start_ = std::chrono::steady_clock::now();
    }

    // 1回呼ばれたことを記録する
    void tick() {
        if (started_) {
            ++count_;
        }
    }

    // 1秒経ったかチェックして、経ってたら回数を返してリセット
    // 1秒経ってなければ -1
    int get_and_reset_if_ready() {
        if (!started_) return -1;

        using namespace std::chrono;
        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - start_);

        if (elapsed.count() >= 1000) {
            int result = count_;
            count_ = 0;
            start_ = now;
            return result;
        }
        return -1;
    }

private:
    int count_;
    bool started_;
    std::chrono::steady_clock::time_point start_;
};


#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

static std::string make_run_id()
{
    using namespace std::chrono;

    // 現在時刻（秒）
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&tm, &t);
#endif

    // run_YYYYMMDD_HHMMSS を作成
    std::ostringstream oss;
    oss << "run_"
        << std::put_time(&tm, "%Y%m%d_%H%M%S");

    return oss.str();
}