# Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）のフォーク撤去の準備

Issue #108 の Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の完了条件
「全プラットフォームが正式版の同じ Qt で構築でき、LibrePaint のソースがフォーク固有の API を
参照しない。主指標が全プラットフォームで Q1（全プラットフォームを Qt 6.11.3 にそろえる）の
値より悪化しない」の準備資料である。
対象の指標は、Surface Pro 7 のホバー輪郭とインクの入力から表示まで（中央値と P95）と、
Windows の表示の正しさである。2026-11-11 に、LibrePaint のソースがフォーク固有の API と
分岐をどこで使うかを一覧にし、正式版 Qt 6.11.3 にしたときに失われる機能を利用者に見える
違いとして整理した。コードは変えていない。

## 結論（要約）

- フォークを使うのは Windows の既定の構築だけである。Linux、macOS、Android、iOS は既に
  正式版 Qt 6.11.3 で構築されている。したがって Q4（フォークをやめ、全プラットフォームで
  正式版の同じ Qt にする）で新たに変わるのは Windows である。
- Windows で新たに失う主なものは、ANGLE（OpenGL を Direct3D に変換する経路）、HDR（10bit と
  scRGB / Rec.2020 PQ の表示）、Wintab の対応範囲の設定（Qt6 では起動時に保存された設定が
  反映されない）、Windows Ink の押下と Enter/Leave の修正、タッチの受理、クリップボードの
  CF_DIB の回避である。
- このうち Wintab の対応範囲と Windows Ink の押下・Enter/Leave は、外すとペンの操作が壊れる
  「必要」の 9 件である。Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の
  前に、公式に取り込むか、LibrePaint 側で作り直すかを決める必要がある。
- その 9 件の見積もり（2.7、2026-11-11）は、正式版 Qt 6.11.3 へのパッチ（P）が合計 305 行で
  3 件が他の改変に依存し、Windows だけのパッチになる。LibrePaint 側の実装（L）は、共通の
  進入・離脱ガードで 3 件をまとめられ、共通ガードと主な 3 件で約 190〜310 行、全部を L で
  作ると約 500〜700 行である。
- ANGLE は外して desktop の OpenGL にする候補が最も効果が大きい。2026-11-10 の L1（Windows の
  ペン入力から表示までの約 50ms の遅れの原因を仮説ごとに数値で特定する）の H7（フォークなし・
  ANGLE を使わない OpenGL）の前段で、H1（先積みのフレーム数）=1 と H5（ホバーの間引き）の組
  よりホバー −21.8ms、インク −21.2ms だった。ただし表示の正しさ、HDR、Wintab、CPU・電力は
  未確認である。
- HDR は CHARTER の「UI と操作の見かけは旧版から変えない」に関わるため、利用者の判断が要る。
  基準機 Surface Pro 7 の内蔵画面は SDR で、外部の HDR 画面がないと確認できない。
- Android の 5 つの分岐（インターフェースの倍率、Xiaomi / OnePlus のスタイラスの回避、ソフト
  キーボードの窓の移動）は、現在の LibrePaint の Android 版でも既に偽である。Q4（フォークを
  やめ、全プラットフォームで正式版の同じ Qt にする）で新たに失うものではないが、Krita の
  Android 版が提供する機能との差として残る。

## 1. 現在のプラットフォーム別の Qt

| プラットフォーム | Qt の出所 | フォーク固有のマクロ | 根拠 |
| --- | --- | --- | --- |
| Linux x86_64 | 正式版 Qt 6.11.3（`nix/qt/overlay.nix`） | 偽 | `nix/linux/krita.nix` は `pkgs.qt6` を使い、`nix/qt/overlay.nix` が `srcs.nix` の公式ソースに差し替える |
| macOS | 正式版 Qt 6.11.3（同じ overlay） | 偽 | 同じ |
| Android | 正式版 Qt 6.11.3（`nix/android/qtbase.nix` は `pkgs.qt6.qtbase` を使う） | 偽 | `nix/android/default.nix` の `androidHost` に共通の overlay を渡す。`qtbase.nix` は `qtbase.src` をそのまま使う |
| iOS | 正式版 Qt 6.11.3（`nix/ios/packages/qtbase.nix` は `qt6Packages.qtbase.src` を使う） | 偽 | `packageSpec.version == qtbase.version` を検査している |
| Windows（既定） | `dimula73/qtbase` の `for-krita/6.11.3`（`d918f368da`） | 真 | `nix/windows/default.nix:5` の `useQtFork ? true`、`nix/windows/krita.nix:145-152` |
| Windows（Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の候補） | 正式版 Qt 6.11.3（`useQtFork = false`） | 偽 | `nix/windows/default.nix:12` の `windows-upstream-qt`、`flake.nix` の `librepaint-windows-upstream-qt-*` |

## 2. フォーク固有の API と分岐の一覧

### 2.1 CMake の検出マクロ

「正式版の値」は、Windows の `useQtFork = false`（`windows-upstream-qt`）と、Linux・macOS・
Android・iOS の現在の構築での値である。

| マクロ | 検出方法 | 主な使用箇所（ファイル:行） | 使い方 | 正式版の値 | 正式版での代わり |
| --- | --- | --- | --- | --- | --- |
| `KRITA_QT_HAS_ENTER_LEAVE_PATCH` | `QtGui/qevent.h` の `QT_HAS_ENTER_LEAVE_PATCH`（`CMakeLists.txt:687`） | `libs/input/ui/kis_input_manager.cpp:755, 790, 810` | Linux でタブレットの押下・移動・解放を受理せず、合成マウスを 1 回捨てる回避コードを有効にする | 偽 | Linux は現状の回避コードのまま。Windows はこの分岐の外なので、別の Enter/Leave の修正（`41adadd8db` など）が要る |
| `KRITA_QT_HAS_UNBALANCED_KEY_PRESS_RELEASE_PATCH` | `QtGui/qevent.h` の `QT_HAS_UNBALANCED_KEY_PRESS_RELEASE_PATCH`（`CMakeLists.txt:693`） | `libs/input/ui/kis_input_manager_p.cpp:242` | 非ラテン配列で KeyPress と KeyRelease が釣り合わないときの回避コードを有効にする。現在は macOS で常に、Linux でマクロが偽のときに有効 | 偽 | Windows でも回避コードを有効にする（数行）。`747423036c`、`f8a61ebc5b`、`21d400db4f` の代替 |
| `KRITA_QT_HAS_UPDATE_COMPRESSION_PATCH` | `QRhi::isLastFrameCompletedOnGPU` の有無をコンパイルして検査（`CMakeLists.txt:707-716`） | `krita/main.cc:737`、`libs/ui/opengl/kis_opengl_canvas2.cpp:265, 576, 595, 682`、`libs/ui/canvas/kis_canvas2.cpp:245, 355, 407, 1181, 1192`、`libs/ui/dialogs/kis_dlg_preferences.cc:2034, 2102` | 真なら Qt のフレームレート圧縮に任せ、偽なら `KisCanvas2` の `KisSignalCompressor` で更新をまとめる。`KisOpenGLCanvas2` は前のフレームが GPU で未完了なら描画を飛ばす | 偽 | `KisCanvas2` の 1 系統になる（Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の「描画の更新の仕組みを 1 系統にする」の到達点）。性能設定の「FPS を検出」は隠れる |
| `KRITA_QT_HAS_ANDROID_QPLATFORMSCREEN_DENSITY_ADJUSTMENT` | `QPlatformScreen::setDensityAdjustment` と `densityAdjustment`（`CMakeLists.txt:892-900`） | `libs/application/ui/orchestration/KisApplication.cpp:204, 262, 585, 1406`、`libs/application/ui/workspace/KisMainWindow.cpp:197, 300, 675, 1891, 3195`、`libs/ui/CMakeLists.txt:2195, 2425`、`libs/ui/canvas/KisAndroidScaling.cpp:148-150` | Android で画面の密度を実行時に変え、インターフェースの倍率をダイアログで選べるようにする | 偽（Android も含む） | なし。起動時の `QT_SCALE_FACTOR` か Android のシステム設定に限られる。Krita の Android 版の「Change Interface Scale」に相当する機能は失われる |
| `KRITA_QT_HAS_ANDROID_EMULATE_MOUSE_BUTTONS_FOR_PAGE_UP_DOWN` | `QCoreApplication::setKritaAttribute(KRITA_QATTRIBUTE_ANDROID_EMULATE_MOUSE_BUTTONS_FOR_PAGE_UP_DOWN)`（`CMakeLists.txt:842-848`） | `KisApplication.cpp:718-720`、`libs/ui/dialogs/kis_dlg_preferences.cc:1755, 1819, 3281` | Xiaomi のスタイラスの Page Up / Page Down を右・中クリックに変える | 偽（Android も含む） | なし。設定のチェックボックスも出ない。Krita の Android 版の回避策に相当する機能は失われる |
| `KRITA_QT_HAS_ANDROID_EMULATE_MOUSE_BUTTONS_FOR_HIGH_FUNCTION_KEYS` | 同じく `KRITA_QATTRIBUTE_ANDROID_EMULATE_MOUSE_BUTTONS_FOR_HIGH_FUNCTION_KEYS`（`CMakeLists.txt:860-866`） | `KisApplication.cpp:735-738`、`kis_dlg_preferences.cc:1760, 1826, 3286` | OnePlus のスタイラスの F21 キーを中クリックに変える | 偽（Android も含む） | なし。設定のチェックボックスも出ない |
| `KRITA_QT_HAS_ANDROID_IGNORE_HISTORIC_TABLET_EVENTS` | `KRITA_QATTRIBUTE_ANDROID_IGNORE_HISTORIC_TABLET_EVENTS`（`CMakeLists.txt:851-857`） | `KisApplication.cpp:751-753`、`kis_dlg_preferences.cc:1765, 1833, 3291` | Xiaomi のスタイラスが報告する誤った履歴を無視し、線のギザギザを減らす | 偽（Android も含む） | なし。設定のチェックボックスも出ない |
| `KRITA_QT_HAS_ANDROID_INPUT_PLATFORM_DATA_SOFT_INPUT_ADJUST_NOTHING` | `Qt::ANDROID_INPUT_PLATFORM_DATA_SOFT_INPUT_ADJUST_NOTHING`（`CMakeLists.txt:879-885`） | `plugins/tools/svgtexttool/SvgTextCursor.cpp:1081, 1091` | キャンバス上のテキスト入力で、ソフトキーボードが出たときに窓全体が上に動くのを止める | 偽（Android も含む） | なし。入力中に窓が動くのを受け入れるか、LibrePaint 側で入力の位置を調整する |
| `KRITA_QT_HAS_XKB_CONTEXT_IN_NATIVE_INTERFACE` | `QNativeInterface::QWaylandApplication::xkbContext`（`plugins/platforms/wayland/CMakeLists.txt:25`） | `plugins/platforms/wayland/KisWaylandKeyboardWatcher.cpp:21-46` | Wayland で Qt と共有する xkb のコンテキストを公開インターフェースから取る | 偽 | 偽のときは `QWaylandIntegration::display()->xkbContext()` という非公開の経路に退避する。現在の Linux はこの退避で動いている。機能は失わないが、Qt の内部 API への依存が残る |
| `KRITA_QSCROLLER_PATCH` | `QScroller` のヘッダーの印（`CMakeLists.txt:869-877`） | 検出だけ。本体の参照はない | キネティックスクロールをウィジェットが止められるようにする改変の有無 | 偽 | なし（LibrePaint はこの API を使っていない） |
| `HAVE_HDR` | Windows で `QOpenGLWidget::setTextureColorSpace` の有無（`CMakeLists.txt:784-792`）、Qt5 では色空間の列挙（`:765-773`） | `plugins/dockers/smallcolorselector/KisGLImageWidget.cpp:65-66`、`plugins/dockers/lut/lutdocker_dock.cpp:56, 244, 252`、`libs/ui/opengl/kis_opengl.cpp:813, 839, 920, 1025`、`libs/ui/opengl/KisScreenInformationAdapter.cpp:152, 158`、`libs/ui/opengl/KisOpenGLModeProber.cpp:32, 84, 276, 339`、`libs/ui/dialogs/kis_dlg_preferences.cc:2170, 2338, 2389` | HDR の画面（scRGB、Rec.2020 PQ）を選び、10bit / 16bit のサーフェスと色変換を使う。HDR の設定ページを出す | 偽 | 8bit の sRGB だけになる（Wayland の画面色管理を使う場合は 10bit まで）。HDR の設定ページは無効になる |
| `HAVE_QMDIAREA_ALWAYS_SHOW_SUBWINDOW_TITLE` | `QMdiArea::AlwaysShowSubwindowNameInTitleBar`（`CMakeLists.txt:817-824`） | `libs/application/ui/workspace/KisMainWindow.cpp:251-253` | タブ表示でも、子ウィンドウ（文書）の名前を窓の題名に出す | 偽 | 浮いた子ウィンドウのときだけ名前が出る。タブ表示の題名は `KisMainWindow` 側で `subWindowActivated` などに合わせて設定する（`f786da6451` の代替） |
| `QT5_HAS_WINTAB_SWITCH` | Qt5 の `Qt::AA_MSWindowsUseWinTabAPI`（`CMakeLists.txt:735-755`） | `krita/main.cc:896, 1144`、`libs/ui/dialogs/kis_dlg_preferences.cc:1748, 1792, 3268` | Qt5 で Wintab と Windows Ink のどちらを使うかを切り替える | Qt6 では評価されない | Qt6 の Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）では不要。ただし Qt6 で `useWin8PointerInput` を実際に適用するコードが別にあるかを確認する |

### 2.2 直接呼ぶフォーク固有の API と列挙

| API・列挙 | 使用箇所 | 使い方 | 正式版での代わり |
| --- | --- | --- | --- |
| `QOpenGLWidget::setTextureColorSpace` | `plugins/dockers/smallcolorselector/KisGLImageWidget.cpp:66` | 小さな色選択器のテクスチャーに画面の色空間を設定する | なし。`HAVE_HDR` が偽になるので呼ばれない。HDR を残すなら色変換を LibrePaint 側に作る |
| `QRhi::isLastFrameCompletedOnGPU` | `CMakeLists.txt:711`（検出だけ） | 前のフレームが GPU で完了したかを知る API の有無で、Qt の更新の圧縮の有無を判定する | なし。検出が偽になり、`KisCanvas2` の圧縮に切り替わる |
| `QMdiArea::AlwaysShowSubwindowNameInTitleBar` | `libs/application/ui/workspace/KisMainWindow.cpp:252` | タブ表示でも文書名を窓の題名に出す | なし。`KisMainWindow` で題名を更新する |
| `QPlatformScreen::densityAdjustment` / `setDensityAdjustment` | `libs/ui/canvas/KisAndroidScaling.cpp:148-150` | Android の画面密度を実行時に変える | なし。起動時の `QT_SCALE_FACTOR` などに限る |
| `QCoreApplication::setKritaAttribute` と `Qt::KRITA_QATTRIBUTE_ANDROID_*` | `libs/application/ui/orchestration/KisApplication.cpp:720, 736, 753` | Android のスタイラスの回避策を Qt のプラットフォーム統合に伝える | なし。LibrePaint 側のイベントフィルターで代替するか、失う |
| `Qt::ANDROID_INPUT_PLATFORM_DATA_SOFT_INPUT_ADJUST_NOTHING` | `plugins/tools/svgtexttool/SvgTextCursor.cpp:1081, 1091` | ソフトキーボードが出たときの窓の移動を止める | なし。窓の移動を受け入れるか、入力の位置を調整する |
| `QNativeInterface::QWaylandApplication::xkbContext` | `plugins/platforms/wayland/KisWaylandKeyboardWatcher.cpp:22, 37` | Wayland の xkb コンテキストを Qt と共有する | 偽のときは `QWaylandIntegration::display()->xkbContext()` に退避する（現在の Linux はこちら） |

`QSurfaceFormat::colorSpace`、`QOpenGLWidget::frameSwapped`、`QWindow::requestUpdate`、
`QScreen::refreshRate` は正式版 Qt にもある。フォーク固有の API として数えない。

### 2.3 フォークだけが読む環境変数

| 環境変数 | 設定する箇所 | 読む側（フォークの改変） | 正式版での動作 | 失われる動作 |
| --- | --- | --- | --- | --- |
| `QT_WINTAB_DESKTOP_RECT` | `libs/ui/dialogs/KisDlgCustomTabletResolution.cpp:149-151`、`krita/main.cc:900` | Wintab の対応範囲を指定された矩形にする（`051139ed72`） | 無視する | タブレットの対応範囲の設定（任意の矩形）が効かない。Surface Pro 5 のような、内蔵タブレットが主画面だけに対応する機器で座標がずれるおそれ |
| `QT_IGNORE_WINTAB_MAPPING` | `KisDlgCustomTabletResolution.cpp:155-161`、`krita/main.cc:901` | Wintab の対応範囲を仮想画面全体として扱う（`051139ed72`） | 無視する | 「仮想画面全体に合わせる」の設定が効かない |
| `QT_USE_PREMATURE_RESIZE_EVENTS` | `krita/main.cc:706-708` | ANGLE のリサイズ中の黒い筋を減らす（`1a543f2832` ほか） | 無視する | ANGLE を使わないので通常は関係しない。desktop の OpenGL でもリサイズのちらつきが残るかは未確認 |
| `QT_ANGLE_MANUALLY_UPDATE_SURFACE_SIZE` | `krita/main.cc:710-712` | ANGLE のサーフェスの大きさの更新を手動にする（`02216d26ba` ほか） | 無視する | 同上 |
| `QT_PREFILL_RHI_SURFACE` | `krita/main.cc:714-716` | ANGLE のサーフェスを先に埋める（`77d2b02daf` ほか） | 無視する | 同上 |
| `QT_BACKING_STORE_USE_FAST_QIMAGE_TRANSFER` | `krita/main.cc:738-740` | QImage のコピーを遅延させ、HiDPI の画面で 15ms 程度かかるコピーを減らす（`fb923e4a97`） | 無視する | CPU のコピーが増える可能性。L2（Windows の遅れを直す）の「CPU、電力を悪化させない」の確認対象 |
| `QT_FRAME_RATE_OVERRIDE` | `krita/main.cc:742-745` | Qt のフレームレート圧縮の上限を `fpsLimit` にする（`71fd8157aa`） | 無視する | Qt 側の圧縮がないので関係しない。`KisCanvas2` の圧縮は `fpsLimit` を使う |
| `QT_FRAME_COMPRESSION_MODE` | `krita/main.cc:754-755` | Qt の圧縮を切る（H6（フォークの更新の間引きの歯止め）の実験用） | 無視する | H6（フォークの更新の間引きの歯止め）の切り替えは正式版では意味を持たない。L1（Windows のペン入力から表示までの約 50ms の遅れの原因を仮説ごとに数値で特定する）の切り替えの削除条件に含める |
| `QT_DISABLE_ACCESSIBILITY` | `krita/main.cc:725-733` | Windows のアクセシビリティを切る（`9dc7c77358`） | 無視する | `DisableAccessibilityInQt` の設定が無効になる。動作は変わらないが、設定項目が効かない |
| `QT_GLX_SWAP_INTERVAL_SYNC_WORKAROUND` | 参照なし | X11 の交換間隔の再適用（`5874f1f131`） | 正式版 Qt 6.11.3 の QTBUG-147854 の修正で不要 | なし（`nix/qt/overlay.nix` のコメントと DECISIONS.md 2026-11-07） |

### 2.4 構築の分岐

| 箇所 | 内容 | Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）での扱い |
| --- | --- | --- |
| `nix/windows/default.nix:5` | `useQtFork ? true` | 既定を `false` にするか、引数ごと消す |
| `nix/windows/krita.nix:145-152` | `dimula73/qtbase` の `for-krita/6.11.3`（`d918f368da`）を qtbase にする | 削除し、`nix/qt/overlay.nix` の公式ソースだけにする |
| `nix/windows/krita.nix:99, 653` | ANGLE（`google-angle.nix`）を依存に加える | `useQtFork = false` では既に加えない。Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）で完全に削除する |
| `nix/windows/krita.nix:794` | `-DHAVE_HDR=ON` | 正式版では渡さない。HDR を残すなら LibrePaint 側の実装に置き換える |
| `nix/windows/krita.nix:153-155` | `qwindowseglcontext.cpp` の `VersionHelpers.h` の綴りを直す | ANGLE の経路を消すと不要 |
| `nix/windows/default.nix:12` | `incrementalPlatform = "windows-upstream-qt"` | Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の確認用の入口として残す |

### 2.5 `docs/perf/qt-fork-patches.md` の判定との対応

92 件の内訳は、不要 40 件、正式版で解決済み 6 件、LibrePaint 側で代替 4 件、必要 11 件、
保留 31 件である（2026-11-10 に画面更新の間引きの 7 件を不要にし、`d850335b2d` は 2026-11-09 に
不要から保留へ見直した）。上の一覧との対応は次のとおり。

| まとまり（件数） | ソース側の入口 | Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）ですること |
| --- | --- | --- |
| Windows の ANGLE と OpenGL ES の経路（18：保留 11、不要 7） | `QT_CONFIG(egl)`（`libs/ui/opengl/kis_opengl_canvas2.cpp:77, 526`）、`KisOpenGL::RendererOpenGLES`、`OpenGLRenderer` の既定 `angle`（`krita/main.cc:657`）、`QT_USE_PREMATURE_RESIZE_EVENTS` ほか、`nix/windows/krita.nix:99, 653` | ANGLE をやめるなら 11 件は不要になる。HDR の 2 件（`7ad4a8bc17`、`70086804f9`）は HDR の判断に従う。desktop の表示の正しさと CPU・電力を確認する |
| 画面更新の間引き（10：保留 3、不要 7） | `KRITA_QT_HAS_UPDATE_COMPRESSION_PATCH`、`main.cc:737`、`kis_canvas2.cpp` の圧縮、`kis_opengl_canvas2.cpp` の飛ばし | Qt の圧縮を外し、`KisCanvas2` の 1 系統にする。部分再描画（`e121cb08e5`）、fast transfer（`fb923e4a97`）、クリップ（`f1cf04e958`）の 3 件は個別に確認する |
| HDR と色変換（4：保留 4） | `HAVE_HDR`、`QOpenGLWidget::setTextureColorSpace`、`KisSurfaceColorSpaceWrapper` の色空間 | HDR を残すなら色変換を LibrePaint 側に作る。やめるなら `HAVE_HDR` の分岐と設定ページを外す。利用者の判断が要る |
| Windows のペン入力（14：必要 9、保留 2、不要 3） | `KRITA_QT_HAS_ENTER_LEAVE_PATCH`、`QT_WINTAB_DESKTOP_RECT` / `QT_IGNORE_WINTAB_MAPPING`、タッチの受理（`a60ed01a25`） | 必要 9 件を公式に取り込むか、LibrePaint 側で作り直す。Wintab の相対モードとタッチは実機で確認する。Wintab の対応範囲は Qt6 で実際に使われているかを先に確認する（2.6） |
| Wayland のペン入力とウィンドウ（10：正式版で解決済み 3、保留 4、不要 3） | ソース側の分岐はない（Qt の内部の修正）。`KRITA_QT_HAS_XKB_CONTEXT_IN_NATIVE_INTERFACE` はキーボード用で別 | 正式版で解決済みの 3 件はそのまま。保留 4 件は Wayland で実際のペンを使って確認する |
| キーボードとショートカット（12：必要 2、LibrePaint 側で代替 3、保留 5、不要 2） | `KRITA_QT_HAS_UNBALANCED_KEY_PRESS_RELEASE_PATCH`、`KisShortcutMatcher` の ShortcutOverride の扱い | 代替 3 件は Windows でも回避コードを有効にする。必要 2 件は公式に取り込むか、`KisShortcutMatcher` 側で対処する。日本語配列で確認する |
| 不具合とクラッシュの修正（13：正式版で解決済み 2、保留 2、不要 9） | なし（`d850335b2d` はクリップボード、`8dadf099b8` はドック） | Chrome / Edge への貼り付けと、ドックを外して戻す操作を確認する |
| その他（11：LibrePaint 側で代替 1、正式版で解決済み 1、不要 9） | `HAVE_QMDIAREA_ALWAYS_SHOW_SUBWINDOW_TITLE`、`QT_DISABLE_ACCESSIBILITY`、`KRITA_QSCROLLER_PATCH`（検出だけ） | 窓の題名を `KisMainWindow` 側で設定する。アクセシビリティの設定項目の扱いを決める。`QScroller` の検出は消す |

### 2.6 ソースを読んで分かった確認点

- `KisDlgCustomTabletResolution`（Wintab の対応範囲のダイアログ）は、設定の「タブレット」ページの
  `TabletSettingsTab::slotResolutionSettings()`（`libs/ui/dialogs/kis_dlg_preferences.cc:1869-1876`）から
  開ける。`accept()` は `kritadisplayrc` を書き、その場で `applyConfiguration()` を呼んで
  `QT_WINTAB_DESKTOP_RECT` / `QT_IGNORE_WINTAB_MAPPING` を設定する。しかし、起動時に
  `kritadisplayrc` を読んで同じ環境変数を設定するのは Qt5 の `krita/main.cc:895-910` の分岐だけである。
  したがって Qt6 では、ダイアログで変更したセッションでは効くが、再起動すると保存された設定が
  入力経路に反映されない。Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）で
  この設定を残すなら、Qt6 の起動時に `kritadisplayrc` を読む処理を LibrePaint 側に足す必要がある
  （約 10 行）。
- Qt6 で Wintab と Windows Ink のどちらを使うかを切り替えるコードは、起動時の
  `Qt::AA_MSWindowsUseWinTabAPI`（`krita/main.cc:896, 1144`）だけが Qt5 の分岐にある。設定ページの
  保存時は、Qt6 のネイティブインターフェース
  `QNativeInterface::Private::QWindowsApplication::setWinTabEnabled()`
  （`kis_dlg_preferences.cc:3270-3278`）で切り替える。このインターフェースは正式版 Qt 6.11.3 にも
  ある（`src/gui/platform/windows/qwindowsnativeinterface.h`）。したがって Wintab と Windows Ink の
  切り替え自体は Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の前後で同じに
  動く。
- Android の 5 つのマクロは、現在の Android の構築が使う正式版 Qt 6.11.3 では偽になる。Q4（フォークを
  やめ、全プラットフォームで正式版の同じ Qt にする）で新たに失う機能ではない。
- `HAVE_HDR` は Windows だけで検査する。Linux と macOS は、Wayland の画面色管理を使う場合を除き、
  8bit の sRGB である。HDR の確認は外部の HDR 画面が要る。


### 2.7 ペンに必要な 9 件の取り込み方法の見積もり（2026-11-11）

2026-11-11 08:05 の監査役の「次にすること」に従い、`docs/perf/qt-fork-patches.md` の「必要」のうち
Windows のペン入力の 9 件（c41d66281e、df3a678309、051139ed72、33a49f25d1、f5a75c4071、
41adadd8db、0027358f20、8f8eabfb72、c132390390）について、次の 2 つの方法を 1 件ずつ見積もった。
コードは変えていない。

- (P) 正式版 Qt 6.11.3 に、その改変だけを `nix/qt/overlay.nix` のパッチとして Windows で当てる。
- (L) LibrePaint 側（`libs/input/ui/kis_input_manager.cpp`、`KisShortcutMatcher`、
  `KisInputEventSuppressor`、Windows のネイティブイベントフィルターなど）で同じ動作を作る。

#### 見積もりの方法

- 正式版 Qt 6.11.3 のソースは、`nix/qt/srcs.nix` の `qtbase`
  （`qtbase-everywhere-src-6.11.3.tar.xz`、sha256 `13wg5z37nm6ana0kgi83d4f191m3v9a47r3a8ifmm54lnl80gpj6`、
  ファイルの sha256 `46de0710b594945a5d446ae44354daa386141c6903c53781b2ca547bc62f8f8f`）を展開して使った。
- 各改変のパッチは `https://github.com/dimula73/qtbase/commit/<sha>.patch` から取得した
  （2026-11-11）。「単独」は他の改変を当てていない 6.11.3 にその 1 件だけを当てた結果、
  「累積」はブランチの順（c41d66281e → df3a678309 → 051139ed72 → 33a49f25d1 → f5a75c4071 →
  41adadd8db → 0027358f20 → 8f8eabfb72 → c132390390）に当てた結果である。どちらも
  `patch -p1 --dry-run` と `git apply --check` で確かめた。
- 行数はパッチの追加行と削除行の合計（テストは含まない。9 件のパッチにテストの変更はない）。
- コンパイル、リンク、実機の動作は未確認である。`patch --dry-run` が通ることは、そのまま
  ビルドが通ることを意味しない。

#### (P) 正式版 Qt 6.11.3 へのパッチ

| コミット | 内容 | 変更行（追加/削除） | 単独 | 累積 | 主なファイル | Qt 更新ごとの保守 |
| --- | --- | --- | --- | --- | --- | --- |
| c41d66281e | タブレットが窓の非クライアント領域に入ったときの Enter/Leave | 38（+21/−17） | 当たる | 当たる | `qwindowspointerhandler.cpp/.h` | 高。ポインターハンドラは変更が多い |
| df3a678309 | タブレットの進入時に `WM_MOUSELEAVE` の購読を外す | 2（+0/−2） | 当たらない（c41d が必要） | 当たる | `qwindowspointerhandler.cpp` | 高。c41d と同じ箇所 |
| 051139ed72 | Wintab の対応範囲を `lcSys{Org,Ext}` から取る | 101（+98/−3） | 当たる | 当たる | `qwindowstabletsupport.cpp/.h` | 中。Wintab の変更は少ない |
| 33a49f25d1 | Windows Ink の押下をウィジェットへ届ける | 103（+92/−11） | 当たらない（c41d が必要） | 当たる | `qguiapplication.cpp`、`qwindowsysteminterface.cpp`、`qwindowspointerhandler.cpp` | 高。GUI のカーネルとポインターハンドラ |
| f5a75c4071 | 受理した押下のストローク全体の合成マウス | 39（+35/−4） | 当たらない（c41d と 33a4 が必要） | 当たる | `qwindowspointerhandler.cpp/.h` | 高。33a4 の後 |
| 41adadd8db | 受理した `QTabletEvent` の Enter/Leave の合成 | 12（+12/−0） | 当たる | 当たる | `qwidgetwindow.cpp` | 中。ウィジェットのカーネル |
| 0027358f20 | Enter/Leave パッチの存在を示すヘッダーの印 | 4（+4/−0） | 当たる | 当たる | `qevent.h` | 低。41ad と一緒に当てる |
| 8f8eabfb72 | WinTab の合成マウスの二重を止める | 4（+4/−0） | 当たる | 当たる | `qwindowscontext.cpp` | 低 |
| c132390390 | `WM_POINTERLEAVE` の Leave の戻し | 2（+0/−2） | 当たる | 当たる | `qwindowspointerhandler.cpp` | 中。上流の変更に追随する |

- 合計は 305 行（+266/−39）、9 ファイルである。3 件（df3a678309、33a49f25d1、f5a75c4071）は
  単独では当たらず、前の改変（c41d66281e、または c41d66281e と 33a49f25d1）が要る。
  9 件をブランチの順に当てると、すべて `git apply --check` を通る。
- 33a49f25d1 は `qguiapplication.cpp` と `qwindowsysteminterface.cpp`、41adadd8db は
  `qwidgetwindow.cpp`、0027358f20 は `qevent.h` を変える。これらは Windows 専用のファイルでは
  ないため、`nix/qt/overlay.nix` で Windows（MinGW）のときだけ当てる条件が要る（約 10 行）。
  条件を付けずに当てると、Linux、macOS、Android の Qt の動作も変わる。
- その結果、Windows の Qt は Linux、macOS と版は同じでも、9 件のパッチの分だけ構成が異なる。
  Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の「正式版の同じ Qt」は、
  「同じ版の正式版に、Windows だけのパッチを当てた Qt」になる。
- Qt を更新するたびに 9 件の再ベースと `patch --dry-run` が要る。`qwindowspointerhandler.cpp` が
  変わると、c41d66281e、df3a678309、33a49f25d1、f5a75c4071、c132390390 の 5 件（184 行）を
  まとめて直す。見込みは、マイナー更新で半日〜1日、パッチ更新で2〜4時間である。ビルドと実機の
  確認は別に 1 回 1〜2 時間（正式版 Qt の Windows の差分構築は 36 分 5 秒の実績）かかる。

#### (L) LibrePaint 側の実装

| コミット | LibrePaint の実装箇所と方法 | 実装行数（テスト別） | Qt の内部に踏み込む度合い | 176a8164a3（Enter の復帰）で足りるか |
| --- | --- | --- | --- | --- |
| c41d66281e | `KisInputManager` か `KisApplication` の `QAbstractNativeEventFilter` で `WM_NCPOINTER*` と `WM_NCMOUSEMOVE` を見て、キャンバスへ Enter/Leave を送り、カーソルを戻す | 80〜120 | 中。Win32 のメッセージとカーソルに依存する。Qt の内部 API は使わない | 足りない。押下の復帰だけでは非クライアント領域のカーソルは戻らない |
| df3a678309 | `KisInputManager::eventFilter` の `QEvent::Leave` を、タブレットが近接中か押下中は無視する（共通ガードの一部） | 10〜20 | 低。公開の Qt イベントだけ | 一部。押下時は戻るが、ホバーの途切れは共通ガードで補う |
| 051139ed72 | Wintab の生の座標が `QTabletEvent` にないため、自前の Wintab コンテキスト（`WTOpen`、`WT_PACKET`）を持つか、設定を失う | 200〜300（機能を残す場合） | 高。Wintab API と Qt の Wintab 経路の二重化 | 足りない。P を推奨する |
| 33a49f25d1 | キャンバスの HWND に `SetGestureConfig` を設定して長押し・タップのシステムジェスチャーを切り、受理を WndProc の戻り値に依存しない形にする | 30〜60 | 中。Win32 のウィンドウとジェスチャー | 足りない。176a8164a3 は action の復帰だけで、システムジェスチャーは止めない |
| f5a75c4071 | 受理したタブレットの押下・移動・解放に `QMouseEvent` を作ってウィジェットへ送る | 60〜100（必要なら） | 高。Qt のイベント配送の再実装 | 足りないが、キャンバスはタブレットを直接使うため優先度は低い |
| 41adadd8db | 共通ガードでキャンバスの Enter/Leave を戻す。全ウィジェットに同じ合成をするならアプリのイベントフィルターを足す | 20〜30（キャンバス）、+40〜60（全ウィジェット） | 低〜中。公開の Qt イベントだけ | 押下時は足りる。ホバーは共通ガードで補う |
| 0027358f20 | L では不要。CMake の検出を偽のままにし、LibrePaint の既存の分岐を使う | 0 | なし | 関係しない。P の 41adadd8db と一緒に当てるときだけ 4 行 |
| 8f8eabfb72 | `KisInputEventSuppressor` の `setSuppressSyntheticMouseEvents` を Windows の WinTab でも使う、または近接中の合成マウスを落とす | 20〜40 | 低。既存の抑止の枠組みを使う。WinTab の OS レベルのマウスは `QMouseEvent::source()` が合成と印されないため、近接の状態で落とす判定を足す | 足りない |
| c132390390 | 共通ガードのタッチ分岐で、最後の指が離れたときの Leave を無視する | 10〜20 | 低。公開の Qt イベントだけ | 足りない。176a8164a3 は押下の復帰だけ |

- df3a678309、c132390390、41adadd8db の 3 件は、1 つの「タブレット近接・進入のガード」で
  まとめられる。タブレットの進入（`TabletEnterProximity`、`TabletMove`、`TabletPress`）で
  `containsPointer` を立て、近接中や押下中の `QEvent::Leave` を無視し、離れた後に
  `QCursor::pos()` がキャンバスの外なら Leave を確定する。共通ガードは 60〜90 行、テストは
  80〜120 行である。
- 共通ガードと c41d66281e、33a49f25d1、8f8eabfb72 を L で作る範囲は、約 190〜310 行である。
  f5a75c4071 と 051139ed72 も L で作ると、約 500〜700 行になる。
- L の共通ガードは公開の Qt イベントだけを使うため、Qt の内部 API への依存は増えない。
  ただし、Windows のメッセージを使う c41d66281e と 33a49f25d1 は `#ifdef Q_OS_WIN` の
  ネイティブコードになり、Linux、macOS では動かさない。共通ガードは全プラットフォームで
  動くため、Linux と macOS のホバーとタッチの回帰を確認する必要がある。
- 051139ed72 は、Qt6 では起動時に保存された設定が入力経路に反映されない（ダイアログで変更した
  セッションでは効く。2.6）。P を当てて機能を残す場合も、Qt6 の起動時に `kritadisplayrc` を
  読んで `QT_WINTAB_DESKTOP_RECT` / `QT_IGNORE_WINTAB_MAPPING` を設定する約 10 行が要る。
  機能を残さないなら、この件は Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt に
  する）で新たに失うものはない。

#### 見積もりから分かること（判定の材料）

- P は 9 件で 305 行と小さいが、Qt の内部（`QWindowsPointerHandler`、`QWindowsTabletSupport`、
  `QWidgetWindow`）に依存し、Windows だけパッチを当てた Qt を Qt の更新ごとに保守する。
  Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の「同じ Qt」は、
  パッチの分だけ完全にはそろわない。
- L は Qt の内部 API に依存しないが、Windows のメッセージ、Wintab、ジェスチャーの再実装が
  要る。df3a678309、c132390390、41adadd8db は 1 つの共通ガードにまとめられ、176a8164a3 と
  合わせてキャンバスのペン操作を保てる見込みである。
- 1 件ずつの候補（監査役の判定前の材料）：
  - c41d66281e：P（38 行）か L（80〜120 行）。カーソルの見た目だけなら L を後回しにできる。
  - df3a678309：L の共通ガード（P は c41d66281e が前提）。
  - 051139ed72：機能を残すなら P（101 行）＋L（約 10 行）。残さないなら現状維持。
  - 33a49f25d1：L の `SetGestureConfig`（30〜60 行）を先に試し、足りなければ P（103 行）。
  - f5a75c4071：L はキャンバスに不要と見る。非キャンバスのウィジェットで必要になったときだけ
    P（39 行）か L（60〜100 行）。
  - 41adadd8db：L の共通ガード（20〜30 行）。P（12 行）は 0027358f20 と一緒に当てるときだけ。
  - 0027358f20：P の 41adadd8db を当てないなら不要。
  - 8f8eabfb72：L（20〜40 行）か P（4 行）。
  - c132390390：L の共通ガード（10〜20 行）か P（2 行）。


## 3. 正式版 Qt にしたときに失われる機能（利用者に見える違い）

「今」はフォーク版の Windows、「後」は正式版 Qt 6.11.3 の Windows である。Linux、macOS、
Android、iOS は「今」も正式版なので、この表の「後」と同じである。

| 機能 | 今（フォーク版） | 後（正式版） | 利用者に見える違い | 根拠・出典 | Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の候補 |
| --- | --- | --- | --- | --- | --- |
| ANGLE（OpenGL ES を Direct3D に変換する経路） | Windows の既定。`OpenGLRenderer=angle` | ANGLE がない。desktop の OpenGL に落ちる | GPU とドライバーによっては起動しない、または表示が違うおそれ。遅れは desktop の方が小さい（H7（フォークなし・ANGLE を使わない OpenGL）の前段で −21.8 / −21.2ms） | Krita Manual「Display Settings」の ANGLE の説明（https://docs.krita.org/en/reference_manual/preferences/display_settings.html）、DECISIONS.md 2026-11-10、`nix/windows/krita.nix:99, 653` | desktop を既定にする案を推奨。表示の正しさ、HDR、Wintab、CPU・電力を確認してから決める |
| ANGLE のリサイズ・ダイアログのちらつきの回避 | `QT_USE_PREMATURE_RESIZE_EVENTS` ほかを設定 | 環境変数は無視される。ANGLE がないので通常は関係しない | desktop の OpenGL でリサイズやダイアログのちらつきが出るかは未確認 | `1a543f2832`、`02216d26ba`、`77d2b02daf` のコミットメッセージ（https://github.com/dimula73/qtbase/commit/1a543f2832） | desktop でリサイズとダイアログを確認する |
| HDR（10bit、scRGB、Rec.2020 PQ） | HDR の設定ページがあり、HDR の画面を選べる | 8bit の sRGB だけ（Wayland の画面色管理を使う場合は 10bit まで）。HDR の設定ページは無効 | HDR の画面で、明るい色と階調の見え方が SDR になる | Krita Manual「HDR Display」（https://docs.krita.org/en/reference_manual/hdr_display.html）、`HAVE_HDR` の使用箇所、`7ad4a8bc17`、`70086804f9` | 利用者の判断。残すなら色変換を LibrePaint 側に作る。やめるなら設定ページと `HAVE_HDR` の分岐を外す |
| Wintab の対応範囲の設定 | 「タブレットが提供する情報」「仮想画面全体」「任意の矩形」を選べる。ただし Qt6 では起動時に保存された設定を適用するコードが Qt5 の分岐にしかなく、再起動すると設定が戻る（ダイアログで変更したセッションでは効く。2.6） | 環境変数が無視され、設定が効かない | Surface Pro 5 のような内蔵タブレットの機器や複数画面で、ペンの位置がずれるおそれ。Qt6 で設定自体が使われていないなら、Q4 で新たに失うものはない | Krita Manual「Tablet Settings」の Advanced Tablet Settings（https://docs.krita.org/en/reference_manual/preferences/tablet_settings.html）、`051139ed72`、`KisDlgCustomTabletResolution.cpp:149-161` | 機能を残すなら、P（`051139ed72`、101 行）と Qt6 の起動時の読み込み（約 10 行）。残さないなら現状維持（Q4 で新たに失わない） |
| Wintab の相対モード | フォークは相対モードを切っている（`7bb46828ae`） | 正式版は相対モードを切らない可能性がある | Wintab のドライバーによっては、ペンの座標が飛ぶ、またはカーソルが相対移動になるおそれ | `7bb46828ae`（https://github.com/dimula73/qtbase/commit/7bb46828ae）、`docs/perf/qt-fork-patches.md` の「保留」 | 実際の Wintab ドライバーで確認する。Surface の合成入力は WM_POINTER なので、この確認には別のタブレットが要る |
| Windows Ink の押下と Enter/Leave | 押下がツールに届き、長押しが右クリックにならず、ホバーの状態が途切れない | 押下が届かない、長押しが右クリックになる、ホバーが途切れるおそれ | キャンバスが描き直されない、線が始まらない、メニューの操作が誤る | `33a49f25d1`、`f5a75c4071`、`41adadd8db`、`8f8eabfb72`、`c132390390`、`0027358f20`、KDE bug 409613 / 424319 | 必要 9 件を公式に取り込むか、LibrePaint 側で Enter の復帰（`176a8164a3`）と押下の処理を作り直す |
| タッチの受理 | ウィジェットがタッチを受理し、Windows のグローバルなジェスチャーや合成マウスを抑えられる | 受理の制御ができない | タッチのパン・ズームと描画が、ペンの操作と競合するおそれ | `a60ed01a25`（https://github.com/dimula73/qtbase/commit/a60ed01a25） | Surface のタッチで確認する。失ってよいかの判断は利用者 |
| クリップボードの CF_DIB の回避 | アルファ付きの QImage で CF_DIB を出さず、CF_PNG と CF_DIBV5 を出す | CF_DIB も出る | Chrome と Edge に貼ると、画像が壊れて見える | `d850335b2d`（https://github.com/dimula73/qtbase/commit/d850335b2d）、KDE bug 449624 / 464839 | Windows でコピーして Chrome と Edge に貼り、壊れるなら LibrePaint 側で `setImageData` をやめるか PNG だけにする |
| タブ表示の文書名 | タブ表示でも窓の題名に文書名が出る | 浮いた子ウィンドウのときだけ文書名が出る | タブで複数の文書を開いたとき、窓の題名から文書が分からない | `f786da6451`（`QMdiArea::AlwaysShowSubwindowNameInTitleBar` を追加するコミット、https://github.com/dimula73/qtbase/commit/f786da6451）と `KisMainWindow.cpp:251-253` | `KisMainWindow` でタブの切り替えに合わせて題名を設定する（数行） |
| Android のインターフェースの倍率 | Android で起動時に倍率を選べる（Krita の Android 版） | LibrePaint の Android 版は現在もこの機能がない（マクロが偽） | LibrePaint の Android 版では、画面の密度に合わせた倍率の調整ができない | Krita Manual「Interface Scaling」の Android Interface Scale（https://docs.krita.org/en/reference_manual/interface_scaling.html）、`KRITA_QT_HAS_ANDROID_QPLATFORMSCREEN_DENSITY_ADJUSTMENT` | Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）では新たに失わない。必要なら別の作業で作る |
| Android のスタイラスの回避（Xiaomi、OnePlus） | Krita の Android 版は Page Up / Page Down、F21、履歴の誤りを回避する | LibrePaint の Android 版は現在もこの回避がない | Xiaomi のスタイラスで線がギザギザになる、ボタンが誤動作するおそれ | Krita Manual「Tablet Settings」の Xiaomi の記述（https://docs.krita.org/en/reference_manual/preferences/tablet_settings.html）、`KRITA_QT_HAS_ANDROID_*` | Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）では新たに失わない。必要なら別の作業で作る |
| Android のソフトキーボードの窓の移動 | キャンバス上のテキスト入力で窓が動かない | LibrePaint の Android 版は現在も窓が動く | テキスト入力中に画面が上に動く | `KRITA_QT_HAS_ANDROID_INPUT_PLATFORM_DATA_SOFT_INPUT_ADJUST_NOTHING`、`SvgTextCursor.cpp:1081` | Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）では新たに失わない。必要なら別の作業で作る |
| 非ラテン配列のショートカット | Windows でも配列に合ったショートカットが一致する | 一部のショートカットが一致しない、KeyPress と KeyRelease が釣り合わない | 日本語配列で Ctrl+Tab や AltGr のショートカットが効かないおそれ | `82d2864a8a`、`dc27f27d8e`、`747423036c`、`f8a61ebc5b`、`21d400db4f`、KDE bug 409613 / 424319 | Windows でも `KisInputManager` の回避コードを有効にし、日本語配列で確認する |
| Wayland のペンのカーソルとツールの切り替え | 正式版で解決済みの 3 件は同じ。保留 4 件は未確認 | 正式版でも保留 4 件の修正がない | Wayland でペンのカーソルが更新されない、複数のツールの切り替えが乱れるおそれ | `d2aa04db1a`、`b069509b06`、`483ceed535`、`3b6b57d0b7`、`docs/perf/qt-fork-patches.md` | Linux 開発機にタブレットがないため、実機で確認する |
| 部分再描画の範囲とクリップ | 部分更新の dirty rect と GUI テクスチャの切り抜きが正しい | 部分更新で artifacts が出るおそれ、または全体を描き直す | 4K の画面で、キャンバスの一部の更新が重い、または表示が乱れる | KDE bug 413504（`e121cb08e5`）、KDE bug 517248（`f1cf04e958`） | 整数倍率の部分更新を残すなら、この 2 件を LibrePaint 側で作るか、部分更新を切る |
| fast transfer | QImage のコピーを遅延させる | 毎フレームのコピーが残る | HiDPI の画面で CPU の負荷が増える可能性 | `fb923e4a97`（コミットメッセージに最大 15ms とある） | CPU と電力を測る。悪化が 10% を超えたら小さな対策を検討する |
| Qt のフレームレート圧縮 | 表示 1 回あたりの描画が 1.05 回 | 1.9 回程度（`KisCanvas2` の圧縮だけでは輪郭の更新をまとめない） | 描画の回数が増え、CPU と電力が増える可能性。遅れは悪化しない見込み | DECISIONS.md 2026-11-11 の `paint_cycle` の行、`docs/perf/q4-canvas-updates.md` | 2026-11-11 07:30 の監査役の決定どおり、今は表示の周期に合わせる仕組みを実装しない。CPU と電力を測り、10% を超えたら設計を見直す |
| アクセシビリティを切る設定 | `DisableAccessibilityInQt` が効く | 環境変数が無視される | 設定項目が効かない（動作は変わらない） | `9dc7c77358`、`krita/main.cc:725-733` | 設定項目を残すなら LibrePaint 側で代替する。不要なら設定項目を外す |
| ドックのクラッシュの修正 | ドックのドッキング中の未初期化を直している | 修正がない | ドックを外して戻す操作で落ちるおそれ | `8dadf099b8`、`docs/perf/qt-fork-patches.md` の「保留」 | ドックを外して戻す操作を繰り返し、落ちないか確認する |

### 3.1 内部の性能と正しさ（利用者に見えにくい違い）

- `KRITA_QT_HAS_UPDATE_COMPRESSION_PATCH` が偽になると、`KisCanvas2` の圧縮が有効になる。これは
  Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の「1 系統」の到達点である。
  ただし、H1（先積みのフレーム数）=1 は ANGLE の EGL の経路にしか効かないため、Q4（フォークを
  やめ、全プラットフォームで正式版の同じ Qt にする）の後は H5（ホバーの間引き）が主な対策に
  なる（DECISIONS.md 2026-11-10）。
- `QT_BACKING_STORE_USE_FAST_QIMAGE_TRANSFER` と Qt の圧縮がない分、CPU と電力が増える可能性が
  ある。2026-11-11 07:30 の監査役の決定では、CPU と電力の増加が 10% を超えたら、表示の周期に
  合わせる設計か、より小さい案を実装するとしている。
- `HAVE_HDR` が偽でも、Wayland の画面色管理を使う Linux では 10bit のサーフェスが使える
  （`KRITA_USE_SURFACE_COLOR_MANAGEMENT_API`）。Windows の HDR とは別の経路である。
- `QScroller` の検出（`KRITA_QSCROLLER_PATCH`）は、本体が参照していない。Q4（フォークをやめ、
  全プラットフォームで正式版の同じ Qt にする）で検出だけを消せる。
- `HAVE_NO_QT_UPDATE_COMPRESSIO`（`libs/ui/canvas/kis_canvas2.cpp:1335`）は綴りが最後の `N` で
  切れており、定義されない。このため `updateCanvasToolOutlineWdg()` はフォーク版でも正式版でも
  `slotDoCanvasUpdate()` を直接呼ぶ。フォーク固有の API ではないが、Q4（フォークをやめ、全
  プラットフォームで正式版の同じ Qt にする）の「1 系統」を数えるときに、この分岐を
  「圧縮を使う経路」と数えないこと。

## 4. 他の編集ソフトの扱いと出典

Krita は LibrePaint の upstream であり、独立した編集ソフトの例としては弱い。ここでは、
Krita の公式マニュアルが利用者に約束している機能と、独立した編集ソフトである Adobe Photoshop の
公式ヘルプの記述を、比較の基準として引用する。GIMP などの扱いを足す場合は、この節に追記する。

| 項目 | 他の編集ソフトの扱い | 出典 |
| --- | --- | --- |
| ANGLE | Krita は Windows で Direct3D を使う ANGLE を選べる。ANGLE は OpenGL の呼び出しを Direct3D に変換し、Windows の一般的なドライバーとの互換性が高い | Krita Manual「Display Settings」の Canvas Graphics Acceleration（https://docs.krita.org/en/reference_manual/preferences/display_settings.html） |
| HDR | Krita は 4.2 から Windows と Linux（Wayland）で HDR の画像を画面に HDR として表示できる。Windows では Display の設定で優先するサーフェスを選ぶ | Krita Manual「HDR Display」（https://docs.krita.org/en/reference_manual/hdr_display.html） |
| 10bit（30-bit）の表示（独立した例） | Adobe Photoshop は、性能の設定に「30-bit display support」（10bit/色）の項目を持つ。GPU と画面が対応するときに 30bit の表示を使う | Adobe Photoshop Help「Configure Photoshop performance preferences」（https://helpx.adobe.com/photoshop/desktop/get-started/settings-and-preferences/configure-photoshop-performance-preferences.html） |
| Wintab と Windows Ink | Krita は Windows 8 以上で WinTab と Windows 8+ Pointer Input を選べる。WinTab は Wacom に推奨、Windows 8+ Pointer Input は N-Trig（Surface）などに推奨 | Krita Manual「Tablet Settings」の Tablet Input API（https://docs.krita.org/en/reference_manual/preferences/tablet_settings.html） |
| Wintab と Windows Ink（独立した例） | Adobe Photoshop は、Windows 10 では Windows Ink を既定の入力とし、WinTab を従来のドライバーの経路として扱う。ペンの入力方式を利用者が選ぶ必要がある | Adobe Photoshop Help「Tablet support FAQ」（https://helpx.adobe.com/photoshop/kb/tablet-support-faq-photoshop.html） |
| Wintab の対応範囲 | Krita は複数画面や画面付きタブレットで、タブレットが提供する情報、仮想画面全体、任意の矩形を選べる。ずれの調整に使う | Krita Manual「Tablet Settings」の Advanced Tablet Settings for WinTab（同上） |
| Android のインターフェースの倍率 | Krita は Android で起動時にインターフェースの倍率を選べる。Settings ▸ Change Interface Scale で戻せる | Krita Manual「Interface Scaling」の Android Interface Scale（https://docs.krita.org/en/reference_manual/interface_scaling.html） |
| Android のスタイラスの回避 | Krita は Xiaomi のスタイラスの Page Up / Page Down と履歴の誤りを回避する設定を Android で提供する | Krita Manual「Tablet Settings」の Xiaomi の記述（同上） |
| クリップボードの CF_DIB | Chrome はアルファ付きの CF_DIB を壊れた画像として貼るため、Qt のフォークは CF_DIB を出さない。KDE bug 449624 と 464839 | `d850335b2d` のコミットメッセージ（https://github.com/dimula73/qtbase/commit/d850335b2d）、https://bugs.kde.org/show_bug.cgi?id=449624、https://bugs.kde.org/show_bug.cgi?id=464839 |

## 5. 判定の候補（効果の大きい順）

1. Windows のペンの「必要」9 件（Wintab の対応範囲、Windows Ink の押下、Enter/Leave、合成マウス）
   を、公式に取り込むか、LibrePaint 側で作り直すか。外すとキャンバスが描き直されない、線が
   始まらない、メニューの操作が誤るため、Q4（フォークをやめ、全プラットフォームで正式版の
   同じ Qt にする）の前に必須である。見積もりは 2.7：P は 305 行（Windows だけのパッチ）、
   L は共通ガードで 3 件をまとめて約 190〜310 行、全部を L で作ると約 500〜700 行である。
2. ANGLE をやめて desktop の OpenGL を既定にする。H7（フォークなし・ANGLE を使わない OpenGL）
   の前段で遅れは大きく下がった。表示の正しさ、HDR、Wintab、CPU・電力を確認してから決める。
3. HDR を残すか。残すなら LibrePaint 側の色変換が必要で、コード量が増える。CHARTER の
   「UI と操作の見かけは旧版から変えない」に照らすと、HDR の画面の見かけを変えてよいかの
   利用者の判断が要る。
4. タッチ、クリップボード、Wayland のペン、日本語配列のショートカット、ドックのクラッシュを、
   実機で確認して「必要」か「不要」かにする。確認の順は、Q4（フォークをやめ、全プラットフォームで
   正式版の同じ Qt にする）の主指標への影響が大きいものからにする。
5. Android の 5 つの分岐は、現在も偽なので Q4（フォークをやめ、全プラットフォームで正式版の
   同じ Qt にする）では新たに失わない。Krita の Android 版との差を埋めるかは別の作業にする。
6. Qt のフレームレート圧縮と fast transfer の loss は、CPU と電力を測ってから決める。2026-11-11
   07:30 の監査役の決定どおり、表示の周期に合わせる設計は今は実装しない。

## 6. 判定をお願いしたいこと

- A（推奨）：上の 1〜6 の順で決める。まず Windows のペンの「必要」9 件を公式に取り込む交渉を
  始め、並行して LibrePaint 側で作り直す場合の見積もりを出す。ANGLE は desktop を既定にする
  案で、表示の正しさと CPU・電力を確認する。HDR は利用者の判断を待つ。
- B：ANGLE を残す。この場合、ANGLE の 11 件は「必要」に戻り、Windows の Qt のフォークを
  残すことになる。Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の
  「フォークをやめる」は満たせない。
- C：HDR とタッチを失ってよいとし、ANGLE もやめる。コード量は最も少ないが、CHARTER の
  「UI と操作の見かけは旧版から変えない」との関係を利用者が確認する必要がある。
- D：Windows のペンの「必要」9 件を LibrePaint 側で作り直す。公式の Qt に依存しないが、
  プラットフォームの内部に踏み込むコードが増える。

## 7. 出典

- Krita Manual「Display Settings」：https://docs.krita.org/en/reference_manual/preferences/display_settings.html
- Krita Manual「HDR Display」：https://docs.krita.org/en/reference_manual/hdr_display.html
- Krita Manual「Tablet Settings」：https://docs.krita.org/en/reference_manual/preferences/tablet_settings.html
- Krita Manual「Interface Scaling」：https://docs.krita.org/en/reference_manual/interface_scaling.html
- Krita Manual「Shortcut Settings」：https://docs.krita.org/en/reference_manual/preferences/shortcut_settings.html
- Adobe Photoshop Help「Configure Photoshop performance preferences」：https://helpx.adobe.com/photoshop/desktop/get-started/settings-and-preferences/configure-photoshop-performance-preferences.html
- Adobe Photoshop Help「Tablet support FAQ」：https://helpx.adobe.com/photoshop/kb/tablet-support-faq-photoshop.html
- Krita の Qt フォーク `dimula73/qtbase` の `for-krita/6.11.3`：https://github.com/dimula73/qtbase/tree/for-krita/6.11.3
- フォークの各コミット：`051139ed72`、`7bb46828ae`、`a60ed01a25`、`d850335b2d`、`e121cb08e5`、
  `fb923e4a97`、`f1cf04e958`、`1a543f2832`、`02216d26ba`、`77d2b02daf`、`7ad4a8bc17`、
  `70086804f9`、`82d2864a8a`、`dc27f27d8e`、`f786da6451`、`9dc7c77358`、`5874f1f131`
  （https://github.com/dimula73/qtbase/commit/&lt;sha&gt;）
- KDE bug 409613：https://bugs.kde.org/show_bug.cgi?id=409613
- KDE bug 413504：https://bugs.kde.org/show_bug.cgi?id=413504
- KDE bug 424319：https://bugs.kde.org/show_bug.cgi?id=424319
- KDE bug 449624：https://bugs.kde.org/show_bug.cgi?id=449624
- KDE bug 464839：https://bugs.kde.org/show_bug.cgi?id=464839
- KDE bug 517248：https://bugs.kde.org/show_bug.cgi?id=517248
- `docs/perf/qt-fork-patches.md`（92 件の判定）
- `docs/perf/DECISIONS.md` の 2026-11-07、2026-11-10、2026-11-11 の行
- `docs/perf/q4-canvas-updates.md`（描画の更新の 1 系統化の設計）
