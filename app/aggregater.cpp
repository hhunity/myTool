#include <map>
#include <vector>
#include <mutex>

class ResultReorderAndAverage
{
public:
    using json = nlohmann::json;

    ResultReorderAndAverage(AsyncJsonLogger& logger,
                            int start_id = 0,
                            std::size_t window_size = 4)
        : logger_(logger)
        , next_id_(start_id)
        , window_size_(window_size)
    {}

    // 並列ステージから「結果が1個返ってきた」ときに呼ぶ
    void on_result(FrameResult r)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // id をキーに pending に貯める
        pending_[r.frame_id] = std::move(r);

        // 取れるところまで順番に取り出して window に詰める
        flush_ordered();
    }

private:
    void flush_ordered()
    {
        for (;;)
        {
            auto it = pending_.find(next_id_);
            if (it == pending_.end())
                break;  // 次に欲しいIDがまだ来ていない

            // 次に欲しいIDが来ていたら window に移す
            window_.push_back(std::move(it->second));
            pending_.erase(it);
            ++next_id_;

            // 4個（window_size_）たまったら平均を取ってログ
            if (window_.size() == window_size_)
            {
                double sum = 0.0;
                for (auto& e : window_)
                    sum += e.value;
                double avg = sum / static_cast<double>(window_.size());

                int start_id = window_.front().frame_id;
                int end_id   = window_.back().frame_id;

                json extra = {
                    {"start_id",    start_id},
                    {"end_id",      end_id},
                    {"count",       window_.size()},
                    {"avg_value",   avg}
                };
                logger_.log_event("info", "window average", extra);

                window_.clear();
            }
        }
    }

private:
    AsyncJsonLogger& logger_;

    std::mutex mutex_;

    // まだ順番待ちのフレーム（id → 結果）
    std::map<int, FrameResult> pending_;

    // 次に欲しい frame_id
    int next_id_;

    // 何個そろったら平均を出すか
    std::size_t window_size_;

    // いま集計中のウィンドウ
    std::vector<FrameResult> window_;
};