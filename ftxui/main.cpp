#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// メニューの結果
enum class MenuResult {
    RunWorker,  // 10秒間 cout を出す
    Exit,       // 終了
};

// FTXUI でメニューを表示して、選ばれた結果を返す関数
MenuResult ShowMenu() {
    using namespace ftxui;

    auto screen = ScreenInteractive::TerminalOutput();

    std::vector<std::string> entries = {
        "1) 10秒間 cout を出し続ける",
        "2) Exit",
    };
    int selected = 0;
    MenuResult result = MenuResult::Exit;  // デフォルトは Exit にしておく

    auto menu = Menu(&entries, &selected);

    // 見た目を定義
    auto layout = Renderer(menu, [&] {
        return vbox({
                   text("myTool : v1.0") | bold,
                   separator(),
                   menu->Render() | border,
                   separator(),
                   text("↑↓ or 1/2: 選択, Enter: 決定") | dim,
               }) |
               border;
    });

    // キーイベント処理
    auto ui = CatchEvent(layout, [&](Event e) {
        if (e.is_mouse()) {
            return true;   // 「処理したことにする」＝中には渡さない
        }
        
        // '1', '2' でも選べるように
        if (e.is_character()) {
            if (e.character() == "1") selected = 0;
            if (e.character() == "2") selected = 1;
        }

        // Enter で決定
        if (e == Event::Return) {
            if (selected == 0) {
                result = MenuResult::RunWorker;
            } else {
                result = MenuResult::Exit;
            }
            screen.Exit();  // ここで FTXUI のループを抜ける
            return true;
        }
        return false;  // それ以外のキーはデフォルト処理
    });

    screen.Loop(ui);  // メニュー表示・操作ループ
    return result;
}

int main() {
    using namespace std::chrono_literals;

    while (true) {
        // 1. メニューを表示して選択させる
        MenuResult r = ShowMenu();
        if (r == MenuResult::Exit) {
            std::cout << "アプリケーションを終了します。\n";
            break;
        }

        // 2. RunWorker が選ばれた場合:
        std::cout << "\n=== 10秒間 cout を出し続けます ===\n";

        // 別スレッドで10秒間 cout を出し続ける
        auto start = std::chrono::steady_clock::now();
        std::thread worker([start]() {
            using namespace std::chrono;
            int count = 0;
            while (steady_clock::now() - start < 10s) {
                std::cout << "[worker] tick " << count++ << std::endl;
                std::this_thread::sleep_for(1s);
            }
            std::cout << "[worker] 終了しました。\n";
        });

        // メインスレッドは worker が終わるのを待つ
        worker.join();

        // 3. 終わったら Enter 待ち
        std::cout << "\n10秒の処理が終わりました。Enter を押すとメニューに戻ります..." << std::flush;
        std::string dummy;
        std::getline(std::cin, dummy);

        // 4. while ループ先頭に戻って、また ShowMenu() へ
    }

    return 0;
}