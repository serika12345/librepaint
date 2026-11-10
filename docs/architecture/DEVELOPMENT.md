# LibrePaint開発マニュアル

環境構築、日常の編集と試験、各OSの成果物作成、実機配備、保守の手順を本書にまとめる。
コマンドはリポジトリルートで実行する。`<...>`と`[...]`は置き換える値と任意引数を表す。

| やりたいこと | 本書の参照先 |
| --- | --- |
| 最初の準備、普段のビルドと試験 | [開発環境](#開発環境)、[編集と検証](#編集と検証) |
| macOS／Linux／Windowsのアプリを作る | [デスクトップ](#デスクトップ) |
| AndroidのAPK／AABを作る、端末で試験する | [Android](#android) |
| iPhone／iPadのIPAを作る、実機へ配備する | [iOS／iPadOS](#iosipados) |
| 依存物、キャッシュ、文書を更新する | [保守](#保守) |
| 作業を再開する、Issueへ結果を残す | [Issueによる作業管理](#issueによる作業管理) |
| 複数担当で実装する | [責務単位の並列実装](#責務単位の並列実装) |

## 開発環境

Nix（Flakes有効）とdirenvを用意し、シェルのdirenv連携を有効にする。
コンパイラー、CMake、Ninja、CTest、検査ツールはNixから供給する。

| 対象 | 構築ホスト | 増分構築の指定 |
| --- | --- | --- |
| macOS | macOS | `native` |
| Linux | Linux | `native` |
| Windows | x86_64 Linux | `windows` |
| Windows（正式版Qt、Q4の前段） | x86_64 Linux | `windows-upstream-qt` |
| Android ARM64／x86_64 | x86_64 Linux | `android`／`android-x86_64` |
| iOS／iPadOS arm64 | Apple Silicon Macと固定Xcode | `ios` |

初回に次を実行する。

```sh
direnv allow
```

以後は`.envrc`が`test`シェルを読み込み、`scripts/`を`PATH`へ追加する。
日常の作業は同じ環境と永続Ninja木を再利用する。

```sh
build-incremental native build krita
run-test kis_strokes_queue_test
verify-quick
```

自動処理など、direnvの環境を継承しないプロセスでは次の入口を使う。
主作業ツリーの評価済み`.direnv/flake-profile`を`nix print-dev-env`で読み込む。

```sh
./scripts/run-shared-test-env ./scripts/build-incremental native build krita
./scripts/run-shared-test-env ./scripts/run-test kis_strokes_queue_test
./scripts/run-shared-test-env ./scripts/verify-quick
```

初回プロファイル作成と、開発シェル・Flake入力・`flake.lock`・ソース絞り込みの変更後は、
`nix develop .#test`で環境を評価する。文書と図だけの場合は`nix develop .#docs`を使える。
ローカルFlakeの評価は作業ツリーをNix storeへコピーするため、通常の編集・試験には
評価済み環境を使う。追加ツールは必要な最小のNixシェルへ定義し、一時利用は`nix shell`で用意する。

## 編集と検証

### 増分構築

```sh
build-incremental <platform> <operation> [target]
```

| 操作 | 用途 |
| --- | --- |
| `path` | 選択中のNinja木を表示 |
| `configure` | CMake構成を同期し、依存方向と製品ターゲットの循環を検査 |
| `plan [target]` | 同期済みの構築計画をコンパイル・リンクせず表示 |
| `build [target]` | 対象と必要な依存物の変更分を構築 |
| `bootstrap [target]` | 初回または大きい構成変更後の構築基準を作成 |
| `cache-stats` | ネイティブ・Android・Windowsのコンパイラーキャッシュ統計を表示 |

プラットフォーム固有の追加操作は各OSの節と`build-incremental`の使用方法に従う。
ネイティブの`configure`、`plan`、`build`、`bootstrap`は、ルートの
`compile_commands.json`を選択中の構築木へ接続する。clangdはこの実際の構築条件を読む。

| 対象 | 永続構築木 | コンパイラーキャッシュ |
| --- | --- | --- |
| macOS／Linux | `build/tdd-macos`／`build/tdd-linux` | `.cache/librepaint/ccache/native` |
| iOS | `build-ios/krita/device-incremental/<構成指紋>` | `.cache/librepaint/ccache/ios` |
| Android ARM64／x86_64 | `build/android/<ABI>/<構成指紋>` | `.cache/librepaint/ccache/android`／`android-x86_64` |
| Windows | `build/windows/x86_64/<構成指紋>/ninja` | `.cache/librepaint/ccache/windows` |

iOS、Android、Windowsは依存定義ごとの環境を`build-ios/nix-profiles/`または
`build/nix-profiles/`へ固定する。製品ソースの編集では同じ環境・Ninja木・キャッシュを再利用し、
依存定義の変更では新しい構成指紋の木を使う。

### 実装前の構築範囲監査

対象の実装、利用側、試験、CMake定義、対応Issueを読み、変更で守る結果を決める。
コードと試験を編集する前に、変更のない状態の増分計画と直接依存を確認する。
新設・拡張対象は、空の構築木で必要になるコマンド数を最も近い既存契約と比較する。

```sh
build-incremental native plan <target>
ninja -C "$(build-incremental native path)" -t commands <target> | wc -l
```

CMake定義とFile API応答で、試験が利用する具体的な所有者へ依存していることを確認する。
アプリ全体、全プラグイン集合、`all`、無関係なUIへの依存は、挙動変更の前に責務を分けて縮小する。
大きい構築範囲が必要な場合は、直接依存、工程数、縮小に必要な製品分割を会話で報告する。
Issueへの反映には、後述の「Issueによる作業管理」の依頼・文面承認手順を適用する。

公開ヘッダーの変更波及は、同期済みの構築木で変更前後を測定する。

```sh
./scripts/architecture/measure_change_impact.py \
  "$(build-incremental native path)" libs/global/KoID.h \
  --platform macos --owner-target kritaglobal --contract-target KoIDContractTest
```

直接取込み、推移的なコンパイル工程、翻訳単位、直接リンク、再リンク、空構築の工程数を
同じ条件で比較する。機械可読出力は`--json`、時間とキャッシュは`time`と`cache-stats`を使う。

### テスト駆動開発と既存試験

1. 利用者、操作、観測結果、失敗時の具体的な不具合を定める。
2. 最小の振る舞い試験を追加し、期待する最初の診断を確認する。
3. 契約を満たす製品変更を実装する。
4. 試験が成功したまま責務、依存方向、所有寿命、公開APIを整理する。
5. 影響範囲のCTestと必要なプラットフォーム検査を実行する。

既存試験は呼出元の必要な保証を維持する。宣言形状だけを固定する検査は、その互換性要件を
確認して保守する。互換性検査には利用者と維持対象を記載する。
振る舞いの試験は公開操作の結果を検証し、呼出元の必要な保証と合わせてレビューする。

画像試験はキャンバス、色空間、ブラシ、入力列、乱数種、並列実行条件、比較方法と出典を固定する。
基準画像は維持する契約、既知不具合、未確定事項へ分類して受け入れる。

### 検証階層

| 変更範囲 | 実行する検証 |
| --- | --- |
| 文書 | 参照先と設計の整合をレビュー。図の変更はSVGを再生成 |
| 設計境界の方針・検査スクリプト | `verify-quick` |
| 構築・配布スクリプト | 変更した操作と成果物の境界検査 |
| 一つの振る舞いの変更 | `run-test <target> [ctest-regex]`、影響範囲CTest、`verify-quick` |
| 広い共有境界・統合 | `verify` |
| CMake構成 | 対応する`build-incremental <platform> configure`と対象構築・試験 |
| Nix出力・環境・依存・ソース絞り込み | `nix flake check --no-build --all-systems`と変更出力の構築 |
| 入力・描画・OS境界 | 対象機器の入力、保存、復帰、必要な画像・性能・同期検査 |
| 配布定義 | 名前付き`nix build`出力、成果物検査、対象OSでの起動と主要操作 |

`verify-quick`はパッケージ依存方針、公開ヘッダー、プラグイン登録と、設計境界を扱う
検査器・依存解析の試験を実行する。各OSの構成時に実CMakeグラフを検査し、
配布物の生成・導入時にバイナリーと実行時データの構成境界を確認する。

`run-test`の第1引数はCMakeターゲット、第2引数はCTest名の正規表現。
宣言済み依存だけを構築し、同じ構築木の`bin`から必要な製品プラグインを読み込む。
動的探索するプラグインは、その具体的なモジュールを試験のCMake依存へ追加する。

```sh
run-test kis_strokes_queue_test '^libs-image-kis_strokes_queue_test$'
ctest --test-dir "$(build-incremental native path)" \
  --output-on-failure -R '<影響範囲の正規表現>'
```

`verify`は高速検査、ネイティブ全ターゲットの構築、通常CTestを実行する。
macOSは`tdd-macos`、Linuxは`tdd-linux`のCMakeプリセットを使う。
両方とも`BUILD_TESTING=ON`、`KRITA_ENABLE_BROKEN_TESTS=OFF`、
`LIMIT_LONG_TESTS=ON`、`CRASH_ON_SAFE_ASSERTS=ON`。隔離した試験は原因・決定性・比較規則を
確認し、一件ずつ通常検査へ戻す。

構築とテストの並列数は機械に任せる。プリセットは並列数を指定せず、`cmake --build`はninjaの既定
（論理プロセッサー数＋2）、`verify`と`run-test`のCTestは論理プロセッサー数で動く。メモリーが
足りない機械では、構築を`CMAKE_BUILD_PARALLEL_LEVEL`、CTestを`CTEST_PARALLEL_LEVEL`で下げる
（例：`CMAKE_BUILD_PARALLEL_LEVEL=4 CTEST_PARALLEL_LEVEL=4 ./scripts/verify`）。

通常の共有UI変更はmacOSのネイティブ検査とiPadOSの構築・静的監査を優先する。
他OSを所有する作業、リリース前、そのOSでの問題発覚時は該当OSを追加検証する。
Androidは構築費用が大きいため、この条件に沿って検証する。

### 性能計測（ペン入力から表示まで）

描画性能の主指標は、実機でペン入力から画面表示までの時間（中央値とP95）であり、
ホバー中のブラシ輪郭と描画中のインクを分けて測る（[`docs/perf/CHARTER.md`](../perf/CHARTER.md)）。
性能の判断は、次の手順で得た数値で示す。ハイスピードカメラは最後の体感の確認だけに使う。

計測は、決まった軌跡の合成入力を再生しながら画面を取り込み、各フレームに映った輪郭の中心と
インクの先端が、何ms前の入力位置に当たるかを数える。軌跡の両端の静止区間で、ポインターと
描画の位置のずれを較正する。道具は`scripts/perf/pen-latency/`にある。

| 部品 | 役割 |
| --- | --- |
| `trajectory.py` | 全OS共通の軌跡（ホバーの往復、左から右への線）と、キャンバスに対する相対の再生計画。`speed_profiles`で1本の線の中の速さを変えられる |
| `windows_capture.cpp` | Windows。合成ペン入力（PT_PEN）とDXGI Desktop Duplication。時刻はQueryPerformanceCounter |
| `mutter_capture.py` | Linux（GNOME）。mutterのRemoteDesktopとScreenCast（PipeWire）。時刻はCLOCK_MONOTONIC |
| `macos_capture.mm` | macOS。Quartzのマウスイベントと ScreenCaptureKit。時刻はmach_absolute_time |
| `capture_analyze.py` | 取り込み結果から、入力から表示までの時間を集計し、回ごとの中央値のばらつきを出す。速さごとの値と、速さが変わった直後の値も出す |
| `analyze.py` | WindowsのPresentMonの記録と時刻記録（`LIBREPAINT_LATENCY_LOG`）から、区間ごとの内訳を出す。ストロークの待ち行列から実際に dab を描くまでの時間も出す |
| `presentmon_command.py` | WindowsのPresentMonの引数とセッション名を組み立てる。WindowsのPowerShellなしで契約を確かめる |
| `surface_state.ps1` | Windows。対象が`surface_state`を`on`または`light`にした回だけ、端末の状態を記録する。`on`は計測の前・1秒ごとの計測中・計測の後に、熱ゾーンの温度、`% Processor Performance`、全体のCPU使用率、CPUを多く使う上位5プロセスを取る。`light`は5秒ごとの`Get-Counter`1回でCPUの2つだけを取り、温度と上位プロセスは計測の前後だけにする |
| `presentation_check.py` | 表示時刻を通知する参照アプリの記録と取り込みを突き合わせ、取り込みの時刻を検証する |
| 参照アプリ | 入力を受けたらすぐ描く最小のアプリ。`PenProbe.cs`（Windows）、`reference_app.py`と`wayland_reference_app.c`（Linux）、`macos_reference_app.m`と`macos_metal_reference_app.m`（macOS） |
| 記録用ライブラリ | ソースを変えずに画面への受け渡しを記録する`gl_swap_trace.c`（Linux、LD_PRELOAD）、`macos_gl_trace.m`（macOS、DYLD_INSERT_LIBRARIES）、GUIスレッドの呼び出し履歴を採る`stack_sampler.c`（Linux） |

取り込みの時刻と実際の表示時刻の関係は、OSごとに次のとおり確かめてある。

| OS | 確認方法 | 結果 | 扱い |
| --- | --- | --- | --- |
| Windows | 同じ回のPresentMonの表示時刻 | 中央値の差2ms以内。表示方式は取り込み中も変わらない | 補正なし |
| Linux（GNOME） | `wayland_reference_app.c`のwp_presentationの通知 | 取り込みの時刻が表示より15.8〜16.7ms遅い | 16msを引く |
| macOS | `macos_metal_reference_app.m`のpresentedTime | 差+0.5ms | 補正なし |

Windowsでは、計測対象を利用者の画面のセッションで動かすため、スケジュールされたタスクを
一度登録する。PresentMon（2.x）は計測機の`~/pen-latency/PresentMon.exe`に置く。
PresentMonは、短い固定のセッション名`LibrePaintPenLatency`で、起動中のアプリのプロセスIDを
対象に記録する。runnerは記録の前後に`logman`でそのセッションを止め、開始の有無、終了コード、
CSVと標準エラー・標準出力の大きさを`meta.txt`に残す（セッションは同時に約64までで、回ごとに
名前を変えると強制終了した回のセッションが残るため）。PresentMonが回の途中で止まることがある
ので、`meta.txt`には終了コードに加えて、runnerが止める前にプロセスが終了していたか
（`presentmon_exited_before_stop`）と開始・停止の時刻も残す。回ごとの`status.txt`はその回の
ディレクトリーにも書く。`meta.txt`には、起動時と取り込み前のタイマーの分解能、最前面の窓、
アプリの優先度と親和性も残す（L1（Windowsの約50msの遅れの原因の特定）で状態を決める要因を
比べるため）。

L1の仮説は、同じ版のまま環境変数で切り替える。変数がないときは通常の動作のままである。
`scripts/perf/hosts.json`のWindowsの対象に`env`を書くと、`run.py`が`request.json`に載せ、
`run-surface.ps1`がアプリの起動前に設定して`meta.txt`に`env.<名前>`として残す。

| 変数 | 仮説 | 値 | 効果 |
| --- | --- | --- | --- |
| `LIBREPAINT_L1_MAX_FRAME_LATENCY` | H1（先積みのフレーム数） | 1〜3 | ANGLEのD3D11デバイスに`SetMaximumFrameLatency`を設定する。クライアントの拡張一覧（`eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS)`）とディスプレイの一覧を別々に調べ、EGLのデバイス（`EGL_EXT_device_query`の`EGL_DEVICE_EXT`）からD3D11デバイス（`EGL_ANGLE_device_d3d`の`EGL_D3D11_DEVICE_ANGLE`）を取る。拡張の有無、`eglQueryDisplayAttribEXT`と`eglQueryDeviceAttribEXT`の関数アドレス、拡張の判定にかかわらず呼び出した戻り値、EGLコンテキストとデバイスの取得、`IDXGIDevice1`の成否、設定のHRESULT、読み戻した値を`l1_h1_*`として時刻記録に残す。一覧の文字列は`probe.egl.txt`に1回だけ書く。`analyze.py`が`l1_h1 egl=… client_ext=… display_ext=… device_query=… entry=… display_attrib=… device_attrib=… device=… set=… readback=…`と出す |
| `LIBREPAINT_L1_WAIT_FOR_GPU` | H2（表示待ちの歯止め） | 1 | 前のフレームのGPU完了を待ってから次を描く。待ちは更新の間引きの早期の戻りより前に置き、間引かれた描画でも待ちに入る。待ったかどうかにかかわらず判定を`l1_h2_state`に残し（extraのbit 1が切り替え、bit 2がフェンスの有無、bit 4が完了済み、bit 8が待った、bit 16が間引かれた描画）、待ったときは`gpu_wait_begin`/`gpu_wait_end`も出す（`gpu_wait_end`のextraは1が時間内の完了）。`analyze.py`が`l1_h2 n=… fence=… complete=… waited=… deferred=… wait_done=…`と出し、待ちの長さを`gpu_wait.duration`に出す |
| `LIBREPAINT_L1_SWAP_INTERVAL` | H3（垂直同期） | 0または1 | サーフェスの交換間隔を上書きする |
| `LIBREPAINT_L1_WINDOW_SIZE` | H4（1フレームのGPU負荷） | `幅x高さ` | 起動後の窓の大きさを変える。保存された配置が最大化または全画面なら、その状態を解いてから大きさを設定する |
| `LIBREPAINT_L1_DISABLE_MOVE_COMPRESSION` | H5（ホバーの間引き） | 1 | 10msの移動イベントの圧縮を通さない（`tabletEventsDelay=0`はkritarcで設定する） |
| `LIBREPAINT_L1_DISABLE_UPDATE_THROTTLE` | H6（フォークの更新の間引きの歯止め） | 1 | 前のフレームがGPU上でも描く。Qtの`QT_FRAME_COMPRESSION_MODE=2`も設定する。時刻記録に`last_frame_completed_on_gpu`/`qt_update_deferred`を出す |
| `LIBREPAINT_L1_OPENGL_RENDERER` | H7（フォークなし・ANGLEを使わないOpenGL） | `desktop`/`angle`/`auto` | kritarcの`OpenGLRenderer`を上書きする。正式版Qtの版は別の構築で測る |

切り替えのコードは`libs/global/KisL1Switches.h`がまとめる。それぞれの削除の条件は
L2（Windowsの遅れを直す）で既定にするか、L1の比較が終わった時点で削除する。Windowsの
`meta.txt`の`env.*`は、集計で回ごとの切り替えを確かめるために使う。計測の開始時に
アプリが最前面でなく（`app_foreground_capture=False`）、合成入力の区間の`frame_swapped`
が表示の間隔から見た回数の80%未満の回は、覆われた窓が間引かれて入力に追従していない。
`capture_analyze.py`はこれを`presentation.ratio`と`excluded`に残し、`run.py`がすべての
指標の集計から外して表に「left out」の数を出す。

`hosts.json`のWindowsの対象には、`env`とは別に`kritarc`を書ける。値はkritarcの
空のグループ（最初の`[Group]`より前）のキーで、L2（Windowsの遅れを直す）の
`useMaximumFrameLatencyOne=false`のように隠し設定を回ごとに変えるために使う。
`run.py`が`TARGET_KRITARC`に載せ、`run-surface.ps1`がアプリの起動前にキーを書き、
`kritarc-before.txt`に元のファイルを控えて、アプリの終了後に必ず戻す。`meta.txt`には
`kritarc_original.<キー>`（元の値、なければ`absent`）、`kritarc_override.<キー>`、
`kritarc_restored`を残す。kritarcはWindowsの対象だけで使える。`meta.txt`には、復元の後に
アプリがkritarcへ残した値（`kritarc_after.canvasState`、`kritarc_after.OpenGLRenderer`、
`kritarc_after.useOpenGL`、`kritarc_after.useMaximumFrameLatencyOne`）と、アプリの標準出力・
標準エラーの大きさ（`app_log_bytes`、`app_err_bytes`）も残す。アプリの標準出力と標準エラーは
回のディレクトリーの`app.out`と`app.err`に取る。キャンバスが描き直されない回は、
`canvasState`とアプリのログが、OpenGLの起動に失敗したのか更新の経路が止まったのかを分ける。
回の前に実際に効いていた`[General]`の`newOutlineStyle`と`fpsLimit`は
`kritarc_effective.<キー>`に残す（シナリオのkritarcはWindowsのrunnerが書かないため、
想定ではなく実際の値を残す）。

キャンバスの更新の経路は、時刻記録の次の印で追う。`outline_request`は輪郭の更新の要求、
`outline_state`はその要求の終わりの状態（extraのbit 1が輪郭を持つ道具、bit 2がPaintOp
プリセットがある、bit 4が輪郭の矩形が空でない、bit 8が前の輪郭の矩形が残っている、bit 16が
色のプレビューの矩形がある）、`canvas_outline_doc`はキャンバスのウィジェットに切り取った後の
矩形（x/yが幅と高さ、extraのbit 1が文書の矩形が空）、`canvas_outline_wdg`はキャンバスの
ウィジェットの大きさ（x/y）と切り取りの結果（extraのbit 1が空、bit 2が可視）、
`canvas_update_dispatch`と`canvas_update_issued`は`KisCanvas2`が更新を出した回
（`canvas_update_issued`のx/yはウィジェットの大きさ、extraのbit 1は更新の矩形が空）、
`canvas_paint_event`はQtが届けた描画イベント（x/yが矩形、extraのbit 1が可視、bit 2が最小化、
bit 4が前の`updateRect`が残っている）、`canvas_initialize_gl`と`canvas_resize_gl`は
OpenGLのキャンバスの初期化と大きさの変更を表す。`analyze.py`はこれらを`canvas_path`の1行
（回数とextraの内訳）にまとめる。ストロークが始まらなかった回は、`stroke_begin`のextra
（bit 1が編集できるノード、bit 2が描けるノード、bit 4がPaintOpプリセットがある）で、
押下が届いていないのか、層が描けないのかを分ける。

焦点とツールの状態は、次の印で追う。`qt_event`はFocusIn（x=1）、FocusOut（2）、
TabletPress（3）、TabletRelease（4）、MouseButtonPress（5）、MouseButtonRelease（6）、
Enter（7）、Leave（8）を記録し、yが有効なツールのモード（`KisTool::ToolMode`、ツールが
なければ-1）、extraのbit 1がポインターがキャンバスの中にある、bit 2がマウスのイベントを
抑止している、bit 4が`KisShortcutMatcher`がポインターの進入を見た（`cursorEntered()`）、
bit 8がすべてのactionを外から抑止している、bit 16がポップアップが登録されていることを
表す。TabletPressは、イベントを見た時点の行と、ポップアップの判定を越えて
`buttonPressed()`を呼ぶ直前の行の2回を記録し、後の行のextraのbit 32が1になる。bit 32の
行がない押下は、ショートカットの照合に届いていない。
`outline_gate`は輪郭の要求ごとに、xが表示のズーム、yが実効の物理ズーム、extraのbit 1が
輪郭を持つ道具、bit 2が`isOutlineEnabled()`、bit 4が`isOutlineVisible()`、bit 8が設定の
輪郭の様式が見える、bit 16がホバーのモード、bit 32が描画のモード、bit 64が描画中に輪郭を
出す設定、bit 128がジェスチャーのモードを表す。`tool_action`はツールのactionが有効（extra=1）
または無効（0）になったことを、xに`KisTool::ToolAction`の値、yに有効なツールがあったかで
残す。`analyze.py`はこれらを`focus_gate`の1行（`qt_event`の種類・モード・extraの内訳、
bit 32の行の数、`outline_gate`のextraの内訳とズーム、`tool_action`のactionと有効・無効の数）
にまとめる。

`KisInputManager`は、キャンバスに届いたTabletPressをポインターが内側にある証明として扱い、
`containsPointer`がfalseのときは`KisShortcutMatcher::enterEvent()`を呼んでから
`buttonPressed()`を呼ぶ。Enterが正常に届いた回は`containsPointer`が既にtrueなので動作は
変わらない。Enterを落とした回でも、押下から1回でツールのactionが戻る。この復帰はQ4（フォークを
やめ、全プラットフォームで正式版の同じQtにする）の前段の実験で、正式版Qtで止まらないことを
1セッション（10回）で確かめた後に削除する。
`run-surface.ps1`は`PenInject.cs`が最初のサンプルの直前に取った最前面の窓
（アプリでなければ`SetForegroundWindow`を1回やり直す）を`foreground_inject_start`として
`meta.txt`に残し、`capture_analyze.py`が`foreground_inject`と`inject-foreground`に出す。
これで、取り込みの開始時に最前面だったこと（`app_foreground_capture`）と、入力の直前にも
最前面だったことを分けられる。取り込みを使う計測では`windows_capture.exe`が計画を独自に
再生し`PenInject.cs`を通らないため、`run-surface.ps1`が`Invoke-Capture`の直前に最前面の窓を
同じ形式で書き、取り込みを使わない計測では`PenInject.cs`の値を使う。

回ごとの端末の状態は`surface_state.ps1`が記録する。記録は対象ごとに切り、既定は記録なしとする
（2026-11-11 05:04 の計測で、1秒ごとの WMI の採取が `% Processor Performance` を 42〜50% に下げ、
採取側の WmiPrvSE が上位プロセスに毎秒現れたため）。`hosts.json`のWindowsの対象に
`surface_state`を書くと記録する。`on`は熱ゾーンの温度（`root/wmi`の
`MSAcpi_ThermalZoneTemperature`の最大値）、`% Processor Performance`、全体のCPU使用率、
CPUを多く使う上位5プロセスを、計測の前・1秒ごとの計測中・計測の後に取る。`light`は
`Get-Counter`を5秒ごとに1回呼び、CPU使用率と`% Processor Performance`だけを取る
（温度と上位プロセスは計測の前後だけ）。カウンターのパスは、Perflib の英語（`009`）と
現在の言語（`CurrentLanguage`）の同じ番号の名前から組み立てる。解決したパスは
`surface-state.paths.txt`に残す。
値は回のディレクトリーの`surface-state.csv`と`meta.txt`の`surface_state_*`に残す
（`surface_state_mode`、`surface_state_during_max_temp_c`、
`surface_state_during_min_processor_performance_pct`、
`surface_state_during_max_total_cpu_pct`、`surface_state_during_mean_total_cpu_pct`、
`surface_state_top_process`、計測の前後の値）。`on`のカウンターは言語に依存しない
`Win32_PerfFormattedData_*`のクラスから取る。1秒ごとの採取は別の`powershell.exe`で動かす。
`run.py`は同じ対象の並びで、中央値が前の回から10%以上変わった回、または
`during_max_temp_c`が5℃以上、`during_min_processor_performance_pct`が10ポイント以上、
`during_max_total_cpu_pct`が10ポイント以上変わった回に`marked:`の印を付け、
`== environment changes`に並べる（判定は変えない）。

描画の周期は`paint_cycle`の1行で追う。`paint_interval`は`paintgl_begin`の間隔（4ms未満、
8ms未満、12ms未満、16ms未満、16ms以上の内訳付き）、`present_interval`はPresentMonの
`TimeInQPC`の間隔、`display_period_ms`は表示の変化の間隔の中央値である。`paint/display`と
`present/display`は、表示の変化1回あたりに描画と受け渡しが何回あったかを表す。
`phase`は、直前の表示の変化から`paintgl_begin`までの時間を表示の周期で割った値で、
`concentration`（0が一様、1が同じ位相）と10個の区間のヒストグラムを添える。
`present_modes`はPresentMonの表示方式の回数、`until_displayed`は`MsUntilDisplayed`の分布である。

表示の正しさ（L2（Windowsの遅れを直す）の前段）は、試験画像
`build/perf/l2-display-check.png`をLibrePaintで開き、100%と400%の画面を
`windows_capture.exe --snapshot-frame`で取り込んで、元の画像と比べる。

```sh
# Surfaceへ送るのは run-remote.sh が行う。DISPLAY_CHECK_IMAGEはSurface上のパス
CAPTURE_TOOL=/path/windows_capture.exe \
TARGET_DISPLAY_CHECK='{"image":"C:\\Users\\masato\\pen-latency\\l2-display-check.png"}' \
  scripts/perf/pen-latency/run-remote.sh <host> LibrePaint 1 '<LibrePaint.exeのパス>' <出力先>
# 取り込んだ display-100.bmp / display-400.bmp を比べる（開発機）
scripts/perf/pen-latency/display_check.py <出力先>/<回> --image build/perf/l2-display-check.png
```

`run-surface.ps1`は窓を最大化し、Ctrl+0で100%にし、Ctrl+=を4回送って400%にする
（`krita_default.shortcuts`の`zoom_to_100pct`と`view_zoom_in`）。ズームは画像の中央を
保つので、100%と400%のどちらも画像の中央が画面の中央に来る。`meta.txt`に
`display_check_foreground_100/400`、`display_check_window=maximized`、
`display_check_anchor=image-center`を残す。

`display_check.py`は、画面全体の取り込みから、元の画像の色（色の帯、グレーの段、
線の格子）が続く矩形をキャンバスとして見つけ、その中だけを比べる。見つからない場合は
画面全体を比べる。倍率は線の幅から求め、線が見えない場合はグレーの段か色の帯の幅から
求める。色の帯とグレーの段の差が8bitで8階調以下、1pxの線が残る（線の幅が元の画素の
0.5〜2.5倍、コントラストが128以上、遷移が8回以上）、隣り合う色の遷移の幅が
`max(2, 倍率×1.5)`px以下であることを合格とする。100%と400%の両方が合格したときだけ
終了コードは0になる。

```sh
# 計測機で一度だけ（管理者のPowerShell）
powershell -NoProfile -ExecutionPolicy Bypass -File setup-surface.ps1
# 開発機から。windows_capture.exeはMinGWで構築する（ファイル先頭の構築手順）
CAPTURE_TOOL=/path/windows_capture.exe \
  scripts/perf/pen-latency/run-remote.sh <host> LibrePaint 5 '<LibrePaint.exeのパス>' <出力先>
scripts/perf/pen-latency/capture_analyze.py <出力先>/LibrePaint-*
scripts/perf/pen-latency/analyze.py <出力先>/LibrePaint-*
```

LinuxとmacOSでは、計測対象を毎回新しいホームと設定で起動し、終了させる。
macOSでは`CFFIXED_USER_HOME`で利用者の設定を変えずに起動し、開発シェルの変数は渡さない。
macOSの取り込みと入力には、実行する端末アプリへ画面収録とアクセシビリティの許可が必要。

```sh
# Linux（デスクトップにログインしたセッションで）
scripts/perf/pen-latency/run-linux.sh <名前> 5 <作業ディレクトリー> <出力先> -- <アプリのコマンド>
# macOS（macos_capture.mmをclang++で構築しておく）
MACOS_CAPTURE=/path/macos_capture \
  scripts/perf/pen-latency/run-macos.sh <名前> 5 <作業ディレクトリー> <出力先> -- <アプリのコマンド>
```

`run-linux.sh`と`run-macos.sh`は、既定のkritarcを空のグループ（最初の`[Group]`より前）に
書く。LibrePaintの`KisConfig`と`KisImageConfig`は`KConfigGroup::group(QString())`を読むため、
`[General]`に書いた`newOutlineStyle`、`fpsLimit`、`newCursorStyle`は効かない（2026-11-09以前の
Linuxの計測はこの状態で、設定は既定のままだった）。`run.py`はシナリオの`General`のキーを
空のグループのキーとして書き出す。

複数の機器と対象をまとめて計測するときは、`scripts/perf/run.py`を使う。シナリオ
（`scripts/perf/scenarios/*.json`）が軌跡、設定、回数、ばらつきの上限、悪化と判定する差を定め、
手元の機器の設定（`scripts/perf/hosts.example.json`を写した`build/perf/hosts.json`。Git の管理外）が
各機器への入り方、対象の起動方法、取り込みの補正値を定める。各回で対象を交互に1回ずつ測り、
集計をJSONに保存し、基準の集計と比べた判定の表を出す。判定が`check`の行があると終了コードは1になる。
すべての機器に入れる1台（WindowsとLinuxにはssh、macOSはその機器自身）で実行する。

```sh
scripts/perf/run.py --scenario scripts/perf/scenarios/pen-light.json --hosts build/perf/hosts.json \
  --label <名前> [--only windows,linux] [--baseline <前回の集計.json>] [--summary docs/perf/results/<名前>.json]
scripts/perf/run.py --scenario ... --hosts ... --analyze-only <計測結果のディレクトリー>
```

判定の規則：

- 比較する対象は、同じ機器、同じ窓とキャンバスの大きさ、同じ設定で交互に計測する。
- 各対象を5回測り、回ごとの中央値のばらつき（最大と最小の差を中央値で割ったもの）が5%未満で
  あることを確かめる。満たさない場合は回数を増やす。
- OSをまたぐ比較は、絶対値よりも同じ回の参照アプリとの差で行う。
- 合成入力はOSの入力の入口から始まるため、ペンのハードウェアからOSまでの遅れは含まない。
  この区間は、`PenProbe.cs`で報告レートを数値として記録する。
- Windowsのホバーは、起動ごとに約56msと約63msの2つの状態に分かれる（L1（Windowsの約50msの
  遅れの原因の特定））。`capture_analyze.py`が各回を`hover.frame_to_arrival`の中央値と
  `rate.qt_tablet_move_interval`のp05で`low`/`high`/`unknown`に分け、`run.py`はホバーの
  比較を状態ごとに行う。インクは全回でまとめる。状態の境目は2026-11-09の20回の値
  （低い状態 11.2msと3.0〜3.3ms、高い状態 9.6〜9.8msと0.5〜2.0ms）で決めた。
- Windowsの対象は`presentmon`を`"off"`にするとPresentMonなしで測れる（既定は`on`）。
  PresentMonの記録がない回は集計に`PresentMon=none`と出し、内訳の比較から外す。CSVはあるが
  `TimeInQPC`の範囲が合成入力の期間の95%未満しか覆っていない回は`PresentMon=partial`と出し、
  同じく内訳の比較から外す（`run.py`の表には`present`/`partial`/`none`の回数を出す）。
- 速さの比較には`pen-light-speed-0p5.json`、`pen-light-speed-1p5.json`、
  `pen-light-speed-3p0.json`を使う。軌跡と設定は`pen-light.json`と同じで、`speed`だけが違う。
  `pen-light-speed-varied.json`は1本の線の中で速さを変える（前半を遅くして後半を一気に速くする線と、
  速いまま急に止める線を交互に置く）。`trajectory.py`の`speed_profiles`が、線の距離の割合
  （`until`）ごとに速さと`label`を指定する。`capture_analyze.py`は`label`を`move`で始まる
  フェーズとして1本の線にまとめ、位置から入力の時刻を求める。
- `capture_analyze.py`はインクのフレームを、その位置を通ったときのポインターの速さ
  （px/ms）で`ink.speed_groups`に分ける。各群に入力から表示までの時間と、ペン先と線の端の
  距離（`ink.pen_tip_to_edge_px`。取り込みの画素で、速さ×遅れ）を出す。速さが2倍以上または
  半分以下に変わり、それが続く箇所を`ink.speed_change`とし、変わった直後の80msの最大の遅れ、
  その後の安定した遅れ、安定した値に戻るまでの時間（`settle_ms`、3サンプル連続で
  安定値+max(3ms, 10%)以下）を出す。`run.py`は回をまたいで速さごとの中央値とばらつきを
  「ink speed groups」に出す。80msと250msの窓は、半本を3.0で描く時間（約167ms）に収まる
  解析上の値である。
- ストロークの待ち行列の遅れは、`stroke_paint`（点を待ち行列に入れた時刻）と`stroke_dab`
  （ストロークの処理がペイントデバイスに dab を描いた時刻）の組で測る。`analyze.py`は
  先入れ先出しで組にし、`contact.stroke_paint_to_dab`と、待ち行列に入れた時点の
  ポインターの速さごとの`contact.stroke_paint_to_dab_speed`を出す。`stroke_queue`に
  記録と組の数を出す。`stroke_dab`は`libs/painting/strokes/freehand_stroke.cpp`が
  `maskedPainter`の描画の後に記録する。

## デスクトップ

### macOS

Apple Silicon上で次を実行する。ツールチェーンと機能の定義は
[nix/macos/krita.nix](../../nix/macos/krita.nix)と`flake.lock`が所有する。

```sh
nix build .#librepaint-macos
open result/bin/LibrePaint.app
```

出力は実行時ライブラリーをNixの依存集合に保持する開発用バンドル。
配布用バンドルは、Nix出力から実行時部品をスタンドアロン配置し、選択した方式で
署名した後にDMGへ収録する。`--output-dir`は処理開始時に再作成されるため、専用の
作業ディレクトリーを指定する。

```sh
install_dir="$(nix build .#librepaint-macos --no-link --print-out-paths)"
python3 packaging/macos/macos-deploy.py \
  --install-dir "$install_dir" \
  --output-dir <専用作業ディレクトリー>/standalone \
  --source "$PWD"
python3 scripts/platform/audit-macos-bundle.py \
  <専用作業ディレクトリー>/standalone/LibrePaint.app
```

`macos-deploy.py`は既定でアドホック署名を使う。Developer IDなど別の署名方式を
使用する場合は`--signing-identity <識別名>`または`MACOS_CODESIGN_IDENTITY`で指定する。
実行時閉包の監査は署名前と署名後に自動実行され、署名の検証は署名後の成果物監査が
担当する。全Mach-Oのarm64種別、アプリ内の
依存解決、RPATH、インストール名の衝突、iconvのApple／GNU ABI、シンボリックリンク、
開発・試験用部品の不在、Krita画像プラグイン、Python／PyQt、FFmpeg／MLT、frei0r、
sdl2-compatが動的に読み込むSDL3、フォント、アイコン、翻訳、および最終署名を検査する。
依存収集は元のMach-Oが参照する絶対パスからライブラリーをコピーし、同じ走査で
参照先をバンドル内の相対パスへ変更する。別名リンクを実体へ解決し、追加した部品も
同じ解析待ち集合へ登録する。解析済み数と残件数を表示する。
同名で内容が異なる依存ライブラリーは別の内部ディレクトリーへ保持し、各利用側を
元の部品へ結び付ける。Python利用側は同じ版の内蔵Frameworkを共有する。
Apple iconvの参照はmacOS標準ライブラリーへ戻し、GNU iconvは同梱する。
解決不能な依存や配置済み部品との不一致は対象パスを示して終了する。

空のDMG作業ディレクトリーから次を実行する。生成物は同ディレクトリーの
`_packaging`に保存される。

```sh
python3 <リポジトリー>/packaging/macos/macos-apptodmg.py \
  <専用作業ディレクトリー>/standalone/LibrePaint.app \
  --media-path <リポジトリー>/packaging/macos \
  --dmg_name LibrePaint-<版番号>-aarch64-macos
<リポジトリー>/scripts/platform/check-macos-release-dmg \
  _packaging/LibrePaint-<版番号>-aarch64-macos.dmg
```

`check-macos-release-dmg`はUDIFを検証して読取り専用でマウントし、DMG内の単一Appへ
同じ成果物監査を適用してからデバイスを取り外す。
個別の依存環境を初回に開く入口は`nix develop .#librepaint-macos`。
自動検査後、マウントしたアプリで起動・終了、主要画像形式の読込・保存、Python
プラグインと映像機能を確認する。

### Linux

x86_64 Linux上で、依存物の準備、アプリ構築、起動を行う。

```sh
nix build .#linux-dependencies --no-link
nix build .#librepaint-linux
nix run .#librepaint-linux
```

アプリは`result/bin/LibrePaint`。個別の初回環境は`nix develop .#librepaint-linux`。
配布用AppImageは完成済みアプリと依存物から作成する。

```sh
nix build .#librepaint-linux-appimage --out-link LibrePaint-x86_64.AppImage
```

AppImageの作成段階は`scripts/platform/audit-linux-appimage.py`を実行し、同梱した
x86_64 ELFのインタープリター、共有ライブラリーと実行時探索パス、Nixストア参照、
シンボリックリンクを検査する。Kritaのプラグイン、Python／PyQt、G'MIC、XCF、
FFmpeg／MLT、Qt XCB／Wayland、Breezeアイコンとフォント構成を実行時契約として確認し、
ヘッダー、静的ライブラリー、CMake／pkg-config／qmake／GObject／Valaの構築資料と
構築用実行ファイルを拒否する。
動的リンカーは絶対パスで指定した同梱ファイルとして解決し、各実行時探索パスの
有効性を確認する。同梱実行ファイルが選択するglibc動的リンカーの既定探索先を
共有ライブラリーの解決に使用する。
値全体が空のRPATH／RUNPATHタグは探索先を持たないものとして扱う。
コロンで区切った探索パス内の空要素は現在のディレクトリーを参照するため拒否する。
`/run/opengl-driver/lib`と`/run/opengl-driver-32/lib`だけを描画ドライバーの
明示したホスト境界として許可する。
`scripts/platform/check-release-assets`は公開前後に同じ検査を完成AppImageへ再実行する。

LinuxのOpenColorIOは`flake.nix`の依存定義でインストール先のRPATHを指定する。
`nix/linux/appimage.nix`はlibidn2とlibunistringの配布用コピーから、Nixの基礎依存構築で
消去した参照を除去する。圧縮と成果物監査は別の派生物とし、監査の変更時は圧縮済み出力を再利用する。
配布物のプラグイン集合は必要機能の収録で検査し、個数は監査結果として集計する。

Linuxの色管理構成はQt DBusの検出結果から決まる。Qt DBusを検出した構成は
`kritacolord`を組み込み、検出しない構成はダミー実装を組み込む。検出経路を変更した場合は、
標準構成と`-DHAVE_DBUS=ON`を指定した構成の双方で`KisColorManagerPublicApiTest`と
`KisColordPublicApiTest`を実行し、同じcolord実装を選択することを確認する。

実行にはLinuxユーザー名前空間が必要。配布前に対象システムとGPUでOpenGL表示を確認する。
NixOS以外ではnixGL形式のラッパーが必要になる場合がある。

既存の依存配置からローカルAppImageを作る場合は、Debian互換環境で次を実行する。
依存配置にはQt／KF、Python／PyQt、画像・媒体ライブラリー、翻訳、MIMEデータ、
`linuxdeployqt`、AppImage実行時ファイルを用意する。必要なコマンドは`bash`、`cmake`、
`dpkg`、`git`、`nproc`、`patchelf`、`realpath`、`rsync`とC／C++ツールチェーン。

```sh
LIBREPAINT_DEPS_PATH=/absolute/path/to/dependencies \
  packaging/linux/appimage/build-krita.sh /absolute/path/to/appimage-work "$PWD"
LIBREPAINT_DEPS_PATH=/absolute/path/to/dependencies \
  packaging/linux/appimage/build-image.sh /absolute/path/to/appimage-work "$PWD"
```

作業ディレクトリーに`LibrePaint-<version>-<revision>-<architecture>.AppImage`を生成する。
依存配置の旧変数`KRITA_DEPS_PATH`は`LIBREPAINT_DEPS_PATH`が未設定の場合だけ使う。

### Windows

x86_64 Linuxから`x86_64-w64-mingw32`向けに構築する。
日常の編集は`build-incremental windows build krita`で、専用ソース配置へ変更分を同期する。
配布用の構築は次を使う。

```sh
nix build .#windows-dependencies --no-link
nix build .#librepaint-windows
nix build .#librepaint-windows-archive
```

アプリ出力は`result/bin/LibrePaint.exe`を含む可搬ディレクトリー。
アーカイブ出力は`result/LibrePaint-<version>-x86_64-windows.zip`。
DLL、Qtプラグイン、QML、Python／PyQt、G'MIC、媒体処理、翻訳、フォント、`qt.conf`を同梱する。

正式版Qt版も同じ増分構築の入口を使う。依存は`windows-upstream-qt-dependencies`の
構築結果を再利用し、`krita`ターゲットで`krita.dll`と`libkrita*.dll`だけを作り直す。

```sh
nix build .#windows-upstream-qt-dependencies --no-link
systemd-run --user --unit=librepaint-win-upstream-qt-configure --collect --wait \
  --working-directory "$PWD" \
  bash -lc './scripts/build-incremental windows-upstream-qt configure'
systemd-run --user --unit=librepaint-win-upstream-qt-build --collect --wait \
  --working-directory "$PWD" \
  bash -lc 'CMAKE_BUILD_PARALLEL_LEVEL=5 ./scripts/build-incremental windows-upstream-qt build krita'
./scripts/build-incremental windows-upstream-qt path
```

構築の前に、ほかの構築や`verify`が動いていないことを確かめる。

```sh
systemctl --user list-units "q*" "librepaint-*" "l1-*" "l2-*" "win-*" --no-legend
```

構築は1本のsystemdのuser unitで動かし、終了を待ってから次を始める。

構成指紋がフォーク版と異なるため、ソース配置とNinja木は
`build/windows/x86_64/<構成指紋>/`の別のディレクトリーになる。コンパイラーキャッシュは
フォーク版と同じ`.cache/librepaint/ccache/windows`を共有する。初回だけ`configure`が
ソース配置とCMake構成を作り、以後は`build krita`が変更された翻訳単位とDLLだけを更新する。
構成指紋にはQtの定義、CMakeフラグ、ソース準備が含まれるため、`nix/windows`を変えたときは
新しい木が作られる。配布用のZIPは`nix build .#librepaint-windows-upstream-qt-archive`で作る。

Q4（Qtのフォークをやめる）の準備として、正式版Qt 6.11.3で構築する別の出力を持つ。
Qtの定義はLinuxとmacOSと同じ`nix/qt/overlay.nix`を使い、MinGWの交差構築だけを
`nix/windows/krita.nix`の`useQtFork = false`で切り替える。フォーク版の出力と
キャッシュは変えない。

```sh
nix build .#windows-upstream-qt-dependencies --no-link
nix build .#librepaint-windows-upstream-qt-archive
```

依存だけの出力`windows-upstream-qt-dependencies`は、Qt・KDE Frameworks・コーデックを
アプリの束ねから分けて構築する。中断しても、完了した依存はストアに残り、
`librepaint-windows-upstream-qt-archive`の再実行はその続きから進む。
正式版QtにはANGLEがないため、この版はdesktopのOpenGLだけを使う。
HDR（`setTextureColorSpace`）は正式版にないAPIなので、この版では
`HAVE_HDR`を強制せず、CMakeの検出結果に任せる。

パッケージ構築は`scripts/platform/audit-windows-package.py`を実行し、全PEファイルが
x86_64であること、DLLのベース名が一意であること、各インポートが同梱DLLまたは
WindowsのシステムDLLとして解決することを確認する。同梱DLLは製品の探索先である
`bin`から解決する。Visual C++の実行時DLLも同梱依存として検査する。
主要画像形式、Python／PyQt／SIP、G'MIC、FFmpeg／MLTの実行時部品の収録と、
製品実行時に不要な開発・試験用成果物の除外も同じ検査の契約とする。
`scripts/platform/check-release-assets`はWindows ZIPを展開し、公開前と公開後にこの検査を再実行する。

WindowsのJPEG XL依存は`flake.nix`のMinGW向けlibjxl定義が所有する。
構築、開発用依存集合、配布物は同じ定義を使用する。
MinGW版は[Issue #68](https://github.com/serika12345/librepaint/issues/68)に対応する
暫定設定でAVX2・AVX-512経路を無効化し、SSE経路を使用する。当該経路の復号と
可逆往復がWindows実機で成功した時点で、この設定を除去する。

Windows実機でのコマンドライン検証はコンソール入口`bin/LibrePaint.com`を使う。
JPEG XL依存の変更は、固定JXLのPNG書出しと、可逆設定のPNG→JXL→PNGの往復を
実行し、基準画像との全画素一致を確認する。
`--resource-location`は検証用のリソースとデータベースの保存先を指定する。
アプリケーション設定はWindowsユーザーの設定先を使用する。

```powershell
.\bin\LibrePaint.com --version
.\bin\LibrePaint.com --nosplash --resource-location <検証用リソース先> `
  --export --export-filename <出力PNG> <固定JXL>
```

`winquirks/unistd.h`を変更した場合は、対象WindowsのVisual Studio開発者環境でMSVC契約を
構築して実行する。

```powershell
cmake -S winquirks/tests -B build/winquirks-msvc -A x64
cmake --build build/winquirks-msvc --config RelWithDebInfo
ctest --test-dir build/winquirks-msvc -C RelWithDebInfo --output-on-failure
```

この契約はWindowsのプロセス・利用者・標準ストリーム識別子、`readlink()`の失敗条件、
`sleep()`の待機時間を検査する。

すべてのデスクトップ成果物で、起動、描画、保存・再読込、プラグイン読込を対象OS上で確認する。
署名と包装を変更した場合は、その完成物で同じ操作を確認する。

## Android

x86_64 Linuxで`arm64-v8a`または`x86_64`向けに構築する。後者はWaydroidでも試験できる。
SDK／NDK、Qt／KF、JDK／Gradleの固定値は[nix/android](../../nix/android)、
ソースを共有するiOS依存マニフェスト、`flake.lock`、`nix/android/gradle-deps.json`を参照する。

### APKとAAB

```sh
build-incremental android configure
build-incremental android plan krita
build-incremental android build krita
build-incremental android package-product
build-incremental android-x86_64 build krita
```

Qt 6の`androiddeployqt`が`packaging/android/apk`を使い、
`create-apk-krita`と`create-aab-krita`で包装する。ABIごとの個別環境を初回に開く場合は
`nix develop .#librepaint-android`または`nix develop .#librepaint-android-x86_64`を使う。
端末試験後のクリーンな成果物は次で作成する。

```sh
nix build .#librepaint-android
nix build .#librepaint-android-x86_64
```

各出力の`result/LibrePaint-<ABI>.apk`と`.aab`を使用する。構築時にABI、ELF、16 KiB整列、
単一の共有C++実行時ライブラリー、Qt 6／KF6、プラグイン、資源、Manifest、SDK条件を監査する。
製品成果物の監査は、ライブラリ間で型判定または例外捕捉に使うC++型情報が
所有ライブラリに一意に定義されていることも確認する。
共有ライブラリ間の`dynamic_cast`、`typeid`、例外捕捉を追加するときは、
型情報の所有ライブラリを定め、`audit-android-cross-library-rtti.py`の監査対象に加える。
仮想関数呼出しだけに使う型や、型判定が一つのライブラリ内で完結する型は監査対象外とする。

依存物だけの診断・キャッシュ準備には、`android-source-dependencies`、`qtbase-android`、
`android-kf6`、`android-application-dependencies`、`android-dependencies`のNix出力を使う。
x86_64版は順に`android-x86_64-source-dependencies`、`qtbase-android-x86_64`、
`android-x86_64-kf6`、`android-x86_64-application-dependencies`、`android-x86_64-dependencies`。

### 端末試験と導入

```sh
build-incremental android-x86_64 package-test KisCurveOptionModelTest
adb connect <Waydroidまたは実機の接続先>
build-incremental android-x86_64 run-test KisCurveOptionModelTest [adb-serial]
```

ARM64では`build-incremental android run-test KisCurveOptionModelTest [adb-serial]`を使う。
試験は対象と実行時依存物を専用APKに包装し、`data/`をアプリ専用外部領域へ展開して実行する。
処理は端末ABI確認、試験用複製への署名、導入、Activity起動、結果回収、停止、試験パッケージ削除まで行う。
xUnit XMLとlogcatは構築木の`test-results/<target>/`へ保存する。

共有ライブラリ間のジョブ受け渡しは、対象ABIで次の契約を実行して確認する。

```sh
build-incremental android-x86_64 run-test FreehandStrokeContractTest [adb-serial]
build-incremental android-x86_64 run-test FilterStrokeLibraryBoundaryContractTest [adb-serial]
```

ARM64では`android-x86_64`を`android`に置き換える。

Androidの文書選択画面を通る保存・読込経路を変更した場合は、次の契約で
空白を含む文書名のURIと保存データの往復を確認する。

```sh
build-incremental android-x86_64 run-test KisAndroidContentUriContractTest [adb-serial]
```

クラッシュ処理を変更した場合は、ARM64実機で実コールバックとバックトレース生成を検査する。

```sh
build-incremental android run-test KisAndroidCrashHandlerContractTest [adb-serial]
build-incremental android run-test KisCrashSignalHandlerSetupContractTest [adb-serial]
```

前者は子プロセスで`handler_init()`を実行してSIGTERMを送信し、unwindstackが生成した
先頭フレームを含むクラッシュ記録とシグナル終了を確認する。後者は代替シグナルスタック、
コールバック設定、以前の動作の保存と復元を確認する。

### GitHub Releases向けAPK

公開版のアプリ識別子は`io.github.serika12345.librepaint`、表示名は`LibrePaint`。
Javaの名前空間`org.krita`とネイティブ対象`krita`はコード内の名前として維持する。
版番号と最小Android版は[Gradle定義](../../packaging/android/apk/build.gradle)が所有する。
公開するABIは`arm64-v8a`と`x86_64`で、
利用者は対応する一方のAPKをGitHub Releasesから取得する。
公開案内には操作を確認した端末・Android版を別に記載する。
次の公開版ではこのアプリ識別子と配布証明書を維持し、Gradleの`versionName`を
Releaseの版に合わせ、`versionCode`を前回より増やす。

所有者は`librepaint-release`という別名のPKCS12配布鍵をリポジトリ外で生成し、
原本と暗号化した復旧用コピーを別々に保管する。鍵の有効期間は25年以上にする。
Linuxの署名用シェル、またはJDK 17を含むNix一時シェルで次を実行し、
鍵のパスワードは対話入力で設定する。

```sh
keytool -genkeypair -keystore <リポジトリ外の場所>/librepaint-release.p12 \
  -storetype PKCS12 -alias librepaint-release -keyalg RSA -keysize 4096 \
  -validity 10000 -dname "CN=LibrePaint Android Release, O=LibrePaint, C=JP"
keytool -list -v -keystore <リポジトリ外の場所>/librepaint-release.p12 \
  -alias librepaint-release
```

公開証明書のSHA-256指紋を記録し、GitHubの`android-release`環境に
`ANDROID_RELEASE_CERT_SHA256`という変数として登録する。同環境を`develop`からの
手動実行に限定し、所有者の承認を必要とする。鍵本体はbase64で
`ANDROID_RELEASE_KEYSTORE_BASE64`、パスワードは
`ANDROID_RELEASE_STORE_PASSWORD`と`ANDROID_RELEASE_KEY_PASSWORD`という
環境の秘密情報に登録する。秘密情報の値、原本、復旧用コピーはIssue、ログ、
リポジトリ、Nix storeに配置しない。
実際の保管先、復旧方法、登録状況も機微情報として会話内で扱い、
Issue、Projects、追跡文書には一般化した手順だけを記載する。

固定したソースのタグ・コミットを確認し、NixOSで両ABIの未署名APKを構築する。
同じソースから`librepaintUpdateBaseline`のGradleプロパティを指定し、
Gradle定義にある内部更新試験用の版も包装する。
この先行版は公開しない。両方のNix出力が同じABIのネイティブ成果物を再利用する。
Release準備中はネイティブ中間出力を後続の包装でも再利用できるよう、
現行作業ツリーの`build/nix-profiles`に一時的な参照を置く。
下書きReleaseへの受け渡しと検査が済んだら、`result-*`とこの中間出力の参照を
削除し、日常の増分構築木、開発環境、共有コンパイラキャッシュを保持する。

```sh
mkdir -p build/nix-profiles
nix build .#librepaint-android-native \
  --out-link build/nix-profiles/android-native-arm64
nix build .#librepaint-android-x86_64-native \
  --out-link build/nix-profiles/android-native-x86_64
nix build .#librepaint-android --out-link result-android-arm64
nix build .#librepaint-android-x86_64 --out-link result-android-x86_64
nix build .#librepaint-android-update-baseline --out-link result-android-arm64-update-baseline
nix build .#librepaint-android-x86_64-update-baseline --out-link result-android-x86_64-update-baseline
```

下書きReleaseへ次の4個の未署名APKと、そのSHA-256を列挙した
`SHA256SUMS.android-unsigned`を添付する。基準版も同じ固定ソースから作る。

```text
LibrePaint-<タグ>-arm64-v8a-unsigned.apk
LibrePaint-<タグ>-x86_64-unsigned.apk
LibrePaint-<タグ>-update-baseline-arm64-v8a-unsigned.apk
LibrePaint-<タグ>-update-baseline-x86_64-unsigned.apk
```

作業用ディレクトリーへ4個のAPKを上記の名前でコピーし、そのディレクトリーで
`sha256sum *-unsigned.apk > SHA256SUMS.android-unsigned`を実行する。
固定コミットを指すタグを用意して、変更点を記した説明文とともに下書きを作る。
以下の`<タグ>`は`v`付きのリリースタグ、`<版番号>`は`v`を除いた版番号、
`<version-code>`はGradle定義の公開版`versionCode`を指定する。

```sh
gh release create <タグ> --draft --verify-tag \
  --title "LibrePaint <タグ>" --notes-file <説明文ファイル>
gh release upload <タグ> <作業用ディレクトリー>/*-unsigned.apk \
  <作業用ディレクトリー>/SHA256SUMS.android-unsigned
gh workflow run sign-android-release.yml --ref develop \
  -f tag=<タグ> -f source_commit=<タグのコミットSHA> -f version_code=<version-code>
```

GitHub Actionsの`workflow_dispatch`は[ワークフローが既定ブランチにも存在すること](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow)を要求する。
リポジトリの既定ブランチ`master`にも同じ署名ワークフローを置き、
ジョブの`develop`条件で署名の実行元を限定する。
`sign-android-release.yml`を`develop`から手動起動し、タグ、対応する40桁の
コミットSHA、`<version-code>`を入力する。所有者が`android-release`環境の実行を承認すると、
CIは入力コミットをチェックアウトし、下書きとタグ、入力ハッシュ、APKの識別子・版番号、ABI、資源、ELF、
16 KiB整列を確認し、配布鍵で署名する。署名後にも同じ監査と証明書指紋を検査する。
完成した両ABIのAPK、内部更新試験用APK、`SHA256SUMS.android`を下書きへ置き、
未署名入力を下書きから除く。署名用ツールは`nix develop .#android-release`が供給する。

下書きの完成APKをARM64実機とx86_64 Waydroidへそれぞれ新規導入し、起動、
新規文書、描画、保存・再読込、書き出し、画面回転、休止・復帰、終了を確認する。
別に内部更新試験用APKで作品を作成・保存してから完成APKで上書き更新し、
作品データの保持と再読込を確認する。既存Kritaとの同時導入も確認する。
導入前に対象端末の機種とAndroid版を取得し、各操作の結果とともに会話で報告する。
公開する検証情報は必要な範囲に絞り、Issueへの反映は依頼と文面承認を経て行う。

```sh
adb -s <端末番号> shell getprop ro.product.model
adb -s <端末番号> shell getprop ro.build.version.release
adb -s <端末番号> install <完成APK>
adb -s <端末番号> logcat
```

更新試験は初期状態に戻した検証環境で行う。先行版で作品を保存した後に完成版を
上書き導入し、版番号と作品の再読込を確認する。

```sh
adb -s <端末番号> install <内部更新試験用APK>
adb -s <端末番号> install -r <完成APK>
adb -s <端末番号> shell dumpsys package io.github.serika12345.librepaint
```

WindowsのZIP、LinuxのAppImage、iOSのIPA、macOSのDMGも同じ版番号で揃え、
各成果物の対象プラットフォームの検査結果を確認する。WindowsとLinuxは
x86_64 Linux、iOSとmacOSはApple SiliconのmacOSで構築する。
各出力をアップロードする端末の同じ作業用ディレクトリーに集め、
IPAとDMGは版番号を付けた名前で添付する。

```sh
nix build .#librepaint-windows-archive --out-link result-windows-release
nix build .#librepaint-linux-appimage --out-link result-linux-release
nix build .#librepaint-ios-ipa --out-link result-ios-release
gh release upload <タグ> \
  <作業用ディレクトリー>/LibrePaint-<版番号>-x86_64-windows.zip \
  <作業用ディレクトリー>/LibrePaint-<版番号>-x86_64.AppImage \
  <作業用ディレクトリー>/LibrePaint-iOS-<タグ>-unsigned.ipa \
  <作業用ディレクトリー>/LibrePaint-<版番号>-aarch64-macos.dmg
```

内部更新試験用APKを下書きから削除し、
公開対象が両ABIの完成APK、`SHA256SUMS.android`、他4平台の成果物だけであることを
確認してから手動公開する。ReleaseにはABI、Manifestが定める最小Android版、
実操作を確認したAndroid版、ADB導入方法、変更点、タグ・コミット、完成APKのSHA-256を記す。
公開前と公開後には署名用シェルで次を実行し、両APKの再取得、
`SHA256SUMS.android`、署名証明書、APK構成と他4平台の添付を照合する。

```sh
nix develop .#android-release --command bash \
  scripts/platform/check-release-assets <タグ> <version-code> <公開証明書のSHA-256>
```

添付前の公開候補は、同じ検査の第4引数へ候補ディレクトリーを渡して検証する。
同ディレクトリーに7個の公開成果物と`RELEASE_NOTES.md`を配置する。説明文には
タグ、対応コミット、APKとDMGのSHA-256を記載し、配布処理を更新した場合は、その
ソースの対応コミットも記載する。検査は候補を入力として一時領域へ展開し、
公開済み成果物と同じ依存、構成、署名、版番号、ハッシュの条件を適用する。

```sh
scripts/platform/check-release-assets <タグ> <version-code> \
  <公開証明書のSHA-256> <候補ディレクトリー>
```

macOSでLinux AppImageを検査するときは、大小文字を区別するファイルシステムの
作業領域の絶対パスを`TMPDIR`へ指定する。Windowsの検査にはPEを解析できる`objdump`を
`WINDOWS_OBJDUMP`へ指定する。次の例では検査後に一時ボリュームを取り外す。

```sh
hdiutil create -size 8g -fs 'Case-sensitive HFS+' -type SPARSEBUNDLE \
  -volname LibrePaintReleaseInspection <作業領域>/inspection.sparsebundle
mkdir -p <作業領域>/inspection-mount
hdiutil attach -nobrowse -mountpoint <作業領域>/inspection-mount \
  <作業領域>/inspection.sparsebundle
mkdir -p <作業領域>/inspection-mount/tmp
TMPDIR=<作業領域>/inspection-mount/tmp WINDOWS_OBJDUMP=<PE解析器> \
  scripts/platform/check-release-assets <タグ> <version-code> \
  <公開証明書のSHA-256> <候補ディレクトリー>
hdiutil detach <作業領域>/inspection-mount
```

公開操作は`gh release edit <タグ> --draft=false`。公開後に同じ検査を再実行する。

検証結果は会話で報告する。Issue #70への反映は依頼と文面承認を経て行う。

### Android依存物の更新

Gradle依存物は同じAGP、AndroidX、SDKの最小プロジェクトで応答とハッシュを更新する。
包装時に解決される依存物も記録した後、ネットワークなし構築と両ABIを検証する。

```sh
gradle_update_script="$(nix build --no-link --print-out-paths \
  .#librepaint-android.gradleDepsUpdate)"
"$gradle_update_script"
```

ActivityはQt初期化前に`QT_ANDROID_DISABLE_ACCESSIBILITY=1`を設定する。
Qt 6.11.1のアクセシビリティ照会とOpenGL画面作成の競合を避ける互換性設定である。
[QTBUG-140490](https://qt-project.atlassian.net/browse/QTBUG-140490)、
[QTBUG-140674](https://qt-project.atlassian.net/browse/QTBUG-140674)、
[修正案735089](https://codereview.qt-project.org/c/qt/qtbase/+/735089)を参照する。
依存Qtの更新時は修正の取り込みを確認し、設定解除後のダイアログとアクセシビリティ操作を
実機で反復検証する。競合が解消されていることを確認して互換性設定を解除する。

## iOS／iPadOS

### MacとXcodeの準備

Apple Silicon Mac、Nix 2.31以降、`/Applications/Xcode.app`に固定版Xcodeを用意する。
正確な版は[packaging/ios/versions.env](../../packaging/ios/versions.env)が所有する。
IPAはarm64、iOS／iPadOS 17以降のiPhoneとiPadを対象とし、iPhoneでも既存のiPad画面を使う。

Nixデーモンの設定は次とし、Xcodeは`__impureHostDeps`を宣言する派生物だけへ公開する。
`sandbox-paths`へのXcode追加は全派生物へ公開されるため使用しない。

```nix
nix.settings.sandbox = true;
nix.settings.sandbox-fallback = false;
nix.settings.extra-allowed-impure-host-deps = [
  "/Applications/Xcode.app"
];
```

依存物やホストの診断を行うセッションでは、初回にiOSシェルを開き、検査する。
以後の診断は同じシェルを使う。利用者キャッシュに書けない環境は、起動時に
`XDG_CACHE_HOME="$PWD/.cache/nix"`を指定する。

```sh
nix develop .#librepaint-ios
packaging/ios/scripts/check-host.sh
```

### 増分構築とIPA

```sh
build-incremental ios path
build-incremental ios bootstrap
build-incremental ios plan
build-incremental ios build
```

`bootstrap`は新しい構成の最初に一度実行する。通常の`build`と`deploy`は200工程を超える
計画を拒否する。大きい構成変更では計画を確認して`bootstrap`で基準を更新する。
閾値の明示変更には`KRITA_IOS_INCREMENTAL_MAX_STEPS`を使う。
共通入口は`packaging/ios/scripts/build-librepaint-incremental.sh`へ委譲する。

クリーンなアプリと未署名IPAは次で作成する。IPA出力は必要なアプリ・依存物も構築する。

```sh
nix build .#librepaint-ios-app --out-link build-ios/nix-results/librepaint-ios-app
nix build .#librepaint-ios-ipa --out-link build-ios/nix-results/librepaint-ios-ipa
```

出力はそれぞれ`LibrePaint.app`と`LibrePaint-iOS-unsigned.ipa`。
Apple ID、署名鍵、プロビジョニング、端末認証情報はローカルの署名環境で管理する。

### AltStoreへの配備

端末をUSB接続し、ロック解除、Macへの信頼、開発者モードを設定する。
端末のAltStoreを設定し、MacでAltServerを起動する。両者がローカルネットワークで通信できる状態で実行する。

```sh
xcrun devicectl list devices
build-incremental ios deploy [device-id]
```

省略時は最初の利用可能なCoreDeviceを選ぶ。増分構築、バイナリー・静的資源・インストールデータの
検査、IPA生成、AltStore署名・導入、起動、ログ回収を順に行う。
タイムスタンプ付きIPAと`librepaint.log`は`build-ios/deploy/`へ保存する。
`deploy-altstore.sh --skip-build`は正確な構築木を渡す内部呼出しで使用する。

配備先は`KRITA_IOS_DEVICE`、版は`KRITA_IOS_BUNDLE_VERSION`、通信ポートは
`KRITA_IOS_DEPLOY_PORT`、起動待機は`KRITA_IOS_LAUNCH_SETTLE_SECONDS`でも指定できる。
配備の最後に使用中のNix入力を保護し、空き容量が閾値未満ならキャッシュ保守を行う。

### LiveContainerへの導入

設定済みのLiveContainerへ`build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iOS-unsigned.ipa`を
インポートする。iOS 26ではJITなしモードでの新規インポート・起動が確認されている。
IPAはディレクトリー`0755`、データ`0644`、実行形式`0755`へ正規化し、
読取り専用属性など、インポート後の修正・署名・片付けを妨げる属性を検査する。

過去の失敗で読取り専用の一時`Payload`が残ると、修正版IPAの読込み前に失敗する場合がある。
必要なアプリデータを保護し、LiveContainer側の残存状態を確認する。
具体的な画面操作による復旧手順の実機確認は[Issue #67](https://github.com/serika12345/librepaint/issues/67)で扱う。

### 依存物と最小アプリの診断

現在の依存物全体と利用側の最終リンクは次で検査する。
ソースからの構築を確認する場合は`--no-substitute`を追加する。

```sh
nix build .#ios-dependencies --no-link
nix build .#kf6-consumer-check --no-link
```

個別の診断用配置は次の順序で構築する。`build-dependencies.sh device harfbuzz`のように
名前を付けると対象と依存物に限定できる。

```sh
packaging/ios/scripts/build-dependencies.sh device
packaging/ios/scripts/probe-dependencies.sh device
packaging/ios/scripts/build-qt.sh device
packaging/ios/scripts/build-dependencies.sh device
packaging/ios/scripts/probe-qt.sh device
packaging/ios/scripts/build-frameworks.sh device
packaging/ios/scripts/probe-frameworks.sh device
packaging/ios/scripts/build-smoke.sh device
packaging/ios/scripts/build-smoke.sh simulator
```

実機とシミュレーターの配置を分け、静的アーカイブの全要素と試験用アプリのAppleプラットフォームを検査する。
Qtの再利用はソース、レシピ、Xcode／SDKの構成指紋で決める。Qt専用配置の再作成は
`build-qt.sh device --clean`。KF6の設定生成器はmacOS上で実行し、ライブラリーはiOSへリンクする。
依存定義は`packaging/ios/deps/dependencies.json`、`packaging/ios/frameworks/frameworks.json`、
`nix/ios/packages/`を参照する。

### プラグインと実機検証

採用する機能の正本は[initial-plugin-profile.json](../../packaging/ios/manifests/initial-plugin-profile.json)。
プラグインを追加・削除した場合は目録を再生成する。

```sh
python3 packaging/ios/scripts/inventory-plugins.py
```

目録は`packaging/ios/manifests/plugins.json`と`dependencies.json`、構築物は`build-ios/`、
コマンドログは`logs/ios/`にある。ソース内の権利表記と
[資産の採用・帰属資料](../ios/non-code-assets.md)を維持する。

実機試験は端末、ビルド、設定、ファイル、試験時間を固定し、構築成功・IPA検査・操作の結果を分けて記録する。

| 変更範囲 | 実機で確認する操作 |
| --- | --- |
| 描画・入力 | Pencilの押下・移動・解放、筆圧・傾き、指との分離、ダブルタップ、アンドゥ／リドゥ |
| キャンバス・タッチ画面 | 通常ピンチ・回転、クイックピンチ、割込み、画面回転、ツールバー下への全体表示、通常画面への復帰 |
| ファイル保存 | KRA／ORA／PNG／JPEGをアプリ内と外部Files提供元へ保存・再読込し、寸法・画素・文書内容を確認 |
| バックアップ・復旧 | バックアップ有無、既存文書の上書き、自動保存、提供元の失敗、完成済み一時書出しの保持 |
| OSライフサイクル | 前景復帰、休止中の回転、未保存文書、メモリー警告、キャンバス再表示と描画再開 |
| 機能・資源追加 | 対象ツール・ドッカー・形式・バンドルの実操作、登録と読込み |

実施済み結果と残る受入れは会話で報告する。
[Issue #67](https://github.com/serika12345/librepaint/issues/67)と子Issueへの反映は依頼と文面承認を経て行う。
採用プロファイルにはPython／PyQt、G'MIC、PrintSupport、外部プロセス型の媒体処理を含めない。
これらはiOSで実行環境や製品側対応が必要なため、専用作業として扱う。iPhone専用画面と
App Store向け配布・製品署名も対応する受入れ範囲を定めて進める。

## 保守

### Nix環境とキャッシュ

依存物、製品コンパイル、アプリ包装、署名、配備を変更頻度と権限に沿って分離する。
日常の編集は既存のNinja木とキャッシュを使い、成果物の検査時に名前付きNix出力を構築する。

大きな作業でローカルFlakeを意図的に再評価する前後は、未使用ソースの件数と容量を比較する。

```sh
nix-store --gc --print-dead | rg -- '-(source|librepaint-source)$' | wc -l
nix-store --gc --print-dead | rg -- '-(source|librepaint-source)$' \
  | xargs nix path-info --json -S | jq 'map(.narSize // 0) | add'
```

想定外の増加時は評価済みプロファイルへ戻し、原因・増加量・再開条件を会話で報告する。
ローカルの容量や保存物の詳細は会話内で扱う。
削除は対象の未使用パスを特定し、利用中のプロファイル、主増分構築木、共有キャッシュを保護してから行う。

iOSのローカルバイナリキャッシュは次の入口で保存・復元する。

```sh
packaging/ios/scripts/publish-nix-cache.sh .#zlib-ios
packaging/ios/scripts/restore-nix-cache.sh .#zlib-ios
```

既定の保存先は`build-ios/nix-binary-cache`。`KRITA_IOS_NIX_CACHE_URI`で私有ストアを指定でき、
ファイル以外の宛先には`KRITA_IOS_NIX_CACHE_SIGNING_KEY`も必要。
共有時はキャッシュと信頼する公開鍵をNixへ登録する。Apple由来の成果物は私有キャッシュで扱い、
Xcode／SDK／Clangの版ごとに分離する。署名の認証情報はローカル環境で管理する。

配備時の`maintain-build-cache.sh`は現在の構築グラフの入力を保護する。
依存レシピを固定し直す専用操作
`packaging/ios/scripts/bootstrap-ios-dependencies.sh --confirm-pinning-complete`は、
旧GCルートを解放し、**Nixストア全体のごみ収集**後に集約依存物を再構築する。
全レシピが固定・コミット済みで、削除範囲への承認を得た保守時に使う。

### Ninjaの依存記録の修復

`premature end of file; recovering`とともに変更のない対象が再構築される場合は、
対象構築を停止し、依存記録を退避してNix環境のNinjaで再圧縮する。macOSの例:

```sh
cp -p build/tdd-macos/.ninja_deps build/tdd-macos/.ninja_deps.backup
cp -p build/tdd-macos/.ninja_log build/tdd-macos/.ninja_log.backup
./scripts/run-shared-test-env ninja -C build/tdd-macos -t recompact
```

対象を一度構築して不足記録を再生成し、続く構築が`ninja: no work to do.`になることを確認する。

### 設計・文書・図の保守

| 正本 | 所有する情報 |
| --- | --- |
| 本書 | 全OSの開発操作と保守手順 |
| [アーキテクチャガイド](README.md) | 現在の責務、依存方向、実行経路、ソースの調査入口 |
| [GitHub Issues](https://github.com/serika12345/librepaint/issues) | 目的、対象範囲、前提、完了条件、検証結果、残作業 |
| [GitHub Projects](https://github.com/users/serika12345/projects/2) | 作業の着手順と進行状態 |
| GitHubの子Issue・依存関係 | 作業の分割と完了を待つ条件 |
| [AGENTS.md](../../AGENTS.md) | エージェントの作業規則 |
| [資産の採用・帰属資料](../ios/non-code-assets.md)と各ソースの通知 | 配布資産の範囲とライセンス根拠 |

新しい操作手順は本書の対応する節を更新する。固定バージョン、プラグイン一覧、資産一覧は
定義ファイルを正本とし、文書からその定義へリンクする。過去の作業結果はIssueとGit履歴で参照する。

責務・所有ターゲット・許可する直接リンクを変更するときは
`docs/architecture/package-boundaries.json`とCMakeを同期する。各OSの`configure`は
CMake File APIから所有と依存方向、製品ターゲット全体の非循環性を検査する。
ソースの調査はアーキテクチャガイドの[変更内容から見る場所](README.md#変更内容から見る場所)を起点に、
呼出元、近傍CMake、プラグインJSON、試験を確認する。

D2を編集したらdocsシェルでSVGを再生成し、設計と図の対応をレビューする。

```sh
scripts/docs/render-architecture.sh
```

図の生成元は`docs/architecture/*.d2`。Markdownから参照するSVGも追跡する。

## Issueによる作業管理

Issueへの書き込みは、利用者の明示的な依頼を受け、対象と文面・変更内容の全体を提示し、
その案への承認を得てから行う。起票、コメント、本文編集、ラベル・状態変更、削除のすべてに適用する。
実装依頼や作業完了、文書の同期はIssueへの書き込み許可を兼ねない。
既存投稿の修正にも同じ手順を適用し、承認された変更だけを実行する。
本書のIssueへの記録・同期手順はすべてこの条件に従う。

ローカルの作業内容と秘密情報の運用状況は、秘密値が含まれなくても機微情報として扱う。
実際の鍵の保管・復旧方法、認証情報の設定状況、個人のパス、ホストや端末の稼働状況、
保存物や容量の詳細は会話内で扱う。非公開の保存先は利用者が指定したものを使う。
Issue、Projects、追跡文書には、一般化した手順と公開可能な製品・開発情報を置く。

作業開始時はProjectsで進行中・レビュー待ちの作業と着手順を確認し、対象Issueの本文、親子関係、
依存関係、関連PR、直近の検証結果を現在のファイルへ照合する。コマンドラインでは次を入口にする。

```sh
git status --short --branch
gh project item-list 2 --owner serika12345
gh issue list --repo serika12345/librepaint --state open
gh issue view <issue-number> --repo serika12345/librepaint --comments
```

起票前に既存Issueを検索する。同じ目的は集約し、大きい作業は有限な子Issueへ分ける。
各Issueには目的、観測する結果、範囲と所有ファイル、構造変更の移動元・移動先、
完了条件、プラットフォーム別検証、停止条件を記載する。前提は必要な成果と所有Issueを併記し、
単独で理解できる文面にする。作業の分割は子Issue、他Issueの完了待ちは依存関係へ登録する。
部分的な成果を利用できる関係や調査と本番統合の条件は、本文で区別する。

進行状態と着手順はProjectsを正本とし、Issueのopen／closedと終了理由で受入れ・中止を表す。
停止した作業はIssueへ再開条件を記録する。Milestoneは期限や完了条件を持つ成果物・リリースに使用する。
起票フォームは`.github/ISSUE_TEMPLATE/work.yml`で管理する。

検証は会話で報告し、公開用の案には挙動と検証を説明するために必要な情報だけを含める。
作業完了時は設計文書と開発手順を更新し、検証結果、残作業、次の操作を会話で報告する。
Issueへの反映は依頼と文面承認を経て行い、未反映の作業詳細は会話内で扱う。
コミット、プッシュ、マージ、ブランチ削除、成果物公開は明示的な依頼後に行う。

## 責務単位の並列実装

並列作業は一人の統合担当と最大三つの実装担当で行う。製品ヘッダー、実装、試験、CMake、
生成物の所有が重ならない範囲へ分ける。統合担当は開始前に同じ基準コミットから担当票を作り、
担当票と`preparing`、`implementing`、`ready`、`blocked`、`integrated`の状態を作業中の会話で共有する。

```text
担当識別子・基準コミット・作業ツリー絶対パス:
目的と観測する挙動:
公開ヘッダー・public API識別子・変更許可パス:
担当CMakeファイルと対象・最も近い既存契約:
対象プラットフォーム・共有コンパイラーキャッシュ:
構築実行許可: waiting | granted
Git操作権限: uncommitted | transport-commit
追加委任: forbidden | authorized
統合順・停止条件:
```

統合担当は`AGENTS.md`、アーキテクチャ文書、共有生成物を所有する。
明示的なブランチ作成権限を得て、存在しない兄弟ディレクトリーへ専用作業ツリーを作る。

```sh
task_primary_root="$(pwd -P)"
task_base_commit="$(git rev-parse HEAD)"
task_lane="<担当識別子>"
task_worktree="$(dirname "$task_primary_root")/librepaint-work-$task_lane"
test ! -e "$task_worktree"
git worktree add -b "work/$task_lane" "$task_worktree" "$task_base_commit"
```

各担当は専用Ninja木を使い、コンパイラーキャッシュだけを共有する。
主作業ツリーの評価済み環境を`run-shared-test-env`で読み込む。担当票のキャッシュ絶対パスを指定する。

```sh
task_shared_ccache="<共有コンパイラーキャッシュの絶対パス>"
LIBREPAINT_SHARED_CCACHE="$task_shared_ccache" \
  ./scripts/run-shared-test-env ./scripts/run-test <target> [ctest-regex]
```

環境自体の変更は主作業ツリーを割り当てた担当が行う。各担当は変更前の計画、直接依存、空構築範囲を
確認し、構築許可後に対象試験・反復・影響範囲・高速検査を実行する。
Linux検証はLinux担当が対象ホストで実施し、未実施条件を他OSの結果と区別する。
許可パス・公開API・依存を越える変更、他担当との重複、曖昧な挙動分類を発見したら停止する。

引渡しには次を記録する。

```text
状態: ready | blocked
基準コミット・担当先端・変更パス・移動元と移動先:
保証する挙動と分類・利用者から観測する結果と試験:
期待した最初の診断:
変更前後の計画・直接依存・コマンド数・入力数:
対象CTest・反復・影響範囲・高速検査の結果:
未実施プラットフォーム・残る危険・次の操作:
```

`transport-commit`の担当は許可パスの変更を引渡しコミットにまとめる。
`uncommitted`の担当は未コミットのまま引き渡す。統合担当は一件ずつ範囲と非重複を確認して取り込み、
設計文書と開発手順を同期し、対象CTestと高速検査を再実行する。
Issueへの反映は依頼と文面承認を経て行う。

統合と検証が済んだ担当作業ツリーは、処理終了、変更なし、ブランチによる履歴保持を確認して除去する。
`git worktree remove --force <絶対パス>`は登録済み担当パスに限定し、無視対象の専用構築木も同時に除去する。
未統合・未コミットの作業ツリーは保持する。利用中の主増分構築木・共有キャッシュ・利用者の成果物を保護し、
回収量と保持した生成物を会話で報告する。ブランチ削除は別途明示的な依頼を受けて行う。
