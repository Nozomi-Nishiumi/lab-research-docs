#!/usr/bin/env python3
# ============================================================
# capture_min.py — OpenCV だけでカメラを映す最小構成
#
# 取り込み(cap.read)と表示(imshow)を1つのループで行う、
# いちばん素朴な形。まずこれでカメラと OpenCV の動作を確認する。
#
# 使い方:
#   python capture_min.py            # カメラ 0 を開く
#   python capture_min.py 1          # カメラ番号を指定
#   ESC で終了
#
# 必要: pip install opencv-python
# ============================================================

import sys
import time
import cv2

camera_id = int(sys.argv[1]) if len(sys.argv) > 1 else 0

cap = cv2.VideoCapture(camera_id)
if not cap.isOpened():
    raise SystemExit(f"カメラ {camera_id} を開けません")

# 解像度の要求(カメラが対応していなければ別の値になる)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
print("解像度:", int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)), "x",
      int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)))

n, fps, t0 = 0, 0.0, time.perf_counter()
while True:
    ok, frame = cap.read()          # 1フレーム取り込む(ブロッキング)
    if not ok:
        break

    # 実測fpsを左上に描く(0.5秒ごとに更新)
    n += 1
    dt = time.perf_counter() - t0
    if dt >= 0.5:
        fps = n / dt
        n, t0 = 0, time.perf_counter()
    cv2.putText(frame, f"{fps:5.1f} fps", (20, 40),
                cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 2)

    cv2.imshow("camera", frame)     # 表示(ウィンドウは OpenCV 任せ)
    if cv2.waitKey(1) & 0xFF == 27:  # ESC で終了。waitKey が無いと描画されない
        break

cap.release()
cv2.destroyAllWindows()
