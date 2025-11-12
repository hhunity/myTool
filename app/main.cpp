#include <iostream>
#include <string>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "lineStore.hpp"

static void ensure_grayscale(cv::Mat& m) {
    if (m.channels() == 3) {
        cv::cvtColor(m, m, cv::COLOR_BGR2GRAY);
    } else if (m.channels() == 4) {
        cv::cvtColor(m, m, cv::COLOR_BGRA2GRAY);
    }
}

int main(int argc, char** argv) {
    try {
        // ---- 引数 ----
        // usage: ./demo <image_path> [roiX] [roiW] [warmupMax] [capLines] [winW] [winH] [x0]
        if (argc < 2) {
            std::cout << "usage: " << argv[0]
                      << " <image_path> [roiX=0] [roiW=auto] [warmupMax=6] [capLines=auto] [winW=auto] [winH=64] [x0=0]\n";
            return 0;
        }
        std::string path = argv[1];
        int roiX      = (argc >= 3) ? std::stoi(argv[2]) : 0;
        int warmupMax = (argc >= 5) ? std::stoi(argv[4]) : 6;
        int winH      = (argc >= 8) ? std::stoi(argv[7]) : 64;
        int x0        = (argc >= 9) ? std::stoi(argv[8]) : 0;

        // ---- 画像読込 ----
        cv::Mat img = cv::imread(path, cv::IMREAD_ANYDEPTH | cv::IMREAD_UNCHANGED);
        if (img.empty()) {
            std::cerr << "failed to read image: " << path << "\n";
            return 1;
        }
        ensure_grayscale(img); // グレースケール化
        if (img.depth() != CV_8U && img.depth() != CV_16U) {
            // 8/16以外は8bitに変換（簡易対応）
            double minv, maxv;
            cv::minMaxLoc(img, &minv, &maxv);
            if (maxv <= 0) maxv = 1.0;
            img.convertTo(img, CV_8U, 255.0 / maxv);
        }

        const int srcWidth = img.cols;
        const int srcHeight = img.rows;
        const int elemBytes = (img.depth() == CV_16U) ? 2 : 1;
        PixelType pt = (elemBytes == 2) ? PixelType::U16 : PixelType::U8;

        if (roiX < 0) roiX = 0;
        if (roiX >= srcWidth) roiX = srcWidth - 1;
        int roiW = (argc >= 4) ? std::stoi(argv[3]) : (srcWidth - roiX);
        if (roiW <= 0) roiW = srcWidth - roiX;
        if (roiX + roiW > srcWidth) roiW = srcWidth - roiX;

        // capLines: 画像の全行 + α（ここでは + warmupMax）
        long long capLines = (argc >= 6) ? std::stoll(argv[5]) : (static_cast<long long>(srcHeight) + warmupMax);
        if (capLines < warmupMax) capLines = warmupMax;

        // winW: ROI幅がデフォルト
        int winW = (argc >= 7) ? std::stoi(argv[6]) : roiW;
        if (winW > roiW) winW = roiW;

        // ストライド（1行あたりのバイト）
        const int srcStrideBytes = static_cast<int>(img.step[0]);

        std::cout << "Image: " << srcWidth << "x" << srcHeight
                  << " depth=" << ((elemBytes==1)?"8U":"16U")
                  << " stride=" << srcStrideBytes << " bytes\n";
        std::cout << "ROI: x=" << roiX << " w=" << roiW
                  << "  win=(" << winW << "x" << winH << ", x0=" << x0 << ")\n";
        std::cout << "warmupMax=" << warmupMax << "  capLines=" << capLines << "\n";

        // ---- LineStore 構築 ----
        LineStore store(srcWidth, roiX, roiW, capLines, warmupMax, pt);

        // ---- ウォームアップ: 先頭から warmupMax 行を投入 ----
        // * 実運用ではカメラから渡される "ブロック先頭の時刻" を使う
        if (srcHeight > 0) {
            const int warm = std::min(warmupMax, srcHeight);
            const void* srcPtr = img.data; // 画像先頭
            const double t0 = std::chrono::duration<double>(
                                std::chrono::system_clock::now().time_since_epoch()).count();
            store.PushBlock(srcPtr, warm, srcStrideBytes, t0);
        }

        // ---- Commitして線形追記に切替 ----
        store.Commit();

        // ---- 残りの行をチャンクで追記（例: 64行ブロック）----
        int y = warmupMax;
        const int chunk = 64;
        while (y < srcHeight) {
            int can = std::min(chunk, srcHeight - y);
            const std::uint8_t* srcPtr = img.data + static_cast<size_t>(y) * srcStrideBytes;
            const double t = std::chrono::duration<double>(
                                std::chrono::system_clock::now().time_since_epoch()).count();
            if (!store.PushBlock(srcPtr, can, srcStrideBytes, t)) {
                std::cout << "Store is full at line " << y << "\n";
                break;
            }
            y += can;
        }

        std::cout << "HeadTotal=" << store.HeadTotal()
                  << " StoredLines=" << store.StoredLines() << "\n";

        // ---- 最新窓を取得して Mat 化 ----
        const void* winPtr = nullptr;
        int stride = 0;
        double tTop = 0.0;
        if (store.TryGetLatestWindowPtr(winW, winH, x0, winPtr, stride, tTop)) {
            std::cout << "Latest window: ptr=" << winPtr
                      << " stride=" << stride << " bytes  tTop=" << tTop << " sec\n";

            int cvtype = (elemBytes == 1) ? CV_8UC1 : CV_16UC1;
            // LineStore の内部バッファをラップ（コピーしない）
            cv::Mat winMat(winH, winW, cvtype, const_cast<void*>(winPtr), static_cast<size_t>(stride));

            // 確認用に保存
            std::string outPath = "latest_window.png";
            if (elemBytes == 2) {
                // 可視化のため 16U → 8U 正規化
                cv::Mat vis;
                double minv, maxv; cv::minMaxLoc(winMat, &minv, &maxv);
                double scale = (maxv > 0) ? (255.0 / maxv) : 1.0;
                winMat.convertTo(vis, CV_8U, scale);
                cv::imwrite(outPath, vis);
            } else {
                cv::imwrite(outPath, winMat);
            }
            std::cout << "Saved: " << outPath << "\n";

            // 画面表示（任意）
            // cv::imshow("latest", (elemBytes==2)? vis : winMat);
            // cv::waitKey(0);
        } else {
            std::cout << "No window available (StoredLines < winH?)\n";
        }

        return 0;

    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << "\n";
        return 1;
    }
}