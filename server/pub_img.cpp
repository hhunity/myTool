#include <zmq.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

struct ImgMeta {
    uint32_t w, h, stride;
    const char* pixfmt;   // "GRAY8" / "GRAY16" / "BGR24" etc
    uint64_t frame_id;
    uint64_t ts_ns;
};

// 超簡易：JSON文字列にする（本番は MessagePack等も検討）
static std::string make_header_json(const ImgMeta& m) {
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        R"({"ver":1,"frame_id":%llu,"ts_ns":%llu,"w":%u,"h":%u,"stride":%u,"pixfmt":"%s"})",
        (unsigned long long)m.frame_id, (unsigned long long)m.ts_ns,
        m.w, m.h, m.stride, m.pixfmt);
    return std::string(buf);
}

int main() {
    zmq::context_t ctx{1};
    zmq::socket_t pub{ctx, zmq::socket_type::pub};

    // 詰まり対策
    pub.set(zmq::sockopt::sndhwm, 1);
    pub.set(zmq::sockopt::linger, 0);
    // conflate が使える環境なら有効化（libzmqのビルド/バインディング依存）
    // pub.set(zmq::sockopt::conflate, 1);

    pub.bind("tcp://127.0.0.1:5557");

    const std::string topic = "img.preview";

    while (true) {
        ImgMeta meta{512,512,512*2,"GRAY16", /*frame_id*/ 1, /*ts*/ 0};

        // 画像バイト列（例）
        std::vector<uint8_t> img(meta.h * meta.stride);
        // img を埋める…

        std::string hdr = make_header_json(meta);

        // multipart送信
        pub.send(zmq::buffer(topic), zmq::send_flags::sndmore);
        pub.send(zmq::buffer(hdr),   zmq::send_flags::sndmore);

        // 送れないなら捨てる（non-blocking）
        auto ok = pub.send(zmq::buffer(img.data(), img.size()),
                           zmq::send_flags::dontwait);
        (void)ok;
    }
}