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