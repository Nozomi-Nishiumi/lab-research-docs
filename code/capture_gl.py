#!/usr/bin/env python3
# ============================================================
# capture_gl.py — カメラキャプチャ(独立スレッド) + OpenGL テクスチャ描画
#
# キャプチャ専用スレッドがカメラの上限速度でフレームを取り込み、
# メインスレッドは vsync(ディスプレイのリフレッシュレート)に合わせて
# 「最新フレームだけ」をテクスチャとして描画する。
# → 撮影レートが描画レートに律速されない。
#
# 使い方:
#   python capture_gl.py                # カメラ 0 を開く
#   python capture_gl.py --camera 1    # カメラ番号を指定
#   python capture_gl.py --test        # カメラ無しで合成映像を使う
#   python capture_gl.py --seconds 5   # 5秒後に自動終了(動作確認用)
#   ESC または ウィンドウを閉じると終了
#
# タイトルバーに camera fps(取り込み)と display fps(描画)を表示。
# camera fps が display fps を上回っていれば分離が効いている。
#
# 必要: pip install opencv-python glfw PyOpenGL
# ============================================================

import argparse
import threading
import time

import numpy as np
import cv2
import glfw
from OpenGL.GL import (
    GL_BGR, GL_CLAMP_TO_EDGE, GL_COLOR_BUFFER_BIT, GL_LINEAR, GL_QUADS,
    GL_RGB, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_TEXTURE_MIN_FILTER,
    GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_UNPACK_ALIGNMENT,
    GL_UNSIGNED_BYTE, glBegin, glBindTexture, glClear, glClearColor,
    glDisable, glEnable, glEnd, glGenTextures, glPixelStorei, glTexCoord2f,
    glTexImage2D, glTexParameteri, glTexSubImage2D, glVertex2f, glViewport,
)


class CaptureThread:
    """カメラを専用スレッドで回し、常に最新フレームだけを保持する。

    cv2.VideoCapture.read() は C 側で GIL を解放するため、
    Python の threading でも取り込みと描画は実際に並列動作する。
    """

    def __init__(self, camera_id=0, test=False, width=1280, height=720, req_fps=120):
        self.test = test
        self.size = (width, height)
        self.lock = threading.Lock()
        self.frame = None          # 最新フレーム(BGR, np.ndarray)
        self.seq = 0               # フレーム通し番号(描画側の新着判定用)
        self.fps = 0.0             # 実測キャプチャfps(0.5秒窓)
        self.running = True

        if not test:
            self.cap = cv2.VideoCapture(camera_id)
            if not self.cap.isOpened():
                raise RuntimeError(
                    f"カメラ {camera_id} を開けません。--test で合成映像を試せます")
            # 高速化の要:MJPG圧縮・バッファ1枚・解像度/レートの要求
            self.cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"MJPG"))
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
            self.cap.set(cv2.CAP_PROP_FPS, req_fps)
            self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

        self.thread = threading.Thread(target=self._loop, daemon=True)
        self.thread.start()

    def _make_test_frame(self, t):
        """カメラ無しでの動作確認用:動くグラデーションと時刻を描く"""
        w, h = self.size
        x = np.linspace(0, 1, w, dtype=np.float32)
        y = np.linspace(0, 1, h, dtype=np.float32)[:, None]
        phase = t * 2.0
        img = np.empty((h, w, 3), dtype=np.uint8)
        img[:, :, 0] = ((np.sin(x * 6.28 + phase) * 0.5 + 0.5) * 255)      # B
        img[:, :, 1] = ((np.cos(y * 6.28 + phase) * 0.5 + 0.5) * 255)      # G
        img[:, :, 2] = ((x * 0 + (np.sin(phase) * 0.5 + 0.5)) * 255)       # R
        cv2.putText(img, f"TEST {t:8.3f}s", (40, h // 2),
                    cv2.FONT_HERSHEY_SIMPLEX, 2.0, (255, 255, 255), 4)
        return img

    def _loop(self):
        n = 0
        win_t0 = time.perf_counter()
        t0 = win_t0
        while self.running:
            if self.test:
                frame = self._make_test_frame(time.perf_counter() - t0)
                time.sleep(1 / 240)          # 合成は240fps相当に制限
                ok = True
            else:
                ok, frame = self.cap.read()  # ここがカメラ律速(ブロッキング)
            if not ok:
                self.running = False
                break
            with self.lock:                  # 参照の差し替えのみ(コピー不要)
                self.frame = frame
                self.seq += 1
            # 0.5秒ごとに実測fpsを更新
            n += 1
            now = time.perf_counter()
            if now - win_t0 >= 0.5:
                self.fps = n / (now - win_t0)
                n = 0
                win_t0 = now

    def latest(self):
        """(seq, frame) を返す。フレームが無ければ (0, None)。"""
        with self.lock:
            return self.seq, self.frame

    def stop(self):
        self.running = False
        self.thread.join(timeout=2.0)
        if not self.test:
            self.cap.release()


def main():
    ap = argparse.ArgumentParser(description="camera capture -> OpenGL texture")
    ap.add_argument("--camera", type=int, default=0, help="カメラ番号(既定 0)")
    ap.add_argument("--test", action="store_true", help="カメラ無しの合成映像で実行")
    ap.add_argument("--seconds", type=float, default=0,
                    help="指定秒数で自動終了(0 = ESC まで)")
    args = ap.parse_args()

    cap = CaptureThread(camera_id=args.camera, test=args.test)

    if not glfw.init():
        raise RuntimeError("GLFW の初期化に失敗")
    # 既定の互換プロファイル(macOS では OpenGL 2.1)で十分
    window = glfw.create_window(960, 540, "capture_gl", None, None)
    if not window:
        glfw.terminate()
        raise RuntimeError("ウィンドウ生成に失敗")
    glfw.make_context_current(window)
    glfw.swap_interval(1)   # vsync ON:描画はディスプレイのレートで回る

    tex = glGenTextures(1)
    glBindTexture(GL_TEXTURE_2D, tex)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1)

    tex_size = None      # 確保済みテクスチャの (w, h)
    last_seq = 0
    disp_n, disp_fps = 0, 0.0
    win_t0 = time.perf_counter()
    start = win_t0

    while not glfw.window_should_close(window):
        if glfw.get_key(window, glfw.KEY_ESCAPE) == glfw.PRESS:
            break
        if args.seconds and time.perf_counter() - start >= args.seconds:
            break

        seq, frame = cap.latest()
        if frame is not None and seq != last_seq:
            last_seq = seq
            h, w = frame.shape[:2]
            if tex_size != (w, h):
                # 初回(または解像度変更時)のみ領域確保
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                             GL_BGR, GL_UNSIGNED_BYTE, frame)
                tex_size = (w, h)
            else:
                # 2回目以降は転送のみ(高速)。BGR のまま渡して変換を省く
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                                GL_BGR, GL_UNSIGNED_BYTE, frame)

        fb_w, fb_h = glfw.get_framebuffer_size(window)
        glViewport(0, 0, fb_w, fb_h)
        glClearColor(0.08, 0.08, 0.10, 1.0)
        glClear(GL_COLOR_BUFFER_BIT)

        if tex_size:
            glEnable(GL_TEXTURE_2D)
            glBegin(GL_QUADS)   # 画像は上下反転して貼る(画像は上原点、GLは下原点)
            glTexCoord2f(0, 1); glVertex2f(-1, -1)
            glTexCoord2f(1, 1); glVertex2f(+1, -1)
            glTexCoord2f(1, 0); glVertex2f(+1, +1)
            glTexCoord2f(0, 0); glVertex2f(-1, +1)
            glEnd()
            glDisable(GL_TEXTURE_2D)

        glfw.swap_buffers(window)   # vsync 待ちはここ(描画スレッドだけが待つ)
        glfw.poll_events()

        # 表示側の実測fpsとタイトル更新
        disp_n += 1
        now = time.perf_counter()
        if now - win_t0 >= 0.5:
            disp_fps = disp_n / (now - win_t0)
            disp_n = 0
            win_t0 = now
            glfw.set_window_title(
                window,
                f"capture_gl — camera {cap.fps:5.1f} fps | display {disp_fps:5.1f} fps")

    cap.stop()
    glfw.terminate()


if __name__ == "__main__":
    main()
