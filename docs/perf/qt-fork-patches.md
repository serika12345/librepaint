# Krita の Qt フォークの改変一覧（for-krita/6.11.3）

Issue #108 フェーズQ2 と Issue #68 の作業資料。Krita の Qt フォーク（`dimula73/qtbase` の `for-krita/6.11.3`、先頭 d918f368da）が Qt 正式版 6.11.3 に加えている 92 件の改変を、目的ごとにまとめ、LibrePaint の依存と判定の方法を記録する。判定が決まったら、この表の「判定」と DECISIONS.md を更新する。

「正式版」の列は、改変の件名で qt/qtbase の履歴を検索して見つかったコミットを示す（2026-11-08 時点）。件名が変わって取り込まれたものは見落としている可能性がある。

## 判定の区分

| 区分 | 意味 |
| --- | --- |
| 不要 | LibrePaint がその経路を使わない。または、その改変なしで LibrePaint の動作が変わらない |
| 正式版で解決済み | Qt 正式版（6.11.3 以降）に同等の修正が入っている |
| LibrePaint 側で代替 | 正式版にはないが、LibrePaint のコードで同じ結果にできる。代替する箇所を書く |
| 必要 | 正式版の Qt では LibrePaint の動作が壊れる。Q4 では、修正を公式に入れるか LibrePaint 側で作り直すまで外せない |
| 保留 | 数値か機能の確認で決める。決めるための実験を書く |

根拠は、改変の差分（`gh api repos/dimula73/qtbase/commits/<sha>`）、LibrePaint の依存箇所（`git grep` の結果）、正式版 Qt 6.11.3 での扱いの3つで示す。判断は CHARTER の「一般的な水準を超えて作り込まない」に従い、公式 Krita が全プラットフォームでその改変を使っていることと、LibrePaint が既に正式版の Qt で動いている環境（Linux、macOS）を基準にする。

| まとまり | 件数 | 判定 |
| --- | --- | --- |
| Windows の ANGLE と OpenGL ES の経路 | 18 | 保留（16件）と不要（2件）。L1 の H7（ANGLE を使わない OpenGL）の結果で決める。ANGLE を残すなら必要、なくすなら不要 |
| 画面更新の間引き（フレームレート圧縮） | 10 | 保留：L1 の H6 で Windows の遅れへの寄与を測り、`KisCanvas2` 側の間引きと1系統にまとめるかを決める |
| HDR と色変換（バックバッファー、QRhi） | 4 | 保留：Windows の HDR 表示を維持するかと合わせて決める。LibrePaint は `setTextureColorSpace` で依存する |
| Windows のペン入力（WinTab、Windows Ink、タッチ） | 14 | 必要（9件）、保留（2件）、不要（3件） |
| Wayland のペン入力とウィンドウ | 10 | 正式版で解決済み（3件）、保留（4件）、不要（3件） |
| キーボードとショートカット（配列、不均衡な押下と解放） | 12 | 必要（2件）、LibrePaint 側で代替（3件）、保留（5件）、不要（2件） |
| 不具合とクラッシュの修正（ウィジェット、GUI、コア） | 13 | 正式版で解決済み（2件）、不要（11件） |
| その他（構築、UI、プラットフォーム固有の調整） | 11 | LibrePaint 側で代替（1件）、正式版で解決済み（1件）、不要（9件） |

## Windows の ANGLE と OpenGL ES の経路

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| 927cee712e | Fix difinition in QEGLPlatformContext | src/gui/opengl/platform | なし | 保留 |
| 731ab8bba8 | Fix openGL 2.x support in QOpenGL2PaintEngine | src/opengl/qopenglpaintengine.cpp, src/opengl/qopenglpaintengine_p.h | なし | 保留 |
| 8ec599f593 | [eglfs] Don't enable QOpenGLCompositor on platforms it is not used | src/opengl/CMakeLists.txt | なし | 不要 |
| 50d61a44d5 | [xcb] Fix QPlatformIntegration::openGLModuleType() for XCB platform | src/gui/kernel/qplatformintegration.cpp, src/plugins/platforms/xcb | なし | 不要 |
| f5a65b7761 | [win] Recover ANGLE support on Windows | cmake/3rdparty/extra-cmake-modules/find-modules, cmake/FindGLESv2.cmake, src/gui/CMakeLists.txt ほか | なし | 保留 |
| 7ad4a8bc17 | QSurfaceFormat: implement color space selection for OpenGL ES | src/gui/kernel/qsurfaceformat.cpp, src/gui/kernel/qsurfaceformat.h, src/gui/opengl/platform ほか | なし | 保留 |
| 95bb9c74f1 | Deprecate QT_FEATURE_opengl_dynamic | src/gui/CMakeLists.txt, src/gui/configure.cmake, src/gui/kernel/qopenglcontext.cpp ほか | なし | 保留 |
| 747eca7b69 | Let hellogles3 to manually switch API on Windows | examples/opengl/hellogles3/CMakeLists.txt, examples/opengl/hellogles3/app.manifest, examples/opengl/hellogles3/glwindow.cpp ほか | なし | 不要 |
| 72a68d87fd | Fix building on platforms without EGL | src/gui/CMakeLists.txt | なし | 不要 |
| 02216d26ba | [win+angle] Fix flickering when resizing the Window | src/plugins/platforms/windows | なし | 保留 |
| 1efb3f6434 | [win] Implement callback for ANGLE debugging information | src/plugins/platforms/windows | なし | 不要 |
| 261c23275e | [win] Implement support for ANGLE's openGL backend | src/gui/kernel/qplatformopenglcontext.h, src/gui/rhi/qrhigles2.cpp, src/gui/rhi/qrhigles2_p.h ほか | なし | 保留 |
| 14ebe92549 | Remove dynamicgl from mkspecs | mkspecs/features/win32/opengl.prf | なし | 保留 |
| aced4e6119 | Fix crash on Linux when using wayland plugin | src/gui/opengl/platform | なし | 不要 |
| 7f50d93a7f | Fix a crash when opening multiple windows in Compatibility openGL profile | src/opengl/qopenglpaintengine.cpp, src/opengl/qopenglpaintengine_p.h | なし | 不要 |
| 1a543f2832 | Add workaround for flickering on resizing on ANGLE and Qt6 | src/plugins/platforms/windows | なし | 保留 |
| 77d2b02daf | Fix flickering when opening dialogs on ANGLE and Qt6 | src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h, src/gui/painting/qplatformbackingstore.cpp ほか | なし | 保留 |
| 70086804f9 | Fix EGL format being reset to 0 alpha instead of 2 | src/gui/opengl/platform | なし | 保留 |

**根拠**

- まとまりの判定：LibrePaint の Windows 版は既定で ANGLE（`krita/main.cc:656` の `OpenGLRenderer` の既定値 `angle`）を使い、ANGLE 専用の環境変数（`QT_USE_PREMATURE_RESIZE_EVENTS`、`QT_ANGLE_MANUALLY_UPDATE_SURFACE_SIZE`、`QT_PREFILL_RHI_SURFACE`、`krita/main.cc:677-697`）を設定する。f5a65b7761 などの ANGLE 復活の一群は、ANGLE を残すなら必要、やめるなら不要になる。判定は L1 の H7（ANGLE を使わない OpenGL で Windows の遅れが変わるか）で決める。
- 不要の根拠：
  - 8ec599f593 は eglfs 専用の変更で、LibrePaint は eglfs を使わない（Linux は xcb と Wayland）。
  - 50d61a44d5 は xcb の `openGLModuleType()` の報告値の変更で、正式版の Qt で Linux 版が動いている（DECISIONS.md 2026-11-08 の色プロファイルの修正で 20/16ms）。
  - 747eca7b69 は example の変更で、製品に含まれない。
  - 72a68d87fd は EGL のない環境の構築の修正で、LibrePaint の Linux と macOS の構築は正式版で通っている。
  - 1efb3f6434 は ANGLE のデバッグ情報で、LibrePaint は使わない。
  - aced4e6119 は Linux の Wayland でのクラッシュの修正で、正式版で Wayland の LibrePaint が動いている（同上）。
  - 7f50d93a7f は複数のトップレベル窓を互換プロファイルで開いたときのクラッシュで、LibrePaint は `KisMainWindow` の MDI（トップレベル窓は1つ）で使う。互換プロファイル自体は `libs/ui/opengl/kis_opengl.cpp:604` で要求するが、窓は1つ。
- 保留の実験：L1 の H7 で、Windows の LibrePaint を ANGLE なし（デスクトップ OpenGL）で起動し、合成ペン入力のホバー輪郭とインクの中央値・P95 を 1.0.3 と比べる。ANGLE を残す場合はこの一群を「必要」、やめる場合は「不要」にする。7ad4a8bc17 と 70086804f9 は HDR の判定にも従う。

## 画面更新の間引き（フレームレート圧縮）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| e121cb08e5 | Pass proper dirty rect in paint event to QOpenGLWidget with partial updates | src/openglwidgets/qopenglwidget.cpp, src/widgets/kernel/qwidget.cpp, src/widgets/kernel/qwidget_p.h ほか | なし | 保留 |
| a9b60ead54 | Implement framerate compression in QWidgetRepaintManager | src/corelib/CMakeLists.txt, src/corelib/kernel/qsignalcompressor.cpp, src/corelib/kernel/qsignalcompressor_p.h ほか | なし | 保留 |
| 7ec0f9c727 | Replace QSignalCompressor with a specialized QFrameRateCompressor | src/corelib/CMakeLists.txt, src/corelib/kernel/qframeratecompressor.cpp, src/corelib/kernel/qframeratecompressor_p.h ほか | なし | 保留 |
| fb923e4a97 | Add fast-transfer option for QImage when rendering QWidget | src/gui/painting/qbackingstoredefaultcompositor.cpp | なし | 保留 |
| 6d39b2c286 | Compress frames in a more parallel way | src/gui/rhi/qrhi.cpp, src/gui/rhi/qrhi.h, src/gui/rhi/qrhi_p.h ほか | なし | 保留 |
| dc3246d166 | Fix possible recursion issue in QFrameRateCompressor | src/corelib/kernel/qframeratecompressor.cpp, src/corelib/kernel/qframeratecompressor_p.h | なし | 保留 |
| 6e540ae34f | Implement ability to switch FPS compression mode | src/widgets/kernel/qwidgetrepaintmanager.cpp | なし | 保留 |
| c7d92e97b8 | Implement optional FPS stats printing | src/corelib/CMakeLists.txt, src/corelib/kernel/qmeasureavgportion_p.h, src/gui/rhi/qrhigles2.cpp | なし | 保留 |
| 71fd8157aa | Add QT_FRAME_RATE_OVERRIDE switch | src/widgets/kernel/qwidgetrepaintmanager.cpp | なし | 保留 |
| f1cf04e958 | Fix clipping when cutting QOpenGLWidget through the GUI texture | src/widgets/kernel/qwidget.cpp | なし | 保留 |

**根拠**

- LibrePaint は、この一群があるときは `krita/main.cc:715-726` で `QT_BACKING_STORE_USE_FAST_QIMAGE_TRANSFER` と `QT_FRAME_RATE_OVERRIDE` を設定し、間引きを Qt（`QFrameRateCompressor`）に任せる。ないときは `KisCanvas2` 側の自前の間引き（`libs/ui/canvas/kis_canvas2.cpp:245`、`:355`、`:407`、`:1181`、`:1192`）が働く。この分岐が「描画の更新の仕組みが2系統ある」の実体で、Q4 の完了条件「1系統にする」はここを指す。
- L1 の H6 の実験で、Windows の現行版（フォーク、間引きあり）を `QT_FRAME_COMPRESSION_MODE=2`（間引きなし）でも測り、遅れへの寄与を求める。同時に、正式版の Qt で `KisCanvas2` 側の間引きが働いた場合の値と比べる。差が 2ms 未満なら「不要」にし、`KisCanvas2` の1系統にまとめる。
- e121cb08e5（部分更新の dirty rect）と f1cf04e958（GUI テクスチャの切り抜き）は、間引きとは別に、部分再描画の範囲とクリップの正しさを変える。H6 の計測で部分再描画を残す場合は、この2件だけを「必要」として残す候補にする。

## HDR と色変換（バックバッファー、QRhi）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| 752559eb9b | Implement color conversion for the backing store texture | src/gui/CMakeLists.txt, src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h ほか | なし | 保留 |
| c55693d623 | Implement a manual test for checking is HDR features work | examples/opengl/CMakeLists.txt, examples/opengl/hdr-qopenglwidget/CMakeLists.txt, examples/opengl/hdr-qopenglwidget/KisGLImageF16.cpp ほか | なし | 保留 |
| a1fa6f1629 | Create color conversion pipelines on-demand | src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h | なし | 保留 |
| 5da2f94813 | Implement feature QRhi::DynamicBlendSwitch | src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h, src/gui/rhi/qrhi.h ほか | なし | 保留 |

**根拠**

- LibrePaint の HDR 表示は、この一群の上に成り立つ。`CMakeLists.txt:784-793` は `QOpenGLWidget::setTextureColorSpace` の有無を `HAVE_HDR` として調べ、`libs/ui/opengl/kis_opengl.cpp:813`（HDR 形式の判定）、`:920`（16bit のサーフェスの形式の一覧）、`libs/ui/opengl/KisScreenInformationAdapter.cpp:152`（画面の HDR の検出）、`plugins/dockers/lut/lutdocker_dock.cpp:244`、`plugins/dockers/smallcolorselector/KisGLImageWidget.cpp:65-66` がそれに依存する。`QOpenGLWidget::setTextureColorSpace` は正式版の Qt にない。
- したがって、この一群を外すと Windows の HDR 表示（10bit のキャンバスと色変換）がなくなる。CHARTER の「UI と操作の見かけは旧版から変えない」に照らすと、HDR の画面での見かけが変わるため、判定は Windows の HDR 表示を残すかどうかで決まる。
- 決めるための確認：基準機 Surface Pro 7 の内蔵画面は 8bit で、HDR の確認は外部の HDR 画面が要る。確認は「Windows の HDR を有効にした画面で、旧版と Q1 版のキャンバスと色選択器の見かけを並べる」で行う。HDR を残す場合は、Q4 で正式版の Qt に対する同等の色変換（バックバッファーの色空間の変換）を LibrePaint 側に作るか、公式の修正を待つ。HDR をやめる場合はこの一群を「不要」とし、`HAVE_HDR` の分岐を外す。

## Windows のペン入力（WinTab、Windows Ink、タッチ）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| c41d66281e | [win] Generate Enter/Leave events on touching non-client area of the window | src/plugins/platforms/windows | なし | 必要 |
| df3a678309 | [win] Remove subscription on WM_MOUSELEAVE when a tablet enters the window | src/plugins/platforms/windows | なし | 必要 |
| 7bb46828ae | Disable tablet relative mode in Qt | src/plugins/platforms/windows | なし | 保留 |
| 051139ed72 | Fetch mapped screen size from the Wintab driver | src/plugins/platforms/windows | なし | 必要 |
| 2357bbfe90 | Add a workaround for button mapping on Lenovo Yoga C940 | src/plugins/platforms/windows | なし | 不要 |
| 33a49f25d1 | Add workaround for handling table press correctly in WinInk mode | src/gui/kernel/qguiapplication.cpp, src/gui/kernel/qwindowsysteminterface.cpp, src/plugins/platforms/windows | なし | 必要 |
| 7ca0b0e289 | Revert "Query the mouse buttons initially in case the middle button was used" | src/plugins/platforms/windows | なし | 不要 |
| f5a75c4071 | WinInk: Synthesize mouse events for full pen stroke | src/plugins/platforms/windows | なし | 必要 |
| 41adadd8db | Synthesize Enter/LeaveEvent for accepted QTabletEvent | src/widgets/kernel/qwidgetwindow.cpp | なし | 必要 |
| 0027358f20 | Poison Qt's headers with a mark about presence of Enter/Leave patch | src/gui/kernel/qevent.h | なし | 必要 |
| 8f8eabfb72 | Fix duplicated syntetic mouse events on WinTab | src/plugins/platforms/windows | なし | 必要 |
| c132390390 | Revert "Send LeaveEvent as reaction to WM_POINTERLEAVE to reset hover states" | src/plugins/platforms/windows | なし | 必要 |
| a60ed01a25 | [win] Allow explicit accepting of touch events on Windows | src/gui/kernel/qguiapplication.cpp, src/plugins/platforms/windows | なし | 保留 |
| d918f368da | Windows: implement touchpad gestures using Direct Manipulation | src/gui/configure.cmake, src/gui/kernel/qevent.cpp, src/plugins/platforms/direct2d ほか | なし | 不要 |

**根拠**

- 必要の根拠（LibrePaint が直接使う、または外すとペンの操作が壊れる）：
  - 051139ed72 は `QT_WINTAB_DESKTOP_RECT` と `QT_IGNORE_WINTAB_MAPPING` を追加する。LibrePaint は `libs/ui/dialogs/KisDlgCustomTabletResolution.cpp:149-161` で両方を設定し、タブレットの対応範囲の設定（Wintab の範囲、仮想画面、任意の矩形）を UI として持つ。この設定は正式版の Qt では効かなくなる。基準機 Surface Pro 7 の内蔵タブレットは主画面にしか対応しないため、この改変がないと座標がずれる。
  - c41d66281e は、タブレットが窓の非クライアント領域に入ったときの Enter/Leave を作る。ないと、枠を触ったあとカーソルがリサイズの形のまま残る。
  - df3a678309 は、タブレットが窓に入ったときに `WM_MOUSELEAVE` の購読を外す。ないと、ストロークの途中に余計な leave が届いてホバーの状態が途切れる。
  - 33a49f25d1 は、Windows Ink の経路でウィジェットが押下を離すまで合成マウスを受け取れない問題（長押しが右クリックになる）を直す。メニューやボタンをペンで操作する経路に効く。
  - f5a75c4071 は、Windows Ink の押下を受理したときに、ストローク全体（update と up）の合成マウスを作る。
  - 41adadd8db は、`QTabletEvent` を受理したときに Enter/Leave を合成する。ないと、キャンバス以外のウィジェットでホバーの状態が変わらない。0027358f20 は、その修正があることを示す印で、`CMakeLists.txt:687` の判定に使う。
  - 8f8eabfb72 は、WinTab で合成マウスが二重に届くのを止める。二重の押下は、クリックやドラッグの誤動作になる。
  - c132390390 は、`WM_POINTERLEAVE` で Leave だけを送っていた変更を戻し、Enter/Leave の対応を保つ。
- 不要の根拠：
  - 2357bbfe90 は Lenovo Yoga C940 のドライバ固有の回避で、基準機と対象の機種に当てはまらない。
  - 7ca0b0e289 は、WinInk が左ボタンしか報告しないのに他のボタンを初期化していた処理の戻しで、動作を変えない。
  - d918f368da はタッチパッドのジェスチャを滑らかにするだけで、外すと 1.0.3 と同じ粗いスクロールに戻る（機能は失わない）。6.11.3 のフォークの先頭にある新しい改変で、性能の指標にも関係しない。
- 保留の実験：
  - 7bb46828ae（タブレットの相対モードを切る）は、Surface で合成ペン入力の座標の飛びと、`scripts/perf/pen-latency/` の PenProbe の報告レート・取りこぼしを、相対モードのある場合とない場合で比べる。
  - a60ed01a25（タッチの受理）は、Surface のタッチでパンとズーム、タッチでの描画の設定を確かめる。タッチの操作がペンの操作を妨げないことを確かめてから決める。

## Wayland のペン入力とウィンドウ

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| ed56d12b50 | Fix unexpected window transparency after enabling CSD on Wayland | src/plugins/platforms/wayland | f1fbd15105（2026-02-27） | 正式版で解決済み |
| d2aa04db1a | Fix cursor update when tablet moves between native surfaces | src/plugins/platforms/wayland | なし | 保留 |
| b069509b06 | Fix cursor update on tablet transition between alien widgets | src/plugins/platforms/wayland | なし | 保留 |
| 483ceed535 | [wayland] Handle the case when a tablet has multiple tools | src/plugins/platforms/wayland | なし | 保留 |
| 3b6b57d0b7 | [wayland] Emit native Enter/Leave events on tablet proximity events | src/plugins/platforms/wayland | なし | 保留 |
| bc9fec0a20 | [wayland] Implement raise() and lower() methods for subsurfaces | src/plugins/platforms/wayland, tests/auto/widgets/kernel | なし | 不要 |
| 5020e751e6 | [wayland] Make positioners reactive by default | src/plugins/platforms/wayland | なし | 不要 |
| 435e604932 | wayland: keep a popup in place when it is resized | src/plugins/platforms/wayland, tests/auto/wayland/shared, tests/auto/wayland/xdgshell | c3cc50d89d（2026-09-09） | 正式版で解決済み |
| eb6579e6d1 | [wayland] Fix flickering when resizing popup windows | src/plugins/platforms/wayland | なし | 不要 |
| 693de2c4fb | Wayland client: update cursor position after sending QTabletEvent | src/plugins/platforms/wayland | 4332e5e2c4（2026-07-23） | 正式版で解決済み |

**根拠**

- 正式版で解決済みの3件は、qt/qtbase に同等のコミットがある（ed56d12b50 → f1fbd15105、435e604932 → c3cc50d89d、693de2c4fb → 4332e5e2c4）。6.11.3 以降の正式版を使えば同じ結果になる。
- 不要の根拠：
  - bc9fec0a20 はネイティブの subsurface の raise/lower で、LibrePaint は窓の重なり順をこの経路で操作しない。
  - 5020e751e6 はポップアップの位置が表示の順に依存する不具合の修正で、正式版の Qt の Linux 版でダイアログとポップアップの位置のずれは出ていない（Wayland の計測は 20/16ms で、色プロファイルの修正後は公式 Krita と同程度。DECISIONS.md 2026-11-08）。
  - eb6579e6d1 はポップアップのリサイズ時のちらつきで、表示だけの問題。主指標（キャンバスの輪郭とインク）に効かず、正式版での計測でも出ていない。
- 保留の4件（d2aa04db1a、b069509b06、483ceed535、3b6b57d0b7）は、いずれも Wayland のタブレット（ペン）の経路の修正で、カーソルの更新、複数のツールの同時の近接、タブレットの Enter/Leave に関わる。LibrePaint の Linux 版は正式版の Qt で動いているが、これまでの計測は合成マウス入力で、ペンの経路は確かめていない（Linux 開発機にタブレットがない）。
- 決めるための実験：Wayland で実際のペン（または uinput で作ったタブレットの入力）を使い、(1) 近接したときのカーソルの形、(2) 2つのツールを近づけたときの切り替え、(3) キャンバスとドックの間でペンを移動したときのホバーの状態、の3つを確かめる。不具合が出なければ「不要」にする。

## キーボードとショートカット（配列、不均衡な押下と解放）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| 82d2864a8a | Don't eat ShortcutOverride events when there is a partial-match | src/gui/kernel/qshortcutmap_p.h, src/gui/kernel/qwindowsysteminterface.cpp | なし | 必要 |
| dc27f27d8e | Fix unbalanced KeyPress/Release events in children of QMdiArea | src/widgets/widgets/qmdiarea.cpp | なし | 必要 |
| 18a07bf888 | Fix shortcuts assigned to special keys on non-latin layouts | src/plugins/platforms/windows | なし | 保留 |
| 3fab97e30b | Fix multiple shortcuts match from different keyboard layouts | src/gui/kernel/qshortcutmap.cpp | なし | 保留 |
| adf861033b | Fix fallback latin shortcuts not to be the top priority on Windows | src/plugins/platforms/windows | なし | 保留 |
| 747423036c | [win] Fix unbalanced KeyPress/Release events when using | src/plugins/platforms/windows | なし | LibrePaint 側で代替 |
| f8a61ebc5b | [win] Fin unbalanced shortcuts on non-latin layouts again | src/plugins/platforms/windows | なし | LibrePaint 側で代替 |
| 9bfd8a120f | Make sure AltGr-based shortcuts are disallowed when textbox is in focus | src/widgets/widgets/qwidgetlinecontrol.cpp | なし | 保留 |
| c8049a518f | Fix composing of dead keys with short form of system locale | src/plugins/platforminputcontexts/compose | なし | 不要 |
| 07a57fa505 | Fix shortcuts with special keys on non-US layouts on Linux | src/gui/platform/unix | なし | 保留 |
| 21d400db4f | Fix unbalanced KeyPress/Release on non-latin keyboard on Linux | src/plugins/platforms/xcb | なし | LibrePaint 側で代替 |
| 35296972ce | Poison Qt's headers with a mark about presence of KeyPress/Release patches | src/gui/kernel/qevent.h | なし | 不要 |

**根拠**

- 必要の根拠：
  - 82d2864a8a は、Qt が部分一致のショートカットで ShortcutOverride を食べないようにする。件名に Krita とあり、LibrePaint は独自のショートカット処理 `KisShortcutMatcher`（`libs/input/kis_shortcut_matcher.h:73`）を持つため、この改変がないと独自処理に届かないショートカットが出る。
  - dc27f27d8e は、QMdiArea の子で Ctrl+Tab を押したときの KeyPress/KeyRelease の不均衡を直す。LibrePaint は文書の切り替えに QMdiArea を使う（`libs/application/ui/workspace/KisMainWindow.cpp:245-251`）。
- LibrePaint 側で代替の根拠：747423036c、f8a61ebc5b、21d400db4f（非ラテン配列での KeyPress/KeyRelease の不均衡）は、`libs/input/ui/kis_input_manager_p.cpp:242` の回避コード（`useUnbalancedKeyPressEventWorkaround`）と同じ問題を扱う。この回避コードは macOS で常に、Linux では印がないとき（`!KRITA_QT_HAS_UNBALANCED_KEY_PRESS_RELEASE_PATCH`）に有効になり、`KRITA_FIX_UNBALANCED_KEY_EVENTS` で切り替えられる。Q4 では Windows でも印がないときに有効にする（3行程度の変更）。
- 不要の根拠：
  - c8049a518f は、システムのロケールが `en_US` の短い形のときに compose の表が見つからない問題で、LibrePaint は端末のロケール（`ja_JP.UTF-8` など）をそのまま使う。
  - 35296972ce は印だけの改変で、動作を変えない。印がないと上の回避コードが有効になる。
- 保留（非ラテン配列のショートカットの実際の一致）：18a07bf888、3fab97e30b、adf861033b、07a57fa505、9bfd8a120f は、非ラテン配列で特殊キー（`\`、`/`、`;`、`` ` ``、`'`）や AltGr を使ったときの一致の優先順位を変える。LibrePaint の利用者の配列（日本語）で実際に不具合が出るかを確かめる。確かめ方は、日本語配列で (1) テキストツールの入力中に AltGr 相当の文字を打ち、(2) 特殊キーに割り当てたショートカットを押し、(3) 同じキーに複数の配列で割り当てがある状態でショートカットを押す、の3つを旧版と比べる。不具合が出なければ「不要」にする。

## 不具合とクラッシュの修正（ウィジェット、GUI、コア）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| 29d93a5d62 | Remove chunk size limits | src/gui/image/qpnghandler.cpp | なし | 未判定 |
| cf05dd04c8 | Check whether there is a QGestureRecognizer at all | src/widgets/kernel/qgesturerecognizer.cpp | なし | 未判定 |
| d2507d05fa | Check if we reached the end of Iterator | src/widgets/kernel/qgesturemanager.cpp | なし | 未判定 |
| 93bd31ff0f | Use DebugBreak instead of fast-fail exception to abort | src/corelib/global/qassert.cpp | なし | 未判定 |
| f1302dbbc8 | Do not crash if the foundry name is an empty space | src/gui/text/qfontdatabase.cpp | なし | 未判定 |
| 8dadf099b8 | Check for uninitialized QDockAreaLayoutInfo | src/widgets/widgets/qdockarealayout.cpp | なし | 未判定 |
| 2ea7817457 | Return 0 for Window if there is no widget | src/widgets/accessible/qaccessiblewidget.cpp | なし | 未判定 |
| bf2ee30931 | QMenu: make less sensitive to mouse jitter | src/widgets/widgets/qmenu.cpp | 4292ad0292（2020-08-19） | 未判定 |
| df46429bb3 | Fix slowdown caused by making window system events synchronous. | src/gui/kernel/qwindowsysteminterface.cpp, src/gui/kernel/qwindowsysteminterface.h | なし | 未判定 |
| 7c155ae659 | Implement better parsing of EDID model number | src/gui/util/qedidparser.cpp | なし | 未判定 |
| 83e4fd6099 | Sync buffers of the destination file after QFile::copy() | src/corelib/io/qfile.cpp | なし | 未判定 |
| d850335b2d | [win] Fix pasting copy-pasting QImages to Chrome | src/plugins/platforms/windows | なし | 未判定 |
| 24ea6b40c2 | Fix QHeaderView failing to init memory mode | src/widgets/itemviews/qheaderview.cpp, tests/auto/widgets/itemviews | e3d05b35b3（2026-10-27） | 未判定 |

## その他（構築、UI、プラットフォーム固有の調整）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| a7c925fb66 | [windeployqt] Fix windeployqt to find ICU built with Meson | src/tools/windeployqt/main.cpp | なし | 未判定 |
| 55a0448923 | Use fast path for unsupported mime types | src/plugins/platforms/windows | なし | 未判定 |
| d002731cb6 | [moc] Remove double checks for file-exists in searchIncludePaths() | src/tools/moc/preprocessor.cpp | なし | 未判定 |
| f786da6451 | Add QMdiArea::AlwaysShowSubwindowNameInTitleBar | src/widgets/widgets/qmdiarea.h, src/widgets/widgets/qmdisubwindow.cpp, src/widgets/widgets/qmdisubwindow_p.h | なし | 未判定 |
| d343ee345f | Warn about downgraded precise timers | src/corelib/kernel/qeventdispatcher_win.cpp | cf8cd71476（2026-10-27） | 未判定 |
| 54de8a5b6d | Add icon caching in QFusionStyle | src/widgets/styles/qfusionstyle.cpp | なし | 未判定 |
| 7db708f3ff | Assume widgets are rendered in pass-through mode on MacOS | src/gui/painting/qbackingstoredefaultcompositor.cpp | なし | 未判定 |
| 81a5ac7eb5 | Allow widgets to block kinetic scrolling | src/widgets/util/qflickgesture.cpp, src/widgets/util/qscroller.cpp, src/widgets/util/qscroller.h | なし | 未判定 |
| 5874f1f131 | Implement QT_GLX_SWAP_INTERVAL_SYNC_WORKAROUND | src/plugins/platforms/xcb | なし | 未判定 |
| b2cc4909b0 | [macos] Add bundle-local path to GenericDataLocation | src/corelib/io/qstandardpaths_mac.mm | なし | 未判定 |
| 9dc7c77358 | Add a switch to disable accessibility on Windows | src/plugins/platforms/windows | なし | 未判定 |
