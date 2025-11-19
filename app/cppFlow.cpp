#include <taskflow/taskflow.hpp>              // コアAPI
#include <taskflow/algorithm/pipeline.hpp>    // Pipeline / Pipe
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <optional>
#include <chrono>
#include <functional>  // std::function

// ===============================
//  スレッドセーフなキュー
//  （③コールバック → ④以降 の接続）
// ===============================
template <class T>
class SafeQueue {
public:
    // プロデューサ側（コールバックスレッドなど）から push
    void push(T v) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(std::move(v));
        }
        cv_.notify_one();
    }

    // コンシューマ側から pop（結果が来るまでブロック）
    T pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&]{ return !q_.empty(); });
        T v = std::move(q_.front());
        q_.pop();
        return v;
    }

    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(m_);
        if (q_.empty()) return std::nullopt;
        T v = std::move(q_.front());
        q_.pop();
        return v;
    }

private:
    std::queue<T> q_;
    std::mutex m_;
    std::condition_variable cv_;
};

// ===============================
//  フレームと処理結果のデータ構造
// ===============================
struct Frame {
    int id = -1;
    // 実際にはここに画像バッファやメタ情報を持たせる
};

struct FrameResult {
    int   frame_id   = -1;
    double score     = 0.0;   // 例：スコア
    bool   defect    = false; // 例：欠陥あり/なし
    bool   is_end    = false; // 終了通知用の番兵
};

// ===============================
//  疑似：非同期画像処理ライブラリ
//  submit_image_job()
//     - 即時 return
//     - 内部スレッドで疑似処理 → callback 呼び出し
// ===============================
void submit_image_job(
    Frame frame,
    std::function<void(FrameResult)> callback
) {
    std::thread([frame, cb = std::move(callback)]() mutable {
        // 疑似的な処理時間
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        FrameResult r;
        r.frame_id = frame.id;
        r.score    = 0.5 * frame.id;      // 適当な値
        r.defect   = (frame.id % 7 == 0); // 7の倍数フレームを "欠陥あり" としてみる

        // 非同期完了コールバック
        cb(std::move(r));
    }).detach();
}

// ===============================
//  メイン
// ===============================
int main() {
    constexpr int NUM_FRAMES = 20;   // 入力する総フレーム数
    constexpr int NUM_LINES  = 4;    // パイプライン並列数（ライン数）

    // ①〜③ 用のフレームバッファ
    std::vector<Frame> frames(NUM_FRAMES);

    // ③コールバック → ④ の橋渡し
    SafeQueue<FrameResult> result_queue;

    // ④〜⑤ パイプライン用の「1ラインぶんの結果バッファ」
    //   - pl_back の stage4 がここに書き込み、
    //   - ⑤-1 / ⑤-2 が同じ line index から読む
    std::vector<FrameResult> line_results(NUM_LINES);

    // 何個の非同期ジョブが未完了か（③で++ / コールバックで--）
    std::atomic<int> pending_jobs{0};

    // =======================================
    // Taskflow 準備
    // =======================================
    tf::Taskflow taskflow; //それを「実行する人」
    tf::Executor executor; //処理の「設計図」際にスレッドを動かして処理するクラス

    // ---------------------------------------
    // パイプライン A: ①〜③（フロント側）
    //   1. ソース
    //   2. Pre処理
    //   3. 非同期 submit（結果は result_queue に流れる）
    // ---------------------------------------
    tf::Pipeline pl_front(
        NUM_LINES,//これは パイプラインに「同時に何個のトークンを流すか」を決める数。
        // line 0: stage1 → stage2 → stage3
        // line 1: stage1 → stage2 → stage3
        // line 2: stage1 → stage2 → stage3
        // line 3: stage1 → stage2 → stage3
        // 1. ソースノード
        tf::Pipe{tf::PipeType::SERIAL, [&](tf::Pipeflow& pf) {
            if (pf.token() >= NUM_FRAMES) {
                pf.stop();  // これ以上フレームを供給しない
                return;
            }

            int id = static_cast<int>(pf.token());
            frames[id].id = id;

            std::cout << "[1:source] token=" << pf.token()
                      << " frame_id=" << frames[id].id << "\n";

            // 実際はここでカメラキャプチャや DMA 結果を Frame に詰める
        }},

        // 2. Pre処理ノード（並列）
        tf::Pipe{tf::PipeType::SERIAL, [&](tf::Pipeflow& pf) {
            auto& f = frames[pf.token()];
            // 前処理（色変換・正規化・ROI 切り出し 等）
            std::cout << "  [2:pre]    frame_id=" << f.id << "\n";
        }},

        // 3. 非同期画像処理ノード
        tf::Pipe{tf::PipeType::SERIAL, [&](tf::Pipeflow& pf) {
            auto& f = frames[pf.token()];

            std::cout << "    [3:submit] frame_id=" << f.id
                      << " (async submit)\n";

            // 非同期ジョブ数を +1
            pending_jobs.fetch_add(1, std::memory_order_relaxed);

            // 非同期ライブラリへジョブ投入
            submit_image_job(
                f,
                // コールバックラムダ
                [&, id = f.id](FrameResult r) {
                    // ここは「ライブラリ側スレッド」で実行される想定

                    // 念のため frame_id を上書き
                    r.frame_id = id;

                    // ④ 以降に渡すため、結果をキューへ投入
                    result_queue.push(std::move(r));

                    // 未完了ジョブ数を -1
                    pending_jobs.fetch_sub(1, std::memory_order_relaxed);
                }
            );
        }}
    );

    // ---------------------------------------
    // パイプライン B: ④・⑤-1・⑤-2（バック側）
    //
    //  4. 分配ノード（キューから1件 pop → line_results[line] に格納）
    //  5-1. Logノード（全フレーム）
    //  5-2. 送信ノード（4フレームに1回）
    //
    //  - token の数は特に意識せず、
    //    ④の中で result_queue.pop() して結果が無くなるまで走らせるイメージ。
    //  - is_end=true の番兵を受け取ったら pf.stop() してパイプライン停止。
    // ---------------------------------------
    tf::Pipeline pl_back(
        NUM_LINES,

        // 4. 分配ノード（キューから結果を1件取得）
        tf::Pipe{tf::PipeType::SERIAL, [&](tf::Pipeflow& pf) {
            // キューから1件取り出し（結果が来るまでブロック）
            FrameResult r = result_queue.pop();

            if (r.is_end) {
                // 番兵を受け取ったらパイプライン停止
                std::cout << "[4:dispatch] got end marker, stop pipeline.\n";
                pf.stop();
                return;
            }

            // このラインに対応するスロットに格納して次ステージへ
            line_results[pf.line()] = std::move(r);
        }},

        // 5-1. Logノード（全フレームに対してログ出力）
        tf::Pipe{tf::PipeType::SERIAL, [&](tf::Pipeflow& pf) {
            auto& r = line_results[pf.line()];
            std::cout << "[5-1:log]  frame_id=" << r.frame_id
                      << " score=" << r.score
                      << " defect=" << (r.defect ? "true" : "false")
                      << "\n";
        }},

        // 5-2. 送信ノード（4フレームに1回だけ送る）
        tf::Pipe{tf::PipeType::SERIAL, [&](tf::Pipeflow& pf) {
            auto& r = line_results[pf.line()];

            if (r.frame_id % 4 == 0) {
                std::cout << "  [5-2:send] frame_id=" << r.frame_id
                          << " (every 4th frame)\n";
                // 実際にはここで別PC/サーバへ送信する
            }
        }}
    );

    // Taskflow に両方のパイプラインを組み込む
    taskflow.composed_of(pl_front).name("front_pipeline");
    taskflow.composed_of(pl_back ).name("back_pipeline");

    // パイプラインを非同期で開始
    auto fu = executor.run(taskflow);

    // まずは ③ の非同期ジョブが全部終わるのを待つ
    while (pending_jobs.load(std::memory_order_relaxed) > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 「もう結果こないよ」という番兵をキューに入れる
    FrameResult end_msg;
    end_msg.is_end = true;
    result_queue.push(end_msg);

    // ④ が番兵を受け取って pf.stop() → pl_back が終わる
    fu.wait();

    std::cout << "All done.\n";
}