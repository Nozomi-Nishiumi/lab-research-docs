// ============================================================
// capture_min.cpp — OpenCV だけでカメラを映す最小構成
//
// 取り込み(cap.read)と表示(imshow)を1つのループで行う、
// いちばん素朴な形。まずこれでカメラと OpenCV の動作を確認する。
//
// ビルド:
//   macOS : brew install opencv pkg-config
//           clang++ -std=c++17 -O2 capture_min.cpp -o capture_min \
//             $(pkg-config --cflags --libs opencv4)
//   Windows: 同梱の CMakeLists.txt + vcpkg を使用(解説ページ参照)
//
// 使い方:
//   ./capture_min            # カメラ 0 を開く
//   ./capture_min 1          # カメラ番号を指定
//   ESC で終了
// ============================================================

#include <opencv2/opencv.hpp>
#include <chrono>
#include <cstdio>

int main(int argc, char** argv) {
    const int cameraId = (argc > 1) ? std::atoi(argv[1]) : 0;

    cv::VideoCapture cap(cameraId);
    if (!cap.isOpened()) {
        std::fprintf(stderr, "カメラ %d を開けません\n", cameraId);
        return 1;
    }

    // 解像度の要求(カメラが対応していなければ別の値になる)
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    std::printf("解像度: %d x %d\n",
                int(cap.get(cv::CAP_PROP_FRAME_WIDTH)),
                int(cap.get(cv::CAP_PROP_FRAME_HEIGHT)));

    cv::Mat frame;
    int n = 0;
    double fps = 0.0;
    auto t0 = std::chrono::steady_clock::now();

    while (cap.read(frame)) {       // 1フレーム取り込む(ブロッキング)
        // 実測fpsを左上に描く(0.5秒ごとに更新)
        ++n;
        const double dt = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();
        if (dt >= 0.5) {
            fps = n / dt;
            n = 0;
            t0 = std::chrono::steady_clock::now();
        }
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%5.1f fps", fps);
        cv::putText(frame, buf, {20, 40}, cv::FONT_HERSHEY_SIMPLEX,
                    1.0, {0, 255, 0}, 2);

        cv::imshow("camera", frame);   // 表示(ウィンドウは OpenCV 任せ)
        if (cv::waitKey(1) == 27) break;  // ESC で終了。waitKey が無いと描画されない
    }
    return 0;
}
