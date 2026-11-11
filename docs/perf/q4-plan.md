# Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）の計画

## 0. 目的と範囲

この文書は、Q4 を進めるための実装計画である。読者は監査役（設計と監督）と作業者（実装）である。範囲は、利用者の決定（`docs/perf/DECISIONS.md` の 2026-11-11 の Q4、描画方式、HDR、自動の確認の行）で定まった次の 4 つである。

1. Windows も正式版 Qt 6.11.3 で構築し、フォーク固有の API と構築定義の参照をなくす。
2. Windows のペンの操作に必要なフォークの改変 9 件を、LibrePaint 側で作り直す（第 2 節）。
3. 9 件以外の Windows の改変を、旧版の見た目と操作を保つ範囲で作り直すか捨てる（第 3 節）。
4. Windows の描画は、ドライバーの OpenGL を直接使う方式（Qt の呼び名は desktop）を既定にする。ANGLE は正式版 Qt にないため、経路から外れる。

次の作業は範囲の外に置く。

- D3D11 への表示経路の移し替え（`QRhiWidget`）。利用者の決定で行わない。理由は、ドライバーの OpenGL を直接使う方式のアプリの CPU の増加が機械全体で約 1.7 ポイントと小さいこと、表示経路は #108 のフェーズ G で wgpu に置き換える予定で、その前に別の経路を作ると作業が二重になることである。
- Windows の HDR の表示（10bit、scRGB、Rec.2020 PQ）。フェーズ G の wgpu の表示経路で作り直す（利用者の決定）。それまでの Windows の開発版は HDR なしで進める。
- Linux と macOS のキャンバスの変更。今の `QOpenGLWidget` のキャンバスのままにする。

範囲の外の作業が必要に見えたときは、作業者は止まって判定を依頼し、監査役は利用者に尋ねる。

指標は、Surface Pro 7 のホバー輪郭とインクの入力から表示まで（中央値と P95）、アプリの CPU（機械全体で何ポイント増えるか）、電池の消費電力、表示の正しさ、ペンの操作の正しさである。

## 1. 前提と現状

### 1.1 決定済みのこと

- Q4 は上策で進む（利用者の決定、2026-11-11）。範囲は第 0 節のとおり。
- Windows の既定の描画方式は desktop であり、D3D11 への移し替えは行わない（利用者の決定、2026-11-11）。アプリの CPU の増加は絶対値で記録し、電池の消費電力を確認する。
- Windows の HDR はフェーズ G の wgpu の表示経路で作る（利用者の決定、2026-11-11）。
- フォークを使うのは Windows の既定の構築だけで、Linux、macOS、Android、iOS は既に正式版 Qt 6.11.3 で構築されている。
- 正式版 Qt の Windows 版は、ANGLE を持たないため、ドライバーの OpenGL を直接使う方式になる。起動時の最初のフレームの検査（`KisOpenGLStartupCheck`）が失敗した描画方式は、次の起動で飛ばされる。
- 確認は Surface の自動の確認（`scripts/perf/check.py`、シナリオ `scripts/perf/scenarios/q4-e2e.json`）で行う。合成入力で再現できない項目（実物のペンの近接、指のタッチ、Wintab のパケット）だけを、Q4 の最後に実物のペンで確かめる。
- 段 1、段 2、段 3、段 4、段 10 の実装と、L2 の CPU の候補の切り替えの削除は終わっている（progress.md 2026-11-11）。残りは、段 2・3・10 の自動の確認、段 4 の実物のペンの確認、Windows の既定を desktop にする変更、フォークの構築定義と L1 の切り替えの削除である。

### 1.2 基準にする値（Surface Pro 7、同じ合成入力の軌跡）

| 版 | ホバー中央値 | インク中央値 | アプリの CPU（入力 32.7 秒） | 機械全体の CPU |
| --- | --- | --- | --- | --- |
| 1.0.3（フォーク版、ANGLE） | 約 62ms | 約 61ms | 未計測 | 未計測 |
| フォーク版、ANGLE（先積み 1、ホバーの間引きなし） | 35.1ms | 39.1ms | 6.45 秒（論理 CPU 1 個の 19.7%） | 約 2.5% |
| フォーク版、ドライバーの OpenGL を直接使う方式 | 12.4〜13.6ms | 17.6〜18.6ms | 10.84 秒（33.2%） | 約 4.1% |
| 正式版 Qt、段 3（2026-11-11 14:13） | 9.9ms | 22.3ms | 9.97 秒（30.5%） | 約 3.8% |

機械全体の CPU は、Surface Pro 7 の 8 個の論理 CPU を 100% とした値である。desktop と ANGLE の差は、入力 32.7 秒で 4.39 CPU 秒、論理 CPU 1 個の 13.4%、機械全体で 1.7 ポイントである。main スレッドは 5.21 秒から 9.06 秒に増え、1 フレームあたり 1.77ms の増加、合計 4.43ms/フレームで、16.7ms の表示の周期に収まる。

正式版 Qt のインクがフォーク版より約 4ms 遅いのは、正式版 Qt の描画の回数が表示 1 回あたり 1.7〜2.0 回で回ごとに変わり、2 回未満の回で線の端が 1 表示分古いフレームに残るためである（progress.md 2026-11-11 12:55）。

電池の消費電力は未計測である。`hosts.json` の Windows の対象の `power` を `on` にし、AC を外して `power.py` で入力の区間の平均の放電の速さ（mW）と放電量（mWh）を測る。

### 1.3 完了条件

- 全プラットフォームが正式版の同じ Qt 6.11.3 で構築でき、LibrePaint のソースと Nix の構築定義がフォーク固有の API と入力を参照しない（`scripts/tests/test_q4_no_fork_api.py`）。
- 主指標（ホバー輪郭とインクの入力から表示まで、中央値と P95）が全プラットフォームで Q1 の値より悪化しない。
- Windows のアプリの CPU は、機械全体で何ポイント増えるかを絶対値で示す。目安は、ANGLE の H1（先積みのフレーム数）=1 と H5（ホバーの間引き）の組から +2 ポイント以内であり、main スレッドが 1 フレーム 16.7ms に収まることである。
- 電池の放電電力を絶対値（mW と mWh）で記録する。差が計測のばらつき（5%）を超えるときは、監査役が利用者に表示経路を相談する。
- Surface の自動の確認が、段 2・段 3・段 10 の項目ですべて合格する。

## 2. ペンに必要な 9 件を LibrePaint 側で作り直す

### 2.1 置き場所の方針

9 件は、次の 4 つの場所で作る。Qt の内部クラス（`QWindowsPointerHandler`、
`QWindowsTabletSupport`、`QWidgetWindow`）は使わない。

- 共通ガード：`libs/input/ui/kis_input_manager.cpp` の `KisInputManager::eventFilterImpl` と
  `KisInputManager::Private`（`containsPointer`、`ProximityNotifier`）に、タブレットの
  近接・押下中の `QEvent::Leave` を無視する小さなガードを置く。公開の Qt イベントだけで足りる。
- Windows のネイティブフィルター：`libs/input/ui/` に新しい
  `KisWindowsPointerEventFilter`（`QAbstractNativeEventFilter`）を置き、`krita/main.cc`
  の `KisLatencyProbe::installNativeFilter(&app)` の隣で
  `qApp->installNativeEventFilter()` する。`windows_generic_MSG` の `WM_POINTER*`、
  `WM_NCPOINTER*`、`WM_NCMOUSEMOVE`、`WM_MOUSELEAVE` を見る。既存の
  `KisLatencyProbe::NativeFilter` を土台にする。計測用のフィルターとは分け、通常の起動でも動くようにする。
- 入力の抑止：`libs/input/KisInputEventSuppressor.{h,cpp}` に、Windows の Wintab で
  合成マウスを落とす判定を足す。公開の Qt イベントと既存の枠組みだけで足りる。
- Wintab の対応範囲：`libs/input/ui/wintab/` に `KisWintabMapping` を置き、Wintab の
  論理座標（`lcSysOrg`/`lcSysExt`）と Windows の仮想画面の矩形を比べ、違うときだけ
  `QTabletEvent` の位置を補正する。`wintab32.dll` を動的に読み、Wintab API を直接呼ぶ。
  パケットの時刻の扱いは `KisWintabPacketPeeker`、押下での補正の信頼の判定は
  `KisWintabTrust` が持つ。

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
- 段 2、段 3、段 4 の実装は入っている（2026-11-11）。段 4 は同じ時刻のパケットの順序と、
  押下の位置の差が 2.0px を超える近接で補正を止める判定まで含む。残りは実物のペンでの確認である。

### 2.3 リスク

- Qt6 の Wintab と Windows Ink の切り替えは、正式版 Qt 6.11.3 にもある
  `QNativeInterface::Private::QWindowsApplication::setWinTabEnabled()` /
  `isWinTabEnabled()` をそのまま使う。9 件のために新しい QPA のネイティブインターフェースは足さない。
- ネイティブフィルターは GUI スレッドのメッセージ処理の前後に挟まる。計測用の
  `KisLatencyProbe` と同じく、通常の起動では仕事をしない（フラグ 1 回の判定）形にし、
  ペンの経路に文字列の確保やロックを足さない。
- `KisWintabMapping` は Wintab のコンテキストを Qt と二重に持たない。設定の読み取りだけを
  行い、パケットは Qt の経路に任せる。二重のコンテキストが必要になった場合は、
  Qt の Wintab を切り、自前のコンテキストに置き換える（その時点で行数を見積もり直す）。
- Linux と macOS では共通ガードだけが動く。ホバーとタッチの回帰を、Windows の後ではなく
  同じ段で確かめる（`verify` のテストと、Mac の実機の確認）。

## 3. 9 件以外の Windows の改変の扱い

1 件ずつ、残す・捨てる・測ってから決めるを決める。機器を選ばない項目は段 10 として実装済みである。

| 改変 | 決め方（1 行） |
| --- | --- |
| タッチの受理（`a60ed01a25`） | 残す。Surface のタッチでパン・ズームとペンの競合を確かめ、競合するときだけ `RegisterTouchWindow` と `SetGestureConfig` をネイティブフィルターに足す（30〜60 行）。 |
| クリップボードの CF_DIB の回避（`d850335b2d`） | 残す。実装済み。アルファを持つ画像を CF_PNG と `image/png` で出し、不透明な画像だけ `setImageData()` を使う。Chrome と Edge への貼り付けを自動の確認で確かめる。 |
| タブ表示の文書名（`f786da6451`） | 残す。実装済み。`KisMainWindow::setActiveView()` でビューの題名を窓へ写す。自動の確認で確かめる。 |
| 非ラテン配列のショートカット（`82d2864a8a` ほか） | 残す。実装済み。`KisInputManager` の既存の回避コードを Windows でも既定で有効にする。自動の確認で Ctrl+Tab と修飾キーの残りを確かめる。 |
| アクセシビリティを切る設定（`9dc7c77358`） | 起動時だけ残す。実装済み。`QT_DISABLE_ACCESSIBILITY` を `QApplication` の前に設定する今の形を維持し、実行中の切り替えは作らない。 |
| Qt のフレームレート圧縮（7 件） | 作り直さない。正式版 Qt の更新の経路をそのまま使い、CPU と電力を測る。機械全体の CPU の増加が +2 ポイントを超えたときだけ、`KisCanvas2` の更新の圧縮を小さく足す。 |
| fast transfer（`fb923e4a97`） | 作り直さない。CPU と電力を測り、機械全体の CPU の増加が +2 ポイントを超えたときだけ、テクスチャーの転送のまとめ方を変える。 |
| ドックのクラッシュの修正（`8dadf099b8`） | 再現を試す。再現したら LibrePaint 側で直す（20〜50 行）。再現しなければ作らない。 |
| Wintab の相対モード（`7bb46828ae`） | Wintab の実機で確かめる。相対モードになるなら `KisWintabMapping` で絶対座標に直す（20〜40 行）。 |
| ANGLE のリサイズ・ダイアログのちらつきの回避（`1a543f2832` ほか） | 捨てる。実装済み。ANGLE がないため環境変数の設定を削除する。desktop でリサイズとダイアログを自動の確認で確かめる。 |
| `KRITA_QSCROLLER_PATCH` の検出 | 捨てる。実装済み。本体が参照していないため、CMake の検出と設定を削除する。 |
| Android の 5 つの分岐 | 触らない。現在も偽で、Q4 で新たに失わない。Krita の Android 版との差を埋めるかは別の作業にする。 |
| `QT_BACKING_STORE_USE_FAST_QIMAGE_TRANSFER` と `QT_FRAME_RATE_OVERRIDE` | 捨てる。実装済み。フォークの圧縮の環境変数であり、正式版 Qt では効かない。設定の削除と一緒に消す。 |
| `QT_USE_PREMATURE_RESIZE_EVENTS` ほかの ANGLE の回避 | 捨てる。実装済み。ANGLE の分岐ごと削除する。 |

## 4. 実装の順序と完了条件

各段は小さなコミットに分け、`verify-quick` を通して push する。境界を変える段は `verify` も通す。構築は systemd の user unit で 1 本ずつ動かす。各段の確認は Surface の自動の確認（`scripts/perf/check.py`）を入口にする。主指標と CPU・電力は、同じ実行の `run.py` の計測で読む。

### 4.1 実施済みの段

| 段 | 内容 | 状態 |
| --- | --- | --- |
| 1 | フォーク固有の CMake の検出マクロと API の参照を外し、Windows の既定を正式版 Qt にする | 完了（2026-11-11） |
| 2 | ペンの共通ガード（df3a678309、41adadd8db、c132390390、8f8eabfb72）を公開の Qt イベントで作る | 完了（2026-11-11、実物のペンでも確認） |
| 3 | Windows のネイティブフィルター（c41d66281e、33a49f25d1） | 完了（2026-11-11、実物のペンでも確認） |
| 4 | Wintab の対応範囲（051139ed72）を `KisWintabMapping` で作る | 実装完了（2026-11-11）。実物のペンの確認待ち |
| 10 | 9 件以外の Windows の改変のうち機器を選ばない項目 | 実装と verify は完了。自動の確認待ち |
| C | L2 の CPU の候補の切り替え（テクスチャーの記憶域、GL のフェンス、`makeCurrent` の省略、カラーセレクタ） | 監査役の判定で削除済み（2026-11-11 17:57）。印は計測の道具として残す |

### 4.2 残る段の順序、完了条件、見込みの行数

旧段 5〜7（Windows のキャンバスを QRhiWidget の D3D11 に移す）と旧段 8（Linux と macOS を同じ RHI に移す）は計画から外す。旧段 9（HDR と 10bit の DXGI のウィンドウ）はフェーズ G の wgpu に移す。旧段 11 からは GL のキャンバスの削除を外し、フォークの構築定義と残る L1 の切り替えだけを削除する。

| 順 | 内容 | 完了条件 | 確認 | 見込み行数 |
| --- | --- | --- | --- | --- |
| 1 | 段 2・3・10 の自動の確認を確定する | 段 3 と段 10 で `clipboard`、`tabs`、`keyboard`、`flicker` が合格し、ペンの 7 項目（`pen.enter_leave`、`pen.edge_line`、`pen.single_line`、`pen.long_press`、`pen.dot`、`pen.nonclient_return`、`pen.first_line`）が合格する。ちらつきは同じ実行のフォーク版の回数に許容（真っ白 2、更新なし 2）を足した範囲に収まる | `scripts/perf/check.py` と `scripts/perf/scenarios/q4-e2e.json`。結果は `checks.json` に根拠のパスとともに残す | 0（確認のみ） |
| 2 | 段 4 の Wintab の経路を実物のペンで確かめる | `kritadisplayrc` を一時的に `useWin8PointerInput=false` にして起動し、`probe.csv` に `wintab_peek`、`wintab_fix`、`wintab_trust` が出る。`wintabResolutionMode=custom` で画面の左半分を指定したとき、タブレットの全面の線が左半分に収まる。確認の後に設定を戻す | 実物のペン（合成入力は WM_POINTER なので `WT_PACKET` を作れない）。手順は `docs/architecture/DEVELOPMENT.md` にある。Surface の Windows Ink の経路は順 1 の `check.py` で確かめる | 0（確認のみ） |
| 3 | Windows の既定を desktop にし、正式版 Qt の範囲で戻す先を整える | 起動時に desktop が選ばれ、失敗したときは Qt のソフトウェア OpenGL と QPainter に戻る。最初のフレームの検査が失敗した方式を次の起動で飛ばす。主指標が Q1 より悪化せず、機械全体の CPU の増加が +2 ポイント以内、main スレッドが 1 フレーム 16.7ms 以内、電池の放電電力の差が 5% 以内 | `check.py` の全項目と、同じ回の `run.py`（`power=on`、AC を外す）。契約テストは既定、候補の順、明示した選択の優先 | 本体 60〜120、テスト 40〜80 |
| 4 | フォークの構築定義と、残る L1 の実験用の切り替えを削除する。GL のキャンバスは残す | `nix flake check --no-build --all-systems` が通る。フォーク固有の API と構築入力の参照が 0 件。Q1 の主指標より悪化しない。`check.py` の全項目が合格する | `scripts/tests/test_q4_no_fork_api.py`、`verify`、全プラットフォームの構築、`check.py` | 削除 300〜500、追加 0〜50、テスト 100〜150 |

残る段の合計は、本体 60〜120 行、テスト 140〜230 行、削除 300〜500 行である。

順序は、実装済みの段の確認を先に確定し、その後に Wintab の実物のペンの確認、desktop の既定、削除とする。desktop の既定をペンの確認の後に置くのは、ペンの経路の変更と描画方式の変更を同じ回に混ぜないためである。削除を最後に置くのは、比較に使うフォーク版の構築と L1 の切り替えを、ほかの確認が終わるまで残すためである。

## 5. 範囲の外に置いたものと、その扱い

| 項目 | 扱い |
| --- | --- |
| D3D11 への表示経路の移し替え | 行わない。電池の消費電力の計測でフォーク版の ANGLE との差が大きく出た場合だけ、監査役が利用者に相談する |
| Windows の HDR | フェーズ G の wgpu の表示経路で作る。HDR の設定ページは、それまで「この版では使えない」と表示する |
| Linux と macOS のキャンバス | 今の `QOpenGLWidget` のまま |
| 正式版 Qt の描画の回数（表示 1 回あたり 1.7〜2.0 回） | Q4 の範囲の外。インクの遅れへの影響（約 4ms）は L2（Windows の遅れを直す）の課題として #108 に記録し、扱いを利用者に尋ねる |
| タッチの受理、ドックのクラッシュ、Wintab の相対モード、フレームレート圧縮、fast transfer | 第 3 節の表のとおり。測ってから決める項目は、その測定の結果を監査役が利用者に示してから作るかを決める |

## 6. 出典

- `docs/perf/q4-fork-removal.md` 2.7（9 件の P と L の見積もり）と 3（失われる機能）
- `docs/perf/qt-fork-patches.md`（9 件の判定と根拠）
- `docs/perf/DECISIONS.md` 2026-11-11 の Q4、描画方式、HDR、自動の確認の行
- `docs/perf/l2-flip-presentation.md` と `docs/perf/l2-renderer-choice.md`（表示経路の比較と、採用しなかった案の記録）
- `build/agent-loop/progress.md` の 2026-11-11 の節（段 1〜4 と段 10 の実装、CPU の候補の計測と削除）
- `scripts/perf/check.py`、`scripts/perf/scenarios/q4-e2e.json`、`scripts/perf/hosts.example.json`（自動の確認の入口と項目）
- `docs/architecture/DEVELOPMENT.md` の性能計測の節（Wintab の実物のペンの手順、`events.log`、自動の確認）
