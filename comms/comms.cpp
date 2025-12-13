#include <zmq.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    zmq::context_t context(1);

    // ---- REP ソケット（コマンド応答）----
    zmq::socket_t rep(context, zmq::socket_type::rep);
    rep.bind("tcp://*:5555");

    // ---- PUB ソケット（状態配信）----
    zmq::socket_t pub(context, zmq::socket_type::pub);
    pub.bind("tcp://*:5556");

    while (true) {

        // ---- REQ → REP の受信処理 ----
        zmq::pollitem_t items[] = {
            { static_cast<void*>(rep), 0, ZMQ_POLLIN, 0 }
        };

        // 10ms poll
        zmq::poll(items, 1, std::chrono::milliseconds(10));

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            rep.recv(msg, zmq::recv_flags::none);

            std::string s(static_cast<char*>(msg.data()), msg.size());
            json cmd = json::parse(s);

            std::cout << "cmd: " << cmd.dump() << std::endl;

            // ---- JSON を返す ----
            json reply = {
                {"ack", true},
                {"cmd", cmd}
            };
            std::string rep_str = reply.dump();
            rep.send(zmq::buffer(rep_str), zmq::send_flags::none);
        }

        // ---- PUB の状態配信 ----
        json status_msg = {
            {"time", static_cast<double>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count() / 1000.0)},
            {"status", "running"},
            {"fps", 30.5},
            {"temperature", 45.3}
        };

        std::string json_text = status_msg.dump();
        pub.send(zmq::buffer(json_text), zmq::send_flags::none);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}