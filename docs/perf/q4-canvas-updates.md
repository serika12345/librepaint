# Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の描画更新の1系統化

Issue #108 の Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の完了条件
「描画の更新の仕組みを 1 系統にする」の設計資料。対象の指標は、Surface Pro 7 のホバー輪郭と
インクの入力から表示まで（中央値と P95）と、画面への受け渡しの回数である。

## 分かっていること

正式版 Qt 6.11.3 の Windows 版では、合成入力の 32.68 秒に `paintgl_begin` が 3,718〜4,112 回、
表示の変化が 1,943〜1,949 回あり、表示 1 回あたり 1.91〜2.11 回の描画になっていた。フォーク版の
Qt は同じ条件で 1.05 回である（2026-11-11 の `paint_cycle` の再集計）。原因は、フォーク版の
`QWidgetRepaintManager` にあるフレームレート圧縮が正式版にないこと、そしてホバーの輪郭更新が
`KisCanvas2::updateCanvasToolOutlineWdg()` から `QWidget::update()` を直接呼び、`KisCanvas2` の
圧縮を通らないことである。

## 比べた方法

| 方法 | 出典 | 利点 | 制約 |
| --- | --- | --- | --- |
| `QOpenGLWidget::frameSwapped` を契機に次の更新を出す | Qt 6 `QOpenGLWidget::frameSwapped`（https://doc.qt.io/qt-6/qopenglwidget.html#frameSwapped）。同じ信号を持つ `QOpenGLWindow::frameSwapped` の説明は「垂直同期に合わせて描き続けたいアプリは、この信号で `update()` を出すと、タイマーより滑らかになる」と明記している（https://doc.qt.io/qt-6/qopenglwindow.html#frameSwapped） | 実際の画面合成と交換の完了を契機にする。タイマーの位相に依存せず、表示 1 回につき最大 1 回にまとめられる。`QOpenGLWidget` の描画経路そのものに使える | 交換が起きない状態（非表示、ドライバーの停止）では信号が来ないため、期限付きの退避が必要 |
| 最上位の `QWindow::requestUpdate()` を契機にする | Qt 6 `QWindow::requestUpdate()`（https://doc.qt.io/qt-6/qwindow.html#requestUpdate）。表示の垂直同期に合う環境では同期し、合わない環境では最大 5ms 後に届く。複数回呼んでも 1 回にまとまる | 公式がアニメーション用に用意した垂直同期の入口である | 届くのは `QWindow` の `QEvent::UpdateRequest` で、`QOpenGLWidget` の `QWidget::update()` の描画経路とは別である。ウィジェットへ橋渡しすると、`QWidgetWindow` の更新処理と順序の取り決めが要る |
| 画面のリフレッシュレートのタイマー | Qt 6 `QScreen::refreshRate`（https://doc.qt.io/qt-6/qscreen.html#refreshRate-prop）は「タイマーでアニメーションを駆動せず `QWindow::requestUpdate()` を使うこと」と警告している | 実装が単純で、交換が来ない環境でも動く | タイマーの位相が表示とずれ、入力から次の表示の直前を狙えない。位相によっては待ちを 1 周期足す |
| 公式 Krita が正式版 Qt で取っている方法 | Krita の `libs/ui/canvas/kis_canvas2.cpp`（https://invent.kde.org/graphics/krita/-/blob/master/libs/ui/canvas/kis_canvas2.cpp）は、`KRITA_QT_HAS_UPDATE_COMPRESSION_PATCH` が偽のとき `KisSignalCompressor` を `fpsLimit`（既定 100、10ms）で使う。パッチの有無は `CMakeLists.txt` の `QRhi::isLastFrameCompletedOnGPU` の検出で決まる（https://invent.kde.org/graphics/krita/-/blob/master/CMakeLists.txt） | フォークのパッチがない環境でも、一般のキャンバス更新をまとめられる | 10ms の圧縮は表示の周期（約 16.7ms）と同期しない。また公式 Krita でも輪郭の更新はこの圧縮を通らず、正式版 Qt では入力のまとまりごとに描く（LibrePaint の実測と同じ経路） |

## 採用する仕組み

正式版 Qt で OpenGL キャンバスを使うとき、LibrePaint 側の 1 つのゲート
（`KisCanvasFrameUpdateGate`）がすべてのキャンバス更新を通す。

1. 更新要求が来て、前のフレームが描画中でなければ、すぐに `slotDoCanvasUpdate()` を呼ぶ
   （最初の要求は待たせない）。
2. 前のフレームの描画中に来た要求は、領域だけを溜めて待つ。
3. `QWindow::requestUpdate()`（画面の垂直同期に合う環境ではそれに同期し、合わない環境では
   最大 5ms 後に届く）で、溜めた領域があれば次の更新を出す。前回の描画から表示 1 周期未満なら、
   残りの時間を待ってから出す。
4. `QOpenGLWidget::frameSwapped`（画面合成と交換の完了）で、前のフレームの描画が終わったことを
   記録する。交換の信号が 2 表示周期来ない場合だけ、退避のタイマーで待ちを解く。これは通常の
   間隔を作るためのタイマーではない。

これにより、入力のまとまりごとに `paintGL` を走らせず、表示 1 回につき最大 1 回にそろえる。
最初の要求は待たせないので、入力から表示までの待ちを新たに足さない。フォーク版の Qt
（`KRITA_QT_HAS_UPDATE_COMPRESSION_PATCH` が真）では、これまでどおり Qt の圧縮に任せ、
LibrePaint 側のゲートは使わない。

比較した範囲（Qt 6.11.3、公式 Krita、Krita のフォーク）では、画面の更新に合わせる仕組みは
「交換の完了か、プラットフォームの更新要求を契機にする」形にそろっている。時間だけを刻む
タイマーで描く方法は、Qt の文書も `QScreen::refreshRate` の用途として警告している。この実装は
その形に合わせ、タイマーは交換の信号が来ないときの退避だけに使う。

従来に戻す隠し設定は kritarc の `useCanvasFrameAlignedUpdates`（既定 `true`）である。
`false` にすると、正式版 Qt では以前のアプリ側の圧縮（輪郭は直接の更新）に戻る。隠し設定は
起動時に読み、再起動で反映する。

## Linux で同じ仕組みが起きているか

2026-11-09 の Q1（正式版 Qt 6.11.3、X11）の `probe.csv` を数え直した。

| 回 | `paintgl_begin` | 計測の長さ | 1秒あたり | 描画の間隔の中央値 |
| --- | --- | --- | --- | --- |
| `Q1-20261109-141713-1` | 4,037 | 34.5秒 | 116.9回 | 5.28ms |
| `Q1-20261109-141901-1` | 4,106 | 36.3秒 | 113.0回 | 5.23ms |
| `Q1-20261109-142424-1` | 4,055 | 34.9秒 | 116.3回 | 5.33ms |

表示は 60Hz なので、表示 1 回あたり約 1.9〜2.0 回の描画になる。Windows の正式版 Qt と同じ
仕組みが Linux でも起きている。2026-11-08 の Wayland の記録（`DevFix-20261108-132523-2`）も
35.5 秒に 3,144 回（88.6回/秒）で、60Hz を超えていた。

したがって、この変更の効果は Linux の X11 でも同じ手順で確かめられる。差分構築の実行ファイルを
`run-linux.sh` に渡し、`probe.csv` の `paintgl_begin` と `frame_swapped` の回数、間隔の中央値、
取り込みのホバーとインクを、隠し設定 `useCanvasFrameAlignedUpdates=false` の回と比べる。
Windows は `build-incremental windows-upstream-qt build krita` で作った DLL を
`build/windows/upstream-qt/` の ZIP に差し替え、正式版 Qt の新しい版と対照（フォーク版の
desktop）を同じ回で交互に測る。判定は、表示 1 回あたりの `paintgl_begin` が約 1.0 に下がり、
取り込みのインクが悪化しないこと（改善すればなお良い）とする。
