#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/pipeline.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>

// ============================================================================
// SafeQueue
// ============================================================================
template <class T>
class SafeQueue {
public:
    void push(T v) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(std::move(v));
        }
        cv_.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&]{ return !q_.empty(); });
        T v = std::move(q_.front());
        q_.pop();
        return v;
    }
private:
    std::queue<T> q_;
    std::mutex m_;
    std::condition_variable cv_;
};

// ============================================================================
// Frame / Result
// ============================================================================
struct Frame {
    int id = -1;
};

struct FrameResult {
    int   frame_id = -1;
    double score   = 0.0;
    bool   defect  = false;
    bool   is_end  = false;
};

// ============================================================================
// 非同期画像処理（別スレッドで結果を返す）
// ============================================================================
void submit_image_job(
    Frame frame,
    std::function<void(FrameResult)> callback
) {
    std::thread([frame, cb = std::move(callback)]() mutable {

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        FrameResult r;
        r.frame_id = frame.id;
        r.score    = frame.id * 0.5;
        r.defect   = ((frame.id % 7) == 0);

        cb(std::move(r));

    }).detach();
}

// ============================================================================
// Main
// ============================================================================
int main() {

    constexpr int NUM_FRAMES = 100;
    constexpr int NUM_LINES  = 4;

    std::vector<Frame> frames(NUM_LINES);

    SafeQueue<FrameResult> q_dispatch;
    SafeQueue<FrameResult> q_log;
    SafeQueue<FrameResult> q_send;

    std::atomic<int>  pending_jobs{0};
    std::atomic<bool> front_done{false};
    std::atomic<bool> backend_alive{true};

    tf::Executor executor;
    tf::Taskflow taskflow;

    // =========================================================================
    // フロントパイプライン処理（ステージ 1〜3）
    // =========================================================================
    tf::Pipeline pl_front(
        NUM_LINES,

        // ① Source
        tf::Pipe{tf::PipeType::SERIAL, [&](tf::Pipeflow& pf){
            if (pf.token() >= NUM_FRAMES) {
                front_done.store(true);
                pf.stop();
                return;
            }

            Frame& f = frames[pf.line()];
            f.id = pf.token();

            std::cout << "[1:src]  line=" << pf.line()
                      << " frame=" << f.id << "\n";
        }},

        // ② Pre
        tf::Pipe{tf::PipeType::SERIAL, [&](tf::Pipeflow& pf){
            Frame& f = frames[pf.line()];
            std::cout << "  [2:pre] line=" << pf.line()
                      << " frame=" << f.id << "\n";
        }},

        // ③ async submit
        tf::Pipe{tf::PipeType::SERIAL, [&](tf::Pipeflow& pf){
            if (pf.token() >= NUM_FRAMES) {
                front_done.store(true);
                pf.stop();
                return;
            }

            Frame& f = frames[pf.line()];
            pending_jobs.fetch_add(1);

            submit_image_job(
                f,
                [&, id = f.id](FrameResult r){
                    r.frame_id = id;
                    q_dispatch.push(std::move(r));
                    pending_jobs.fetch_sub(1);
                }
            );

            std::cout << "    [3:submit] line=" << pf.line()
                      << " frame=" << f.id << "\n";
        }}
    );

    taskflow.composed_of(pl_front);

    // =========================================================================
    // Backend Dispatcher Task
    // =========================================================================
    tf::Task t_dispatch = taskflow.emplace([&](){
        while (backend_alive.load()) {

            FrameResult r = q_dispatch.pop();

            if (r.is_end) {
                q_log.push(r);
                q_send.push(r);
                break;
            }

            q_log.push(r);

            if (r.frame_id % 4 == 0) {
                q_send.push(r);
            }
        }
    });

    // =========================================================================
    // Stage 5-1 (Log)
    // =========================================================================
    tf::Task t_log = taskflow.emplace([&](){
        while (true) {
            FrameResult r = q_log.pop();
            if (r.is_end) break;

            std::cout << "[5-1:log] frame=" << r.frame_id
                      << " score=" << r.score
                      << " defect=" << (r.defect ? "true" : "false") << "\n";
        }
    });

    // =========================================================================
    // Stage 5-2 (Send)
    // =========================================================================
    tf::Task t_send = taskflow.emplace([&](){
        while (true) {
            FrameResult r = q_send.pop();
            if (r.is_end) break;

            std::cout << "  [5-2:send] frame=" << r.frame_id << "\n";
        }
    });

    // タスク依存
    t_dispatch.precede(t_log, t_send);

    // =========================================================================
    // Run
    // =========================================================================
    auto fu = executor.run(taskflow);

    while (!front_done.load() || pending_jobs.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    FrameResult end_msg;
    end_msg.is_end = true;
    q_dispatch.push(end_msg);

    backend_alive = false;

    fu.wait();
    std::cout << "\nAll done.\n";
}