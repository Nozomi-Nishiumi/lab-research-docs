// ============================================================
// capture_gl.cpp — カメラキャプチャ(独立スレッド) + OpenGL テクスチャ描画
//
// キャプチャ専用スレッドがカメラの上限速度でフレームを取り込み、
// メインスレッドは vsync(ディスプレイのリフレッシュレート)に合わせて
// 「最新フレームだけ」をテクスチャとして描画する。
// → 撮影レートが描画レートに律速されない。
//
// ビルド:
//   macOS : brew install opencv glfw pkg-config
//           clang++ -std=c++17 -O2 capture_gl.cpp -o capture_gl \
//             $(pkg-config --cflags --libs opencv4 glfw3) -framework OpenGL
//   Windows: 同梱の CMakeLists.txt + vcpkg を使用(解説ページ参照)
//
// 使い方:
//   ./capture_gl                # カメラ 0 を開く
//   ./capture_gl --camera 1    # カメラ番号を指定
//   ./capture_gl --test        # カメラ無しで合成映像を使う
//   ./capture_gl --seconds 5   # 5秒後に自動終了(動作確認用)
//   ESC または ウィンドウを閉じると終了
//
// タイトルバーに camera fps(取り込み)と display fps(描画)を表示。
// camera fps が display fps を上回っていれば分離が効いている。
// ============================================================

#define GL_SILENCE_DEPRECATION      // macOS の OpenGL 非推奨警告を抑制
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

using Clock = std::chrono::steady_clock;

static double secondsSince(Clock::time_point t0) {
    return std::chrono::duration<double>(Clock::now() - t0).count();
}

// ---- キャプチャスレッドと共有する状態 --------------------------------
struct SharedFrame {
    std::mutex mtx;
    cv::Mat frame;                    // 最新フレーム(BGR)
    uint64_t seq = 0;                 // フレーム通し番号(新着判定用)
    std::atomic<bool> running{true};
    std::atomic<double> fps{0.0};     // 実測キャプチャfps(0.5秒窓)
};

// カメラ無しでの動作確認用:動くグラデーションと経過時間を描く
static cv::Mat makeTestFrame(int w, int h, double t) {
    cv::Mat img(h, w, CV_8UC3);
    const double phase = t * 2.0;
    for (int y = 0; y < h; ++y) {
        auto* row = img.ptr<cv::Vec3b>(y);
        const double gy = std::cos(y * 6.28 / h + phase) * 0.5 + 0.5;
        for (int x = 0; x < w; ++x) {
            const double gx = std::sin(x * 6.28 / w + phase) * 0.5 + 0.5;
            row[x] = cv::Vec3b(uchar(gx * 255), uchar(gy * 255),
                               uchar((std::sin(phase) * 0.5 + 0.5) * 255));
        }
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "TEST %8.3fs", t);
    cv::putText(img, buf, {40, h / 2}, cv::FONT_HERSHEY_SIMPLEX, 2.0,
                {255, 255, 255}, 4);
    return img;
}

// ---- キャプチャスレッド本体 ------------------------------------------
static void captureLoop(SharedFrame& sh, int cameraId, bool test,
                        int width, int height, int reqFps) {
    cv::VideoCapture cap;
    if (!test) {
        cap.open(cameraId);
        if (!cap.isOpened()) {
            std::fprintf(stderr,
                "カメラ %d を開けません。--test で合成映像を試せます\n", cameraId);
            sh.running = false;
            return;
        }
        // 高速化の要:MJPG圧縮・バッファ1枚・解像度/レートの要求
        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
        cap.set(cv::CAP_PROP_FRAME_WIDTH,  width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        cap.set(cv::CAP_PROP_FPS, reqFps);
        cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
    }

    cv::Mat frame;
    int n = 0;
    auto t0 = Clock::now();
    auto winT0 = t0;
    while (sh.running) {
        if (test) {
            frame = makeTestFrame(width, height, secondsSince(t0));
            std::this_thread::sleep_for(std::chrono::microseconds(4166)); // ≈240fps
        } else {
            if (!cap.read(frame)) {      // ここがカメラ律速(ブロッキング)
                sh.running = false;
                break;
            }
        }
        {
            std::lock_guard<std::mutex> lk(sh.mtx);
            cv::swap(sh.frame, frame);   // バッファ交換(コピー無し)
            sh.seq++;
        }
        // 0.5秒ごとに実測fpsを更新
        ++n;
        const double dt = secondsSince(winT0);
        if (dt >= 0.5) {
            sh.fps = n / dt;
            n = 0;
            winT0 = Clock::now();
        }
    }
}

int main(int argc, char** argv) {
    int cameraId = 0;
    bool test = false;
    double seconds = 0;                 // 0 = ESC まで
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--camera") && i + 1 < argc) cameraId = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--test")) test = true;
        else if (!std::strcmp(argv[i], "--seconds") && i + 1 < argc) seconds = std::atof(argv[++i]);
    }

    SharedFrame sh;
    std::thread capThread(captureLoop, std::ref(sh), cameraId, test, 1280, 720, 120);

    if (!glfwInit()) { sh.running = false; capThread.join(); return 1; }
    // 既定の互換プロファイル(macOS では OpenGL 2.1)で十分
    GLFWwindow* window = glfwCreateWindow(960, 540, "capture_gl", nullptr, nullptr);
    if (!window) { glfwTerminate(); sh.running = false; capThread.join(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);                // vsync ON:描画はディスプレイのレートで回る

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int texW = 0, texH = 0;             // 確保済みテクスチャの寸法
    uint64_t lastSeq = 0;
    cv::Mat local;                      // 描画スレッド側のフレーム
    int dispN = 0;
    auto winT0 = Clock::now();
    const auto start = winT0;

    while (!glfwWindowShouldClose(window) && sh.running) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;
        if (seconds > 0 && secondsSince(start) >= seconds) break;

        // 新着フレームがあれば受け取る(ロックは差し替えの瞬間だけ)
        bool fresh = false;
        {
            std::lock_guard<std::mutex> lk(sh.mtx);
            if (sh.seq != lastSeq && !sh.frame.empty()) {
                lastSeq = sh.seq;
                sh.frame.copyTo(local);
                fresh = true;
            }
        }
        if (fresh) {
            if (local.cols != texW || local.rows != texH) {
                // 初回(または解像度変更時)のみ領域確保
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, local.cols, local.rows,
                             0, GL_BGR, GL_UNSIGNED_BYTE, local.data);
                texW = local.cols; texH = local.rows;
            } else {
                // 2回目以降は転送のみ(高速)。BGR のまま渡して変換を省く
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH,
                                GL_BGR, GL_UNSIGNED_BYTE, local.data);
            }
        }

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (texW > 0) {
            glEnable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);  // 画像は上下反転して貼る(画像は上原点、GLは下原点)
            glTexCoord2f(0, 1); glVertex2f(-1, -1);
            glTexCoord2f(1, 1); glVertex2f(+1, -1);
            glTexCoord2f(1, 0); glVertex2f(+1, +1);
            glTexCoord2f(0, 0); glVertex2f(-1, +1);
            glEnd();
            glDisable(GL_TEXTURE_2D);
        }

        glfwSwapBuffers(window);        // vsync 待ちはここ(描画スレッドだけが待つ)
        glfwPollEvents();

        // 表示側の実測fpsとタイトル更新
        ++dispN;
        const double dt = secondsSince(winT0);
        if (dt >= 0.5) {
            char title[128];
            std::snprintf(title, sizeof(title),
                          "capture_gl — camera %5.1f fps | display %5.1f fps",
                          sh.fps.load(), dispN / dt);
            glfwSetWindowTitle(window, title);
            dispN = 0;
            winT0 = Clock::now();
        }
    }

    sh.running = false;
    capThread.join();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
