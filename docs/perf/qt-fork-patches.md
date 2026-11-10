# Krita の Qt フォークの改変一覧（for-krita/6.11.3）

Issue #108 フェーズQ2 と Issue #68 の作業資料。Krita の Qt フォーク（`dimula73/qtbase` の `for-krita/6.11.3`、先頭 d918f368da）が Qt 正式版 6.11.3 に加えている 92 件の改変を、目的ごとにまとめ、LibrePaint の依存と判定を記録する。「判定」は 2026-11-08 に埋めた。保留の一部は 2026-11-10 の L1 の計測結果で見直した（「L1 の結果による保留の見直し（2026-11-10）」）。判定の要約は DECISIONS.md の同じ日の行にあり、保留の決定は L1（計測）と Q4（フォークの撤去）で行う。

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

## 正式版 Qt で構築するときのフォーク固有 API

2026-11-10 に、正式版 Qt 6.11.3 の Windows 版を構築するため、LibrePaint のソースが
フォークの API に頼る箇所を一覧にした。正式版の側を選ぶ分岐は、フォーク版の動作を
変えない。

| 箇所 | フォークの API | 正式版での扱い |
| --- | --- | --- |
| `plugins/dockers/smallcolorselector/KisGLImageWidget.cpp` | `QOpenGLWidget::setTextureColorSpace` | CMake の `HAVE_HDR` の検出が偽になる。Windows の正式版の構築では `-DHAVE_HDR=ON` を渡さない（`nix/windows/krita.nix` はフォーク版だけに渡す） |
| `libs/ui/opengl/kis_opengl_canvas2.cpp`（更新の間引き、H2（表示待ちの歯止め）、H6（フォークの更新の間引きの歯止め）） | `QRhi::isLastFrameCompletedOnGPU` と `QOpenGLWidget` の更新の圧縮 | CMake の `KRITA_QT_HAS_UPDATE_COMPRESSION_PATCH` が偽になり、LibrePaint 自身の `KisCanvas2` の圧縮を使う |
| `libs/ui/canvas/kis_canvas2.cpp`、`libs/input/ui/*` | `QT_HAS_ENTER_LEAVE_PATCH`、`QT_HAS_UNBALANCED_KEY_PRESS_RELEASE_PATCH` | CMake の `check_cxx_symbol_exists` が偽になり、LibrePaint 側の回避策を使う |
| `libs/ui/opengl/kis_opengl_canvas2.cpp`（H1（先積みのフレーム数）） | ANGLE の EGL と `IDXGIDevice1` | `QT_CONFIG(egl)` が偽なのでコンパイルされない。正式版には ANGLE の経路がない |
| `krita/main.cc` | `OpenGLRenderer` の既定 `angle` | 正式版では OpenGL ES の構成が支持されず、desktop の OpenGL に落ちる。計測では `OpenGLRenderer=desktop` を明示できる |

正式版 Qt で構築する出力は `librepaint-windows-upstream-qt-archive` で、依存だけを
構築する `windows-upstream-qt-dependencies` と分けてある（`docs/architecture/DEVELOPMENT.md`）。

| まとまり | 件数 | 判定 |
| --- | --- | --- |
| Windows の ANGLE と OpenGL ES の経路 | 18 | 保留（11件）、不要（7件）。H7 前段で desktop はホバー 35.4→13.6ms、インク 39.1→17.9ms（16:37 の 10 回）。ANGLE を残すかと、表示の正しさ・HDR・Wintab の確認で決める |
| 画面更新の間引き（フレームレート圧縮） | 10 | 保留（3件）、不要（7件）。H6 で H1（先積みのフレーム数）=1 の下の差が 0.3ms 未満（13:41 の 15 回）なので、間引きは `KisCanvas2` の1系統にまとめる。部分再描画・fast transfer・クリップの3件は保留 |
| HDR と色変換（バックバッファー、QRhi） | 4 | 保留：H7 前段で desktop は起動したが、HDR は内蔵画面が SDR で未確認。Windows の HDR 表示を維持するかと合わせて決める。LibrePaint は `setTextureColorSpace` で依存する |
| Windows のペン入力（WinTab、Windows Ink、タッチ） | 14 | 必要（9件）、保留（2件）、不要（3件）。L1 の合成入力は全回 WM_POINTER（`native_wintab`=0） |
| Wayland のペン入力とウィンドウ | 10 | 正式版で解決済み（3件）、保留（4件）、不要（3件）。L1 は Windows だけで、Wayland のペンは未計測 |
| キーボードとショートカット（配列、不均衡な押下と解放） | 12 | 必要（2件）、LibrePaint 側で代替（3件）、保留（5件）、不要（2件）。L1 は合成ペンだけで、配列は未計測 |
| 不具合とクラッシュの修正（ウィジェット、GUI、コア） | 13 | 正式版で解決済み（2件）、保留（2件）、不要（9件） |
| その他（構築、UI、プラットフォーム固有の調整） | 11 | LibrePaint 側で代替（1件）、正式版で解決済み（1件）、不要（9件） |

## Windows の ANGLE と OpenGL ES の経路

| コミット | 件名 | 変更範囲 | 正式版 | 判定 | L1 の根拠（2026-11-10） |
| --- | --- | --- | --- | --- | --- |
| 927cee712e | Fix difinition in QEGLPlatformContext | src/gui/opengl/platform | なし | 保留 | L1 H7 前段（16:37 の 10 回）: desktop は起動し、ホバー 35.4→13.6ms、インク 39.1→17.9ms。表示の正しさ・HDR・Wintab は未確認 |
| 731ab8bba8 | Fix openGL 2.x support in QOpenGL2PaintEngine | src/opengl/qopenglpaintengine.cpp, src/opengl/qopenglpaintengine_p.h | なし | 保留 | L1 H7 前段（16:37 の 10 回）: desktop は起動し、ホバー 35.4→13.6ms、インク 39.1→17.9ms。表示の正しさ・HDR・Wintab は未確認 |
| 8ec599f593 | [eglfs] Don't enable QOpenGLCompositor on platforms it is not used | src/opengl/CMakeLists.txt | なし | 不要 | — |
| 50d61a44d5 | [xcb] Fix QPlatformIntegration::openGLModuleType() for XCB platform | src/gui/kernel/qplatformintegration.cpp, src/plugins/platforms/xcb | なし | 不要 | — |
| f5a65b7761 | [win] Recover ANGLE support on Windows | cmake/3rdparty/extra-cmake-modules/find-modules, cmake/FindGLESv2.cmake, src/gui/CMakeLists.txt ほか | なし | 保留 | L1 H7 前段（16:37 の 10 回）: desktop は起動し、ホバー 35.4→13.6ms、インク 39.1→17.9ms。表示の正しさ・HDR・Wintab は未確認 |
| 7ad4a8bc17 | QSurfaceFormat: implement color space selection for OpenGL ES | src/gui/kernel/qsurfaceformat.cpp, src/gui/kernel/qsurfaceformat.h, src/gui/opengl/platform ほか | なし | 保留 | 同 H7 前段で desktop は起動（16:37 の 10 回）。HDR は内蔵画面が SDR で未確認、10bit の色も未計測 |
| 95bb9c74f1 | Deprecate QT_FEATURE_opengl_dynamic | src/gui/CMakeLists.txt, src/gui/configure.cmake, src/gui/kernel/qopenglcontext.cpp ほか | なし | 保留 | L1 H7 前段（16:37 の 10 回）: desktop は起動し、ホバー 35.4→13.6ms、インク 39.1→17.9ms。表示の正しさ・HDR・Wintab は未確認 |
| 747eca7b69 | Let hellogles3 to manually switch API on Windows | examples/opengl/hellogles3/CMakeLists.txt, examples/opengl/hellogles3/app.manifest, examples/opengl/hellogles3/glwindow.cpp ほか | なし | 不要 | — |
| 72a68d87fd | Fix building on platforms without EGL | src/gui/CMakeLists.txt | なし | 不要 | — |
| 02216d26ba | [win+angle] Fix flickering when resizing the Window | src/plugins/platforms/windows | なし | 保留 | 同 H7 前段で desktop は起動（16:37 の 10 回）。desktop のリサイズとダイアログのちらつきは未確認 |
| 1efb3f6434 | [win] Implement callback for ANGLE debugging information | src/plugins/platforms/windows | なし | 不要 | — |
| 261c23275e | [win] Implement support for ANGLE's openGL backend | src/gui/kernel/qplatformopenglcontext.h, src/gui/rhi/qrhigles2.cpp, src/gui/rhi/qrhigles2_p.h ほか | なし | 保留 | L1 H7 前段（16:37 の 10 回）: desktop は起動し、ホバー 35.4→13.6ms、インク 39.1→17.9ms。表示の正しさ・HDR・Wintab は未確認 |
| 14ebe92549 | Remove dynamicgl from mkspecs | mkspecs/features/win32/opengl.prf | なし | 保留 | L1 H7 前段（16:37 の 10 回）: desktop は起動し、ホバー 35.4→13.6ms、インク 39.1→17.9ms。表示の正しさ・HDR・Wintab は未確認 |
| aced4e6119 | Fix crash on Linux when using wayland plugin | src/gui/opengl/platform | なし | 不要 | — |
| 7f50d93a7f | Fix a crash when opening multiple windows in Compatibility openGL profile | src/opengl/qopenglpaintengine.cpp, src/opengl/qopenglpaintengine_p.h | なし | 不要 | — |
| 1a543f2832 | Add workaround for flickering on resizing on ANGLE and Qt6 | src/plugins/platforms/windows | なし | 保留 | 同 H7 前段で desktop は起動（16:37 の 10 回）。desktop のリサイズとダイアログのちらつきは未確認 |
| 77d2b02daf | Fix flickering when opening dialogs on ANGLE and Qt6 | src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h, src/gui/painting/qplatformbackingstore.cpp ほか | なし | 保留 | 同 H7 前段で desktop は起動（16:37 の 10 回）。desktop のリサイズとダイアログのちらつきは未確認 |
| 70086804f9 | Fix EGL format being reset to 0 alpha instead of 2 | src/gui/opengl/platform | なし | 保留 | 同 H7 前段で desktop は起動（16:37 の 10 回）。HDR は内蔵画面が SDR で未確認、10bit の色も未計測 |

### 根拠

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

| コミット | 件名 | 変更範囲 | 正式版 | 判定 | L1 の根拠（2026-11-10） |
| --- | --- | --- | --- | --- | --- |
| e121cb08e5 | Pass proper dirty rect in paint event to QOpenGLWidget with partial updates | src/openglwidgets/qopenglwidget.cpp, src/widgets/kernel/qwidget.cpp, src/widgets/kernel/qwidget_p.h ほか | なし | 保留 | L1 H6 は部分再描画の正しさを測っていない。部分更新の dirty rect は未確認 |
| a9b60ead54 | Implement framerate compression in QWidgetRepaintManager | src/corelib/CMakeLists.txt, src/corelib/kernel/qsignalcompressor.cpp, src/corelib/kernel/qsignalcompressor_p.h ほか | なし | 不要 | L1 H6（09:39 の 10 回）: 外すと +24.5/+11.7ms。H1=1 の下（13:41 の 15 回）は +0.3/0ms で、PresentMon の差 0.1ms 未満。H1=1 を既定にするため不要 |
| 7ec0f9c727 | Replace QSignalCompressor with a specialized QFrameRateCompressor | src/corelib/CMakeLists.txt, src/corelib/kernel/qframeratecompressor.cpp, src/corelib/kernel/qframeratecompressor_p.h ほか | なし | 不要 | L1 H6（09:39 の 10 回）: 外すと +24.5/+11.7ms。H1=1 の下（13:41 の 15 回）は +0.3/0ms で、PresentMon の差 0.1ms 未満。H1=1 を既定にするため不要 |
| fb923e4a97 | Add fast-transfer option for QImage when rendering QWidget | src/gui/painting/qbackingstoredefaultcompositor.cpp | なし | 保留 | L1 H6 の切り替えでも fast transfer は有効のままで、単独の効果は未計測 |
| 6d39b2c286 | Compress frames in a more parallel way | src/gui/rhi/qrhi.cpp, src/gui/rhi/qrhi.h, src/gui/rhi/qrhi_p.h ほか | なし | 不要 | L1 H6（09:39 の 10 回）: 外すと +24.5/+11.7ms。H1=1 の下（13:41 の 15 回）は +0.3/0ms で、PresentMon の差 0.1ms 未満。H1=1 を既定にするため不要 |
| dc3246d166 | Fix possible recursion issue in QFrameRateCompressor | src/corelib/kernel/qframeratecompressor.cpp, src/corelib/kernel/qframeratecompressor_p.h | なし | 不要 | L1 H6（09:39 の 10 回）: 外すと +24.5/+11.7ms。H1=1 の下（13:41 の 15 回）は +0.3/0ms で、PresentMon の差 0.1ms 未満。H1=1 を既定にするため不要 |
| 6e540ae34f | Implement ability to switch FPS compression mode | src/widgets/kernel/qwidgetrepaintmanager.cpp | なし | 不要 | L1 H6（09:39 の 10 回）: 外すと +24.5/+11.7ms。H1=1 の下（13:41 の 15 回）は +0.3/0ms で、PresentMon の差 0.1ms 未満。H1=1 を既定にするため不要 |
| c7d92e97b8 | Implement optional FPS stats printing | src/corelib/CMakeLists.txt, src/corelib/kernel/qmeasureavgportion_p.h, src/gui/rhi/qrhigles2.cpp | なし | 不要 | L1 H6（09:39 の 10 回）: 外すと +24.5/+11.7ms。H1=1 の下（13:41 の 15 回）は +0.3/0ms で、PresentMon の差 0.1ms 未満。H1=1 を既定にするため不要 |
| 71fd8157aa | Add QT_FRAME_RATE_OVERRIDE switch | src/widgets/kernel/qwidgetrepaintmanager.cpp | なし | 不要 | L1 H6（09:39 の 10 回）: 外すと +24.5/+11.7ms。H1=1 の下（13:41 の 15 回）は +0.3/0ms で、PresentMon の差 0.1ms 未満。H1=1 を既定にするため不要 |
| f1cf04e958 | Fix clipping when cutting QOpenGLWidget through the GUI texture | src/widgets/kernel/qwidget.cpp | なし | 保留 | L1 H6 はクリップの正しさを測っていない。GUI テクスチャの切り抜きは未確認 |

### 根拠

- LibrePaint は、この一群があるときは `krita/main.cc:715-726` で `QT_BACKING_STORE_USE_FAST_QIMAGE_TRANSFER` と `QT_FRAME_RATE_OVERRIDE` を設定し、間引きを Qt（`QFrameRateCompressor`）に任せる。ないときは `KisCanvas2` 側の自前の間引き（`libs/ui/canvas/kis_canvas2.cpp:245`、`:355`、`:407`、`:1181`、`:1192`）が働く。この分岐が「描画の更新の仕組みが2系統ある」の実体で、Q4 の完了条件「1系統にする」はここを指す。
- L1 の H6 の実験で、Windows の現行版（フォーク、間引きあり）を `QT_FRAME_COMPRESSION_MODE=2`（間引きなし）でも測り、遅れへの寄与を求める。同時に、正式版の Qt で `KisCanvas2` 側の間引きが働いた場合の値と比べる。差が 2ms 未満なら「不要」にし、`KisCanvas2` の1系統にまとめる。
- e121cb08e5（部分更新の dirty rect）と f1cf04e958（GUI テクスチャの切り抜き）は、間引きとは別に、部分再描画の範囲とクリップの正しさを変える。H6 の計測で部分再描画を残す場合は、この2件だけを「必要」として残す候補にする。

## HDR と色変換（バックバッファー、QRhi）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 | L1 の根拠（2026-11-10） |
| --- | --- | --- | --- | --- | --- |
| 752559eb9b | Implement color conversion for the backing store texture | src/gui/CMakeLists.txt, src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h ほか | なし | 保留 | L1 H7 前段で desktop は起動（16:37 の 10 回）。HDR は内蔵画面が SDR で未確認、10bit の色と表示の正しさは未計測 |
| c55693d623 | Implement a manual test for checking is HDR features work | examples/opengl/CMakeLists.txt, examples/opengl/hdr-qopenglwidget/CMakeLists.txt, examples/opengl/hdr-qopenglwidget/KisGLImageF16.cpp ほか | なし | 保留 | L1 H7 前段で desktop は起動（16:37 の 10 回）。HDR は内蔵画面が SDR で未確認、10bit の色と表示の正しさは未計測 |
| a1fa6f1629 | Create color conversion pipelines on-demand | src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h | なし | 保留 | L1 H7 前段で desktop は起動（16:37 の 10 回）。HDR は内蔵画面が SDR で未確認、10bit の色と表示の正しさは未計測 |
| 5da2f94813 | Implement feature QRhi::DynamicBlendSwitch | src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h, src/gui/rhi/qrhi.h ほか | なし | 保留 | L1 H7 前段で desktop は起動（16:37 の 10 回）。HDR は内蔵画面が SDR で未確認、10bit の色と表示の正しさは未計測 |

### 根拠

- LibrePaint の HDR 表示は、この一群の上に成り立つ。`CMakeLists.txt:784-793` は `QOpenGLWidget::setTextureColorSpace` の有無を `HAVE_HDR` として調べ、`libs/ui/opengl/kis_opengl.cpp:813`（HDR 形式の判定）、`:920`（16bit のサーフェスの形式の一覧）、`libs/ui/opengl/KisScreenInformationAdapter.cpp:152`（画面の HDR の検出）、`plugins/dockers/lut/lutdocker_dock.cpp:244`、`plugins/dockers/smallcolorselector/KisGLImageWidget.cpp:65-66` がそれに依存する。`QOpenGLWidget::setTextureColorSpace` は正式版の Qt にない。
- したがって、この一群を外すと Windows の HDR 表示（10bit のキャンバスと色変換）がなくなる。CHARTER の「UI と操作の見かけは旧版から変えない」に照らすと、HDR の画面での見かけが変わるため、判定は Windows の HDR 表示を残すかどうかで決まる。
- 決めるための確認：基準機 Surface Pro 7 の内蔵画面は 8bit で、HDR の確認は外部の HDR 画面が要る。確認は「Windows の HDR を有効にした画面で、旧版と Q1 版のキャンバスと色選択器の見かけを並べる」で行う。HDR を残す場合は、Q4 で正式版の Qt に対する同等の色変換（バックバッファーの色空間の変換）を LibrePaint 側に作るか、公式の修正を待つ。HDR をやめる場合はこの一群を「不要」とし、`HAVE_HDR` の分岐を外す。

## Windows のペン入力（WinTab、Windows Ink、タッチ）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 | L1 の根拠（2026-11-10） |
| --- | --- | --- | --- | --- | --- |
| c41d66281e | [win] Generate Enter/Leave events on touching non-client area of the window | src/plugins/platforms/windows | なし | 必要 | — |
| df3a678309 | [win] Remove subscription on WM_MOUSELEAVE when a tablet enters the window | src/plugins/platforms/windows | なし | 必要 | — |
| 7bb46828ae | Disable tablet relative mode in Qt | src/plugins/platforms/windows | なし | 保留 | L1 の合成入力は全回 WM_POINTER（native_wintab=0）。Wintab の相対モードは実機の Wintab ドライバーで未確認 |
| 051139ed72 | Fetch mapped screen size from the Wintab driver | src/plugins/platforms/windows | なし | 必要 | — |
| 2357bbfe90 | Add a workaround for button mapping on Lenovo Yoga C940 | src/plugins/platforms/windows | なし | 不要 | — |
| 33a49f25d1 | Add workaround for handling table press correctly in WinInk mode | src/gui/kernel/qguiapplication.cpp, src/gui/kernel/qwindowsysteminterface.cpp, src/plugins/platforms/windows | なし | 必要 | — |
| 7ca0b0e289 | Revert "Query the mouse buttons initially in case the middle button was used" | src/plugins/platforms/windows | なし | 不要 | — |
| f5a75c4071 | WinInk: Synthesize mouse events for full pen stroke | src/plugins/platforms/windows | なし | 必要 | — |
| 41adadd8db | Synthesize Enter/LeaveEvent for accepted QTabletEvent | src/widgets/kernel/qwidgetwindow.cpp | なし | 必要 | — |
| 0027358f20 | Poison Qt's headers with a mark about presence of Enter/Leave patch | src/gui/kernel/qevent.h | なし | 必要 | — |
| 8f8eabfb72 | Fix duplicated syntetic mouse events on WinTab | src/plugins/platforms/windows | なし | 必要 | — |
| c132390390 | Revert "Send LeaveEvent as reaction to WM_POINTERLEAVE to reset hover states" | src/plugins/platforms/windows | なし | 必要 | — |
| a60ed01a25 | [win] Allow explicit accepting of touch events on Windows | src/gui/kernel/qguiapplication.cpp, src/plugins/platforms/windows | なし | 保留 | L1 はペンのみ。タッチのパン・ズームと描画は未確認 |
| d918f368da | Windows: implement touchpad gestures using Direct Manipulation | src/gui/configure.cmake, src/gui/kernel/qevent.cpp, src/plugins/platforms/direct2d ほか | なし | 不要 | — |

### 根拠

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

| コミット | 件名 | 変更範囲 | 正式版 | 判定 | L1 の根拠（2026-11-10） |
| --- | --- | --- | --- | --- | --- |
| ed56d12b50 | Fix unexpected window transparency after enabling CSD on Wayland | src/plugins/platforms/wayland | f1fbd15105（2026-02-27） | 正式版で解決済み | — |
| d2aa04db1a | Fix cursor update when tablet moves between native surfaces | src/plugins/platforms/wayland | なし | 保留 | L1 は Windows の WM_POINTER のみ。Wayland のペンは Linux 開発機にタブレットがなく未確認 |
| b069509b06 | Fix cursor update on tablet transition between alien widgets | src/plugins/platforms/wayland | なし | 保留 | L1 は Windows の WM_POINTER のみ。Wayland のペンは Linux 開発機にタブレットがなく未確認 |
| 483ceed535 | [wayland] Handle the case when a tablet has multiple tools | src/plugins/platforms/wayland | なし | 保留 | L1 は Windows の WM_POINTER のみ。Wayland のペンは Linux 開発機にタブレットがなく未確認 |
| 3b6b57d0b7 | [wayland] Emit native Enter/Leave events on tablet proximity events | src/plugins/platforms/wayland | なし | 保留 | L1 は Windows の WM_POINTER のみ。Wayland のペンは Linux 開発機にタブレットがなく未確認 |
| bc9fec0a20 | [wayland] Implement raise() and lower() methods for subsurfaces | src/plugins/platforms/wayland, tests/auto/widgets/kernel | なし | 不要 | — |
| 5020e751e6 | [wayland] Make positioners reactive by default | src/plugins/platforms/wayland | なし | 不要 | — |
| 435e604932 | wayland: keep a popup in place when it is resized | src/plugins/platforms/wayland, tests/auto/wayland/shared, tests/auto/wayland/xdgshell | c3cc50d89d（2026-09-09） | 正式版で解決済み | — |
| eb6579e6d1 | [wayland] Fix flickering when resizing popup windows | src/plugins/platforms/wayland | なし | 不要 | — |
| 693de2c4fb | Wayland client: update cursor position after sending QTabletEvent | src/plugins/platforms/wayland | 4332e5e2c4（2026-07-23） | 正式版で解決済み | — |

### 根拠

- 正式版で解決済みの3件は、qt/qtbase に同等のコミットがある（ed56d12b50 → f1fbd15105、435e604932 → c3cc50d89d、693de2c4fb → 4332e5e2c4）。6.11.3 以降の正式版を使えば同じ結果になる。
- 不要の根拠：
  - bc9fec0a20 はネイティブの subsurface の raise/lower で、LibrePaint は窓の重なり順をこの経路で操作しない。
  - 5020e751e6 はポップアップの位置が表示の順に依存する不具合の修正で、正式版の Qt の Linux 版でダイアログとポップアップの位置のずれは出ていない（Wayland の計測は 20/16ms で、色プロファイルの修正後は公式 Krita と同程度。DECISIONS.md 2026-11-08）。
  - eb6579e6d1 はポップアップのリサイズ時のちらつきで、表示だけの問題。主指標（キャンバスの輪郭とインク）に効かず、正式版での計測でも出ていない。
- 保留の4件（d2aa04db1a、b069509b06、483ceed535、3b6b57d0b7）は、いずれも Wayland のタブレット（ペン）の経路の修正で、カーソルの更新、複数のツールの同時の近接、タブレットの Enter/Leave に関わる。LibrePaint の Linux 版は正式版の Qt で動いているが、これまでの計測は合成マウス入力で、ペンの経路は確かめていない（Linux 開発機にタブレットがない）。
- 決めるための実験：Wayland で実際のペン（または uinput で作ったタブレットの入力）を使い、(1) 近接したときのカーソルの形、(2) 2つのツールを近づけたときの切り替え、(3) キャンバスとドックの間でペンを移動したときのホバーの状態、の3つを確かめる。不具合が出なければ「不要」にする。

## キーボードとショートカット（配列、不均衡な押下と解放）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 | L1 の根拠（2026-11-10） |
| --- | --- | --- | --- | --- | --- |
| 82d2864a8a | Don't eat ShortcutOverride events when there is a partial-match | src/gui/kernel/qshortcutmap_p.h, src/gui/kernel/qwindowsysteminterface.cpp | なし | 必要 | — |
| dc27f27d8e | Fix unbalanced KeyPress/Release events in children of QMdiArea | src/widgets/widgets/qmdiarea.cpp | なし | 必要 | — |
| 18a07bf888 | Fix shortcuts assigned to special keys on non-latin layouts | src/plugins/platforms/windows | なし | 保留 | L1 は合成ペンのみ。日本語配列のショートカットは未確認 |
| 3fab97e30b | Fix multiple shortcuts match from different keyboard layouts | src/gui/kernel/qshortcutmap.cpp | なし | 保留 | L1 は合成ペンのみ。日本語配列のショートカットは未確認 |
| adf861033b | Fix fallback latin shortcuts not to be the top priority on Windows | src/plugins/platforms/windows | なし | 保留 | L1 は合成ペンのみ。日本語配列のショートカットは未確認 |
| 747423036c | [win] Fix unbalanced KeyPress/Release events when using | src/plugins/platforms/windows | なし | LibrePaint 側で代替 | — |
| f8a61ebc5b | [win] Fin unbalanced shortcuts on non-latin layouts again | src/plugins/platforms/windows | なし | LibrePaint 側で代替 | — |
| 9bfd8a120f | Make sure AltGr-based shortcuts are disallowed when textbox is in focus | src/widgets/widgets/qwidgetlinecontrol.cpp | なし | 保留 | L1 は合成ペンのみ。日本語配列のショートカットは未確認 |
| c8049a518f | Fix composing of dead keys with short form of system locale | src/plugins/platforminputcontexts/compose | なし | 不要 | — |
| 07a57fa505 | Fix shortcuts with special keys on non-US layouts on Linux | src/gui/platform/unix | なし | 保留 | L1 は合成ペンのみ。日本語配列のショートカットは未確認 |
| 21d400db4f | Fix unbalanced KeyPress/Release on non-latin keyboard on Linux | src/plugins/platforms/xcb | なし | LibrePaint 側で代替 | — |
| 35296972ce | Poison Qt's headers with a mark about presence of KeyPress/Release patches | src/gui/kernel/qevent.h | なし | 不要 | — |

### 根拠

- 必要の根拠：
  - 82d2864a8a は、Qt が部分一致のショートカットで ShortcutOverride を食べないようにする。件名に Krita とあり、LibrePaint は独自のショートカット処理 `KisShortcutMatcher`（`libs/input/kis_shortcut_matcher.h:73`）を持つため、この改変がないと独自処理に届かないショートカットが出る。
  - dc27f27d8e は、QMdiArea の子で Ctrl+Tab を押したときの KeyPress/KeyRelease の不均衡を直す。LibrePaint は文書の切り替えに QMdiArea を使う（`libs/application/ui/workspace/KisMainWindow.cpp:245-251`）。
- LibrePaint 側で代替の根拠：747423036c、f8a61ebc5b、21d400db4f（非ラテン配列での KeyPress/KeyRelease の不均衡）は、`libs/input/ui/kis_input_manager_p.cpp:242` の回避コード（`useUnbalancedKeyPressEventWorkaround`）と同じ問題を扱う。この回避コードは macOS で常に、Linux では印がないとき（`!KRITA_QT_HAS_UNBALANCED_KEY_PRESS_RELEASE_PATCH`）に有効になり、`KRITA_FIX_UNBALANCED_KEY_EVENTS` で切り替えられる。Q4 では Windows でも印がないときに有効にする（3行程度の変更）。
- 不要の根拠：
  - c8049a518f は、システムのロケールが `en_US` の短い形のときに compose の表が見つからない問題で、LibrePaint は端末のロケール（`ja_JP.UTF-8` など）をそのまま使う。
  - 35296972ce は印だけの改変で、動作を変えない。印がないと上の回避コードが有効になる。
- 保留（非ラテン配列のショートカットの実際の一致）：18a07bf888、3fab97e30b、adf861033b、07a57fa505、9bfd8a120f は、非ラテン配列で特殊キー（`\`、`/`、`;`、`` ` ``、`'`）や AltGr を使ったときの一致の優先順位を変える。LibrePaint の利用者の配列（日本語）で実際に不具合が出るかを確かめる。確かめ方は、日本語配列で (1) テキストツールの入力中に AltGr 相当の文字を打ち、(2) 特殊キーに割り当てたショートカットを押し、(3) 同じキーに複数の配列で割り当てがある状態でショートカットを押す、の3つを旧版と比べる。不具合が出なければ「不要」にする。

## 不具合とクラッシュの修正（ウィジェット、GUI、コア）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 | L1 の根拠（2026-11-10） |
| --- | --- | --- | --- | --- | --- |
| 29d93a5d62 | Remove chunk size limits | src/gui/image/qpnghandler.cpp | なし | 不要 | — |
| cf05dd04c8 | Check whether there is a QGestureRecognizer at all | src/widgets/kernel/qgesturerecognizer.cpp | なし | 不要 | — |
| d2507d05fa | Check if we reached the end of Iterator | src/widgets/kernel/qgesturemanager.cpp | なし | 不要 | — |
| 93bd31ff0f | Use DebugBreak instead of fast-fail exception to abort | src/corelib/global/qassert.cpp | なし | 不要 | — |
| f1302dbbc8 | Do not crash if the foundry name is an empty space | src/gui/text/qfontdatabase.cpp | なし | 不要 | — |
| 8dadf099b8 | Check for uninitialized QDockAreaLayoutInfo | src/widgets/widgets/qdockarealayout.cpp | なし | 保留 | L1 の計測では再現しない（対象外）。ドックの操作は未確認 |
| 2ea7817457 | Return 0 for Window if there is no widget | src/widgets/accessible/qaccessiblewidget.cpp | なし | 不要 | — |
| bf2ee30931 | QMenu: make less sensitive to mouse jitter | src/widgets/widgets/qmenu.cpp | 4292ad0292（2020-08-19） | 正式版で解決済み | — |
| df46429bb3 | Fix slowdown caused by making window system events synchronous. | src/gui/kernel/qwindowsysteminterface.cpp, src/gui/kernel/qwindowsysteminterface.h | なし | 不要 | — |
| 7c155ae659 | Implement better parsing of EDID model number | src/gui/util/qedidparser.cpp | なし | 不要 | — |
| 83e4fd6099 | Sync buffers of the destination file after QFile::copy() | src/corelib/io/qfile.cpp | なし | 不要 | — |
| d850335b2d | [win] Fix pasting copy-pasting QImages to Chrome | src/plugins/platforms/windows | なし | 保留 | L1 の計測では再現しない（対象外）。Windows の Chrome/Edge への貼り付けは未確認 |
| 24ea6b40c2 | Fix QHeaderView failing to init memory mode | src/widgets/itemviews/qheaderview.cpp, tests/auto/widgets/itemviews | e3d05b35b3（2026-10-27） | 正式版で解決済み | — |

### 根拠

- 正式版で解決済みの2件は、qt/qtbase に同等のコミットがある（bf2ee30931 → 4292ad0292、24ea6b40c2 → e3d05b35b3）。
- 不要の根拠：
  - 29d93a5d62 は Qt の PNG の読み書きで libpng の利用者の上限を外す。外れる上限は、保持するチャンクの数 1000 と、1つのチャンクの確保 8MB（nixpkgs の libpng 1.6.59 の `pnglibconf.h` の `PNG_USER_CHUNK_CACHE_MAX` と `PNG_USER_CHUNK_MALLOC_MAX`）。当たるのは、チャンクが 1000 を超える PNG（tEXt/iTXt の多い画像）と、1つのチャンクの確保が 8MB を超える PNG（大きな iCCP やテキスト）を Qt の PNG の読み書きで扱う場合。LibrePaint の文書の PNG の入出力は libpng を直接使う（`libs/impex/ui/kis_png_converter.cpp` の `png_read_image`）ので対象外。Qt の PNG の読み込みを使うのは、ブラシ（`libs/brush/kis_png_brush.cpp:52`）、PaintOp プリセット（`libs/image/brushengine/kis_paintop_preset.cpp:180`）、パターン（`libs/pigment/resources/KoPattern.cpp:301`）、参照画像（`libs/ui/canvas/KisReferenceImage.cpp:69`）で、上限に当たる大きさの例はない。当たったとき（これらの PNG が読めない）に再検討する。
  - cf05dd04c8 と d2507d05fa は Qt のジェスチャの管理のヌルと反復の終端の検査で、LibrePaint のジェスチャの使い方でその条件が起きた例はない。
  - 93bd31ff0f は Windows の DrMingw でバックトレースを取るための変更で、デバッグの道具にだけ関係する。
  - f1302dbbc8 はフォントの foundry 名が空白だけのときのクラッシュで、そのフォントを使う例がない。
  - 2ea7817457 は画面読み上げ（UI オートメーション）の `window()` の戻り値で、画面読み上げを使わない通常の操作に関係しない。
  - df46429bb3 は、タブレットのイベントを Windows 以外で非同期に戻す（Windows では同期のまま）。比較に使う値の出所は、macOS が 2026-11-07 の v1.0.3（21.5/23.3ms。ホバー/インク、DECISIONS.md 2026-11-07 と #108 の計測の要約で同日・同版）、Linux Wayland が 2026-11-08 の色プロファイル探索の修正後（20/16ms。同様に DECISIONS.md 2026-11-08 と #108 の計測の要約）。どちらも正式版の Qt（もともと非同期）の値で、この改変で値は変わらない。
  - 7c155ae659 は EDID からの画面の名前と製造番号の読み取りで、表示の名前だけが変わる。
  - 83e4fd6099 は `QFile::copy()` のあとの fsync で、電源断のときの耐久性を上げる。文書の保存は `QSaveFile` の経路で、`QFile::copy()` はテンプレート、バックアップ、プロファイルの複製に使う（`libs/global/KisBackup.cpp:37`、`libs/ui/dialogs/kis_dlg_preferences.cc:1527` ほか）。電源断での耐久は CHARTER の「一般的な水準を超えない」に照らして要件にしない。
- 保留の2件：
  - 8dadf099b8 は、ドックのドッキングのアニメーション中に `QDockAreaLayoutInfo` が未初期化のまま使われるクラッシュの修正で、LibrePaint のドックの使い方で起こりうる。決めるための確認：ドックを外して戻す操作を繰り返しながら、アンドゥとビューの操作を行い、落ちないかを確かめる。落ちるなら「必要」とする。
  - d850335b2d は、アルファを持つ QImage をクリップボードに出すときに CF_DIB を出さず CF_PNG と CF_DIBV5 だけにする改変（Chrome がアルファ付きの CF_DIB を壊れた画像として貼るため）。LibrePaint のコピーは `KisClipboard::setClip` が内部の形式に加えて `mimeData->setImageData(qimage)` で QImage も出す（`libs/impex/ui/kis_clipboard.cc` の 175〜196 行）。キャンバスの画像はアルファを持つため、Windows の Qt は CF_DIB も出し（`src/plugins/platforms/windows/qwindowsmimeregistry.cpp` の `QWindowsMimeImage::formatsForMime`）、Chrome に貼ると壊れる経路に当たる。したがって「主な経路に関係しない」とは言えない。決めるための確認：Windows で LibrePaint のキャンバスの範囲をコピーし、Chrome と Edge に貼って、欠け・色の化けがないかを見る。壊れるなら「必要」、正常なら「不要」とする。

## その他（構築、UI、プラットフォーム固有の調整）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 | L1 の根拠（2026-11-10） |
| --- | --- | --- | --- | --- | --- |
| a7c925fb66 | [windeployqt] Fix windeployqt to find ICU built with Meson | src/tools/windeployqt/main.cpp | なし | 不要 | — |
| 55a0448923 | Use fast path for unsupported mime types | src/plugins/platforms/windows | なし | 不要 | — |
| d002731cb6 | [moc] Remove double checks for file-exists in searchIncludePaths() | src/tools/moc/preprocessor.cpp | なし | 不要 | — |
| f786da6451 | Add QMdiArea::AlwaysShowSubwindowNameInTitleBar | src/widgets/widgets/qmdiarea.h, src/widgets/widgets/qmdisubwindow.cpp, src/widgets/widgets/qmdisubwindow_p.h | なし | LibrePaint 側で代替 | — |
| d343ee345f | Warn about downgraded precise timers | src/corelib/kernel/qeventdispatcher_win.cpp | cf8cd71476（2026-10-27） | 正式版で解決済み | — |
| 54de8a5b6d | Add icon caching in QFusionStyle | src/widgets/styles/qfusionstyle.cpp | なし | 不要 | — |
| 7db708f3ff | Assume widgets are rendered in pass-through mode on MacOS | src/gui/painting/qbackingstoredefaultcompositor.cpp | なし | 不要 | — |
| 81a5ac7eb5 | Allow widgets to block kinetic scrolling | src/widgets/util/qflickgesture.cpp, src/widgets/util/qscroller.cpp, src/widgets/util/qscroller.h | なし | 不要 | — |
| 5874f1f131 | Implement QT_GLX_SWAP_INTERVAL_SYNC_WORKAROUND | src/plugins/platforms/xcb | なし | 不要 | — |
| b2cc4909b0 | [macos] Add bundle-local path to GenericDataLocation | src/corelib/io/qstandardpaths_mac.mm | なし | 不要 | — |
| 9dc7c77358 | Add a switch to disable accessibility on Windows | src/plugins/platforms/windows | なし | 不要 | — |

### 根拠

- LibrePaint 側で代替の1件：f786da6451 の `QMdiArea::AlwaysShowSubwindowNameInTitleBar` は LibrePaint が使う（`libs/application/ui/workspace/KisMainWindow.cpp:250`）。ただし `CMakeLists.txt:817-826` が有無を `HAVE_QMDIAREA_ALWAYS_SHOW_SUBWINDOW_TITLE` として調べ、`KisMainWindow.cpp:249-251` は印がないとき従来の動作（浮いた窓のときだけ子の名前を出す）になる。1.0.3 はタブ表示でも子の名前を窓の題名に出していたため、Q4 で LibrePaint 側から題名を設定する代替を作る（タブの切り替えで題名を更新する処理は `KisMainWindow` に既にある）。
- 正式版で解決済みの1件：d343ee345f は qt/qtbase の cf8cd71476 に相当する。
- 不要の根拠：
  - a7c925fb66 は windeployqt の ICU の探索で、LibrePaint の配布物は Nix で組み立てる。
  - 55a0448923 は Windows のクリップボードの形式の一覧を返すときの最適化で、主指標（ペンの入力から表示まで）に関係せず、遅さの計測も出ていない。
  - d002731cb6 は moc の構築時間だけを短くする。
  - 54de8a5b6d は QFusionStyle のアイコンのキャッシュで、窓のリサイズの遅さを直す。主指標の対象ではなく、必要になったときに計測して入れる。
  - 7db708f3ff は macOS のウィジェットの描画の前提で、macOS は正式版の Qt で 21.5/23.3ms（DECISIONS.md 2026-11-07）。
  - 81a5ac7eb5 は QScroller のフィルターの追加で、LibrePaint のコードは `kis_qscroller_filter` を参照しない（`KRITA_QSCROLLER_PATCH` の検査は `CMakeLists.txt:869-877` にだけある）。
  - 5874f1f131 は GLX の交換間隔の回避で、Qt 6.11.3 の QTBUG-147854 の修正で不要になった（DECISIONS.md 2026-11-07）。
  - b2cc4909b0 は macOS のバンドル内のデータの探索で、macOS 版は正式版の Qt で動いている。
  - 9dc7c77358 は Windows のアクセシビリティを切る環境変数の追加で、LibrePaint は `krita/main.cc:703-712` で `DisableAccessibilityInQt` の設定に応じて設定する。正式版の Qt ではこの変数が効かず、アクセシビリティを切る設定が無効になるだけで、動作は変わらない。Q4 でこの設定項目の扱いを決める。

## L1 の結果による保留の見直し（2026-11-10）

L1（Windows のペン入力から表示までの約 50ms の遅れの原因を仮説ごとに数値で特定する）の結果で、保留のうち 7 件を「不要」にした。各行の「L1 の根拠（2026-11-10）」に、計測の回と ms を書いた。比較はすべて同じ回で交互に測った値で、回ごとの中央値のばらつきは 5% 未満である。

- 画面更新の間引きのうち、フレームレート圧縮の実装（a9b60ead54、7ec0f9c727、6d39b2c286、dc3246d166、6e540ae34f、c7d92e97b8、71fd8157aa）を「不要」にした。
  - H6（フォークの更新の間引きの歯止め）の計測（2026-11-10 09:39、`l1-h6-throttle`、基準と切り替えを各 5 回）：H1（先積みのフレーム数）=1 のない基準では、外すとホバー 57.5 → 82.0ms（+24.5ms）、インク 61.3 → 73.0ms（+11.7ms）だった。
  - H1=1 の下（2026-11-10 13:41、`l1-h1-remeasure-ext`、3 対象を各 5 回）：H1=1 のホバー 49.9ms、H1=1 で間引きも外した回 50.2ms（+0.3ms）、インクはどちらも 40.1ms、PresentMon の入力から表示の差は 0.1ms 未満だった。H1=1 では前のフレームがほぼ毎回 GPU で完了しているため、間引きは遅れに寄与しない。
  - L2（Windows の遅れを直す）は H1=1 を ANGLE の既定にするので、Q4 では Qt の圧縮を外し、`KisCanvas2` の1系統にまとめる。desktop の経路は H1 が効かないため、Q4 の後に `KisCanvas2` の間引きだけで測り直す。差が 2ms 以上なら、その時点で1系統の設計を見直す。
  - e121cb08e5（部分更新の dirty rect）、fb923e4a97（fast transfer）、f1cf04e958（GUI テクスチャのクリップ）は、H6 の切り替えでは効果を分離できない。部分再描画とクリップの正しさ、fast transfer の単独の効果を確かめるまで保留とする。
- ANGLE と OpenGL ES の 11 件は保留のままにした。H7（フォークなし・ANGLE を使わない OpenGL）の前段（2026-11-10 16:37、`l1-h7-desktop-smoke`、2 対象を各 5 回）で、desktop は起動し、H1=1＋H5（ホバーの間引き）のホバー 35.4ms・インク 39.1ms から、H5＋desktop は 13.6ms・17.9ms になった（−21.8ms / −21.2ms）。ただし desktop を既定にするかは、表示の正しさ、HDR、Wintab、取りこぼし、CPU・電力の確認の後で決める（18:05 と 18:20 の記録）。ANGLE を残す判断ならこの一群は必要、やめる判断なら不要になる。
- HDR と色変換の 4 件は保留のままにした。確かめた範囲は「desktop の OpenGL が起動し、合成入力のホバーとインクを測れる」ことまでである。HDR は Surface の内蔵画面が SDR のため未確認で、10bit の色、色変換、表示の正しさは確かめていない。7ad4a8bc17 と 70086804f9 も同じ確認に従う。
- Windows のペンの保留 2 件は保留のままにした。確かめた範囲は、Surface の合成入力が全回 WM_POINTER（`native_wintab`=0）で届き、ペンの描画が 4 通りの入力の設定で問題なかったことである（18:20）。Wintab の相対モード（7bb46828ae）は Wintab ドライバーを持つ実機で、タッチの受理（a60ed01a25）はタッチのパン・ズームと描画で確かめていない。
- Wayland のペン 4 件、キーボードの配列 5 件、ドックのクラッシュ 1 件、クリップボードの CF_DIB 1 件は、L1 の計測の対象外なので保留のままにした（L1 は Windows の WM_POINTER と合成ペンだけで、Linux 開発機にタブレットがなく、キーボードとクリップボードも使っていない）。
- Q4 の判断材料：正式版 Qt 6.11.3 には ANGLE がないため、Q4 は desktop の OpenGL だけになる。H1=1 は ANGLE の経路にしか効かず、desktop では H5（ホバーの間引き）が主な対策になる。H1 と H6 の効果は足し合わせない（H1=1 の下では H6 の差が 0.3ms 未満）。正式版 Qt の構築は、desktop を既定にするかの確認が済むまで行わない。

## Q4 への持ち越し（2026-11-08 の判定のまとめ、2026-11-10 に更新）

92 件の内訳は、不要 40 件、正式版で解決済み 6 件、LibrePaint 側で代替 4 件、必要 11 件、保留 31 件（2026-11-10 に L1 の結果で画面更新の間引きの 7 件を不要にし、d850335b2d は 2026-11-09 に不要から保留へ見直した）。

- 必要（11件）：c41d66281e、df3a678309、051139ed72、33a49f25d1、f5a75c4071、41adadd8db、0027358f20、8f8eabfb72、c132390390、82d2864a8a、dc27f27d8e。Windows のペンの9件は、`QT_WINTAB_DESKTOP_RECT` などの環境変数とペンのホバーの状態に直結する。Q4 では、公式に取り込まれるのを待つか、LibrePaint 側で作り直すかを1件ずつ決める。
- LibrePaint 側で代替（4件）：747423036c、f8a61ebc5b、21d400db4f は `kis_input_manager_p.cpp:242` の回避コードを使い、Windows でも有効にする。f786da6451 はタブ表示の窓の題名を `KisMainWindow` 側で設定する。
- 保留（31件）：ANGLE 11件は H7 前段で desktop が動き約 21ms 速いが、表示の正しさ・HDR・Wintab の確認が済むまで決めない。画面更新の間引き 3件（e121cb08e5、fb923e4a97、f1cf04e958）は部分再描画・fast transfer・クリップの確認で決める。HDR 4件は Windows の HDR 表示を残すかで決める（内蔵画面は SDR で未確認）。Wayland のペン 4件は Wayland でペンを使った確認で決める。キーボードの配列 5件は日本語配列での確認で決める。Windows のペン 2件は実機の Wintab とタッチで決める。ドックのクラッシュ 1件は再現の確認で決める。クリップボードの CF_DIB 1件は Windows で Chrome と Edge への貼り付けを確認して決める。
