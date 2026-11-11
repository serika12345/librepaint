# Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の上策の計画

## 0. 目的と範囲

この文書は、2026-11-11 の利用者の決定（`docs/perf/DECISIONS.md` の Q4 の行）に従い、
Q4 を上策で進めるための実装計画である。対象は次の 3 つである。

1. Windows のペンに必要なフォークの改変 9 件を、正式版 Qt 6.11.3 の公開 API と
   LibrePaint 側の実装で作り直す。
2. Windows の表示経路を、WGL と GDI の写しから、D3D11 の flip と待てる
   スワップチェーン（QRhiWidget）へ移す。HDR（10bit、scRGB、Rec.2020 PQ）も
   同じ DXGI の経路で作り直す。
3. 9 件以外の Windows の改変を 1 件ずつ残すか捨てるかに決め、実装の順序と
   完了条件を決める。

この文書は計画であり、コードは変えない。指標は Surface Pro 7 のホバー輪郭と
インクの入力から表示まで（中央値と P95）、アプリの CPU、電力、表示の正しさである。
判断は CHARTER の優先順位（主指標の改善、コード量が少ないこと、旧版の挙動に近いこと）
に照らす。

## 1. 前提と現状

### 1.1 決定済みのこと

- Q4 は上策で進む。Windows も正式版 Qt 6.11.3 にし、ペンの 9 件と HDR は
  LibrePaint 側で作り直して残す（利用者の決定、2026-11-11）。
- フォークを使うのは Windows の既定の構築だけである。Linux、macOS、Android、iOS は
  既に正式版 Qt 6.11.3 で構築されている。
- 正式版 Qt 6.11.3 の Windows 版は既に構築できている
  （`librepaint-windows-upstream-qt-archive`、ANGLE なし、`HAVE_HDR` は偽）。
- 正式版 Qt の Windows 版の desktop の OpenGL は、同じ回の比較でフォーク版 desktop より
  ホバーが約 4ms 速く、インクが約 4ms 遅い。`paintGL` は表示 1 回あたり 1.9 回で、
  フォーク版の Qt のフレームレート圧縮がないためである（2026-11-11 の `paint_cycle`）。
- 表示経路を今の desktop の WGL と GDI の写しのままにすると、アプリの CPU は
  ANGLE の H1（先積みのフレーム数）=1 と H5（ホバーの間引き）の組より +68%
  （33.2% 対 19.7%、入力区間 32.7 秒）で、CHARTER の「CPU を悪化させない」を
  満たせない。main スレッドの増分 +1.77ms/回の内訳は、PBO のアップロード約
  0.93ms/回と、WGL の交換・GDI の写し約 0.84ms/回である（`l2-cpu-cause`）。
  PBO を外すとアップロードはワーカーに移るが、合計は −4.2% で 10% に届かない。
  交換間隔 1 は CPU を −7.2% にするが、遅れが約 +20ms 悪化する。

### 1.2 基準にする値（同じ機器・同じ軌跡）

| 経路 | ホバー中央値 | インク中央値 | アプリ CPU | PresentMon の表示方式 |
| --- | --- | --- | --- | --- |
| フォーク版 desktop（H5） | 12.4〜13.6ms | 17.6〜18.6ms | 30.9〜33.2% | Composed: Copy with GPU GDI |
| ANGLE（H1=1 と H5） | 35.1〜35.5ms | 38.9〜39.3ms | 19.7% | Hardware Composed: Independent Flip 91〜95% |
| 正式版 Qt の desktop（2026-11-11） | 8.5〜9.7ms | 21.3〜22.3ms | 未計測（回による） | Composed: Copy with GPU GDI |
| Q1（Windows、フォーク版） | 56.7ms | 60.4ms | — | — |

Q4 の完了条件は「全プラットフォームが正式版の同じ Qt で構築でき、LibrePaint の
ソースがフォーク固有の API を参照しない。主指標が全プラットフォームで Q1 の値より
悪化しない」である。上の表のとおり、Q1 の値に対しては余裕がある。L2（Windows の
遅れを直す）の目標は、フォーク版 desktop の 12.4ms / 17.6ms を基準にする。

## 2. ペンに必要な 9 件を LibrePaint 側で作り直す

### 2.1 置き場所の方針

9 件は、次の 4 つの場所で作る。Qt の内部クラス（`QWindowsPointerHandler`、
`QWindowsTabletSupport`、`QWidgetWindow`）は使わない。

- **共通ガード**: `libs/input/ui/kis_input_manager.cpp` の `KisInputManager::eventFilterImpl` と
  `KisInputManager::Private`（`containsPointer`、`ProximityNotifier`）に、タブレットの
  近接・押下中の `QEvent::Leave` を無視する小さなガードを置く。公開の Qt イベントだけで足りる。
- **Windows のネイティブフィルター**: `libs/input/ui/` に新しい
  `KisWindowsPointerEventFilter`（`QAbstractNativeEventFilter`）を置き、`KisApplication` から
  `qApp->installNativeEventFilter()` する。`windows_generic_MSG` の `WM_POINTER*`、
  `WM_NCPOINTER*`、`WM_NCMOUSEMOVE`、`WM_MOUSELEAVE` を見る。既存の
  `KisLatencyProbe::NativeFilter`（`libs/global/KisLatencyProbe.cpp:139`）を土台にする。
  計測用のフィルターとは分け、通常の起動でも動くようにする。
- **入力の抑止**: `libs/input/KisInputEventSuppressor.{h,cpp}` に、Windows の Wintab で
  合成マウスを落とす判定を足す。公開の Qt イベントと既存の枠組みだけで足りる。
- **Wintab の対応範囲**: `libs/input/ui/wintab/` に `KisWintabMapping` を置き、Wintab の
  論理座標（`lcSysOrg`/`lcSysExt`）と Windows の仮想画面の矩形を比べ、違うときだけ
  `QTabletEvent` の位置を補正する。`wintab32.dll` を動的に読み、Wintab API を直接呼ぶ。

`KisShortcutMatcher`（`libs/input/kis_shortcut_matcher.{h,cpp}`）は、既に
`enterEvent()`、`cursorEntered()`、`leaveEvent()`、`buttonPressed()` を持つ。9 件の
Enter/Leave は、この既存の状態に合わせて呼ぶ。既存の `176a8164a3`（Enter を落とした
押下の復帰）はそのまま残す。

### 2.2 1 件ずつの作り直し

「公開 API」は正式版 Qt 6.11.3 の公開 API だけで足りるか、「ネイティブ」は
QPA のネイティブインターフェースか Win32/Wintab の直接呼び出しが要るかである。
行数は本体だけで、テストは別に数える。

| コミット | 失う動作 | 作り直す場所と方法 | 公開 API | ネイティブ | 行数 | 契約テスト |
| --- | --- | --- | --- | --- | --- | --- |
| c41d66281e | タブレットが窓の非クライアント領域に入ったときの Enter/Leave とカーソル | `KisWindowsPointerEventFilter` で `WM_NCPOINTERUPDATE/DOWN/UP` と `WM_NCMOUSEMOVE` を見て、画面座標をキャンバスのローカル座標へ直し、`KisInputManager` の進入・離脱のガードへ渡す。非クライアント領域では `QCursor` の形を戻す | 足りない（Qt に非クライアントのポインターイベントの公開 API がない） | Win32 のメッセージとカーソル。QPA の公開インターフェースには非クライアントのポインターの入口がない | 80〜120 | メッセージと座標の純粋関数（HWND なし）。非クライアントの座標→キャンバス、カーソルの復帰、領域外の判定 |
| df3a678309 | タブレットの進入時に `WM_MOUSELEAVE` を購読解除する | 共通ガードで、近接中・押下中の `QEvent::Leave` を無視する。離れた後に `QCursor::pos()` がキャンバスの外なら Leave を確定する | 足りる | 不要 | 15〜25 | `TestInputShortcutMatcher` と `kis_input_manager_test` に、近接中の Leave を無視する場合と、離れた後の Leave を確定する場合 |
| 051139ed72 | Wintab の対応範囲を `lcSys{Org,Ext}` から取る | `KisWintabMapping` が起動時に Wintab のシステム矩形を読み、仮想画面の矩形と違うときだけ `QTabletEvent` の位置をアフィン変換する。設定ページの `kritadisplayrc` の値もここで使う（Qt6 の起動時に読む処理を含む） | 足りない（Qt は Wintab の生の座標を公開しない） | Wintab API（`wintab32.dll` の `WTOpen`、`WTInfo`、`WTGet`）と Win32 の `GetSystemMetrics` | 150〜250 | 偽の Wintab API を差し込み、矩形が同じときは無変換、違うときに正しく写す。負の原点、複数画面、範囲外のクランプ |
| 33a49f25d1 | Windows Ink の押下をウィジェットへ届ける。長押しが右クリックにならない | キャンバスの HWND に `SetGestureConfig` を設定し、`GID_PRESSANDTAP`（長押し）とタップのシステムジェスチャーを切る。押下は既存の `176a8164a3` の Enter の復帰と、`KisInputManager` の押下の処理でツールへ渡す | 足りない（受理の制御が公開されていない） | Win32 の `SetGestureConfig` と HWND | 30〜60 | `GESTURECONFIG` の組み立ての純粋関数。`GID_PRESSANDTAP` が無効、他のジェスチャーを変えないこと |
| f5a75c4071 | 受理した押下のストローク全体の合成マウス | キャンバスは `QTabletEvent` を直接使うため不要と見る。メニューなど非キャンバスのウィジェットで必要になったときだけ、`KisInputManager` から `QMouseEvent` を作って送る | 足りる（`QMouseEvent` を作れる） | 不要（必要になった場合も公開 API） | 0（延期）。作る場合 60〜100 | 延期の条件を `DECISIONS.md` に書く。作る場合は押下・移動・解放の順と、二重にならないこと |
| 41adadd8db | 受理した `QTabletEvent` の Enter/Leave の合成 | 共通ガードでキャンバスの Enter を戻し、`KisShortcutMatcher::enterEvent()` を呼ぶ。全ウィジェットに同じ合成が要るなら `KisApplication` のアプリケーションイベントフィルターを足す | 足りる | 不要 | 20〜30（キャンバス）。全ウィジェットは +40〜60 | `TestInputShortcutMatcher` の `recoversPressWhenEnterWasMissed` を拡張し、Enter の合成とホバーの継続を確かめる |
| 0027358f20 | Enter/Leave パッチの存在を示すヘッダーの印 | 作らない。CMake の検出と `QT_HAS_*` の参照を消す | 関係しない | 不要 | 0 | フォーク API の参照が 0 件であることを走査するテスト |
| 8f8eabfb72 | WinTab の合成マウスの二重 | `KisInputEventSuppressor` の `setSuppressSyntheticMouseEvents` を Windows の Wintab でも使う。タブレットが近接・押下中の `QMouseEvent` を落とす。OS の合成マウスは `QMouseEvent::source()` が合成と印されないため、近接の状態で判定する | 足りる | 不要 | 20〜40 | `TestInputEventSuppressor` に、近接中の合成マウスを落とす場合と、タブレット以外のマウスを落とさない場合 |
| c132390390 | `WM_POINTERLEAVE` の Leave の戻し | 共通ガードのタッチ分岐で、指が残っている間の `QEvent::Leave` を無視する。最後の指が離れたときに、カーソルがキャンバスの中なら Leave を確定しない | 足りる | 不要 | 10〜20 | タッチの最後の離脱で Leave を出さない場合と、外へ出たときに出す場合 |

- 9 件のうち 5 件（c41d66281e、051139ed72、33a49f25d1、8f8eabfb72、c132390390）は
  Win32 か Wintab の直接の呼び出しを含む。残り 4 件は公開の Qt イベントだけで作れる。
- `176a8164a3`（押下で Enter を戻す）は既に入っている。c41d66281e のカーソル、
  33a49f25d1 のジェスチャー、41adadd8db のホバーの Enter は、これだけでは足りない。
- 051139ed72 は、Qt6 の起動時に `kritadisplayrc` を読む処理（約 10 行）を
  `main.cc` に足す。`KisDlgCustomTabletResolution` の設定ページは残し、保存した値を
  `KisWintabMapping` が使う。
- 8f8eabfb72 は、Qt の Wintab 経路が OS の合成マウスを止めないため、近接の状態で
  落とす。タッチの合成マウスは既存の `startBlockingTouch` の枠組みを使う。
- f5a75c4071 はキャンバスに不要と見て延期する。非キャンバスのウィジェットで
  ペンの押下が届かない不具合が出たときに作る。延期の条件を `DECISIONS.md` に書く。
- 0027358f20 は作らない。CMake の検出マクロごと消す。

### 2.3 リスク

- ネイティブフィルターは GUI スレッドのメッセージ処理の前後に挟まる。計測用の
  `KisLatencyProbe` と同じく、通常の起動では仕事をしない（フラグ 1 回の判定）形にし、
  ペンの経路に文字列の確保やロックを足さない。
- `KisWintabMapping` は Wintab のコンテキストを Qt と二重に持たない。設定の読み取りだけを
  行い、パケットは Qt の経路に任せる。二重のコンテキストが必要になった場合は、
  Qt の Wintab を切り、自前のコンテキストに置き換える（その時点で行数を見積もり直す）。
- Linux と macOS では共通ガードだけが動く。ホバーとタッチの回帰を、Windows の後ではなく
  同じ段で確かめる（`verify` のテストと、Mac の実機の確認）。

## 3. Windows の表示経路（D3D11 の flip）と HDR

### 3.1 方針

`docs/perf/l2-flip-presentation.md` の中策を出発点にする。

- メインウィンドウの RHI を D3D11 にし、キャンバスを `QRhiWidget` の
  `KisRhiCanvas` に移す。`QT_WIDGETS_RHI_BACKEND=d3d11`、`QT_D3D_MAX_FRAME_LATENCY=1`、
  交換間隔 1 にする。Qt の D3D11 の RHI は `DXGI_SWAP_EFFECT_FLIP_DISCARD`、
  `BufferCount=2` のスワップチェーンを作り、交換間隔が 1 のときは
  `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT` と
  `IDXGISwapChain2::SetMaximumFrameLatency(1)` を使う。交換間隔 0 では待てる
  スワップチェーンを使わないため、1 にする。
- キャンバスの描画は QRhi のコマンドバッファで行う。GL の資源（テクスチャ、
  頂点バッファ、シェーダー、FBO）は QRhi の資源に置き換える。
- キャンバスの更新は、`QWidget::update` から `QRhiWidget::render` へ今の
  `KisCanvas2` の更新の入口をそのまま通す。`update()` は表示の周期で抑えられるので、
  正式版 Qt の「表示 1 回あたり 1.9 回描く」を 1.0 に近づける。
- SDR の既定は `QRhiWidget` にする。HDR と 10bit は、Qt 6.11.3 のウィジェットの
  バックエンドがウィンドウのスワップチェーンを SDR で作り、HDR の形式を選ぶ公開 API を
  持たないため、LibrePaint が DXGI のスワップチェーンを持つ `KisDxgiCanvasWindow`
  で作る（3.5）。

### 3.2 移す部品の一覧

| 今のファイルとクラス | 役割 | 移し方 | 行数（見込み） |
| --- | --- | --- | --- |
| `libs/ui/opengl/kis_opengl_canvas2.{h,cpp}` の `KisOpenGLCanvas2 : QOpenGLWidget` | キャンバスのウィジェット。`initializeGL`、`resizeGL`、`paintGL`、`paintEvent`、IME、フォーカス | `KisRhiCanvas : QRhiWidget` と `KisCanvasWidgetBase`。`initialize()` と `render()` に分ける。イベント処理は今の `KisOpenGLCanvas2` から移す | 400〜600 |
| `libs/ui/opengl/KisOpenGLCanvasRenderer.{h,cpp}` | 画像、チェッカー、グリッド、輪郭、カーソル、表示フィルターの描画 | `KisRhiCanvasRenderer`。`QOpenGLShaderProgram`、`QOpenGLVertexArrayObject`、`QOpenGLBuffer` を `QRhiGraphicsPipeline`、`QRhiBuffer`、`QRhiShaderResourceBindings` に置き換える | 1,500〜2,000 |
| `libs/ui/opengl/kis_opengl_image_textures.{h,cpp}` | 画像のタイルのテクスチャ、ミップマップ、LOD、カラースペース | `KisRhiImageTextures`。`QRhiTexture`（RGBA8、RGB10A2、RGBA16F）と `QRhiResourceUpdateBatch` に移す。タイルのアトラスと LOD の規則は変えない | 800〜1,000 |
| `libs/ui/opengl/kis_texture_tile.{h,cpp}` | 1 タイルの `glTexImage2D` / `glTexSubImage2D` | `KisRhiTextureTile`。アップロードは 1 フレーム分を 1 つの `QRhiResourceUpdateBatch` にまとめる | 350〜450 |
| `libs/ui/opengl/KisOpenGLBufferCircularStorage.{h,cpp}` | PBO の環状のバッファ | QRhi の staging バッファ（`QRhiBuffer` の `Immutable`/`Dynamic`）に置き換える。PBO の切り替えは L1 の実験なので、Q4 の後に削除する | 120〜180 |
| `libs/ui/opengl/KisOpenGLUpdateInfoBuilder.{h,cpp}` と `kis_opengl_update_info.{h,cpp}` | 更新の矩形とタイルの更新情報 | `KisCanvasUpdateInfo` に名前を変え、GL に依存しない構造にする。既存の `KisUpdateInfo` との変換は 1 か所に閉じる | 250〜350 |
| `libs/ui/opengl/kis_opengl_shader_loader.{h,cpp}` と `krita/data/shaders/*` | GLSL の読み込みとコンパイル | `KisRhiShaderLoader`。`krita/data/shaders` の GLSL を Vulkan 風の GLSL に直し、`qt_add_shaders` で `.qsb` に変換する。GL、HLSL、MSL を同じソースから作る | 150〜250（＋シェーダー 500〜700） |
| `libs/ui/canvas/kis_display_filter.h`、`plugins/dockers/lut/ocio_display_filter_vfx2021.{h,cpp}` | OCIO の表示フィルター。`setupTextures(GLFunctions*, QOpenGLShaderProgram*)` が GL に依存 | `KisRhiDisplayFilter`。LUT を `QRhiTexture` にし、`QRhiShaderResourceBindings` で結ぶ。フィルターの GLSL は `.qsb` のシェーダーに埋め込む | 300〜500 |
| `libs/ui/opengl/kis_opengl_sync.h` / `KisOpenGLSync` | 前のフレームの GPU の完了の確認 | QRhi の `framesInFlight` と `QRhiWidget::frameSubmitted` に置き換える。正式版 Qt には `QRhi::isLastFrameCompletedOnGPU` がないため、フェンスの待ちは QRhi の内部に任せる | 80〜120 |
| `libs/ui/opengl/kis_opengl.cpp` の `selectSurfaceConfig`、`KisOpenGLModeProber`、`KisOpenGLStartupCheck` | GL のレンダラーの選択と最初のフレームの検査 | Windows の D3D11 の経路では使わない。Linux と macOS と QPainter のフォールバックのために残し、Windows では D3D11 の RHI の可否を先に検査する | 100〜150 |
| `krita/main.cc` の `QT_WIDGETS_RHI` と `QT_WIDGETS_RHI_BACKEND` | ウィンドウの RHI を OpenGL にする | Windows は `d3d11`、Linux と macOS は `opengl`（移行後は `metal`）にする。`QT_D3D_MAX_FRAME_LATENCY=1` を足す | 20〜40 |
| `plugins/dockers/smallcolorselector/KisGLImageWidget.{h,cpp}`、`KisClickableGLImageWidget`、`KisGLImageF16` | 2 つ目の `QOpenGLWidget`（色相と明度の選択） | `KisRhiImageWidget : QRhiWidget` に移す。1 つのウィンドウに `QOpenGLWidget` と `QRhiWidget` を混在させられないため、キャンバスと同じ段で移す | 250〜400 |

- `KisCanvas2::createCanvas()`（`libs/ui/canvas/kis_canvas2.cpp:784`）に
  `createRhiCanvas()` を足し、Windows では `KisRhiCanvas`、Linux と macOS では
  移行までは `KisOpenGLCanvas2`、RHI が使えないときは `KisQPainterCanvas` を選ぶ。
  選択は隠し設定 `useRhiCanvas`（既定は Windows で true）で戻せるようにし、
  L1/L2 の計測の切り替えと同じ削除の条件をヘッダーに書く。
- `KisOpenGLCanvas2::updateCanvasImage` / `updateCanvasDecorations` の
  `QWidget::update` の入口は変えない。`KisCanvas2` の更新の圧縮と、
  `KisOpenGLCanvas2::isBusy()` の意味は `KisRhiCanvas` でも同じにする。

### 3.3 テクスチャの転送、シェーダー、装飾と輪郭

- **テクスチャの転送**: 今は `KisOpenGLBufferCircularStorage` の PBO に
  `glBufferSubData` してから `glTexSubImage2D` する。QRhi では、1 フレーム分の
  タイルの更新を 1 つの `QRhiResourceUpdateBatch` にまとめ、`uploadTexture` か
  `updateTexture` で送る。D3D11 の RHI は staging のテクスチャを経由するため、
  PBO の有無のような切り替えはなくなる。main スレッドのアップロード
  （約 0.93ms/回）が残るかは、QRhi の実装と回数で決まる。最初の計測で
  `paint_end_to_present` と main の CPU を見る。
- **シェーダー**: `krita/data/shaders` の 20 ファイルのうち、キャンバスが使うのは
  `matrix_transform`、`simple_texture`、`highq_downscale`、`solid_color`、`cursor`、
  グラデーション（`bilinear`、`linear`、`radial`、`conical`、`square`）である。
  `*_legacy` は OpenGL 2 のフォールバック用で、QRhi では使わない。Vulkan 風の
  GLSL に直し、`qt_add_shaders` で `.qsb` にする。OCIO の `USE_OCIO` の
  関数の埋め込みは、`QShader` のソースを実行時に作るのではなく、LUT の
  テクスチャと uniform に変える。
- **装飾**: `KisCanvasWidgetBase::drawDecorations` は `QPainter` で描き、
  `QOpenGLWidget` の上に重ねている。`QRhiWidget` でも `QPainter` で
  バックグラウンドのテクスチャに描けるが、GL の描画と混ぜる順序が変わる。
  装飾は今の `QPainter` の経路を残し、キャンバスの画像の後に描く。
  装飾の位置と見た目が変わらないことを、画素の比較で確かめる。
- **輪郭**: ツールの輪郭は `KisOpenGLCanvasRenderer::paintToolOutline` が
  `QPainterPath` を頂点にして描く。QRhi では線の頂点バッファと
  `QRhiGraphicsPipeline`（線の幅、アンチエイリアス）に移す。ホバーの輪郭は
  L1/L2 の主指標の対象なので、最初に画素と遅れを比べる。
- **部分更新**: `QOpenGLWidget::PartialUpdate` に相当する部分更新は、
  `QRhiWidget` ではバックグラウンドのテクスチャ全体を描き直す。キャンバスの
  部分更新の範囲は、テクスチャの更新（`updateTexture` の矩形）に残し、
  描画は全体でも、画素の結果と GPU の負荷が今と同じ范围になるようにする。
  4K の画面での負荷は、L2 の H4（1 フレームの GPU 負荷）と同じ方法で測る。

### 3.4 Linux と macOS の扱い

- **第一段（Windows の移行中）**: Linux と macOS は今の `KisOpenGLCanvas2`
  （`QOpenGLWidget`）のままにする。理由は、Windows の D3D11 の経路の画素と
  遅れを先に確かめ、Linux と macOS の回帰を別の段で分けるためである。
- **第二段（Windows の合格後）**: 同じ `KisRhiCanvas` を Linux の OpenGL と
  macOS の Metal で動かす。`QShader` は同じソースから GLSL と MSL を作れる。
  `KisCanvas2::createCanvas()` の選択を全プラットフォームで `KisRhiCanvas` に
  変え、`KisOpenGLCanvas2` をフォールバックとして残す。Linux の X11 と Wayland、
  macOS の M2 で、Q1 の値（X11 のホバー 45.9ms、Wayland 29.6ms、macOS の
  既存の値）より悪化しないことを確かめる。
- **第三段（Q4 の完了時）**: `KisOpenGLCanvas2`、`KisOpenGLCanvasRenderer`、
  `KisGLImageWidget`、GL のシェーダーを削除する。Q4 の「描画の更新の仕組みを
  1 系統にする」はここで満たす。削除の前に、RHI の経路を既定にした
  `verify` と、Mac と iPad の実機の確認を行う。iPad（iOS）は QRhi の Metal に
  移す。iPad の移行が難しい場合は、GL のキャンバスを iOS だけに残し、
  削除の条件を分ける。

### 3.5 10bit と scRGB / Rec.2020 PQ の HDR

正式版 Qt 6.11.3 のウィジェットのバックエンドは、ウィンドウのスワップチェーンを
`QRhiSwapChain::SDR`（8bit）で作り、`QSurfaceFormat` の色空間を読まない。
`QRhiSwapChain` には `HDRExtendedSrgbLinear`（16F、scRGB）と `HDR10`
（R10G10B10A2、Rec.2020 PQ）の形式があるが、ウィジェットの経路からは選べない。
したがって HDR と 10bit は、LibrePaint が DXGI のスワップチェーンを持つ
`KisDxgiCanvasWindow`（`QWindow` と `QRhi`）で作る。

- **SDR（既定）**: `KisRhiCanvas`（`QRhiWidget`）。ウィンドウは 8bit の sRGB。
  今の 8bit の見た目を変えない。
- **10bit SDR（30-bit）**: `DXGI_FORMAT_R10G10B10A2_UNORM` と
  `DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709`。画面が 10bit を報告したとき
  （`IDXGIOutput6::GetDesc1` の `BitsPerColor >= 10`）だけ選ぶ。QRhi の `SDR` は
  8bit なので、この経路は DXGI を直接使う。
- **scRGB（Windows の HDR の既定）**: `DXGI_FORMAT_R16G16B16A16_FLOAT` と
  `DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709`。値は線形で、1.0 は 80nit。
  1.0 を超える値をそのまま出し、ディスプレイの色空間への変換は DWM に任せる。
  Windows の HDR の合成の標準の空間であり、まずこれを実装する。
- **Rec.2020 PQ（HDR10）**: `DXGI_FORMAT_R10G10B10A2_UNORM` と
  `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`。シェーダーで
  sRGB 線形→Rec.2020 の原色→ST2084 の PQ へ変換する。HDR10 の静的メタデータは
  `IDXGISwapChain2::SetHDRMetaData` に画面の `MaxLuminance` などを入れる。
  scRGB が使える画面では scRGB を優先し、PQ は HDR10 の出力が要求されたときだけ使う。
- **色の管理**: 今の `KisSurfaceColorSpaceWrapper` と
  `KisDisplayColorConverter` を残し、出力の変換（sRGB→scRGB、または
  sRGB→Rec.2020 PQ）をキャンバスの最後のシェーダーで行う。OCIO の表示フィルターは
  その前段に入れる。SDR の白（80nit か 203nit）は `QRhiSwapChainHdrInfo` か
  `DXGI_OUTPUT_DESC1` の `MaxLuminance` から取り、SDR の画像を HDR の画面で
  白飛びさせない。
- **設定と戻し方**: 表示の設定の HDR のページを残し、「SDR」「10bit SDR」
  「HDR（scRGB）」「HDR10（PQ）」を選べるようにする。既定は画面の能力に合わせ、
  選べない形式は選ばせない。HDR の経路が失敗したときは SDR に戻し、
  `KisUsageLogger` に理由を出す。
- **スワップチェーン**: すべての HDR/10bit の経路で、`CreateSwapChainForHwnd`、
  `DXGI_SWAP_EFFECT_FLIP_DISCARD`、`BufferCount=2`、
  `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT`、
  `SetMaximumFrameLatency(1)`、`Present(1, 0)` を使う。`GetFrameLatencyWaitableObject`
  はフレームの先頭で待ち、入力の処理を待たせない。リサイズではバックバッファを
  作り直す。これは 3.1 の SDR の QRhiWidget と同じ「flip と待てるスワップチェーン」の
  形であり、HDR のためだけに別の提示の仕組みを足さない。QRhi は
  `beginOffscreenFrame` / `endOffscreenFrame` でキャンバスのオフスクリーンの
  テクスチャに描き、そのテクスチャを D3D11 のコピーでスワップチェーンへ出す。
  HDR と 10bit のときだけ GPU の写しが 1 回増えるが、CPU の main スレッドには
  載せない。QRhi の D3D11 のネイティブのデバイスでバックバッファを包める場合は、
  この写しを省く。
- **入力とウィンドウの統合**: `KisDxgiCanvasWindow` は
  `QWidget::createWindowContainer` でメインウィンドウに埋め込む。子の
  `QWindow` のスワップチェーンは DWM が親に合成するため、Independent Flip には
  ならない（Composed: Flip まで）。それでも GDI の写しはなくなり、待てる
  スワップチェーンの効果は得られる。ペンのイベントは子の `QWindow` に届くため、
  `KisInputManager` のイベントフィルターとネイティブフィルターをこのウィンドウにも
  付ける。この統合は実機で確かめるまで未確認である。
- **代替案（採用しない）**: Qt の `QBackingStoreRhiSupport::swapChainForWindow` に
  約 20 行のパッチを当て、ウィンドウのスワップチェーンの形式を選べるようにする案。
  コードは小さいが、Qt の更新のたびに再ベースするパッチになり、利用者の
  「LibrePaint 側で作り直す」という決定と合わない。HDR の画素の変換を Qt の
  合成のシェーダーに足す必要もあり、採用しない。

### 3.6 Surface の内蔵画面は SDR なので、HDR の確認方法

- Surface Pro 7 の内蔵の画面は 8bit の SDR である。`IDXGIOutput6::GetDesc1` は
  `DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709`、`BitsPerColor=8` を返す見込みで、
  `QRhiSwapChain::isFormatSupported(HDR10)` と
  `isFormatSupported(HDRExtendedSrgbLinear)` は偽になる。内蔵の画面では
  **SDR の経路（8bit）と、HDR の形式が選べないときの SDR への戻し方**だけを確かめる。
- HDR の確認には、USB-C か HDMI で HDR 対応の外部の画面をつなぎ、Windows の
  表示の設定で HDR を有効にする。確認する値は次のとおり。
  - `IDXGIOutput6::GetDesc1` の `ColorSpace` が `G10_NONE_P709`（scRGB）または
    `G2084_NONE_P2020`（HDR10）、`BitsPerColor` が 10 以上、`MaxLuminance` が
    画面の公称値と一致すること。
  - `QRhiSwapChain::isFormatSupported()` が真で、`createOrResize()` の後に
    `IDXGISwapChain3::GetColorSpace1()` が要求した色空間を返すこと。
  - テストパターン（scRGB の 0、0.18、1.0、2.0、4.0 の矩形と、PQ の code value
    0、0.58、0.75、1.0）を出し、カラリメーターか、同じ画面の Krita と
    Photoshop の HDR の表示と比べる。画面の写し（SDR のキャプチャ）は
    トーンマップされるため、色の判定に使わない。
  - SDR の画像を HDR の画面に出したとき、白が 80nit か 203nit のどちらかで
    正しく、白飛びと色かぶりがないこと。
  - HDR を切ったときに SDR に戻り、見た目が Q1 と同じであること。
- 10bit SDR も Surface の内蔵の画面（8bit）では確かめられない。外部の 10bit の
  画面で `BitsPerColor >= 10` と、`R10G10B10A2_UNORM` + `G22_NONE_P709` の
  スワップチェーンを確かめる。内蔵の画面では 8bit のままにする。

### 3.7 他の編集ソフトの扱い（基準）

| ソフト | HDR と 10bit | 提示の方式 | ペン |
| --- | --- | --- | --- |
| Krita | Windows と Linux（Wayland）で HDR。Windows は表示の設定で優先するサーフェスを選ぶ。10bit の表示 | Windows は Qt の fork の ANGLE か desktop の GL。HDR は Qt の `setTextureColorSpace` | WinTab と Windows Ink を選べる |
| Adobe Photoshop | 「30-bit display support」。GPU と画面が対応するときに 30bit。HDR の表示は画面と GPU の設定に依存し、公式の記述は 30bit が中心である | OS と GPU の合成 | Windows Ink が既定、WinTab は従来の経路 |
| Chrome | HDR の動画を DXGI の flip で提示 | D3D11 の flip と待てるスワップチェーン。GL は ANGLE | — |
| mpv | HDR を DXGI の色空間で提示 | D3D11 の flip（`--gpu-context=d3d11`）か、D3D9Ex の flip に GL で描く（`dxinterop`） | — |
| OBS | HDR のキャプチャと提示 | D3D11 の `FLIP_DISCARD` と待てるスワップチェーン | — |
| Blender | — | メインウィンドウは WGL。D3D11 のテクスチャの共有に `WGL_NV_DX_interop2` | — |

この表から、HDR と 10bit は「アプリがスワップチェーンの形式と色空間を選ぶ」のが
一般的である。ウィジェットのツールキットの既定の合成に任せる例は少ない。
Chrome、mpv、OBS は D3D11 の flip と待てるスワップチェーンを使い、GL は ANGLE か
テクスチャの共有でつなぐ。LibrePaint の `QRhiWidget` + D3D11 と、
HDR のための DXGI のスワップチェーンは、この一般の形に合う。

### 3.8 遅れ、CPU、電力の見込み

| 項目 | 今 | D3D11 の flip と待てるスワップチェーン | 根拠と不確かさ |
| --- | --- | --- | --- |
| 遅れ（ホバー、インク） | 12.4〜13.6 / 17.6〜18.6ms（フォーク版 desktop） | −0〜−4ms、P95 は −5% 以上を目標 | 待ち行列を 1 に抑える効果は ANGLE の H1=1 で 15.5〜19.6 → 1.5〜2.1ms。desktop は既に交換が 1ms で返るため、効果は小さい可能性がある。表示の周期への量子化は残る |
| 描画の回数 | 表示 1 回あたり 1.04 回（desktop）。正式版 Qt は 1.9 回 | 1.0〜1.1 回（交換間隔 1 で `update` が抑えられる） | 正式版 Qt の 1.9 回は、Qt の圧縮がないため。vsync 1 の QRhi では `render` が表示の周期で呼ばれる見込み |
| アプリの CPU | 33.2%（ANGLE 19.7%、+68%） | 20〜22%（ANGLE +10% 以内を目標） | main の +1.77ms/回のうち、PBO 0.93ms と交換・合成 0.84ms。D3D11 の flip は GDI の写しを外し、描画の回数も減らす。QRhi の staging の写しが残る可能性がある |
| 電力 | 未計測 | −5〜−10% の見込み | GDI の写しと描画の回数が減る。HDR の画面ではバックライトの消費が増えるため、HDR の効果は別に測る |
| HDR の画素 | fork の scRGB / PQ | 同じ | 色の変換を LibrePaint のシェーダーに移すため、1 階調以内を確かめる |

- 条件 6（CPU と電力が ANGLE より +10% 以内）は、CPU が 21.7% 以下なら満たす。
  QRhi の staging の写しとウィジェットの合成の分が残ると届かない可能性がある。
  そのときは、キャンバスをトップレベルの RHI のウィンドウにする案（ウィジェットの
  合成を外す）を、ペンの入力の統合の費用と合わせて見積もり直す。
- 遅れの主因は表示の周期への量子化（16.7ms）で、flip では消えない。Q4 の表示経路の
  変更は、CPU の条件を満たすことと、待ち行列を 1 に抑えることを目的にする。
  1 フレームの量子化を減らす設計は、2026-11-11 07:30 の監査役の決定どおり、
  CPU と電力を測ってからにする。

## 4. 9 件以外の Windows の改変の扱い

1 件ずつ、残す・捨てる・測ってから決めるを決める。

| 改変 | 決め方（1 行） |
| --- | --- |
| タッチの受理（`a60ed01a25`） | 残す。Surface のタッチでパン・ズームとペンの競合を確かめ、競合するときだけ `RegisterTouchWindow` と `SetGestureConfig` をネイティブフィルターに足す（30〜60 行）。 |
| クリップボードの CF_DIB の回避（`d850335b2d`） | 残す。`KisClipboard::setClip` の `mimeData->setImageData(qimage)` をやめ、CF_PNG と CF_DIBV5 だけにする（10〜20 行）。Chrome と Edge に貼って確かめる。 |
| タブ表示の文書名（`f786da6451`） | 残す。`KisMainWindow` でタブの切り替えに合わせて窓の題名を設定する（10〜20 行）。`HAVE_QMDIAREA...` の検出は消す。 |
| 非ラテン配列のショートカット（`82d2864a8a` ほか） | 残す。`KisInputManager` の既存の回避コードを Windows でも有効にする（約 5 行）。日本語配列で Ctrl+Tab と AltGr を確かめる。 |
| アクセシビリティを切る設定（`9dc7c77358`） | 起動時だけ残す。`QT_DISABLE_ACCESSIBILITY` を `QApplication` の前に設定する今の形を維持し、実行中の切り替えは作らない。設定項目の説明を直す。 |
| Qt のフレームレート圧縮（7 件） | 作り直さない。正式版 Qt の更新の経路をそのまま使い、CPU と電力を測る。+10% を超えたときだけ、`KisCanvas2` の更新の圧縮を小さく足す。 |
| fast transfer（`fb923e4a97`） | 作り直さない。CPU と電力を測り、+10% を超えたときだけ、テクスチャーの転送のまとめ方を変える。 |
| ドックのクラッシュの修正（`8dadf099b8`） | 再現を試す。再現したら LibrePaint 側で直す（20〜50 行）。再現しなければ作らない。 |
| Wintab の相対モード（`7bb46828ae`） | Wintab の実機で確かめる。相対モードになるなら `KisWintabMapping` で絶対座標に直す（20〜40 行）。 |
| ANGLE のリサイズ・ダイアログのちらつきの回避（`1a543f2832` ほか） | 捨てる。ANGLE がないため環境変数の設定を削除する（約 10 行の削除）。desktop でリサイズとダイアログを確かめる。 |
| `KRITA_QSCROLLER_PATCH` の検出 | 捨てる。本体が参照していないため、CMake の検出と設定を削除する。 |
| Android の 5 つの分岐 | 触らない。現在も偽で、Q4 で新たに失わない。Krita の Android 版との差を埋めるかは別の作業にする。 |
| `QT_BACKING_STORE_USE_FAST_QIMAGE_TRANSFER` と `QT_FRAME_RATE_OVERRIDE` | 捨てる。フォークの圧縮の環境変数であり、正式版 Qt では効かない。設定の削除と一緒に消す。 |
| `QT_USE_PREMATURE_RESIZE_EVENTS` ほかの ANGLE の回避 | 捨てる。ANGLE の分岐ごと削除する。 |

## 5. 実装の順序、完了条件、契約テスト

### 5.1 順序

主指標とペンを壊さないために、次の順にする。各段は小さなコミットに分け、
`verify-quick` を通して push する。計測が要る段では、progress.md に「計測依頼」を
書いて止まる。

| 段 | 内容 | 完了条件 | 契約テストと計測 | 見込み行数 |
| --- | --- | --- | --- | --- |
| 1 | フォーク固有の CMake の検出マクロと API の参照を外し、Windows の既定を正式版 Qt にする。ANGLE と HDR の分岐を一時的に外す | 正式版 Qt の Windows の構築（差分）が通り、`verify-quick` が通る。フォーク固有の API の参照が 0 件。Surface で起動し、SDR の desktop の GL でキャンバスが出る | `test_q4_no_fork_api.py`（禁止した識別子の走査）、`test_q4_windows_build_config.py`（`useQtFork=false`、ANGLE なし、`HAVE_HDR` なし）、`verify-quick`、Surface の起動の確認（監査役） | 削除 750〜900、追加 100〜150 |
| 2 | ペンの共通ガード（df3a678309、41adadd8db、c132390390、8f8eabfb72）を公開の Qt イベントだけで作る | `verify` の入力のテストが通り、Surface でホバーの Enter/Leave が途切れず、合成マウスが二重にならない。ホバーとインクが段 1 の同じ回の値から 5% を超えて悪化しない | `TestInputShortcutMatcher`、`TestInputEventSuppressor`、`kis_input_manager_test` の新しい場合。Surface の 5 往復（監査役） | 65〜115（本体）、80〜120（テスト） |
| 3 | Windows のネイティブフィルター（c41d66281e、33a49f25d1）を作る | 非クライアント領域からキャンバスへ入ったときのカーソルと Enter/Leave が正しく、長押しが右クリックにならず、押下がツールに届く。ペンの操作の確認項目がすべて通る | メッセージと座標の純粋関数のテスト、`GESTURECONFIG` のテスト。Surface のペンの操作の確認（監査役） | 110〜180（本体）、80〜120（テスト） |
| 4 | Wintab の対応範囲（051139ed72）を `KisWintabMapping` で作る | 複数画面と Wintab の実機で、ペンの位置がずれない。設定ページの値が再起動後も効く。Surface（WM_POINTER）では何も変わらない | 偽の Wintab API のテスト（無変換、変換、負の原点、範囲外）。Wintab の実機の確認（監査役、Surface 以外） | 150〜250（本体）、80〜120（テスト） |
| 5 | `KisRhiCanvas`（`QRhiWidget`）の骨格を作り、Windows で選べるようにする。画像 1 枚を QRhi で描く | 正式版 Qt の Windows で、今と同じ画像が QRhi の D3D11 で出る。`useRhiCanvas=false` で GL に戻せる。ペンの入力が今と同じに届く | `TestCanvasBackendSelection`（Windows は D3D11、RHI なしは QPainter、Linux は GL）。画素の比較（画像 1 枚）。Surface の起動（監査役） | 300〜450（本体）、150〜200（テスト） |
| 6 | テクスチャの転送とレンダラーを移す（画像、チェッカー、グリッド、輪郭、カーソル、表示フィルター、シェーダー） | 8bit の画素が GL の経路と 1 階調以内で一致する。ズーム、パン、LOD、ラップアラウンド、部分更新、OCIO の表示フィルターが動く。ホバーの輪郭とインクの遅れが段 5 から悪化しない | タイルの更新情報のテスト、オフスクリーンの画素の比較、シェーダーのコンパイルのテスト。Surface の 5 往復（監査役） | 3,000〜3,600（本体）、500〜700（テスト） |
| 7 | カラーセレクタを `KisRhiImageWidget` に移し、Windows の既定を D3D11 にする。交換間隔 1 と `QT_D3D_MAX_FRAME_LATENCY=1` を設定する | PresentMon が `Composed: Flip` か `Independent Flip` になり、`MsUntilDisplayed` が悪化しない。ホバーが 12.4ms 以下か P95 が 5% 以上改善、インクが 17.6ms 以下か P95 が 5% 以上改善。CPU が ANGLE +10% 以内。電力が +10% 以内。ペンの操作が壊れない | カラーセレクタの QTest と画素の比較。Surface の交互 5 回（フォーク版 desktop と比べる、監査役）。`verify` | 350〜550（本体）、150〜250（テスト） |
| 8 | Linux、macOS、iPad を同じ `KisRhiCanvas`（OpenGL / Metal）に移す（任意。Q4 の「1 系統」のため） | Linux の X11 と Wayland、macOS、iPad で、Q1 の値より悪化しない。GL のフォールバックが残る | Linux の計測（監査役）、Mac と iPad の実機の確認。`verify` | 500〜800（本体）、150〜250（テスト） |
| 9 | HDR と 10bit の `KisDxgiCanvasWindow` を作る（scRGB、HDR10、10bit SDR） | 外部の HDR と 10bit の画面で、形式と色空間が要求どおりで、テストパターンと SDR の戻しが正しい。SDR の画面で見た目が Q1 と同じ。遅れと CPU が段 7 から悪化しない | `TestDxgiFormatSelection`（画面の情報→形式と色空間）、SDR のフォールバックのテスト、外部の画面の実機の確認（監査役と利用者） | 700〜1,200（本体）、200〜300（テスト） |
| 10 | 9 件以外の Windows の改変を 1 件ずつ実装する（第 4 節の表） | 各項目の確認（Chrome と Edge の貼り付け、タブの題名、日本語配列、タッチ、ドック、Wintab の相対モード）が通る | 各項目の小さなテストと、Surface の手動の確認 | 100〜200（本体）、50〜100（テスト） |
| 11 | フォークの構築定義、L1 の切り替え、GL のキャンバスを削除する | `nix flake check --no-build --all-systems` が通り、全プラットフォームが正式版 Qt で構築できる。フォーク固有の API の参照が 0 件。Q1 の主指標より悪化しない | 走査のテスト、`verify`、全プラットフォームの構築 | 削除 1,000〜1,500、追加 0 |

- 合計の見込みは、本体 5,400〜7,500 行、テスト 1,400〜2,200 行、削除 1,750〜2,400 行。
  段 6 が最も大きく、数週間を見込む。
- 段 3 と段 4 の間で一度、Surface のペンの操作の確認と、ホバーとインクの
  5 往復の計測を監査役に依頼する。段 7 と段 9 でも同じ計測を依頼する。
  段 1 では、正式版 Qt の Windows の起動と、ホバーとインクの基準の 5 往復だけを依頼する。
- 各段の終わりに `verify-quick`、境界を変える段（2、5、8、11）は `verify` を通す。
  構築は `build-incremental windows-upstream-qt` を 1 本だけ systemd で動かし、
  終了を待ってから次へ進む。

### 5.2 段 1 の具体的な変更

段 1 は「フォーク固有の CMake の検出マクロと API の参照を外しても Windows の構築が
通る状態」にする。次の順に小さくコミットする。

1. `CMakeLists.txt:681-799` の `check_cxx_symbol_exists` / `CHECK_CXX_SOURCE_COMPILES`
   を削除し、`config-qt-patches-present.h`、`config-hdr.h`、
   `config-qmdiarea-always-show-subwindow-title.h`、`config_qt5_has_wintab_switch.h`
   の値を明示的に偽にする（`config-*.h.cmake` を残す場合は `#define ... 0`）。
   `KRITA_QT_HAS_*` の `option()` と `add_feature_info` も削除する。
2. `#if KRITA_QT_HAS_*`、`#ifdef HAVE_HDR`、`#ifdef HAVE_QMDIAREA...`、
   `#ifdef QT5_HAS_WINTAB_SWITCH` の分岐を削除し、偽の側のコードだけを残す。
   対象は `krita/main.cc`、`libs/ui/canvas/kis_canvas2.cpp`、
   `libs/ui/opengl/kis_opengl_canvas2.cpp`、`libs/ui/opengl/kis_opengl.cpp`、
   `libs/ui/opengl/KisScreenInformationAdapter.cpp`、
   `libs/ui/opengl/KisOpenGLModeProber.cpp`、
   `libs/ui/dialogs/kis_dlg_preferences.cc`、`libs/application/ui/workspace/KisMainWindow.cpp`、
   `libs/application/ui/orchestration/KisApplication.cpp`、`libs/input/ui/kis_input_manager.cpp`、
   `libs/input/ui/kis_input_manager_p.cpp`、`plugins/tools/svgtexttool/SvgTextCursor.cpp`、
   `plugins/dockers/lut/lutdocker_dock.cpp`、`plugins/dockers/smallcolorselector/KisGLImageWidget.cpp`。
3. フォーク固有の API（`QOpenGLWidget::setTextureColorSpace`、`QSurfaceFormat` の
   HDR の色空間の選択、`QRhi::isLastFrameCompletedOnGPU` の検出、ANGLE の EGL の
   H1 のコード、`QScroller` の印、Android の属性）の参照を削除する。
   `QNativeInterface::Private::QWindowsApplication` は正式版にあるので残す。
4. `nix/windows/krita.nix` と `nix/windows/default.nix` の既定を
   `useQtFork = false` にし、ANGLE の入力と `-DHAVE_HDR=ON` を外す。
   フォーク版の出力は、段 7 の合格まで戻せるように残す（削除は段 11）。
5. HDR の設定ページは、段 9 で作り直すまで「この版では使えない」と表示して
   無効にする。見た目の変更は段 9 の完了時に戻す。
6. `scripts/tests/test_q4_no_fork_api.py` と
   `scripts/tests/test_q4_windows_build_config.py` を足す。前者は
   `KRITA_QT_HAS_`、`HAVE_HDR`、`setTextureColorSpace`、
   `AlwaysShowSubwindowNameInTitleBar`、`QT5_HAS_WINTAB_SWITCH`、
   `KRITA_QSCROLLER_PATCH`、`isLastFrameCompletedOnGPU` を、許可した文書と
   テスト以外で見つけたら失敗する。後者は `nix/windows` の既定と
   `angle = null`、`HAVE_HDR` の指定がないことを確かめる。

段 1 の完了の確認は、`systemctl --user list-units "q*" "librepaint-*"` で
他の構築がないことを確かめ、`build-incremental windows-upstream-qt configure` と
`build krita` を systemd の user unit で 1 本ずつ動かし、`Result=success` を
確かめる。続いて `verify-quick` を通す。Surface の実機の起動と 5 往復は
監査役に依頼する。

## 6. 判定をお願いしたいこと

1. **段の順序**（第 5 節）と、段 1 の範囲（フォーク API の削除、Windows の既定を
   正式版 Qt にし、フォーク版の構築を段 7 まで残す）を認めるか。
2. **表示経路**: Windows の SDR を `QRhiWidget` の D3D11 にし、HDR と 10bit を
   LibrePaint が持つ DXGI の flip のスワップチェーンで作る案を認めるか。
   Qt の `QBackingStoreRhiSupport` に小さなパッチを当てる案は、利用者の
   「LibrePaint 側で作り直す」という決定と合わないため採用しない。
3. **Linux と macOS**: Windows の合格までは今の `QOpenGLWidget` のままにし、
   その後で同じ `KisRhiCanvas` を OpenGL と Metal に移す案を認めるか。
   Q4 の「1 系統」は段 8 と段 11 で満たす。
4. **ペンの 9 件**: すべて L（LibrePaint 側）で作る。051139ed72 は Wintab の
   対応範囲の補正の橋渡しとし、f5a75c4071 はキャンバスに不要として延期、
   0027358f20 は作らない。この取捨を認めるか。
5. **9 件以外の Windows の改変**（第 4 節）の 1 行ずつの決定を認めるか。
   特に、フレームレート圧縮と fast transfer を作り直さず、CPU と電力の計測で
   +10% を超えたときだけ足す点。
6. **計測の点**: 段 1、段 4、段 7、段 9 の後に Surface の 5 往復を依頼する。
   段 7 の合格の目安は、ホバー 12.4ms 以下か P95 5% 改善、インク 17.6ms 以下か
   P95 5% 改善、CPU が ANGLE +10% 以内、電力 +10% 以内、ペンの操作が壊れないこと。

## 7. 出典

- `docs/perf/q4-fork-removal.md` 2.7（9 件の P と L の見積もり）と 3（失われる機能）
- `docs/perf/l2-flip-presentation.md`（flip、待てるスワップチェーン、QRhiWidget、採用例）
- `docs/perf/qt-fork-patches.md`（9 件の判定と根拠）
- `docs/perf/DECISIONS.md` 2026-11-11 の Q4 と表示経路の行
- Qt 6.11.3 のソース: `src/gui/painting/qbackingstorerhisupport.cpp`（ウィジェットの
  スワップチェーンは `QRhiSwapChain::SDR`）、`src/gui/rhi/qrhid3d11.cpp`
  （HDR の形式と色空間、待てるスワップチェーン）、`src/widgets/kernel/qrhiwidget.cpp`
- Qt の文書: `QRhiSwapChain`（`HDRExtendedSrgbLinear`、`HDR10`、`hdrInfo`）、
  `QRhiWidget`（D3D11 が Windows の既定、1 ウィンドウ 1 バックエンド）
- DXGI: `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT`、
  `IDXGISwapChain2::SetMaximumFrameLatency`、`IDXGISwapChain3::SetColorSpace1`、
  `IDXGIOutput6::GetDesc1`、HDR の色空間の定数
- Krita Manual「Display Settings」「HDR Display」「Tablet Settings」
- Adobe Photoshop Help「Configure Photoshop performance preferences」
  「Tablet support FAQ」
- Chrome、mpv、Firefox、Blender、OBS の提示の実装（`l2-flip-presentation.md` の出典）
