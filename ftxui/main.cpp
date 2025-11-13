#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <array>

using namespace ftxui;

// メニューの結果
enum class MenuResult {
    RunWorker,      // 10秒間 cout を出し続ける
    RunEchoInputs,  // 4つの入力をもらって、それを10秒間表示
    Exit,
};

//---------------------------------------------------------
// 100桁の乱数を1つ作る（オマケ用）
//---------------------------------------------------------
std::string GenerateRandom100() {
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, 9);

    std::string s;
    s.reserve(100);
    for (int i = 0; i < 100; ++i) {
        s += char('0' + dist(rng));
    }
    return s;
}

//---------------------------------------------------------
// メニュー画面を表示して、選択された項目を返す
//---------------------------------------------------------
MenuResult ShowMenu() {
    auto screen = ScreenInteractive::TerminalOutput();

    std::vector<std::string> entries = {
        "1) 10秒間 cout を出し続ける",
        "2) 4つの文字列を入力して、それを10秒間表示",
        "3) Exit",
    };
    int selected = 0;
    MenuResult result = MenuResult::Exit;  // デフォルト

    auto menu = Menu(&entries, &selected);

    auto layout = Renderer(menu, [&] {
        return vbox({
                   text("myTool : v1.0") | bold | center,
                   separator(),
                   menu->Render() | border,
                   separator(),
                   text("↑↓ or 1/2/3: 選択, Enter: 決定") | dim,
               }) |
               border;
    });

    // マウスを無効化＆キーイベント処理
    auto ui = CatchEvent(layout, [&](Event e) {
        // マウスは全部握りつぶして無効化
        if (e.is_mouse()) {
            return true;
        }

        // '1','2','3' でも選択できるように
        if (e.is_character()) {
            if (e.character() == "1") selected = 0;
            if (e.character() == "2") selected = 1;
            if (e.character() == "3") selected = 2;
        }

        // Enter で決定
        if (e == Event::Return) {
            if (selected == 0) {
                result = MenuResult::RunWorker;
            } else if (selected == 1) {
                result = MenuResult::RunEchoInputs;
            } else {
                result = MenuResult::Exit;
            }
            screen.Exit();
            return true;
        }

        return false;  // それ以外のキーは Menu コンポーネントに任せる
    });

    screen.Loop(ui);
    return result;
}

//---------------------------------------------------------
// 4つの入力欄を持つ画面を出して、OKで確定して文字列を返す
// 戻り値: true = OK, false = Cancel
//---------------------------------------------------------
bool ShowInputForm(std::array<std::string, 4>& out_values) {
    auto screen = ScreenInteractive::TerminalOutput();

    std::string f1, f2, f3, f4;
    bool done = false;
    bool canceled = false;

    // 4つの入力コンポーネント
    auto input1 = Input(&f1, "Input 1");
    auto input2 = Input(&f2, "Input 2");
    auto input3 = Input(&f3, "Input 3");
    auto input4 = Input(&f4, "Input 4");

    // OK / Cancel ボタン
    auto button_ok = Button(" OK ", [&] {
        done = true;
        canceled = false;
        screen.Exit();
    });
    auto button_cancel = Button(" Cancel ", [&] {
        done = true;
        canceled = true;
        screen.Exit();
    });

    // コンテナにまとめる（Tabでフォーカス移動可能）
    auto container = Container::Vertical({
        input1, input2, input3, input4,
        button_ok, button_cancel,
    });

    // レイアウト（見た目）
    auto layout = Renderer(container, [&] {
        return vbox({
                   text("Input 4 values") | bold | center,
                   separator(),
                   hbox(text("1: "), input1->Render()) ,
                   hbox(text("2: "), input2->Render()) ,
                   hbox(text("3: "), input3->Render()) ,
                   hbox(text("4: "), input4->Render()) ,
                   separator(),
                   hbox({
                       button_ok->Render(),
                       text("  "),
                       button_cancel->Render(),
                   }) | center,
                   separator(),
                   text("Tab: フォーカス移動, Enter/Space: ボタン決定") | dim,
               }) |
               border;
    });

    // マウス無効＋イベント処理
    auto ui = CatchEvent(layout, [&](Event e) {
        if (e.is_mouse()) {
            // マウス操作は全部無効にする
            return true;
        }
        return false;
    });

    screen.Loop(ui);

    if (!done || canceled) {
        return false;  // キャンセル扱い
    }

    out_values[0] = f1;
    out_values[1] = f2;
    out_values[2] = f3;
    out_values[3] = f4;
    return true;
}

//---------------------------------------------------------
int main() {
    using namespace std::chrono_literals;

    while (true) {
        // 1. メインメニュー表示
        MenuResult r = ShowMenu();
        if (r == MenuResult::Exit) {
            std::cout << "アプリケーションを終了します。\n";
            break;
        }

        if (r == MenuResult::RunWorker) {
            // 2-1. 10秒間、適当なスレッドで cout を出し続ける
            std::cout << "\n=== 10秒間、workerスレッドで cout を出し続けます ===\n";

            auto start = std::chrono::steady_clock::now();
            std::thread worker([start]() {
                using namespace std::chrono;
                int count = 0;
                while (steady_clock::now() - start < 10s) {
                    std::cout << "[worker] tick " << count++
                              << "  random=" << GenerateRandom100().substr(0, 8)
                              << std::endl;
                    std::this_thread::sleep_for(1s);
                }
                std::cout << "[worker] 終了しました。\n";
            });

            worker.join();

            std::cout << "\n10秒の処理が終わりました。Enter を押すとメニューに戻ります..."
                      << std::flush;
            std::string dummy;
            std::getline(std::cin, dummy);
        }
        else if (r == MenuResult::RunEchoInputs) {
            // 2-2. 4つの入力フォームを出す
            std::array<std::string, 4> values;
            bool ok = ShowInputForm(values);
            if (!ok) {
                std::cout << "\n入力がキャンセルされました。Enter でメニューに戻ります..."
                          << std::flush;
                std::string dummy;
                std::getline(std::cin, dummy);
                continue;
            }

            // 入力結果を表示
            std::cout << "\n=== 入力された文字列 ===\n";
            for (int i = 0; i < 4; ++i) {
                std::cout << "  [" << (i + 1) << "] " << values[i] << "\n";
            }

            // 10秒間、別スレッドでこの文字列をひたすら出し続ける
            std::cout << "\n=== これらの文字列を 10 秒間 出し続けます ===\n";

            auto start = std::chrono::steady_clock::now();
            std::thread worker([start, values]() {
                using namespace std::chrono;
                int count = 0;
                while (steady_clock::now() - start < 10s) {
                    std::cout << "[echo " << count++ << "]\n";
                    for (int i = 0; i < 4; ++i) {
                        std::cout << "  [" << (i + 1) << "] " << values[i] << "\n";
                    }
                    std::cout << std::endl;
                    std::this_thread::sleep_for(1s);
                }
                std::cout << "[echo] 終了しました。\n";
            });

            worker.join();

            std::cout << "\n10秒の処理が終わりました。Enter を押すとメニューに戻ります..."
                      << std::flush;
            std::string dummy;
            std::getline(std::cin, dummy);
        }
    }

    return 0;
}