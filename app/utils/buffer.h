#include<memory>

using CameraAPI = void;
using FrameHandle=void;

class CameraFrame {
public:
    // 外から渡すのはデータとサイズだけ
    CameraFrame(uint8_t* data, size_t size, void* internal)
        : data_(data), size_(size), internal_(internal)
    {
    }

    ~CameraFrame() {
        release_internal();
    }

    // コピー禁止
    CameraFrame(const CameraFrame&) = delete;
    CameraFrame& operator=(const CameraFrame&) = delete;

    // ムーブは OK（パイプラインで必須）
    CameraFrame(CameraFrame&& other) noexcept {
        *this = std::move(other);
    }

    CameraFrame& operator=(CameraFrame&& other) noexcept {
        if (this != &other) {
            release_internal();
            data_ = other.data_;
            size_ = other.size_;
            internal_ = other.internal_;

            other.data_ = nullptr;
            other.internal_ = nullptr;
        }
        return *this;
    }

    // API を隠蔽したまま必要最低限のアクセスだけ提供
    uint8_t* data() const noexcept { return data_; }
    size_t   size() const noexcept { return size_; }

private:
    // API を完全に隠す
    struct InternalAPI {
        CameraAPI* api;        // eBUS の API ポインタ
        // FrameHandle handle;    // eBUS のハンドル
    };

    void release_internal() {
        if (!internal_) return;
        InternalAPI* i = reinterpret_cast<InternalAPI*>(internal_);
        // if (i->api && i->handle) {
            // i->api->ReleaseFrame(i->handle);   // ここだけ自動実行
        // }
        delete i;
        internal_ = nullptr;
        data_ = nullptr;
    }

    uint8_t* data_ = nullptr;
    size_t size_ = 0;
    void* internal_ = nullptr;   // CameraAPI を外から絶対見えないように opaque にする
};

CameraFrame makeCameraFrame(CameraAPI* api)
{
    uint8_t* p;
    size_t size;
    // FrameHandle h;

    // api->RecvFrame(&p, &size, &h);

    // auto internal = new CameraFrame::InternalAPI{ api, h };

    return CameraFrame(p, size, nullptr /*internal*/);
}

struct FrameInfo {
    CameraFrame frame;
    int index = 0;

    FrameInfo() = default;

    // コピーは禁止
    FrameInfo(const FrameInfo&) = delete;
    FrameInfo& operator=(const FrameInfo&) = delete;

    // ムーブだけ許可
    FrameInfo(FrameInfo&&) = default;
    FrameInfo& operator=(FrameInfo&&) = default;
};


// #nanosec変換

// uint64_t ns = camera_timestamp_ns;

// auto tp = std::chrono::steady_clock::time_point{
//     std::chrono::nanoseconds{ns}
// };
// steady_clock使うこと
//同期
//auto pc_now = std::chrono::steady_clock::now();
