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

別のソース配置へ差分を転送して既存の構築木を再利用する場合は、転送前後の内容ハッシュと
変更した入力一覧を記録する。転送先で内容が変わった入力には、転送時点の更新時刻を設定する。
元の時刻を保持するアーカイブや同期方法では、旧オブジェクトの方が新しく見える場合があるため、
内容が変わった入力とその依存物が増分計画に現れることを確認してから検証する。
実行中の構築・試験が参照するソース配置は、その検証が終了するまで固定する。

AndroidとWindowsの増分入口は、固定した環境がコンパイラー、依存物の探索先とQtの設定を
供給する。呼出し元からはホーム・利用者名・一時保存先・端末、プロキシ、Androidの接続先と、
明示した作業設定を引き継ぐ。作業設定は`LIBREPAINT_CACHE_ROOT`、
`LIBREPAINT_WINDOWS_BUILD_ROOT`、`LIBREPAINT_ANDROID_BUILD_DIR`、
`LIBREPAINT_ANDROID_REQUESTED_PROFILE`、`LIBREPAINT_ANDROID_TEST_POLL_LIMIT`と
`CMAKE_BUILD_PARALLEL_LEVEL`で指定する。
ホスト用の`Boost_DIR`や`Qt6WaylandClient_DIR`が既存のCMakeキャッシュへ保存されている場合は、
対象のキャッシュ項目を取り除き、対応する固定環境で`configure`と`plan`を実行して探索結果と
構築範囲を確認する。

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

`kis_algebra_2d_test`は、円形マーカーの斜線から得た固定の境界入力について、
境界上の点とその両隣の表現可能な座標を許容差なしで区別する。
`kis_marker_painter_test`は直径8、間隔0.16・0.5・2.4の円を負座標から斜めに並べ、
中心線の完全な被覆と既存の基準画像を検査する。

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
Qtの異常終了診断を子プロセスの標準エラーで検査する場合は、子プロセスへ
`QT_FORCE_STDERR_LOGGING=1`を指定して観測先を固定する。

```sh
run-test kis_strokes_queue_test '^libs-image-kis_strokes_queue_test$'
ctest --preset tdd-macos --output-on-failure -R '<影響範囲の正規表現>'
```

Linuxでは`tdd-linux`を使う。プリセットは試験用資源の探索先と標準の画面外方式を設定する。
XCBを要求するLinuxの画面試験は、デスクトップまたは独立したXvfbで実行する。

`verify`は高速検査、ネイティブ全ターゲットの構築、通常CTestを実行する。
macOSは`tdd-macos`、Linuxは`tdd-linux`のCMakeプリセットを使う。
両方とも`BUILD_TESTING=ON`、`KRITA_ENABLE_BROKEN_TESTS=OFF`、
`LIMIT_LONG_TESTS=ON`、`CRASH_ON_SAFE_ASSERTS=ON`。隔離した試験は原因・決定性・比較規則を
確認し、一件ずつ通常検査へ戻す。

通常の共有UI変更はmacOSのネイティブ検査とiPadOSの構築・静的監査を優先する。
他OSを所有する作業、リリース前、そのOSでの問題発覚時は該当OSを追加検証する。
Androidは構築費用が大きいため、この条件に沿って検証する。

`run-test KoSvgTextDefaultsContractTest`は、新しいプロセスの32本の処理スレッドで
SVG文字のフォント設定を同時に初めて解決する。各処理が完全な既定の字体、太さ、幅、
寸法と光学寸法設定を取得することを検査する。

### 外部プラグインのSDKと再ビルド

`nix build .#plugin-sdk-tools`はSDKの生成・再ビルド用コマンドとCMake設定を独立して構築する。
`nix develop .#plugin-sdk`は本体と同じ固定した依存物を読み込み、`plugin-sdk`を提供する。
開発環境の初回作成と意図した更新には、この名前付き環境を使う。
SDKのツール、CMake設定と受入れ用ソースだけを変更した場合、本体のソース入力は維持される。
`nix flake check --no-build --all-systems`は、SDK変更と描画コード変更に対する入力ハッシュの
維持・更新を検査する。SDKの生成物は下記の構築済みライブラリーから作成する。

`scripts/plugin-sdk`は、Nixの開発環境で構築済みのネイティブmacOS／Linux共有ライブラリー構成を
入力にする。SDKの生成は構築済みライブラリーを取り出す処理で、必要な再構築が残る場合は
対応する本体対象の構築を要求する。SDKとプラグインの出力には、新しい版ごとの配置先を使う。
C++向けSDKは公開APIの所有ライブラリーとヘッダーを収録する。Python拡張とSIPの内部生成物は、
そのC++ APIを利用する実行時部品としてアプリケーション側が所有する。

読み込んだ共有環境では次を実行する。自動実行では各コマンドを
`./scripts/run-shared-test-env`から呼び出す。

```sh
sdk_build_tree="$(build-incremental native path)"
./scripts/plugin-sdk package --build "$sdk_build_tree" --output "$PWD/build/plugin-sdk/review"
./scripts/plugin-sdk build --sdk "$PWD/build/plugin-sdk/review" \
  --source /path/to/plugin --build /path/to/plugin-build --output /path/to/plugin-artifacts
./scripts/plugin-sdk check --sdk "$PWD/build/plugin-sdk/review" --output /path/to/plugin-artifacts
```

`build`はプラグインのCMakeディレクトリーを別の最上位プロジェクトから取り込み、
元のファイルを保持する。`find_package(LibrePaintSDK CONFIG REQUIRED)`を持つ独立プロジェクトは
`--project`を指定する。SDKのCMake設定は公開ヘッダー、元の対象名、Qtの生成処理と翻訳処理を
提供する。コンパイラー、Qtと依存ライブラリーは収録した構築環境に合わせる。
導入したプラグインのライブラリー探索経路には、選択したSDKと固定した依存物を含める。
実行側が事前に読み込むライブラリーによらず、プラグイン自身の直接依存を解決する。
`check`はSDKの識別値、収録ファイルとプラグイン成果物を照合する。

SDKの受入れは次で実行する。検証用配置先とログは一時ディレクトリーに保持される。
LinuxはVulkanを利用できるX11の表示環境から実行する。

```sh
./scripts/run-shared-test-env ./scripts/verify-plugin-sdk --build build/tdd-macos
```

Linuxの構築木は`build/tdd-linux`を指定する。試験は丸マーカーブラシ、丸角フィルター、TGA入出力と
画像位置の調整拡張をソース変更なしで構築し、元のソース木・構築木へのコンパイル時の参照を検査する。
丸角フィルターは実際に読み込み、GPUとの画素受渡し、文書への反映と統一したUndo／Redoを検査する。
画素の受渡しにはGPU画像を参照する公開ノードAPIを使い、公開された`Krita`クラスと
新しい画像ヘッダーを同じ利用側でコンパイルする。単独読込みの試験はQtだけをリンクし、
プラグインの実行時依存を成果物自身が解決することを検査する。
他の代表プラグインは、この検査ではソースの再構築と成果物の照合を対象にする。
SDKとプラグインの破損、不一致と既存出力の保護は高速検査が担当する。
この試験はコンパイル・リンク用のNix環境変数を外し、SDKが記録したヘッダーとリンク先で
構築する。`--tool /path/to/plugin-sdk`で、独立したNix出力のコマンドも同じ試験に通せる。

### 圧縮画素形式の契約

KRAとタイル保存が利用するLZFの展開、成分の並替えと復元を、Qt CoreとQt Testで検証する。

```sh
./scripts/run-shared-test-env ./scripts/run-test ImageLzfCompressionContractTest
```

試験は通常の連続値、重なる参照、長い参照、最大距離の参照、入力の途切れ、
不正な引数と出力容量不足を検査する。不正入力では0を返し、指定した出力領域の外を保持する。

`kis_paint_device_test`はKRA保存側が利用する`KisPaintDevice::write()`から、
負座標と透明画素のRGBを含む固定した64タイルを、3通りの生成順で保存する。
タイル列の全バイト、読込み後の全画素と再保存したタイル列を完全一致で照合する。
文書ヘッダー、タイルヘッダー、画素と後続タイルで書込みを拒否し、失敗の伝播、
後続書込みの停止、画素とUndo／Redoの保持、再保存を検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`cpu-ordered-tiles.bin`と`cpu-ordered-tile-pixels.bgra`を出力する。
環境間ではタイル列と画素列をそれぞれバイト単位で比較する。

### 描画操作・履歴・復旧の値契約

円形ブラシの入力保持、形状変更、ラスター画像の採用とタイル置換、文書全体の250操作履歴、
保存対象版、復旧の検証は次で実行する。
Qt Core、Qt Test、zlibで構築し、GPUやウィンドウを必要とする検証は対応する描画・表示所有者で行う。

```sh
./scripts/run-shared-test-env ./scripts/run-test KisGpuOperationDocumentTest
./scripts/run-shared-test-env ./scripts/run-test KisGpuRasterPixelsTest
```

試験は入力の分割、未確定操作の確定・取消し、Redo分岐、レイヤー変更、保存中の編集、
固定版の寿命、容量不足、不正な復旧記録の原子的な拒否を検査する。
属性変更の試験は、連続調整のUndo単位、保存済み位置での区切り、結合前に固定した版の保存、
変更前へ戻った名前と不透明度、直接書込みと選択の保持、復旧形式9の読込みを検査する。
色ラベルは描画レイヤーとグループ、未確定入力、固定版、履歴の両方向と復旧で検査する。
合成成分の指定は形式31以降へ保存し、生成元とハッシュを持つ形式30の固定記録を全成分の指定として読む。
固定版、直接書込み、未確定入力、Undo／Redoと不正な成分値を持つ復旧記録の原子的な拒否を照合する。
固定入力の`projection-channels.recovery`を`LIBREPAINT_GPU_RESULT_DIR`へ出力し、環境間で全バイトを比較する。
形式32の一括属性履歴は、複数レイヤーの属性、後続の直接書込み、Undo／Redoと復旧後の結果を照合する。
生成元とハッシュを持つ形式31の固定記録で、未確定入力と属性履歴の読込みを検査する。
形式33の通過指定は、固定版、描画中の入力、属性の直接変更、一括変更、取消しと履歴の両方向で照合する。
生成元とハッシュを持つ形式32の固定記録を独立合成として読み、`group-pass-through.recovery`を
比較用ディレクトリーへ出力する。非グループの通過指定と不正な真偽値は原子的に拒否する。
形式37の合成方式は、固定版、未確定入力、直接変更、一括変更、履歴、複製と復旧で照合する。
生成元とハッシュを持つ形式33〜36の固定記録を読み、通常・乗算・スクリーン・比較（明）と未確定入力を保持する。
`layer-blend-modes.recovery`、`layer-screen-mode.recovery`、
`layer-darken-mode.recovery`、`layer-lighten-mode.recovery`、
`layer-add-mode.recovery`と`layer-linear-dodge-mode.recovery`を
比較用ディレクトリーへ出力する。未知の方式は文書と履歴の採用前に拒否する。
乗算・スクリーン・比較（暗）・比較（明）の画素とグループの既定色は、ARM64・x86_64のCPU投影から取得した固定RGBA8参照で照合する。
加算・線形覆い焼きは色の飽和を含む固定入力を使い、CPUのBGRA成分指定をRGBAへ対応付けた参照と照合する。
両公開識別子はレイヤー設定、公開API、挿入・複製、KRA往復、PNG・JPEGの出力で保持する。
透明度、不透明度、合成成分、明示タイル・既定色、通常方式への変更とUndo後の再利用を扱う。
画面側の属性試験は、単一・複数選択のプレビュー、取消し、確定、Redo分岐と保存結果を照合する。
確定通知の属性値、百分率で表し切れない8ビット不透明度の復元、編集中に完了した描画と階層変更も検査する。
`KisGpuPaintDocumentTest`は生成元とハッシュを持つ形式29の固定記録で、色ラベルの既定値と階層を照合する。
KRA情報からの色ラベルの採用と、新形式で明記したゼロの値が保持されることも検査する。
圧縮画素の試験は全RGBA成分、負のタイル座標、固定版の寿命、壊れた圧縮入力、
展開長の過不足と末尾データの拒否を検査する。

### 固定ラスター値とKRAの互換契約

```sh
./scripts/run-shared-test-env ./scripts/run-test TestKraRasterCodec
./scripts/run-shared-test-env ./scripts/run-test kis_kra_loader_test
./scripts/run-shared-test-env ./scripts/run-test KisGpuPaintDocumentTest
./scripts/run-shared-test-env ./scripts/run-test KisGpuTileRendererTest
./scripts/run-shared-test-env ./scripts/run-test TestChannel
./scripts/run-shared-test-env ./scripts/run-test KisNodeCommandsAdapterTest
```

`KisGpuOperationDocumentTest::selectionRectIsFrozenInOperationsAndRecovery`は、描画開始時の
選択矩形が操作と復旧記録に残り、入力中の選択変更を拒否し、後の選択変更が既存操作を書き換えないことを検査する。
選択変更は取消し可能な履歴項目となり、Undo／Redoと復旧後の履歴移動で版ごとの選択を復元する。
`KisGpuTileRendererTest::selectionLimitsPaintFillAndEraseAcrossHistoryAndRecovery`は、
矩形選択の外側が描画・塗り・消去の前後で不変であること、内側の画素、Undo／Redo、
復旧後の再描画が同じ結果になることを検査する。現在の対象は文書座標の矩形選択である。
`KisGpuTileRendererTest::featheredSelectionClipsPaintFillEraseHistoryAndRecovery`は、
ぼかし被覆の縁で描画のアルファが被覆率に一致し、内部が完全被覆、外部が不変であること、
塗りつぶしが被覆率を元アルファとして使い、消去が既存アルファの被覆率分だけを減らすこと、
一回の履歴としてのぼかし適用、Undo／Redo、復旧後の再描画が同じ画素になることを検査する。
`KisGpuTileRendererTest::patternFillTilesAcrossSelectionHistoryAndRecovery`は、
4×4パターンを4枚のタイルへまたぐ選択矩形に塗り、文書原点からの位相、選択外の透明保持、
一回の履歴、Undo／Redo、復旧後の同じ画素を検査する。
`KisGpuTileRendererTest::patternFillClipsAlphaCompositesErasesAndRejectsInvalidInput`は、
部分アルファ選択とパターンアルファ・不透明度の乗算、通常合成の直線RGBA、
消去時のRGB保持と選択外の保持、不正なパターン画素の拒否を検査する。
`KisGpuTileRendererTest::fillBlendModesMatchLayerComposition`は、通常の下地に対する
乗算・スクリーン・比較（暗）・比較（明）・加算・線形覆い焼きの塗りがレイヤー合成と同じ
整数RGBA8結果になること、選択内だけの適用、一回の履歴、Undo／Redo、復旧後の同じ画素を検査する。
`KisGpuTileRendererTest::patternFillBlendModeMatchesLayerComposition`は、部分アルファの
パターンと選択矩形を乗算方式で塗り、パターンアルファ・選択外の保持、一回の履歴、
Undo／Redo、復旧後の同じ画素を検査する。
`KisGpuTileRendererTest::fillBlendModePreservesOpacityAlphaLockAndChannelMask`は、
塗り不透明度を乗じた方式合成、アルファ保護時のアルファと透明画素RGBの保持、
チャンネル保護成分の保持を検査する。
`KisGpuOperationDocumentTest::fillBlendModesFreezeHistoryAndRecovery`は、各塗り合成方式を
操作へ固定し、一回の履歴、Undo／Redo、復旧の同一バイトと形式47の通常方式読込みを検査する。
`KisGpuOperationDocumentTest::patternFillBlendModeFreezesHistoryAndRecovery`は、
パターン塗りと方式の同時固定、履歴、Undo／Redo、復旧後の同一操作を検査する。
`KisGpuOperationDocumentTest::rejectedFillBlendModesPreserveHistoryAndInput`は、
未知の方式、輪郭付き図形、塗りのない方式指定を採用前に拒否し、消去が方式を持たないことを検査する。
`KisGpuTileRendererTest::strokeBlendModesMatchFillBlendComposition`は、通常の下地に対する
ブラシ被覆の乗算・スクリーン・比較（暗）・比較（明）・加算・線形覆い焼きが塗りと同じ
整数RGBA8結果になること、一回の履歴、Undo／Redo、復旧後の同じ画素を検査する。
`KisGpuTileRendererTest::strokeBlendModePreservesOpacitySelectionAlphaLockAndChannelMask`は、
ブラシ不透明度、選択被覆、アルファ保護、チャンネル保護を各方式の合成と組み合わせて検査する。
`KisGpuTileRendererTest::gaussianStrokeBlendModeMatchesSeparableComposition`は、
Gaussian被覆の中心でも同じ方式合成と被覆外の保持を検査する。
`KisGpuOperationDocumentTest::strokeBlendModesFreezeHistoryAndRecovery`は、各ブラシ合成方式を
操作開始時に固定し、履歴、Undo／Redo、復旧の同一バイトと形式48以前の通常方式読込みを検査する。
`KisGpuOperationDocumentTest::rejectedStrokeBlendModesPreserveHistoryAndInput`は、
未知の方式と消去を伴う方式指定を入力採用前に拒否することを検査する。
`KisGpuOperationDocumentTest::patternFillFreezesPixelsHistoryAndRecovery`は、
凍結パターン画素、塗りつぶしの履歴種別、一回の履歴とUndo／Redo、復旧の同一バイト、
ラスタ容量超過と輪郭を持たない自由描画への適用拒否を検査する。
`KisGpuOperationDocumentTest::freehandSelectionFreezesCoverageHistoryAndRecovery`は、
任意の閉じた境界から硬い被覆マスクを作り、選択変更が一回の履歴になること、描画操作が
開始時のマスクを固定すること、Undo／Redoと復旧が被覆・反転状態・操作のマスクを保つことを検査する。
`KisGpuOperationDocumentTest::featheredSelectionAddsAlphaHistoryAndRecovery`は、
硬い被覆のぼかしが部分被覆のアルファマスクを作ること、内部の完全選択と縁の部分選択、
ぼかしの適用が一回の履歴になること、Undo／Redoが硬い被覆とアルファ被覆を往復すること、
復旧が両表現・順序・反転状態・描画操作の固定マスクを保つことを検査する。
復旧形式46は共有する任意被覆マスクを256×256タイルの表として一度だけ記録し、文書版と描画操作は
その添字を共有する。加えて各ペイントレイヤーの透明マスク参照と表示状態をレイヤー記録へ保存する。
硬い被覆は1画素1ビット、ぼかし被覆は1画素1バイトで記録し、形式45以前は透明マスクを持たない
記録として読み込む。形式44は1ビットの被覆として読み込み、形式43以前の矩形・楕円選択は従来の記録から読み込む。
`KisGpuDocumentViewTest::rectangularSelectionLimitsPainting`は、通常の矩形選択ツールで
選択範囲を作成し、GPU選択とCPU表示用マスクの一致、選択内だけの描画、選択外の不変、
解除後の描画、選択変更を含むUndoの件数を検査する。
自由選択ツールは任意被覆をGPU文書へ固定する。多角形選択ツールは文書座標の頂点列を閉じた
多角形の硬い被覆として固定し、置換アクションだけを一回の履歴で適用する。ベジェ曲線選択ツールは
単一の閉じた輪郭を多角形へ平坦化して同じ被覆へ変換し、複数輪郭や退化した経路はCPU選択へ
切り替えず拒否して診断を表示する。拡大・縮小・ぼかしオプションは両ツールともGPU文書では拒否する。
自由選択の移動は被覆を平行移動し、CPU表示写像のプレビューと確定時の一回の履歴を使う。
拡大・縮小は矩形半径による被覆の膨張・収縮、反転は画像内の補集合、境界選択は内側境界帯として適用する。
コピー・切り取りは任意被覆をアルファ画像としてクリップボードへ渡す。
`KisGpuDocumentViewTest::featherActionSoftensFreehandSelection`は、自由選択ツールと同じ任意被覆から
`featherselection`操作を実行し、アルファ被覆の内部完全選択と縁の部分選択、一回の履歴、
CPU表示写像の部分選択と半選択の輪郭、部分被覆による塗りつぶし、Undo／Redo、
KRA保存・再読込後のアルファ被覆と画素の一致を検査する。
`KisGpuDocumentViewTest::selectionFillUsesGpuSelection`は、GPU文書の矩形選択内だけの
前景色塗りつぶし、選択外の不変、Undo／Redoを検査する。
`KisGpuDocumentViewTest::selectionMergedCopyUsesGpuSelection`は、矩形選択の投影だけを
クリップボード画像として切り出すことを検査する。
`KisGpuDocumentViewTest::selectionCopyAndCutUseGpuSelection`は、通常のコピーが
アクティブレイヤーの選択範囲だけを切り出し、切り取りが選択内を透明化して
選択外を保持し、Undo／Redoできることを検査する。
`KisGpuDocumentViewTest::selectionMovePreviewsAndCommitsOneHistory`は、ドラッグ中の
表示写像だけの追従、確定時の一回のGPU履歴、Undo／Redo、Escapeによる取消し、
反転選択の穴の移動を検査する。
`KisGpuDocumentViewTest::selectionGrowAndShrinkUseGpuSelection`は、矩形選択のX／Y半径による
拡大・縮小、画像端でのクリップ、選択が空になる場合の解除、一回の履歴、Undo／Redoを検査する。
`KisGpuDocumentViewTest::invertedSelectionLimitsPaintingAndKra`は、反転選択の作成、
矩形内の塗りつぶし除外、矩形外の塗りつぶし、Undo／Redo、KRA保存・再読込後の反転状態、
反転選択の穴に対する拡大・縮小、edgeLock付き拡大・縮小、穴を透明にした合成コピー・レイヤーコピー、
選択された外側だけの切り取りを検査する。
`KisGpuOperationDocumentTest::invertedSelectionSurvivesRecovery`は、反転フラグの履歴と復旧を検査する。
`KisGpuOperationDocumentTest::ellipticalSelectionSurvivesRecovery`は、楕円形状の履歴と復旧を検査する。
`KisGpuDocumentViewTest::copySelectionToNewLayerUsesGpuPixels`は、GPU文書で
選択範囲を新規レイヤーへコピー・切り取りし、画素数、位置、レイヤー数、新規レイヤーの
自動選択、反転矩形・反転楕円の補集合、楕円選択の位置と被覆、Undoを検査する。GPUレイヤーの活性化は`libs-ui-layers-KisGpuDocumentViewTest`でも検査する。
`KisGpuDocumentViewTest::arrowKeysNudgeGpuSelection`は、矢印キーとShift併用が
GPU選択を1画素・10画素・100画素・1000画素単位で移動し、形状と反転状態を保持することを検査する。
`KisGpuDocumentViewTest::reselectRestoresGpuSelection`は、解除した矩形選択と反転楕円選択を
再選択で復元することを検査する。
`KisGpuDocumentViewTest::pasteClipboardImageCreatesGpuLayer`は、クリップボード画像を
GPU文書の新しい描画レイヤーへ一回の履歴で貼り付け、キャンバスより大きい画像の左上トリミングと
Undoによる除去を検査する。
`KisGpuDocumentViewTest::ellipticalSelectionLimitsPaintingAndKra`は、反転楕円のedgeLock付き拡大・縮小を含む楕円内の塗りつぶし、
楕円外の不変、Undo／Redo、KRA保存・再読込後の楕円形状と画素、楕円の拡大縮小、
反転楕円のKRA往復、楕円の合成コピー・レイヤーコピー・切り取り、
反転楕円のコピー・切り取り、反転楕円の穴に対する拡大・縮小を検査する。
`KisGpuDocumentViewTest::ellipticalToolCreatesDocumentSelection`は、通常の楕円選択ツールの
ネイティブドラッグが画像座標の楕円選択を生成し、続くドラッグで楕円と反転楕円の穴を
移動して一回の履歴でUndo／Redoできることを検査する。
`KisGpuDocumentViewTest::freehandToolCreatesArbitraryMaskSelection`は、通常の自由選択ツールの
ネイティブドラッグが任意の閉じた境界をGPU文書の硬い被覆マスクへ固定し、CPU表示用マスクの輪郭、
選択内だけの塗りつぶし、選択外の不変、解除後の再選択が同じ被覆を復元することを検査する。
`KisGpuDocumentViewTest::polygonToolVerticesCreateFrozenCoverage`は、多角形選択ツールが扱う
文書座標の頂点列を共有の変換ヘルパーへ渡し、L字の閉じた多角形の全画素が期待する被覆と一致すること、
硬い被覆として一回の履歴で固定されること、CPU表示写像の選択矩形、拡大・反転・Undoが
自由選択と同じ被覆経路を使うことを検査する。ネイティブクリック列には依存しない。
`KisGpuDocumentViewTest::pathToolClosedPathCreatesPolygonCoverage`は、閉じた直線経路と
曲線経路が単一の多角形へ平坦化されて期待する被覆になること、複数輪郭の経路が空の頂点列として
拒否されることを検査する。
`KisGpuDocumentViewTest::polygonalToolCreatesArbitraryMaskSelection`と
`KisGpuDocumentViewTest::pathToolCreatesArbitraryMaskSelection`は、通常の多角形選択ツールと
ベジェ曲線選択ツールのネイティブ入力が閉じた頂点列を任意被覆の硬いマスクとして固定し、
一回の履歴になることを検査する。各ツールの入力試験は独立したプロセスで実行する。
`TestKraRasterCodec::rectangularSelectionSurvivesSavingAndReopening`は、GPU文書のKRAが
ルートの`selectionmask`と`PIXELSIZE 1`の`.pixelselection`を矩形選択として読み書きし、
再保存・再読込後も選択とレイヤー画素を保持することを検査する。
`TestKraRasterCodec::binaryFreehandSelectionSurvivesSavingAndReopening`は、多角形選択と同じ
硬い任意被覆がKRAの8ビット被覆ストリームへ書き出され、再読込・再保存後も硬い被覆・選択矩形・
`SelectionKind::Freehand`を保つことを検査する。
`TestKraRasterCodec::featheredSelectionAlphaSurvivesSavingAndReopening`は、部分被覆のアルファマスクが
KRAのアルファストリームへ書き出され、再読込・再保存後も被覆値・選択形状・画素・反転状態を
保つことを検査する。
`KisGpuOperationDocumentTest::transparencyMaskHistoryCompositionAndRecovery`は、矩形選択からの
透明マスク追加、描画後の被覆保持、表示切替、削除、Undo／Redo、復旧後の被覆と表示状態、
追加・表示切替・削除の履歴種別を検査する。
`KisGpuOperationDocumentTest::transparencyMaskCoverageReplacementSurvivesRecovery`は、
保持した透明マスクの被覆だけを一回の履歴で置き換え、レイヤーの入力列を変えないこと、
同一被覆とnullの拒否、マスク無しレイヤーの拒否、Undo／Redo、塗りつぶしの履歴種別、
復旧後の被覆・履歴種別・再復旧の同一性を検査する。
`KisGpuOperationDocumentTest::transparencyMaskFollowsCanvasResize`は、キャンバスのリサイズで
透明マスク被覆が新しい原点へ移動し、新しい境界へクリップされ、Undoで元の被覆へ戻ることを検査する。
`KisGpuOperationDocumentTest::transparencyMaskFollowsLayerCrop`は、レイヤー単体のクロップで
透明マスク被覆がクロップ矩形へクリップされ、Undoで元の被覆へ戻ることを検査する。
`KisGpuOperationDocumentTest::transparencyMaskFromSelectionShapesAndRejections`は、楕円・
アルファ被覆・反転選択からのマスク作成、グループとロック済みレイヤーの拒否、
二重追加とマスク無しの表示切替・削除の拒否を検査する。
`KisGpuTileRendererTest::layerTransparencyMaskMultipliesCompositionAlpha`は、マスク被覆が
合成アルファだけを乗じ、レイヤー自身の画素を変えないこと、表示切替と削除、Undo／Redo、
部分アルファ被覆と被覆外の透明化を検査する。
`KisGpuPaintDocumentTest::transparencyMaskComposesSavesReopensAndContinues`は、マスク付き
レイヤーの投影、表示切替、KRA保存・再読込後の被覆・表示状態・投影・画素、再読込後の
マスク外への描画と再保存、削除とUndo／Redoを検査する。
`KisGpuPaintDocumentTest::transparencyMaskAcceptsDirectBrushEraseFillAndRoundTrips`は、
マスク子ノードを対象にしたブラシ描画が被覆だけを広げてレイヤー画素を変えないこと、
消しゴムが被覆だけを減らすこと、選択範囲の塗りつぶしが選択内だけの被覆を加えること、
操作ごとに一回のGPU履歴と履歴種別、Undo／Redo、投影、KRA保存・再読込後の被覆・画素・投影を検査する。
`TestKraRasterCodec::transparencyMasksSurviveSavingAndReopening`は、
ペイントレイヤー子の`transparencymask`要素と`.pixelselection`の往復、アルファ被覆と
表示状態、複数レイヤーのマスク、再保存、マスク削除時の要素とファイルの除去を検査する。
`KisGpuDocumentViewTest::transparencyMaskActionsUseTheActiveGpuPaintLayer`は、
アクティブなGPUペイントレイヤーに対する追加・表示切替・削除の操作、
GPU履歴のUndo／Redo、選択の無い追加の拒否を検査する。
`KisGpuDocumentViewTest::layerPanelShowsAndSelectsGpuTransparencyMaskChild`は、
マスク追加後にレイヤーパネルが親ペイントレイヤーの子として透明マスクノードを表示すること、
被覆率から生成したサムネイル、子ノードの選択によるアクティブマスク化、親レイヤー選択による復帰、
選択自体が履歴を追加しないこと、子ノードを対象にしたGPU描画資源取得が親レイヤーのマスクを指すこと、
ブラシ描画が一回の履歴で被覆と子ノード画素・サムネイルだけを更新すること、
選択範囲の塗りつぶし操作がマスク被覆だけを更新しレイヤー画素を変えないことを検査する。
`KisGpuDocumentViewTest::maskChildFreehandStrokeEditsOnlyCoverage`は、マスク子ノードを選んだ
状態で通常のブラシツールのネイティブ入力が親レイヤーの保持マスクだけを編集し、レイヤー画素・
文書の描画入力列・他の編集を変えず、一回のGPU履歴と子ノード画素を更新することを検査する。
`KisGpuDocumentViewTest::layerPanelTogglesAndRemovesGpuTransparencyMaskChild`は、
パネルの子ノードの表示切替がマスク表示状態の履歴になること、Undo／Redo、
子ノードの削除が保持マスクの削除になり親レイヤーへ戻ること、削除のUndoで子ノードと
削除前の表示状態が復元されること、Redoで再び除去されることを検査する。
`KisGpuDocumentViewTest::layerPanelGpuTransparencyMaskChildSurvivesSaving`は、
KRA保存・再読込後の子ノード、被覆、表示状態、サムネイルのパネル項目を検査する。
`KisGpuTileRendererTest::invertColorsComplementsRgbPreservesAlphaAndClipsSelection`は、
固定RGBA8タイルの選択内だけをRGB反転し、アルファ、選択外の全バイト、透明画素のアルファ0と
隠れRGB、一回の履歴と履歴種別、Undo／Redo、復旧後の同じ画素、適用後の選択変更が
凍結被覆を書き換えないことを検査する。
`KisGpuTileRendererTest::invertColorsInterpolatesFeatheredAndInvertedCoverage`は、
アルファ被覆の部分選択を被覆率で補間した整数RGB8、完全被覆、被覆外の保持、
反転矩形の補集合、楕円被覆の画素判定を検査する。
`KisGpuOperationDocumentTest::invertColorsFreezesCoverageAndRecovers`は、
矩形・楕円・反転・アルファ被覆の凍結、一度の履歴、複製時の操作識別子、
Undo／Redo、復旧後の履歴種別と選択被覆（部分アルファ値を含む）を検査する。
`KisGpuOperationDocumentTest::rejectedInvertColorsPreserveHistoryAndInput`は、
ロック済みレイヤー・グループ・不在レイヤー・入力中の拒否、取消し後の履歴と画素の保持、
終了後の拒否を検査する。
`KisGpuPaintDocumentTest::invertColorsSavesReopensAndRecoversPixels`は、
部分アルファの描画と選択内の反転、透明画素の隠れRGB、一回のGPU履歴とUndo／Redo、
復旧後の同じ画素、KRA保存・再読込後の結果画素、再読込後の新しい選択による再適用を検査する。
`KisGpuTileRendererTest::grayscaleUsesRec709CoefficientsWithIntegerRounding`は、
保存済みsRGB符号化バイトに対するRec.709係数（2126/7152/722、分母10000、+5000の四捨五入）の
輝度、無彩色の厳密な保持、0.7152の切り上げと0.0722の切り捨て、アルファ保持、
透明画素の隠れRGBフィルター、選択外のバイト保持、フィルター種別と被覆の凍結、
一回の履歴、Undo／Redo、復旧後の同じ画素を検査する。
`KisGpuTileRendererTest::grayscaleClipsEllipseInvertedBinaryAndAlphaCoverage`は、
二値マスク・アルファ被覆の補間・反転矩形・楕円被覆によるGrayscaleのクリップと、
選択外画素の保持を検査する。
`KisGpuOperationDocumentTest::grayscaleFilterFreezesCoverageAndRecovers`は、
楕円・反転・アルファ被覆の凍結、Grayscale履歴種別、一度の履歴、Undo／Redo、
復旧後の種別と選択被覆（部分アルファ値を含む）を検査する。
`KisGpuOperationDocumentTest::grayscaleRecoveryKeepsItsKindAcrossFormatBumps`は、復旧記録が
形式53を名乗ること、形式53・52・51ではGrayscaleが復元されること、同じ記録を形式50として
再署名するとInvert Colorsだけを許す検証がGrayscaleを拒否して現行文書を変えないことを検査する。
`KisGpuPaintDocumentTest::grayscaleSavesReopensAndRecoversPixels`は、生タイルの
Grayscale適用、隠れRGBのフィルターとアルファ0の保持、無彩色の保持、選択外の保持、
一回のGPU履歴とUndo／Redo、復旧後の種別と画素、KRA保存・再読込後の結果画素、
再読込後の新しい選択による再適用を検査する。
`KisGpuOperationDocumentTest::hsvAdjustmentFreezesSettingsAndRecovers`は、製品の
`h`・`s`・`v`・`type`・`compatibilityMode`の凍結、楕円・反転・アルファ被覆、
`HistoryAction::HsvAdjustment`の一度の履歴、複製時の新しい操作識別子、
Undo／Redo、復旧後の同じ設定と選択被覆を検査する。
`KisGpuOperationDocumentTest::hsvAdjustmentRejectsInvalidSettingsAndPreservesHistory`は、
製品スライダー範囲外の値と未対応の`type`を履歴・画素を変えずに拒否すること、
ロック済みレイヤー・グループ・不在レイヤー・入力中・終了後の拒否、
範囲境界値の受入れを検査する。
`KisGpuOperationDocumentTest::hsvAdjustmentFreezesLumaDependentModesAndRecovers`は、
HSY・YUV・Colorizeの`type`・`colorize`・実効luma係数の凍結、qreal係数のbinary32丸め、
楕円・反転の凍結、Undo／Redo、復旧後の同じ設定と係数を検査する。
`KisGpuOperationDocumentTest::hsvAdjustmentRejectsUnfrozenLumaAndInvalidRanges`は、
非正係数のRec.709置換、1以上・非有限係数の拒否、未凍結lumaでのHSY・YUV・Colorizeの拒否、
モード別スライダー範囲（YUVの-100〜100、Colorizeの0〜360と0〜100）の境界、
拒否時の履歴保持を検査する。
`KisGpuOperationDocumentTest::hsvAdjustmentRecoveryRequiresFormat53`は、復旧記録が形式53を
名乗ること、形式53で設定・Colorize・luma係数が復元されること、形式52の記録が
Colorizeなしの`type` 0〜2として復元されること、形式52として再署名したHSYが拒否され
現行文書を変えないこと、同じ記録を形式51として再署名するとHSV Adjustmentを拒否することを
検査する。
`KisGpuTileRendererTest::hsvAdjustmentUsesProductValuesAndNeutralPixels`は、
HSV方針の彩度・明度・色相写像、無彩色画素が色相と彩度を無視して明度だけに従うこと、
`int(v * 255 + 0.5)`の四捨五入、アルファ保持、透明画素の隠れRGB、選択外の保持、
HSL方針との差、設定と被覆の凍結、一度の履歴、Undo／Redo、復旧後の同じ画素を検査する。
旧互換HSLのRGB `(200,30,70)`・彩度+50の参照値は、製品変換の`(243,0,48)`を使う。
`LIBREPAINT_GPU_RESULT_DIR`へ`hsv-legacy-hsl.rgba`としてタイル全体を出力する。
`KisGpuTileRendererTest::hsvAdjustmentClipsCoverageAndSupportsLegacyConversion`は、
アルファ被覆の補間、反転矩形、楕円被覆、旧互換モードで彩度上昇が無彩色へ色を与えること、
旧互換の明度規則が現代方針と異なること、旧互換フラグの凍結と復旧を検査する。
`KisGpuTileRendererTest::hsvAdjustmentUsesLumaDependentProductRulesAndFrozenCoefficients`は、
HSYの現代HCY方針と旧HCY変換、YUVの色相・彩度・値写像、Colorize（`type` 0とYUVの
スライダー範囲）、無彩色画素、アルファ保持、透明画素の隠れRGB、選択外の保持、
Rec.709と異なる凍結係数が画素へ反映されること、Undo／Redo、復旧後の同じ画素を検査する。
`KisGpuTileRendererTest::hsvAdjustmentLumaModesClipAlphaAndSelectionCoverage`は、
アルファ被覆によるYUVの補間、反転矩形でのColorize、楕円被覆でのYUV、復旧後の
同じ係数と画素を検査する。
`KisGpuTileRendererTest::yuvResetPreservesStoredPixelsAcrossRecovery`は、RGB各成分と
アルファを変えた65,536画素について、YUVのゼロ設定、Undo／Redoと復旧後の全バイト保持を検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`yuv-reset.rgba`を出力する。非ゼロのYUV参照値は
`RGBToYUV()`・`YUVToRGB()`と製品のスライダー倍率から得た値を使い、選択被覆の整数補間も照合する。
`KisGpuTileRendererTest::colorizeUsesProductLumaRoundingAcrossRecovery`は、Rec.709、
四分の一単位と微小な正係数について、Colorizeの輝度計算と復旧後の全バイトを検査する。
参照計算は製品のRGBA8変換と同じく、入力をbinary32へ正規化し、凍結係数を`qreal`へ
昇格した積和を一度だけbinary32へ丸める。無彩色化の65,536画素には、RGB `(2,43,46)`の
丸め境界とアルファ0の隠れRGBを含む。`LIBREPAINT_GPU_RESULT_DIR`へ係数別の
`colorize-rounding-<case>.rgba`を出力する。
`KisGpuPaintDocumentTest::hsvAdjustmentSavesReopensAndRecoversPixels`は、生タイルへの
HSV Adjustment適用、隠れRGBとアルファ0の保持、選択外の保持、一回のGPU履歴と
Undo／Redo、復旧後の設定と画素、KRA保存・再読込後の結果画素、再読込後の
旧互換モードによる再適用を検査する。
`KisGpuPaintDocumentTest::hsvAdjustmentLumaModesSaveReopenAndRecoverPixels`は、
Rec.709と異なる凍結luma係数を持つHSYの画素、隠れRGBとアルファ0、選択外の保持、
Undo／Redo、復旧後の係数、KRA保存・再読込後の結果画素、再読込後のColorize適用を検査する。
`KisGpuDocumentViewTest::hsvAdjustmentActionUsesProductSettingsOnGpuDocument`は、
`krita_filter_hsvadjustment`アクションが製品設定ウィジェットを開き、CPUフィルター
ストロークを開始せず、確定した`h`・`s`・`v`・`type`をGPU文書のアクティブペイント
レイヤーへ一度の履歴として適用すること、履歴ラベル、選択内だけの変更、選択外の保持、
Undo／Redo、`type`省略時の製品既定HSL、HSY・YUV・Colorizeが文書色空間のluma係数を
凍結してGPU履歴へ適用されること、Colorizeが設定ダイアログからCPUストロークなしで
到達すること、未対応モードとモード別範囲外値が画素と履歴を変えずに診断を表示することを
検査する。
`KisGpuDocumentViewTest::invertColorsActionUsesGpuDocumentAndDiagnosesUnsupportedFilters`は、
`krita_filter_invert`アクションがGPU文書のアクティブペイントレイヤーへ一度の履歴として
適用され、Undo／Redoが同じ画素を復元すること、未対応フィルターが履歴と画素を変えずに
GPU文書向けの診断を表示することを検査する。
`KisGpuDocumentViewTest::desaturateActionAppliesGrayscaleOnGpuDocument`は、
`krita_filter_desaturate`アクションがGPU文書のアクティブペイントレイヤーへ
Grayscaleの一度の履歴として適用され、選択内だけがRec.709輝度になり、選択外が保たれ、
Undo／Redoが同じ画素を復元することを検査する。UI試験は`QT_QPA_PLATFORM=cocoa`で実行し、
`KIS_TEST_PREFIX_PATH`を増分構築ディレクトリーへ設定する。単独実行は
`./scripts/run-shared-test-env ./scripts/run-test KisGpuDocumentViewTest libs-ui-filter-invert-KisGpuDocumentViewTest`、
`./scripts/run-shared-test-env ./scripts/run-test KisGpuDocumentViewTest libs-ui-filter-grayscale-KisGpuDocumentViewTest`、
`./scripts/run-shared-test-env ./scripts/run-test KisGpuDocumentViewTest libs-ui-filter-hsv-KisGpuDocumentViewTest`
を使う。
任意被覆の選択は同じストリームの画素から復元し、0か255の画素が矩形・楕円に一致しない場合に
`SelectionKind::Freehand`の硬い被覆マスクとして、部分的なアルファ値を持つ場合は
同じ種類のアルファ被覆マスクとして読み込む。
`KisGpuPaintDocumentTest::rectangularSelectionSavesReopensAndLimitsPainting`は、
選択内だけの描画、保存・再読込、選択変更後の旧画素保持と追加描画を照合する。
`KisGpuPaintDocumentTest::freehandSelectionMaskClipsPaintFillEraseAndRoundTripsKra`は、
任意被覆マスクによる描画・塗りつぶし・消去のクリップ、マスク外の保持、反転マスク、
KRA保存・再読込後の被覆・画素・反転状態を照合する。
`KisGpuPaintDocumentTest::featheredSelectionClipsPaintAndRoundTripsKra`は、ぼかし被覆による
塗りつぶしの縁の部分アルファ、内部の完全被覆、一回の履歴としてのぼかし、Undo／Redo、
KRA保存・再読込後のアルファ被覆と画素を照合する。
`KisGpuPaintDocumentTest::patternFillSavesReopensAndPreservesSelectionPixels`は、
タイル境界をまたぐ選択へのパターン塗り、パターン位相、一回の履歴とUndo／Redo、
KRA保存・再読込後の同じ画素、透明マスクへのパターン塗りと被覆の復元を照合する。
`KoRgbU8ColorSpaceTester`は、通常合成の厳密な色が5494/33となる固定入力について、
単一画素、ベクトルと端数を含む画素列、開始位置をずらした画素列で、166へ丸める結果を照合する。
異なるRGB成分とレイヤー不透明度を持つ固定画素も、同じ条件でスカラーとSIMDの一致を検査する。
RGBA8の逆アルファマスクは、既存色への適用とブラシ色の塗付けについて、
通常選択とスカラーの各実装で、最近接偶数丸めの結果を照合する。
空の列、SIMD幅の前後、開始位置のずれと一画素ごとの分割は、同じ画素と周囲の保護領域を保持する。
固定入力は端点、半整数と二進数で厳密に表せる中間値を含む。
アルファ合成は、Hard・Creamyの通常選択と汎用実装について、不透明度0.57の固定画素、
ゼロの寄与、マスク、流量と履歴値を照合する。固定種の混合画素列も一括処理と一画素ごとの分割で比較する。
通常合成の混合列は、透明・部分透明・不透明の下地を交互に置き、マスクの有無と開始位置を変える。
通常ブラシのストローク重ね合わせで使う元アルファ250・不透明度0.57・透明な下地は、
単精度の142.5を最近接偶数丸めした142を保持する。
不透明度0.5・0.57、流量1・0.5、同一の履歴値と履歴値0.8について、
元・下地アルファの全組合せを二種類の並び順で照合する。
32ビットのアルファ合成生成と直接のスカラー契約は、GCC/Clangの`-ffp-contract=off`、
MSVCの`/fp:strict`で積と和の途中丸めを保持する。
元アルファ205・下地51・不透明度0.5の結果は112、元150・下地102・不透明度0.57の結果は128となる。
元アルファ51・下地8・不透明度0.5・履歴値0.8の結果は、単精度の商から33へ丸められる。
半筆圧の円形ブラシ契約は、固定対角線上の全画素と転置画素の一致、および全OS共通のRGBA8888ハッシュを照合する。
円形自動ブラシのSIMDマスク行は、GCC/Clangの`-ffp-contract=off`、MSVCの`/fp:strict`で構築する。
筆圧曲線の所有者`kritaimagecubiccurveobjects`も同じ設定で積と和の途中丸めを保持する。
`KisCubicCurveContractTest`は出荷ブラシBasic-5 Size Opacityのサイズ・不透明度曲線について、
256要素の倍精度転送表を共通ハッシュで照合する。`LIBREPAINT_GPU_RESULT_DIR`へ
`default-brush-pressure-<条件>.f64be`を出力し、各値をビッグエンディアンのIEEE 754 binary64として環境間で比較する。
`FreehandStrokeContractTest`の間隔0.25の円形筆跡は、対角線を軸にした全チャネルの対称性と
環境共通のRGBA8888全画素ハッシュを照合する。
AArch64の混色率はハードウェア除算で計算する。数値ライブラリーの演算子を変更する場合は、
実際に選択される命令と、対象環境間の保存画素の一致を確認する。
`KisGpuTileRendererTest`はRGBを固定し、元画素と下地の全アルファ値の組合せを、
レイヤー不透明度1、128、254、255で照合する。参照値はARM64とx86_64のCPU合成から生成し、
画素ごとの処理とSIMDの一致を確認したRGBA列である。試験は実際のGPUと描画基盤を記録し、
`LIBREPAINT_GPU_RESULT_DIR`へ`normalized-opacity-<不透明度>.rgba`を出力する。
環境間の比較はRGBA列全体を対象にする。
`TestChannel`はRGBA8の全アルファ値の組合せを、不透明度255・128、16種類の表示成分、
描画レイヤーとグループで照合する。公開属性の変更が元の画素と履歴位置を保持し、
CPUとGPUの親投影が一致することを検査する。`projection-channels-<条件>-<所有者>.bgra`が比較対象である。
`KisNodeCommandsAdapterTest`はパネルのアルファ継承切替とGPU履歴のUndo／Redoを検査する。
グループの既定画素と合成結果の再利用は、成分の変更後も保持した旧投影と元画素を照合する。
`TestDocument`は、2枚の半透明レイヤーの合成を、公開画素読取り、通常KRA保存の統合画像と
PNG書き出しで検査する。CPU／GPUで同じ入力、UUID、文書情報、日時と設定を使い、
既定設定と、ICC・アルファ・圧縮・メタデータの設定を固定した条件をそれぞれ検査し、
全画素とPNGの全バイトを比較する。`source-over-rounding-<設定条件>-<描画所有者>.kra`と`.png`を保存する。
低い不透明度のグループでは、元レイヤーの画素、透明RGBを持つ部分木の投影、
上位で正規化した文書投影を個別に検査する。一様なグループ既定色も同じ丸め規則を使う。
成分指定を持つレイヤーとグループの保存では、CPU・GPU両方からKRAを再読込みし、
指定した成分、元画素、投影とPNGの全バイトを照合する。
CPU実装別の`kis_composition_benchmark_<実装名>`は色処理へ直接依存し、
32ビット合成の積と和の途中丸めを保持する設定で構築する。
各CPU実装の数値比較と、公開生成関数を通した製品実装の処理時間を分けて記録する。
CPU合成の演算時間は`KoCompositeOpsBenchmark benchmarkCompositeOver -iterations 1 -median 7`で測る。
一回の測定ごとに固定入力を初期化する。反復中に変わる下地のアルファを揃えるため、一試行の反復数を1にする。
`-tickcounter`は同じ入力の細かい計数を供給する。構築との並行実行を分け、処理完了と実表示の遅延を別々に評価する。

形式処理の試験は既存KRAと新しい書出しの往復、非表示レイヤー、キャンバス外と透明画素のRGB、
既定画素、レイヤー位置、UUID、文書XML、ICCと付属ファイルを検査する。
画像情報の更新はプロファイル名とICCを保存・再読込みで保持し、名前の省略は元の名前を保つ。
不正な名前による更新はXMLと付属ファイルを保って拒否する。`TestDocument`は通常のKRA保存と
書出しが画像の実際のプロファイル名を記録することを検査し、失敗した保存成果物を保持する。
`testSavedPngDensityUsesDocumentResolution`はCPU／GPU文書の72dpiと300×150dpiについて、
通常KRA保存の統合PNG・プレビューと通常PNG書出しを検査する。メートル当たりの画素数は
文書のdpiから最近接丸めし、Qtの`QT_FONT_DPI=72`／`96`の各起動条件で固定成果物を照合する。
出力先を`LIBREPAINT_GPU_RESULT_DIR`で指定すると、`document-density-<描画所有者>-<解像度>`の
KRAとPNGを保存する。PNGの全バイトと、同じ描画所有者のKRA全バイトを起動条件間で比較する。
`testPngWithoutPhysicalResolutionKeepsImageDefault`は、通常のPNG読込でpHYs未指定時の画像の既定解像度を検査する。
固定した2×1 RGBA8のPNGを単独、および単位未指定・メートル指定のPNGの後に読み込み、
CPU／GPUの各セッション設定で同じ解像度と画素を照合する。
`testGpuSessionOpensPngWithGpuPixelOwnership`は、通常のPNG読込でGPU画素の所有を検査する。
透明な画素のRGB、部分アルファ、パレット、インターレース、pHYsとXMPを持つ固定入力を使い、
元画素・投影・プロファイル・解像度・UUID、KRA再読込とPNG／JPEG書出しを照合する。
文書情報、選択、日時、色履歴、補助色と圧縮設定を固定し、CPU／GPUのKRA・PNG・JPEGを全バイト比較する。
`LIBREPAINT_GPU_RESULT_DIR`を指定すると、各入力の成果物を`png-import-<入力条件>/`へ出力する。
`testGpuPngRejectedOpenKeepsDocument`は、不正な署名、途中で切れた入力、未対応精度と容量超過について、
元の画像・履歴・変更済み状態・メタデータ・パスの保持を照合する。
通常画面の`testGpuSessionUsesOrdinaryCreationPaintingAndReopen:existing-png`は、PNGを開いてブラシで描画し、
Undo／Redo、KRA再読込後の追加描画とPNG書出し設定の確定を操作する。
同じ出力先の`existing-png/`へ、元PNG、描画段階の画素列、2段階のKRAと最終PNGを出力する。

`TestDocumentKraPreview.cpp`は、3画素、整数・端数倍率の拡大、面積縮小と細長い文書について、
同じRGBA8入力から通常保存したCPU／GPUの統合画素と保存プレビューの全RGBA成分を比較する。
同じ保存に含まれる`mergedimage.png`と`preview.png`も、それぞれ全バイトで照合する。
固定したUUID、入力配列、文書情報と日時を持つ`saved-preview-<入力条件>-<描画所有者>.kra`を
`LIBREPAINT_GPU_RESULT_DIR`へ出力する。KRA全ファイルの比較では、画素の差と格納情報の差を分けて診断する。
線形RGBA8とsRGBのRGBA16は既存のプレビュー画素を保つことを検査する。
形式所有者の行入力は、既存の透明度付き補間・面積平均の固定値、全画素、読取り量と不完全な行の拒否を照合する。
実ICCはKRA画像情報の往復で保持を検査し、埋込みPNGは重複するプロファイル項目の省略を検査する。
文書タイトルの変更では、ICC・注釈と関連ファイルの移動、外側の項目と全レイヤー画素の保持を検査する。
パスに使えないタイトルやパス長の上限では格納名の保持、移動先の衝突では更新全体の拒否を照合する。
`TestDocument`は通常のGPU保存と改名後の書き出しを、既存のKRA読込みによる画像名・ICC・画素の復元まで検査する。
色履歴はCPU・GPU文書の一括設定、スナップショット、別文書への復元と元文書の終了後も最新順を保つことを検査する。
`testGpuDocumentCopyKeepsLayerImageMetadata`はルート・グループ・描画レイヤーの画像メタデータを、
スナップショット、文書複製と別文書への置換で照合する。置換のサイズ変更通知時点の値と、元文書の変更からの独立性も検査する。
`testGpuRasterExportKeepsLayerImageMetadata`は固定したTIFF Make値、RGBA8画素、日時と設定で、
通常操作・公開APIから書き出したPNG・JPEGをCPU参照と全バイト比較する。
`LIBREPAINT_GPU_RESULT_DIR`へ`layer-image-<操作・形式>-{cpu,gpu}.{png,jpg}`を出力する。
`testNativeLayerImageMetadataSurvivesEditing`はCPU保存に由来するルート・グループ・描画レイヤーのXMPを
通常の読込、変更・消去、保存と両エンジンでの再読込、PNG・JPEG書出しで照合する。
TIFF Make・Artist・Model、UUID、RGBA8画素、文書情報・時刻と設定を固定し、書出しをCPU参照と全バイト比較する。
`testNativeLayerRemovalDoesNotReassignMetadata`は削除のUndo／Redo、保存再読込後のレイヤー作成とメタデータ設定を照合する。
`testNativeMalformedLayerMetadataKeepsWarningAndOtherValues`は不正なXMPの警告と、他のレイヤーの値の保持を検査する。
`testGpuNativeAutoSavePinsMetadataAndRecoveryKeepsLatest`は通常の自動保存で開始時の値を固定し、
復旧要求では追加変更を含む最新の値を保存してから完了することを検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`native-layer-metadata-{cpu,gpu}-{source,updated,replacements}.kra`、
`native-layer-metadata-{cpu,gpu}-export.{png,jpg}`と`native-layer-metadata-gpu-autosave-{autosave,recovery}.kra`を出力する。
通常・GPUのKRA保存は固定したUUID、文書情報と日時で`color-history-native.kra`と`color-history-gpu.kra`を出力し、
再読込み後の色順序と、その色を文書の色空間へ変換して描いた画素を全成分で照合する。
同じ文書の色履歴と補助線の共通色を変更して再保存し、`-updated.kra`と`-cleared.kra`で置換と履歴消去を検査する。
形式試験は`document-color-settings-0.kra`から`-2.kra`で既存の色XMLの保持と消去を照合し、
重複する設定、不正なXMLと容量超過では元の文書値を保った拒否を検査する。
復旧保存の試験は開始時点と後続編集後の色設定を、対応するタイトルと画素とともに照合する。
容量超過、不正な圧縮入力、重複タイル、不正なパスやプロファイルでは完全な文書値の公開を保留する。
対応する旧読込み試験は、両処理のレイヤー画素を全成分で比較し、新しい出力を旧処理で開いて照合する。
形式処理の対象はQt Core・Gui・Xml、QuaZip、操作値、LZF圧縮・展開へ依存し、画面と旧CPU文書の構築を必要としない。
`TestKraRasterCodec`は、CPU保存に由来する固定タイル列と、固定した乱数列の非圧縮形式を
全バイトで照合する。符号化後の読込みは透明画素のRGBを含む全成分を保持する。
`unicodePathsIgnoreTheDefaultZipFilenameCodec`は、ZIP項目名の既定文字コードをLatin1へ固定し、
日本語の文書フォルダー、ICC、注釈と全画素の読込み、保存バイト列の保持を検査する。
固定した文書を`LIBREPAINT_GPU_RESULT_DIR`へ`utf8-internal-paths.kra`として出力し、
環境間でファイル全体を照合する。
失敗時のタイル列と入力画素は`LIBREPAINT_GPU_RESULT_DIR`または診断用の一時ディレクトリーへ保持する。
色ラベルの読込み・変更・再保存と、不正な番号表記による文書全体の拒否を形式試験で検査する。
`TestNode`の属性変更試験は、公開APIの色ラベル変更が画素のUndo／Redoと通常のKRA保存再読込みを通じて保持されることを照合する。
文書所有者の試験はMetal／Vulkanで生成した固定画素をKRAへ保存し、再読込みしたレイヤー画素と
保存用合成画像を照合する。後続編集と履歴、元ファイル、保存済み版、ファイル識別、文書終了時の
要求解除、同期通知中の終了と破棄を検査する。ファイル処理とGPU実行の両方を扱う対象は、
両者の小さなオブジェクト対象を直接構築し、通常画面の構築を必要としない。
復旧試験は未確定入力とRedo側の履歴、GPU画素、ファイル識別と付属ファイルを照合する。
壊れた記録、不正な長さ、検査用のハッシュを付け直した不正な付属情報も、現在の文書を保って拒否する。
文書枠を使い切った状態での復旧と、交換前に記録したGPU送信の無効化を共有実行試験で検査する。
UUIDが省略された既存KRAは、読込み時に付けた識別子を再保存と再読込みで保持する。

### 編集・保存結果の環境間比較

`KisGpuPaintDocumentTest::fixedEditsSaveReopenAndExportIdenticalResults`は、
64×64のRGBA8文書、3枚の固定UUIDレイヤー、同梱sRGB ICC、300×150 dpi、
入力列、筆圧、円形ブラシ、乱数種、文書情報を固定する。
不透明度、非表示レイヤーとキャンバス外の画素を含め、保存・再読込み・追加描画・
Undo／Redo・KRA書出しを2回実行する。連続編集と再読込み後の追加編集を照合し、
同一環境の反復では出力全体のバイト一致を検査する。

`KisGpuPaintDocumentTest::colorFillSavesReopensAndContinuesIdentically`は、
32×16のRGBA8文書、2枚の固定UUIDレイヤー、同梱sRGB ICC、300×150 dpi、
文書日時と塗りつぶし設定を固定する。不透明度、アルファ保護、消去、チャンネル保護と
乗算・スクリーン・比較（暗）・比較（明）・加算・線形覆い焼きの各条件で、
対象レイヤーとアクティブレイヤーの区別、キャンバス外の画素、履歴、
再読込後の保護設定と追加の塗りつぶしを検査する。
再読込直後のKRA書出し、連続編集と再読込後の追加編集、2回の反復について、
ファイル全体をバイト単位で照合する。結果は比較用ディレクトリーの
`color-fill-<条件>/saved.kra`と`edited.kra`へ出力する。
失敗時は実際のKRAと反復の期待値を一時ディレクトリーへ保持し、その場所を試験出力へ記録する。

`LIBREPAINT_GPU_RESULT_DIR`を空の比較用ディレクトリーへ指定すると、
試験は保存時・再読込み時・追加編集後のレイヤー画素、合成画素、レイヤー属性と選択、
保存前後のKRAを出力する。画素はRGBA順で、レイヤーは原点(-4, 0)から68×64、
合成画素は原点(0, 0)から64×64の密な配列。各環境で同じコミットを構築し、次を実行する。

```sh
./scripts/run-shared-test-env env LIBREPAINT_GPU_RESULT_DIR=/tmp/gpu-paint-results \
  ./scripts/run-test KisGpuPaintDocumentTest
```

Windowsは対応する実行ファイルへ同じ環境変数を渡す。Androidは試験APKで
同じケースの反復照合を行う。ICCは実行ファイルへ埋め込まれている。
収集したディレクトリー同士は`diff -r`で比較する。
`TestDocument`にも同じ環境変数を渡すと、公開APIと通常の書出しによるPNGを8条件で出力する。
固定した32×16のRGBA画素、300×150 dpi、ICCについて、アルファの保持と白背景への合成、
インターレースの有無を比較する。PNGのファイル全体と復号した画素を照合する。
比較条件にはOS、CPU、GPU、描画基盤、コミットと依存ライブラリーの版を記録する。
このケースは平坦なRGBA8／sRGBの円形ブラシとKRAを検証する。ほかのブラシ、精度、
階層、マスク、効果、画像書出し形式は各機能の比較ケースで受け入れる。

`KisGpuPaintDocumentTest::groupedFilesRetainSubtreeEditsPinnedSavingAndProjection`は、
固定UUIDの描画レイヤーと通常合成グループを使い、独立合成・通過の単一グループと入れ子を検査する。
子の画素編集、親の不透明度、キャンバス外の透明画素のRGB、固定版保存、子の選択と
保存・再読込み後の追加編集を扱う。比較用ディレクトリーへ`hierarchy-<条件>.kra`と、
64×48の密なRGBA配列`hierarchy-<条件>.rgba`を出力する。
`TestDocument::testGpuHierarchyKeepsPublicEditingHistoryAndNativeFiles`は、同じ環境変数で
`public-hierarchy-<条件>.kra`と`.png`を出力する。文書日時とPNG設定を固定し、
公開ノードの型と親子関係、子の選択、編集、履歴、削除と復元、既存のKRA読込みとの一致を検査する。
`KisGpuTileRendererTest::passThroughHierarchyMatchesNativeRootPixels`は、固定CPU参照とGPUの
明示タイル・既定色を、不透明度、入れ子と成分指定ごとに照合する。
公開ノードの通過指定、単体投影、サムネイルとパネルのUndo／Redoは、
`TestNode::testPassThroughGroupsKeepPublicReadsAndPropertyHistory`で検査する。
属性ダイアログの通過指定は、確定・取消し、固定日時のKRA・PNGと再読込みで照合する。
乗算・スクリーン・比較（暗）・比較（明）への変更は公開API、パネルのコマンドと直接属性書込みで、画素、元レイヤーと履歴の境界を照合する。
属性ダイアログの複数選択は確定・取消し、KRA・PNGと既存読込・GPU読込を照合する。
階層保存は通常・乗算・スクリーン・比較（暗）・比較（明）、独立合成と通過、入れ子で、固定版の保存と再読込み後の追加編集を扱う。
通常のブラシ設定による入れ子の子レイヤーへの描画は、`TestNode`の
`testScriptedFigurePaintsRequestedLayerAndPreservesHistory`の`nested`条件で検査する。

### 共有画像の編集と寿命

```sh
./scripts/run-shared-test-env ./scripts/run-test KisGpuPaintImageTest
```

画像所有者の試験は、共有参照からの編集、通知前のGPU要求更新、保存済み版、
Undo／Redo、同じ接続を使う復旧と、最後の参照による接続解放を検査する。
外部の矩形書込みは、同じGPUタイル内の別領域、Undo後の直接書込み、復旧後の再書込み、
タイル境界をまたぐ入力と非表示・キャンバス外の画素を検査する。
直接書込みの通知・履歴の維持と、履歴対象の書込みが一件の編集になることを個別に確認する。
不正な画素共有の復旧記録は、外側のハッシュが正しくても現在の画像と履歴を維持して拒否する。
文書試験は画像を直接編集し、ファイル所有者の破棄後も保持した画像の編集を継続する。
復旧の成功通知は新しい画像状態とファイル情報を同時に観測し、
外側のハッシュが正しい不正な操作記録は、画像とファイル情報を維持して拒否する。

### 通常文書でのGPU作成と読込

移行中の製品起動環境へ`LIBREPAINT_GPU_ENGINE=1`を指定すると、通常の「新規作成」とKRAの
「開く」がGPU文書を作る。同じ文書タブ、ツール、履歴と保存操作を使う。
新規作成は既定の8ビットRGB色空間と、ラスターまたはキャンバスの背景を受け入れる。
ブラシはQuick Brushを選び、通常合成または消去を使う。
KRA取込みは対応するRGBA8・sRGBの描画レイヤーとグループ、合成方式と属性を検査する。
既存の静止画KRAに含まれる空のパレット・資源一覧を受け入れ、本文の時刻設定を読んだ後に
別ファイルで指定した項目を適用する。フレームレート、範囲、現在時刻、連番書出しの設定を
GPU文書の初期状態と保存へ保持し、書出し名の補完は従来の読込規則に従う。
グリッド、ガイド、キーフレームを持つアニメーション、使用する資源、レイヤー効果など未移行の編集情報は、
意味を保持して編集する実装が必要なため理由を表示して受入れを止める。
既定構成の切替えは各機能と対象OSの受入れに従う。
この起動設定と対応する既存エンジンへの分岐は、[#89](https://github.com/serika12345/librepaint/issues/89)の
標準化で撤去する。

```sh
./scripts/run-shared-test-env ./scripts/run-test TestDocument
./scripts/run-shared-test-env ./scripts/run-test TestNode
```

`TestDocument`は通常作成・再読込の画像所有者、背景、レイヤー、解像度、文書情報と形式情報を検査する。
静止画KRAは既定・本文のみ・別ファイル優先・部分指定・書出し名省略を固定し、CPU読込で維持する値と照合する。
不正な時刻設定を拒否した場合は、現在の画像、履歴、ファイル情報と画像変更通知の件数を維持する。
文書の複製は時刻設定を独立して保持する。
`TestNodeDocumentSession.cpp`は新規作成ダイアログと既存の静止画KRAからの通常読込、
Quick Brush編集、Undo／Redo、KRA保存、通常再読込と追加編集、作成拒否後のダイアログ保持を検査する。
画面を表示する環境の検証と、`QT_QPA_PLATFORM=offscreen`で文書・UI接続を検査する実行を区別する。

`KisGpuPixelReadbackTest::synchronousPreviewsCompleteEarlierAsynchronousReads`は、別文書の非同期画素読取り・
透明性判定の直後に、サムネイル・投影画像・描画範囲を同期取得する。先行要求の固定版、
同期取得の結果、新しい編集要求と公開状態の保持を照合する。先行転送が未完了の場合は、
追加の非同期要求が待機せずBusyを返すことも検査する。

### GPUタイルの画素契約

MetalまたはVulkanで操作から実際のRGBA8画素を生成し、次の試験で検証する。
WGSLシェーダーはQt資源から読み、固定したwgpu-nativeでコンパイルする。
Nixの開発環境はCMakeの探索先にネイティブGPU依存を追加し、製品構築は同じ依存の
ヘッダーとライブラリを明示する。Linuxの実行時探索パスにはVulkanローダーを含める。
`nix/gpu/wgpu-native.nix`のパッチは、全OSで同じエラー分類と通知を供給する。
ホスト向けの依存物構築はパッチ内のRust試験を実行し、内部に包まれたGPU喪失・メモリ不足と
内部エラーが正しい通知先へ操作名・原因を渡すことを確認する。
`KisGpuExecutionTest`は実際のC APIで不正な待機番号、未写像バッファーの読取り、
同じ命令列の再送信を拒否させ、通知、後続命令の拒否と所有者の解放を検査する。
GPU喪失を伴う実機検証は、この再現可能な検査エラーの試験とは別に記録する。
`KisGpuPaintImageTest::sequentialConnectionsReleaseDevicesAndKeepGpuDescriptorsBounded`は、
最後の接続参照を反復して解放し、各デバイスの弱参照が失効することを検査する。
Linuxでは`/proc/self/fd`のGPU文字デバイスを各回新たに列挙し、初回の解放後を基準に
接続を反復した後の数が基準値以内に収まることを検査する。Vulkan接続自身の資源数を測る検証は、対象を構築した後、
Qtの非表示基盤で実行ファイルを直接起動する。

```sh
./scripts/run-shared-test-env env QT_QPA_PLATFORM=offscreen \
  build/tdd-linux/bin/KisGpuPaintImageTest \
  sequentialConnectionsReleaseDevicesAndKeepGpuDescriptorsBounded
```

個別デバイスのエラー、未完了読取り、終了・交換後の返却値と所有者の解放は、
既存の`KisGpuExecutionTest`と`KisGpuPixelReadbackTest`で検査する。
共通GPU定義とパッチの変更は、Windows・Android・iOSの増分環境を更新する入力として扱う。
ネイティブの開発環境を更新した後は、対象のプリセットで
`cmake --preset <preset> -U 'WGPUNative_*'`を実行し、GPU依存物の探索結果を更新する。

```sh
./scripts/run-shared-test-env ./scripts/run-test KisGpuTileRendererTest
./scripts/run-shared-test-env ./scripts/run-test KisGpuExecutionTest
./scripts/run-shared-test-env ./scripts/run-test KisGpuPixelReadbackTest
./scripts/run-shared-test-env ./scripts/run-test KisGpuTilePresentationTest
./scripts/run-shared-test-env ./scripts/run-test KisGpuCanvasWindowTest
./scripts/run-shared-test-env ./scripts/run-test KisGpuDocumentViewTest
./scripts/run-shared-test-env ./scripts/run-test TestChannelDocker
```

試験は円形ブラシ、筆圧、消去、入力分割、直線・楕円の変更、取消し、Undo／Redo、
復旧後の画素、疎な斜線、共有タイル、容量超過、GPU版の所有者と寿命を検査する。
`TestNode`は公開APIの未挿入レイヤー複製を既存画像とGPU画像で検査する。
画素・属性、独立した編集、元画像解放後の利用、削除済みレイヤーからの複製を照合する。
作成・複製したレイヤーの公開APIからの挿入は、位置、UUID、投影、変更済み状態、
一回の履歴追加とUndo／Redo後の同じノード・画素デバイスの保持を検査する。
CPU入力の65タイルの分割取込みと、容量超過・不正な属性の拒否後の画素と履歴を照合する。
この対象はKRA読込・保存とPNG読込・書出しのプラグインを宣言済み依存として構築する。
位置を持つKRAレイヤーの読込は、公開`Node.position()`、画像座標の画素、複製、
CPU・GPU複製からの挿入、削除後の参照、履歴、復旧、公開保存と固定版の再読込を照合する。
固定したUUIDと負座標・タイル境界を含む4位置を使い、
`LIBREPAINT_GPU_RESULT_DIR`へ`position-<x>-<y>.bgra`、`.state`と`.kra`を出力する。
`.state`はQt 5.15形式のビッグエンディアンで位置2成分、境界矩形とUUIDを保持する。
環境間で画素と状態の全バイトを比較し、KRAファイル全体の一致は固定した文書情報を含めて判定する。
復旧形式23の固定入力は`KisGpuLegacyRecoveryFixture_p.h`が生成元とハッシュを記録し、
旧記録の画素・属性・Undo／Redoと、原点のレイヤー位置への復旧を検査する。
同じ固定入力集の形式24は位置、未確定入力とRedoを持つ旧所有者の出力を保持する。
整数移動は`TestNode`の公開`Node.move()`から絶対位置、画素、履歴を増やさない変更、
Redo、複製と削除後の参照を既存画像とGPU画像で照合する。
`KisMoveToolDocumentTest`は登録済みの移動ツールからドラッグ、方向キーと座標入力を行い、
プレビュー、取消し、一回の履歴、操作中のUndo／Redo、KRA再読込、PNGの全画素と直接画素書込みを照合する。
`LIBREPAINT_GPU_RESULT_DIR`へ入力方法と描画所有者ごとの`.bgra`、`.state`、`.kra`、`.png`を出力する。
位置、UUIDと境界はQt 5.15形式で固定し、画素・状態・PNGを全バイトで比較する。
作成日時を`2000-01-02T03:04:05Z`、保存設定の`documentSaveTime`を有効な`QDateTime`の
`2001-02-03T04:05:06Z`へ固定する。保存開始後の呼出し側の設定変更から要求を分離し、
不正な日時で元ファイル・画素・履歴・保存先・変更状態が維持されることを検査する。
KRAの全ZIP項目は時刻1980年1月1日、作成OSの識別値3を要求する。
既存画像のKRA読込みとの互換性を確認し、GPU条件ではGPUファイル所有者からも読み直す。
再読込した文書を登録済み移動ツールで右下へ1画素移動し、保存日時を1時間進めて再保存する。
再保存した文書の位置・レイヤー画素・日時とPNGの全画素を検査し、`.edited.kra`と`.edited.png`へ出力する。
各環境の`.kra`と`.png`を含む全出力は、項目の除外や日時の置換をせずバイト単位で比較する。
失敗時は一時出力を保持し、試験結果に場所を記録する。
GPU変更通知中の画面終了は、ツールの破棄と進行中操作の終了を検査する。
同じツールを保持した通知中の取消しは、座標表示と次の方向キー操作を検査する。
macOSのこの対象は非表示のQt基盤、Linuxは専用のX表示を利用する。
`TestNode::testGpuThumbnailTransfersOnlyRequestedPixels`は、公開`Node.thumbnail()`と粗い範囲の
画素デバイスAPIを、4096バイトの転送容量で取得する。負座標、透明画素のRGB、既存画像と
同じ縮小結果、履歴と要求版の保持を検査する。
`KisGpuDocumentViewTest::layerPanelThumbnailsReadOnlyReducedGpuPixels`は、同じ転送容量で実際の
レイヤーパネルのモデルから画像を取得し、描画、Undoと履歴ドッカーの固定画像を照合する。
`LIBREPAINT_GPU_RESULT_DIR`へ`public-thumbnail-{precise,coarse}`、`layer-thumbnail-{before,after}`と
`history-thumbnail-fixed`の`.rgba`と`.png`を出力し、環境間でファイル全体を比較する。
`KisGpuPixelReadbackTest`の縮小画像契約は、64バイト単位の分割転送、固定版と後続の画素変更、
隠しレイヤー、投影の切出し、半透明の既定背景と保管境界の余白、読取り拒否と取消し、
表示版による容量不足と解放後の再試行、デバイス終了後の返却画像を検査する。
`TestDocument::testGpuDocumentPreviewsTransferOnlyRequestedPixels`は、公開`Document.thumbnail()`と
通常の文書プレビューを4096バイトの転送容量で取得し、既存画像の結果と完全比較する。
Bicubicの縮小・拡大、整数倍のピクセルアート、1/256の境界、Qt面積補間、細い文書、
9000×9000の疎な内容と透明背景、半透明背景、線形sRGBへの縮小後の変換を検査する。
33×17の半透明背景を16×16へ縮小する公開画像は`(17,34,51,128)`を保持し、乗算済みの`(27,67,14,90)`は
整数の四捨五入で`(77,190,40,90)`へ戻す。画面用`QPixmap`の形式は表示側で照合する。
格子の色は`(80,96,112)`と`(160,176,192)`、変換先は同梱`sRGB-elle-V2-g10.icc`、CPU処理は1スレッドへ固定する。
`LIBREPAINT_GPU_RESULT_DIR`へ`document-preview-*`と`*-reference`の`.rgba`、`.png`、`.state`を
出力する。状態列は寸法、画像形式、画素密度、入出力ICC識別子とCPUスレッド数をQt 5.15形式で記録する。Qt更新時は、同じ契約で
CPU参照とGPU結果の補間、切出し、画像形式を再比較する。
GPU所有者の投影縮小試験は、16バイトの分割転送、後続編集前の固定版、実行中の拒否と取消し、
閉じた文書の拒否、所有者終了後の返却画像を検査する。複数の表示版で容量を使い切る場合は、
部分画像を返す前の拒否と、表示版を解放した後の同じ要求の成功を照合する。
操作を持たない9000×9000の単色投影は、4バイトの転送上限で16×16の結果を取得する。
`kis_image_test`は、合成成分とアルファ継承を持つ入力の挿入・複製とUndo／Redoを検査する。
通常以外の合成方式、アニメーション・効果と一時描画状態の
挿入拒否が、元の画素・属性と文書履歴を保持することを検査する。ラップ表示中のCPU入力も、
保管画素の位置と透明領域を保って取り込むことを検査する。
公開APIの削除は、作成・clone後の挿入、本人・親からの削除、保持画素の編集とUndo／Redoを
既存画像とGPU画像で照合する。保存済み位置、履歴数、投影と同じノードの復旧を検査し、
進行中のGPU操作と削除済みレイヤーからの要求は画像と履歴を保持して拒否する。
通常画面の最後のレイヤーを公開APIで削除する試験は、選択対象の解除とUndoによる同じレイヤーの再選択を照合する。
公開APIの機能契約はGUI資源を組み込み、直列で実行する。既定のCTest登録は
macOSのCocoa、LinuxのXCB、Windowsの通常画面を利用する。
画面を表示しない実行の結果は、入力・実表示の製品受入れと分けて記録する。
公開APIの線・矩形・楕円・多角形は、選択中とは別のレイヤーへ描画し、前景色・背景色、
線の両端の筆圧、一回の履歴とUndo／Redoを既存画像とGPU画像で検査する。
線の描線指定`None`は前景色で描画する公開契約を両画像で照合する。
同じ契約は、公開APIで複製・挿入した参照からも検査し、対象画像と履歴への接続を照合する。
二頂点の多角形は両端を結ぶ輪郭を描き、色、対象レイヤーと一回の履歴を両画像で検査する。
呼出し後の色・径の変更からの独立性、対象レイヤーの画素と文書投影も照合する。
キャンバス外へ延びる線・矩形と離れた鏡像は、負座標の画素、矩形の正確な境界、
Undo／Redo、KRA再読込後の全領域と、キャンバス内を出力するPNGを公開APIから検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ各条件のKRA・PNGと、再読込に一致したBGRA8列を出力する。
macOSの非表示検証は`QT_QPA_PLATFORM=offscreen`で実行し、ネイティブ表示面の拒否後も
公開APIの描画・保存・書出しを継続できることを照合する。
矩形・楕円・多角形の単色塗りは、前景色・背景色と輪郭の有無、複製後の参照で検査する。
塗り内部、外側の輪郭と完全に被覆された重なりを照合し、色見本の透明度から独立した描画結果を検査する。
公開APIのパスは、直線・三次曲線・複数の部分パスと、偶奇・巻き数による穴の塗り分けを照合する。
`TestNode`の図形グループ契約は、通常のGPU文書に複数の直線・経路を描き、部分パス間の透明領域、
端点の入力値、一回の履歴、Undo／Redo、復旧とKRA再読込を照合する。
入力数が上限ちょうどの確定と、グループ全体の上限超過による画素・履歴・Redoの保持も検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`figure-groups/<condition>/`の描画・復旧・再読込したRGBA8列とKRAを出力する。
通常の消去アクションは、選択中とは別のアクティブレイヤー、単一・複数選択、
ロックされたレイヤー、キャンバス内外の画素、一回の履歴、Undo／Redo、KRA保存とPNG書出しを
`KisGpuDocumentViewTest::clearActionPreservesSelectedLayersAndHistory`で検査する。
`KisGpuPaintImageTest`は消去前の固定版、既定画素、容量内での消去、通知前のGPU要求、
通知中の最終参照解放と、進行中の操作・不正な対象による拒否後の状態を検査する。
消去後の直接画素書込みと復旧は、履歴の種別と通常画面のUndo表示を照合する。
`LIBREPAINT_GPU_RESULT_DIR`を指定すると、消去後のKRA・PNGと、再読込したレイヤー画素・
PNG画素のRGBA8列を出力する。同じ条件の画素列を環境間で完全一致により比較する。
前景色・背景色の通常塗りつぶしと描画不透明度を使う塗りつぶしは、
`KisGpuDocumentViewTest::colorFillActionsPreservePixelsHistoryAndSaving`で実際のアクションから検査する。
アクティブな対象と選択対象の区別、固定した色・不透明度、透明画素のRGB、
アルファ・チャンネル保護、消去、キャンバス外の画素、一回の履歴とUndo／Redoを照合する。
KRA再読込後の全レイヤー画素とPNG書出しを検査し、`LIBREPAINT_GPU_RESULT_DIR`へ
各条件のKRA・PNG・画素列を出力する。現在のパターン資源を使う通常・不透明度・消去の
塗りつぶし、乗算方式のパターン、各対応合成方式の写像、未対応合成の拒否、
通知中の画面終了も検査する。
`KisGpuDocumentViewTest::brushBlendModeUsesEffectiveCompositeOpAndRejectsUnsupported`は、
ブラシ取得時の実効合成方式の写像、未対応方式の診断と描画されないこと、方式を持つ実際の
ブラシ入力が凍結操作と画素へ到達すること、KRAへ結果画素が保存されることを検査する。
消去と塗りつぶしの利用側契約は、`libs-ui-pixels-KisGpuDocumentViewTest`でまとめて実行する。
画像所有者の塗りつぶし試験は固定版、復旧後の履歴種別、容量不足・ロック・進行中操作に
よる拒否と取消し後のRedo、通知前のGPU要求更新と通知中の最終参照解放を照合する。
復旧形式23は塗りつぶしの履歴種別を保持し、形式9から22の入力も引き続き検査する。
復旧形式48は塗りつぶしの合成方式を操作へ保存し、形式47以前は通常方式として読み込む。
復旧形式49はブラシ被覆の合成方式を操作へ保存し、形式48以前は通常方式として読み込む。
復旧形式50は画素フィルター操作とその凍結選択を保存し、形式49以前はフィルターなしとして読み込む。
復旧形式51はフィルター種別（Invert ColorsとGrayscale）を保存し、形式50の記録はInvert Colorsだけを
受け付ける。復旧形式52はHSV Adjustmentの生値`h`・`s`・`v`・`type`・`compatibilityMode`を
同じフィルター操作へ保存し、`type` 0〜2だけを受け付ける。復旧形式53はこれにColorizeフラグと
実効luma係数のbinary32ビット列を加え、形式52以前の記録はColorizeなし・lumaなしとして読む。
形式51以前はHSV Adjustmentを受け付けない。
復旧形式47は凍結パターンの寸法と直線RGBA8画素を操作へ保存し、形式17から46の
図形塗りつぶし入力も引き続き読み込む。
描線・塗りなしの矩形と塗れる面積のない頂点列は、画素を保った一回の履歴とUndo／Redoを照合する。
進行中の別操作と不正な筆圧は、画素・履歴・選択と先行する未確定操作を維持して拒否する。
`KisGpuPaintImageTest`は一枚の保持版の複製、復旧、他画像の参照の拒否と実行所有者の寿命を検査する。
表示と読取りは、文書全体より小さな容量で指定したレイヤーと領域を生成し、全体生成と
タイルの全画素が一致することを検査する。表示移動と編集時の重複領域の共有、
キャンバス外の部分読取り、生成領域と可視レイヤーが不足する合成の拒否も検査する。
矩形読取りは、画素数による出力上限、1タイルずつの転送、負座標、既定画素、
非表示レイヤー、投影のキャンバス外と、GPU所有者の終了後に保持した画素列を検査する。
250操作の復旧は、タイル位置ごとに2枚の書込み領域へ制限して元の固定画素と比較する。
250操作の背景、直前の表示、新しい表示をタイル3枚分の容量で保持し、入力追加、
直線・楕円の変更、取消しと復旧後の調整が、全操作の再生成と同じ画素になることを検査する。
2枚のタイルを使う筆跡の片側だけを伸ばし、旧表示と新表示をタイル3枚分の容量で保持する。
形状変更による旧位置の消去と、その確定後のUndo／Redoも同じ画素契約で検査する。
自由描画の入力追加は、半透明の下地、消去、アルファ固定、チャンネル保護、鏡像、
透明画素のRGBと大きなブラシで、逐次描画と全入力からの再生成を完全一致で照合する。
保持した最初と直前の画素版、取消しによる下地の復元と復旧記録からの再生成も検査する。
比較対象はキャンバス外を含む全レイヤータイルとし、大きなブラシの4×3タイルと
同時に保持する版・下地に必要な容量を割り当てる。少容量での拒否は独立した容量契約で検査する。
キャンバス外と鏡像への直接画素書込みは、固定版、別領域、履歴の分離と復旧を照合する。
3タイルの予算で下地・入力中の筆跡・合成結果を同時に保持し、派生する被覆の保持が
合成と取消しに必要な容量を妨げないことを確認する。
読込画素への描画、タイル置換、Undo／Redo、復旧を実行し、透明画素のRGB値と
キャンバス外の画素を保持することを検査する。交互に繰り返したタイル置換と描画も、
最後の置換から2枚の書込み領域で再生成する。
既定画素を持つレイヤーは、共有タイルからの可視領域生成、描画下地、透明画素のRGB値、
合成順、表示切替、履歴、復旧と位置数の上限を検査する。
描画器の終了後も保持した合成版を読み、最後の画素版を解放するとネイティブGPUも解放される
ことを検査する。表示資源の保持と解放、GUIスレッド以外からのデバイス作成の拒否も検査する。
可視範囲の合成、レイヤー順、半透明色、共有結果、合成時の容量不足と、描画・合成・読取りの
同一命令列での実行も検査する。
表示試験は同じ命令列の計算結果を描画先へ出力し、パン・ズーム・回転、タイルごとの変換、
文書領域と可視範囲の切出し、半透明色と背景色の合成を検査する。
表示上限、不正な変換、別GPUの版を拒否した後も描画先の画素を維持することを検査する。
共有実行試験は、240回の要求集約後の全入力と末尾画素、送信前の公開保留、
取消し・文書終了による旧要求の無効化、文書間の共有容量と再試行、デバイス交換後の再生成を検査する。
実ウィンドウへの送信にはmacOSのCocoa、LinuxのX11画面プラグインを試験ごとに指定する。
未作成の表示面と別GPUの表示面はネイティブ送信前に拒否する。
`libs-canvas-gpu-offscreen-KisGpuCanvasWindowTest`は画面外の表示面作成を拒否した後も、
同じGPUで画素読取りの命令列を開始・取消しできることを検査する。
固定版の読取り試験は、後続編集と公開版の保持、非表示レイヤーと表示用合成の区別、
領域ごとの疎な画素取得、転送容量の上限と原子的な拒否を検査する。
境界取得はGPU保持1タイル・転送容量16バイトで複数タイルの負座標を含む最小・最大画素を取得し、固定版、
後続要求、非表示レイヤー、空の投影、実行中フレームと容量不足を照合する。
公開`Node.bounds()`は透明画素の色と不透明な既定色を含む入力で既存の画素デバイスと
同じ矩形を返すことを、全画素を転送できない容量で検査する。
保存用の領域指定はキャンバス外のレイヤー画素と既定画素を保持し、合成を文書内に切り出す
ことを検査する。
完全読取りは、GPU保持4タイル・一回の転送1タイルの上限で6タイルを持つ文書を保存し、
全レイヤー、非表示とキャンバス外の画素、合成画像、最新要求、公開版と履歴を照合する。
幅1画素の合成画像は実画像のバイト数をCPU容量とし、タイルの余白を蓄積せずに取得する。
不足するCPU容量での原子的な拒否と、GPU終了後の画像の寿命も検査する。
進行中のフレームと終了した文書の読取りを拒否し、デバイス交換後の同一画素と
GPUの終了後も利用できるCPU画素の寿命を検査する。
利用側が読取り結果を手放した後も、ネイティブの完了通知とデバイス終了時の後処理まで
格納先を保持することを、実ウィンドウへ送信した読取りで検査する。
表示ウィンドウ試験は、連続更新の集約、パン・ズームとサイズ変更後の可視領域、
文書終了後の遅延更新、表示中と非表示のウィンドウのデバイス交換を検査する。
容量不足時は操作と公開版を維持し、識別子を伴う失敗通知と次の要求での再試行を検査する。
進行中のフレームは表示面の再作成を拒否し、ネイティブウィンドウの破棄までデバイスを保持する。
固定した入力とブラシ設定の照合値は試験ソースが所有し、MetalとVulkanで同じ値を要求する。

`KisGpuColorSamplingTest`は、登録済み採色ツールと共通の採色ヘルパーから、半径1〜900で
現在レイヤー・文書投影を前景色・背景色へ採取する。混色0〜100%の整数丸め、最初の操作、
要求時の現在色、同じプロファイルを持つ16bitの現在色からの変換、色資源を更新しない採色、
実タイルのRGB成分、円形範囲の平均と混色の組合せを照合する。画素・履歴・変更状態の保持と、透明・半透明の採取色、
混色後のアルファ、不透明な画面色、パレット用の最終通知も検査する。
既存文書では混色100%の現在レイヤー・文書投影・半透明色を照合する。
要求後の編集、文書交換、同じ画像への復旧、通知中の画像終了・ヘルパー破棄・再操作、
採色後から確定までの復旧を使い、固定版と古い通知の破棄を検査する。
この対象は画面外で実行し、ネイティブの表示面・機器入力の受入れは対象OSで別に検証する。
`KisGpuPixelReadbackTest`は、円の境界、負座標、投影のキャンバス端、暗黙画素、
アルファ加重と四捨五入、半径900の整数加算を32バイトの読戻し上限で照合する。
GPU描画済みの固定版、公開版の保持、画像終了・デバイス交換後の結果、容量不足と
不正要求の後に次の採色が成功することも検査する。

```sh
./scripts/run-shared-test-env ./scripts/run-test KisGpuColorSamplingTest
./scripts/run-shared-test-env ./scripts/run-test KisGpuPixelReadbackTest
```

通常画面の試験は、小さい資源バンドルと円形ブラシ設定を試験用の資源データベースへ登録する。
公開文書と通常ウィンドウを使い、GPU画像の提示、編集とUndoの反映、未対応パネルの理由表示、
タブ終了時の文書と表示面の解放、終了後の遅延更新を検査する。
通常画面の契約は、文書・入力の`libs-ui-KisGpuDocumentViewTest`、レイヤーパネルの
`libs-ui-layers-KisGpuDocumentViewTest`、画素操作の`libs-ui-pixels-KisGpuDocumentViewTest`、
背景設定の`libs-ui-background-<condition>-KisGpuDocumentViewTest`、切り抜き表示の
`libs-ui-crop-KisGpuDocumentViewTest`に分けて直列に実行する。
`run-test KisGpuDocumentViewTest`は全群を実行し、各群の既定の時間上限を適用する。
構成変更でネイティブキャンバスを作り直した場合、表示後のウィジェット寸法をビューポートへ再適用し、
ツールオプションの変更でキャンバスが既定寸法へ縮まないことを確認する。
同じ画面でUndo／Redoアクション、公開履歴の件数と位置、保存済み状態、
最初の未確定入力と既存履歴上の未確定入力の取消しを検査する。
履歴パネルの行数、選択位置と、パネルからの履歴位置の変更も照合する。
同じ入力プロファイルとブラシツールへ実際のポインター列を送り、途中表示、一筆一件の履歴、
取消し、消去、筆跡の開始後に変更した色・径・消去設定の固定を検査する。
入力上限超過では画素、履歴と変更済み状態を維持し、状態欄に理由を表示する。
エラー通知の受取り側が文書を閉じる場合も、文書と表示面を解放することを検査する。
通常の保存アクションと公開文書の保存・同期書出しは、KRA画素、解像度、文書情報、ICCと注釈を
照合する。保存済み位置と履歴、書出し後の変更状態、保存失敗時の元ファイルと保存先を検査する。
保存通知中の追加編集、文書終了、画像交換と、完了通知中の所有者破棄も検査する。
公開`Document::exportImage()`と通常のPNG書出しは、画素、半透明、白背景への合成、
ICC、解像度とインターレース設定を照合する。書出し後の履歴、保存済み位置、元のKRAと
保存先を検査する。実際の設定画面での取消し、追加編集、文書終了、画像交換とKRA併記保存は、
元ファイルの保護と出力する固定版を検査する。PNGの名前を付けた保存と再保存は、
公開API、保存先・形式の採用、保存済み位置、Undo／Redo、再読込と失敗後の状態を検査する。
KRAとPNGの保存済み通知中の編集、文書終了、画像交換と完了通知中の破棄も検査する。
これらの契約は`TestDocument`が所有する。
`TestDocument::testDefaultBrushGpuEditingKeepsPixelsHistoryAndFiles`は、同梱の`b) Basic-5 Size Opacity`設定で線分を重ね、筆圧、流量とキャンバス不透明度を固定してCPU・GPUの結果を照合する。
新規作成と固定した文書の読込みから、Undo／Redo、KRA保存・再読込み後の追記、KRA・PNG・JPEGの全バイトを比較する。
成果物は`LIBREPAINT_GPU_RESULT_DIR/default-brush/<条件>/{cpu,gpu}/`へ保持する。新規文書の比較対は一方が生成したレイヤー識別子を共有し、読込み条件は固定識別子を持つ。
`TestKraRasterCodec::defaultOnlyStorageBlocksSurviveSavingAndReopening`は、負座標にある既定画素だけの格納領域を保存・再読込みで保持する。

`TestDocument`の公開APIから生成・変更する
パターンと単色の塗りつぶしには、それぞれの生成プラグインへの直接の構築依存を持つ。
`TestNode`は公開APIによるグレースケールへの変更に使う色管理プラグインへ直接依存する。
`TestNode::testHistoryNavigationWaitsForImageJobs`は背景の画像処理を明示的に待機させ、
入力終了後のUndo／Redoと履歴位置の移動、入力受付中のツールへの要求を区別して検査する。
作業スレッドで文書を複製してGUIスレッドへ移した後も、画像処理完了後のUndoを検査する。
`testDeferredHistoryRequestsPreserveOrderAndLifetime`は、要求順序と一回の通知、画像交換、
文書終了と通知中の破棄、終了通知からの遅い履歴要求、保持したノードの画素を検査する。
この二つの契約は、Qtの画面外環境で実行できる。
`TestDocument::testCollapsedStateSurvivesDocumentCopies`はスナップショット、保存用複製と
公開文書の複製について、階層内の折りたたみ状態と元文書からの独立性を検査する。
`testNativeCollapsedLayersSurviveEditing`は、固定UUIDを持つ入れ子のグループと描画レイヤーを
CPUの通常保存で生成し、CPU・GPUの通常読込、編集、Undo／Redo、削除取消しと再保存を照合する。
折りたたみだけの変更は、画素、変更状態、履歴と変更通知を保持する。
文書情報、時刻、色資源と書出し設定を固定し、KRAの圧縮の有無についてファイル全体を比較する。
`LIBREPAINT_GPU_RESULT_DIR`の`native-collapsed-{raw,compressed}/`へ、入力`source.kra`、
CPU・GPUそれぞれの`.kra`、追加編集後の`-after.kra`、`.png`と`.jpg`を保存する。
`KisGpuPaintDocumentTest`は、形式処理が完成した
固定版の採用と、後続編集・保存先・保存済み位置の復旧を検査する。
`KisDocumentReplaceTest`は、定期自動保存のタイマー入口、公開回復要求と休止時保存を検査する。
固定した画素、UUID、文書情報、解像度とICCをKRAから読み直し、元ファイル、保存先、変更状態と
Undo／Redoを照合する。回復要求の開始時は書込みを予約し、書込み前の追加編集と付属情報の
変更から最初の固定版を独立させる。待合せ中の回復要求は最新の確定版が保存されてから一度完了する。
休止時は予約した保存と後続編集の保存を完了してから戻る。未確定入力の延期と取消し、
書込み失敗後の再試行、転送容量不足、連続失敗後の状態保持を検査する。
開始・成功・失敗・延期の通知中の文書破棄と、予約後・完了通知中の画像交換も検査する。
固定した三つの入口は`LIBREPAINT_GPU_RESULT_DIR`へ`autosave-{timer,recovery-request,application-pause}.kra`
を出力し、対象環境間でファイル全体を照合する。
GPU形式処理の行読取りは、明示した作業予算、256行の境界、最終行、固定版と最新編集の
独立性を検査する。提供側の破棄後は保持した行入力と待機中の背景読取りが取消しを返し、
既に返した画素が保持されることを検査する。16384×16384の文書は48MiBの作業予算で
最初と最後の行を読み、通常のPNG書出しの寸法と終端、文書履歴とGPU保持量を確認する。
幅が最大座標範囲の文書は512MiBの作業予算を超え、既存ファイルを保持して失敗することを検査する。
POSIXの保存先ディレクトリーで新規作成を拒否した場合も、書出し失敗と既存ファイルの保持を検査する。
先行するGPU処理でキューが使用中の場合はイベント処理を続け、解放後に固定版の読取りを再開する。
`KisGpuCanvasWindowTest`は、共有キューの使用中に要求した最新画像を、
キュー解放後の再表示で採用することを検査する。
表示成分の試験はRGBA8/sRGBの固定入力を使い、16通りの成分指定と単一成分の濃淡・色表示を、
CPUの色空間処理から得た値と照合する。表示先の全画素、元のGPUバッファーと復旧記録を検査する。
Linuxの表示窓試験はX11から表示面の画素を取得し、成分切替と表示面の再作成後の結果を照合する。
`TestChannelDocker`は、通常の一覧の有効化、各成分の縮小画像、再接続、
描画入力中の表示切替と設定通知、固定したKRA出力の保持を検査する。
Linuxでは一覧から表示窓までの結果もX11で取得する。
チャンネル合成の参照値には白の市松模様と余白を固定し、模様設定の検査と独立して照合する。
`LIBREPAINT_GPU_RESULT_DIR`を指定した場合、大小の文書の`display-channels-{gpu,gpu-large}.kra`を出力する。
`KisGpuPixelReadbackTest`は非同期読取り後の新しい要求、文書終了とGPU終了からの
CPU結果の独立性を検査する。同期読取りも同じ完了処理を使い、従来の画素契約を維持する。
GPU画素読取り試験は、Qtの画面外環境でもネイティブGPUを使用する。
`KisPngRowWritingContractTest`は、通常・インターレースの8/16 bit、グレースケールと
索引色の画素一致、行入力と描画面入力の色変換・背景合成のバイト一致、別スレッドからの符号化取消し、先頭・画素・終端での書込み失敗後の
再利用と画素所有者の解放を検査する。PNG試験は同じ実行ファイルの資源データベースを共有するため、
CTestの資源ロックで相互の実行を順序付ける。
ファイル名を指定する変換は、取消し・変換拒否・成功と保存先の有無を組み合わせ、元ファイルと
正常な符号化結果を検査する。POSIXでは一時ファイルを作成できないディレクトリー、Windowsでは
他の利用者が置換を許可していないファイルを用いて、置換失敗後の元ファイルの保持を検査する。
`TestDocument::testGpuSessionOpensJpegWithGpuPixelOwnership`は通常のOpenから、
画素、Exifの8種類の向き、レイヤー位置、JFIF・Exifの解像度、ICC、画像メタデータと初期状態を照合する。
CPUの作業スレッドは4に固定し、向き5・7には複数タイルの画像を使って反転と回転の順序を検査する。
編集、Undo／Redo、KRA保存・再読込・追加編集後の再保存、PNG・JPEG書出しをCPU文書と比較する。
文書情報・著者・時刻・圧縮設定を固定し、各形式のファイル全体を完全比較する。
終端マーカー欠落からの既存の回復も同じ契約で検査する。
`testJpegRejectedOpenKeepsDocument`は不正入力、未対応の色・精度・ICCと容量超過に対し、
編集中の画像・画素・履歴・保存先・変更状態と、後続のUndoを検査する。
回転とタイル境界の試験は`plugins/impex/jpeg/tests/data/sources/HPIM0760.JPG`を参照する。
独立した試験環境へ配置するときも、`QFINDTESTDATA`が解決するソース相対位置を保持する。
`LIBREPAINT_GPU_RESULT_DIR`へ`jpeg-import-<条件>/`を作成し、`cpu.kra`と`gpu.kra`、
追加編集後の`cpu-after.kra`と`gpu-after.kra`、それぞれの`.png`と`.jpg`を出力する。
`kis_jpeg_test`は短い読取り・先読み・入力バッファー境界でのインポートUUID、不正ICCの拒否、
復号前の余白込みタイル容量の拒否と、変換所有者が保持する元画像を検査する。
`kis_jpeg_test`はRGBA・Gray・CMYKの8/16 bitとLab16について、描画面入力と行入力の
JPEG全体を比較する。固定した色とアルファ、Lab16からRGB8を経由するsRGB変換の量子化境界、
行の読取順、読取り中の取消し、入力不足と書込み失敗を検査する。
`KisJpegCoefficientStoreTest`は、32KiBの係数バッファー予算で一時領域を使用し、標準libjpegの
メモリー上の係数処理とJPEG全体を比較する。RGB・Gray・CMYK、最適化、プログレッシブと
色差の間引きを検査する。短い読書きの継続、作成・読書き位置の変更・書込み・読込み・途中終端による
失敗と、後続の符号化の成功を照合する。
`TestDocument`のJPEG試験は33×17画素、300×150dpi、文書情報と著者を固定する。
公開APIと通常の書出し操作から、品質、色差の間引き、プログレッシブ、基準形式、最適化、
平滑化、sRGB、ICC、背景色、Exif・IPTC・XMPと匿名化の21条件を既存CPU文書の出力と完全比較する。
設定画面の取消し、追加編集、文書終了、画像交換とKRA併記保存を検査する。
JPEGでの保存は書き込んだ版を保存済みとして採用し、後続編集の通常保存とUndo／Redoによる
変更状態の切替を照合する。書出しの失敗は元のKRA、保存先と履歴を保持することを検査する。
16384×16384のGPU文書は、最適化とプログレッシブのJPEG書出しで、寸法、終端、
縮小した白背景、元の版・履歴・保存済み位置とGPU保持量を検査する。
係数の作業領域はJPEG所有者の割当量で照合し、実メモリーの計測では符号化と読込みを
独立して計上する。
`LIBREPAINT_GPU_RESULT_DIR`へ`<api|menu>-<設定>.jpg`、`dialog-<条件>.jpg`、
`dialog-resaved.jpg`と`<色・精度>-<legacy|rows>.jpg`を出力し、環境間でファイル全体を比較する。
係数の比較結果は`coefficient-<条件>-<expected|actual>.jpg`、大きな文書は
`large-<optimized|progressive>.jpg`へ出力する。

```sh
./scripts/run-shared-test-env ./scripts/run-test kis_jpeg_test
./scripts/run-shared-test-env ./scripts/run-test KisJpegCoefficientStoreTest
./scripts/run-shared-test-env ./scripts/run-test TestDocument
```

`KisImportExportAsyncFeedbackTest`は、背景スレッドの完了待ち中のGUIイベント処理、バッチ保存での進捗画面の抑制と、
既存の操作を遮る進捗表示および固定版用の操作を許可する進捗表示を検査する。
`kis_tiff_test`の`testFiles`は、実際の形式プラグインで8種類の向きを含むTIFFを読み込み、
画像処理の完了後に投影画素を固定画像と照合する。回転と反転の処理順もこの読込契約で維持する。
通常のKRA読込は、公開画像・レイヤーのICC、解像度、UUID、選択レイヤー、注釈と文書情報を
照合する。編集後の再保存で付属ファイルを維持し、不正なファイルの読込では元の画像と保存先を
維持することを検査する。読込通知・完了通知中の文書終了と、保持した画像による画素読取りも検査する。
`kis_kra_loader_test`は、単層・複層と空のレイヤー列の画素互換性を照合する。
空の文書も、通常の読込が返る時点で埋込ICCを公開することを検査する。
`kis_kra_saver_test::testSavedPreviewPreservesImagePixels`は、半透明背景のKRAを保存し、
格納された`preview.png`を画面形式から独立した文書画像と完全比較する。
拡大・整数倍の拡大・縮小・細い文書について、寸法、半透明の色と再読込後の画像を検査する。
作成・保存日時とレイヤーUUIDを固定し、`LIBREPAINT_GPU_RESULT_DIR`へ
`saved-preview-<条件>.kra`と同じ名前の`.png`を出力する。失敗時は比較画像とKRAを保持する。
`TestColorSpaceRegistry`は、QtのsRGBとDisplay P3から同名のICCを生成し、日付を固定する。
異なる内容と同じ宣言済み識別値の二条件で、明示したICCの完全な保持、LCMSによる直接変換と同じ画素、
再登録・古い登録の解除後の色空間と変換の保持を照合する。名前指定による既存プロファイルの再利用と
HDRの別名処理も同じ試験対象で維持する。
複製・ファイル指定による登録解除後も、保持済みの8 bit色空間と16 bitへの変換先を利用できることを照合する。
異なる色モデルへの色空間取得の失敗は、名前と識別値の対応を保持する。
PQプロファイルに線形プロファイルと同じ名前を与え、既存の名前参照と、元のPQプロファイルと
ビット単位で等しい浮動小数点の変換結果を検査する。
LCMSの4試験対象は、実際のレジストリーが読み込む色管理プラグインへ直接依存する。
`TestKoLcmsColorProfile`は、sRGB・Lab・Gray・XYZの同じ定義へ異なる日時とOS情報を与え、
内蔵生成のICCと識別値の完全一致、色を定めるタグ、ICC表現による16bit変換画素を照合する。
識別値はICCのチェックサム規則で検査する。外部から渡したICCは、日時と実データを保持し、
同名の内蔵定義を保ったまま色空間として利用できることを検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`builtin-<rgb|lab|gray|xyz>.icc`と
`imported-<rgb|lab|gray|xyz>.icc`を出力し、環境間でファイル全体を照合する。

```sh
./scripts/run-shared-test-env ./scripts/run-test TestKoLcmsColorProfile
```

`TestColorSpaceRegistry`はレジストリーの公開APIとLCMSを利用し、プラグイン実装の静的リンクを介さず検査する。
未対応の色モデル、不正・空のICC、異なる色モデルのICCと入力データを扱えない形式は、
登録済みプロファイルと色空間を保持したまま拒否することを検査する。
`kis_kra_loader_test`は、QtのsRGB ICCの作成者欄を固定した別データを同名で先に登録する。
固定UUIDの通常のKRAを公開`openPath()`から開き、公開の同期書出し後に再読込する。
両段階で画像・各レイヤーのICCと画素、UUID、既定画素、先に保持した色空間を照合する。
書出しは元の保存先と変更状態を保持する。試験対象はKRAの読込・書出しプラグインへ直接依存する。
複合KRAの読込試験は、反転フィルターを含む全レイヤーの復元に必要なプラグインへも直接依存する。
`LIBREPAINT_GPU_RESULT_DIR`を指定すると、`icc-opened-profile.icc`と`icc-resaved-profile.icc`、
同じ接頭辞の`layer-0.rgba`と`layer-1.rgba`へICCとRGBA画素を保存し、環境間で完全一致を照合できる。
空のKRAでも読込が返る時点のICCを照合し、旧HDRの別名による読込を同じ対象で検査する。
`run-test KisColorSpaceSelectionTest`は、基本・詳細の色選択部品へ同名の異なるICCを渡し、
公開した選択値と通知の実データ、登録解除後の寿命、名前による明示選択を照合する。
通常のキー入力でプロファイルと深度を選び、基本側の手動指定の維持と詳細側の既定値の再選択を検査する。
通知中の再選択・破棄、詳細選択の確定・取消しと親の破棄も対象とする。
この試験は画面外で実行し、色管理プラグインへ直接依存する。

`KisMetadataExportTest`は、IPTC・XMPの実際の形式プラグインへ同じ項目と値を渡す。
項目の追加順と、一時的な項目の追加・削除を変えても、単独データとJPEG用データの全体が
一致することを照合する。XMPの著者配列は読込後も元の順序を保持する。
試験対象はメタデータ所有者と2つの形式プラグインに依存する。
`LIBREPAINT_GPU_RESULT_DIR`を指定すると`<iptc|xmp>-<plain|jpeg>.bin`を出力する。
環境間ではファイル全体を比較し、失敗時は期待値と実データを保存する。

```sh
./scripts/run-shared-test-env ./scripts/run-test KisMetadataExportTest
```

通常キャンバスの入力試験は、KRA読込後にも描画、消去、取消し、Undo／Redoと保存を一巡する。
公開APIからの画像交換は、内容コピーと画像の識別子の交換を区別し、CPU→GPU、GPU→CPU、
CPU→CPUとGPU実行所有者の交換で、新しい寸法、表示、描画先、履歴とKRA保存を照合する。
旧画像を保持して、交換後の画素の独立性と旧通知の遮断を検査する。交換前に別スレッドから
送られたサイズ通知と、待機中のノード再選択も対象にする。描画中の交換は旧入力の取消しと
次の描画の成立、採用前後の通知中のタブ終了は、保持したGPU画像の独立した利用を確認する。
レイヤーパネルの単一・複数・全レイヤー削除は、一つの履歴、最後のレイヤーの空の置換、
Undo／Redo後の公開ノード参照・画素・アクティブレイヤー・複数選択をCPU画像とGPU画像で
照合する。操作所有者は不正な一括要求の拒否、画素と資源量の保持、履歴と復旧を検査する。
保持した削除済みノードの画素読取り・書込み、境界矩形とサムネイルも照合する。
書込み後のUndo／Redoと復旧、履歴の分岐、公開画像終了後の画素編集を検査し、
現在の画像・履歴・保存済み状態を維持することと、最後のノード解放によるGPU実行の終了を確認する。
レイヤーの上下移動は、単一・連続・離れた選択の順序、一件の履歴、公開ノードの参照と画素、
アクティブレイヤーと複数選択、Undo／RedoをCPU画像とGPU画像で照合する。
GPU画像ではKRAの保存順も照合する。
`TestNodeHierarchy.cpp`の上下移動とドラッグ移動の挿入先は、グループの兄弟間移動、展開・折りたたみに応じた出入り、
子と部分木の親への移動、異なる親と親子の同時選択、移動元と移動先のロック、階層端を扱う。
CPUとGPUの親子関係、兄弟順、操作対象、選択、元画素、同じノードのUndo／Redo、保存再読込と追加の画素編集を照合する。
GPUの変更がない操作は履歴を保ち、モデルは循環する親、不正な兄弟、描画中と終了後の拒否、固定版と復旧を検査する。
三通りの文書列挙順で固定UUID・文書情報・時刻のKRA・PNG・復旧記録を作り、
`LIBREPAINT_GPU_RESULT_DIR`へ`hierarchy-order-<条件>-<列挙順>`の名前で出力する。
通常のドラッグ移動は挿入処理が使う`moveNodesDirect()`から実行し、同じ階層・画素・保存契約を照合する。
親子の同時選択では整理後の根が操作対象になり、Undoは元の子、Redoは移動後の根を操作対象にする。
Undo／Redoは履歴位置の変更と画像処理の完了を待ち、GUIの再選択通知を処理してから状態を照合する。
操作モデルは操作対象の指定を親・兄弟順と一括して検査し、拒否した要求と同じ階層への移動で選択も保持する。
固定したGPUのKRA・PNG・復旧記録は`hierarchy-direct-move-<条件>-<列挙順>`へ出力する。
移動後と履歴移動後は、表示の操作対象・選択とGPU操作モデルの値も照合する。
`TestNodeView`は操作対象の変更が初期選択を作り、文書が選択を所有する設定では既存の複数選択を保つことを検査する。
一覧が選択を決める設定では、新しい操作対象の選択も照合する。
`timeline_model_test`は表示用ツリーの交換・破棄・再接続で、同じレイヤーの行と不透明度の曲線を保持し、
選択解除で両方を解放することを検査する。
`TestNodeHierarchy.cpp`は公開ノード試験の一部として、既存グループ内への描画レイヤー追加を
通常のレイヤー操作と公開APIで検査する。展開、子の選択、空、折りたたみ、ロック、ルート末尾と
複製したレイヤーの挿入について、CPU画像とGPU画像で親、兄弟順、操作対象と画素を照合する。
追加と削除の一件ずつの履歴、同じノードの復元、KRA再読込後の階層と画素、
全ルートの削除後に残る空レイヤーも検査する。
`testPublicSubtreeInsertionKeepsChildrenPixelsAndHistory`は、複製したグループを公開
`Node.addChildNode()`から追加する。独立合成・通過の両モードで、元画像と複製画素の独立性、
描画中の拒否、子の順序、Undo後の直接書込みと同じ部分木のRedoをCPU画像とGPU画像で照合する。
グループの新規作成は、通常操作、汎用ノード作成APIとグループ作成APIで、接続前の独立した透明な投影、
子の追加、二段階のUndo／Redo、保持したノードの同一性と画素、KRA再読込を照合する。
履歴から取り外したグループと子が操作対象と選択に残らないことも確認する。
ルートのグループ単独と描画レイヤーとの複数選択の複製は、グループ・描画レイヤーそれぞれを操作対象にして、
複製後の選択、子孫の画素と識別子、一件のUndo／Redo、続けて行う削除とUndoをCPU画像とGPU画像で照合する。
固定したUUID・作成日時・保存日時でKRAを保存し、通常の読込みで階層と画素を復元する。
PNGの全画素を公開画像の取得結果と照合し、GPUのKRA・PNG・復旧記録を
`LIBREPAINT_GPU_RESULT_DIR`へ`root-duplicate-<条件>`の名前で出力する。
グループ内の描画・グループ、異なる親からの選択、親子の同時選択、ロックされた親・祖先を含む複製は、
CPUの通常操作とGPUの通常操作で、複製先、兄弟順、操作対象と選択を照合する。
子を先に置く文書、親を先に置く文書、別の部分木が途中に並ぶ文書は同じ階層として扱う。
複製した子の直接画素書込みは元の子から独立し、保存再読込、Undo／Redo、削除とUndoを経ても保持する。
固定した入力のKRA・PNG・復旧記録は`hierarchy-duplicate-<条件>-<列挙順>`へ出力する。
操作モデルは部分木の選択整理、編集可能な複製先、画素ブロックの共有、一件の履歴と復旧を検査する。
同じ階層のドラッグ複製は、MIMEの複製入力から通常のノード追加処理へ接続して照合する。
親子の同時入力は独立した二つの複製を作り、一方の子への書込みは他方の画素を維持する。
GPUのUndoは元の複数選択と独立した操作対象を復元する。17枚の子を持つグループも、
階層全体で一つの接続を共有して複製し、折りたたみ状態とノードの追加属性を保持する。
準備した一括入力は不正な後続ノード、重複、容量不足、描画中と終了後の拒否で文書と入力を保持し、
採用前の公開ノードと画素デバイスの参照を保つ。成功は一件の履歴と一回の変更通知を発行する。
MIME読込みの拒否は、GPU文書の接続枠を埋めた条件で空の結果、元画素・履歴・通知の保持を検査する。接続枠を解放すると同じ入力を独立した複製として利用できることも確認する。
ドラッグ複製のKRA・PNG・復旧記録は`hierarchy-mime-copy-<条件>-<列挙順>`へ出力する。
KRAの再読込みと復旧は新しい識別子を保持し、PNGの全画素を公開投影と照合する。
MIME複製の識別子は実行ごとに生成される。通常の挿入先を使う条件に加え、既存のMIME挿入境界で
準備済みノードの識別子だけを固定する条件を持つ。固定入力も通常の挿入先へ渡し、
`hierarchy-mime-fixed-<条件>-<列挙順>`のKRA・PNG・復旧記録をバイト単位で比較する。
通常条件の識別子の保持と固定条件の環境間一致は、それぞれの検査結果として記録する。
クリップボードの階層コピーは`TestNodeClipboard.cpp`で、コピー後の元画素の変更と文書終了、
17枚の子、通過指定、親子を独立した根にする入力、繰返しの貼付と一件のUndo／Redoを検査する。
画像・KRAの取得は表示プロファイルをsRGBへ固定し、通常位置、負の座標と元のキャンバス外の画素を照合する。
画像は内容の範囲、KRAは元のレイヤー座標と階層を保持する。GPU結果は
`LIBREPAINT_GPU_RESULT_DIR`へ`clipboard-<形式>-<x>-<y>`の名前で出力する。
接続枠を埋めた取得は保持内容を変更せず拒否し、枠を解放した後の同じ入力から画像・KRAを取得する。
通常の切取りは、取得の拒否で元レイヤーと既存のクリップボードを保持する。取得成功後の削除と
貼付はそれぞれ一件の履歴になり、Undoで空の代替レイヤーを経て元の階層へ戻ることを検査する。
文書所有者の試験は、同じ固定版のKRAをファイルとメモリーへ書き出してバイト単位で照合し、
借用した出力先、保存先、保存済み位置、現在の版と通知の契約を検査する。
合成コピーは通常の`copy_merged`アクションから、グループの不透明度、透明画素のRGB、複数の根、
空の内容とキャンバス外の画素を照合する。直接変更した試験入力は公開の投影更新APIで確定させる。
コピー前後の画素・履歴・変更済み状態とロックを保持し、貼付、Undo／Redo、KRA再読込みを検査する。
クリップボード通知中の画像交換も、取得時の画素を保持する。作業予算不足と接続枠不足は、
既存のクリップボードと元文書を保持し、接続枠の解放後に同じ入力を取得する。
識別子を固定する試験処理は`TestNodeMimeCopyInputs.h`を階層コピーと共有し、通常の挿入先へ渡す。
合成コピーの出力は`merged-clipboard-<挿入条件>-<画素条件>`へ記録する。`fixed`条件は
作成・保存日時とPNG設定も固定し、貼付後のKRA・PNG・復旧記録をバイト単位で比較する。
クリップボードが直接返すKRAには取得時に生成する識別子・日時が含まれるため、貼付後の固定条件の成果物と分けて扱う。
通常の`edit_copy`と`edit_paste`は、複数の根の反復貼付、右下のキャンバス外からの位置補正、
一件ずつのUndo／Redo、元の取得内容、KRA再読込み後の画素と階層の座標を照合する。
接続枠不足は配置の取得と挿入のそれぞれで検査し、枠の解放後に同じ取得内容を貼り付ける。
結果は`normal-layer-paste-<エンジン>-<配置条件>`へ記録する。画素列は環境間で直接比較できる。
通常アクションが生成する識別子と保存日時を持つKRAは、再読込みの契約を検査する成果物として保持する。
操作モデルはグループの座標書込みで子の入力と画素を維持し、履歴件数と固定版、復旧後の座標を照合する。
`paste_new`は、単一部分木の負の座標、キャンバス右下の範囲外、複数の根と統合コピーから、
原点に一枚の描画レイヤーを持つ文書を作る。CPU／GPUの所有形態、色プロファイル、画素と空の履歴を検査する。
作成後の前景色による塗りつぶしは、`roundmarker`プリセット、色、通常合成、消しゴムとアルファ保護の設定を固定し、
Undo／Redo、KRA再読込みとPNG書出しを照合する。元の文書と取得したクリップボードの内容も維持する。
接続枠不足は、範囲の取得と新規画像の採用を別々に拒否させ、登録文書数の保持と枠解放後の再試行を検査する。
画像所有者へ固定UUIDを渡す条件は、文書情報、日時、レイヤー名とPNG設定も固定する。
結果は`paste-new-<作成条件>-<エンジン>-<画素条件>`へ記録し、画素列、KRA、PNGとGPUの復旧記録を比較する。
通常アクションが生成する識別子と日時を持つKRAは、再読込み用の成果物として扱う。
KRAの差は、レイヤー画素と構造、格納先とXML、埋込みPNGの画素、圧縮、解像度とICC情報を分けて診断する。
固定条件の全ファイルの比較が、環境間一致の受入れを支える。
操作モデルは空グループの属性、親、選択、一件の履歴と復旧、子孫付き入力の原子的な拒否を検査する。
固定UUIDの挿入結果は`LIBREPAINT_GPU_RESULT_DIR`へ`empty-group-insertion.recovery`を出力する。
`kis_group_layer_test`は、GPU文書の接続数が上限の一件で埋まっている状態でグループのビューを作り、
グループ自身と文書全体の投影、元画素の更新への追従を検査する。CPU画像のグループ削除とUndoの固定画像も照合する。
`kis_tile_data_store_test`は、既存画像のバッファー確保・複製と文書終了時のメモリー回収を並行実行し、
返されたバッファー全体の画素と複製元の画素を照合する。
`TestNode`は、選択だけの変更が画像更新通知を発行せず、描画プレビュー・確定・Undo／Redoが
通知を発行することを照合する。
操作所有者は不完全・重複・別レイヤーの並べ替えの原子的な拒否、同一順での版の保持、
選択と画素資源の保持、履歴と復旧、進行中の入力と終了後の変更拒否を検査する。
レイヤーの不透明度とロックは、履歴と復旧、KRA読込・再保存、公開ノードの値を照合する。
GPU合成は複数レイヤー、単独レイヤー、既定画素、半透明、透明画素のRGBと不透明度ゼロを検査し、
不透明度の変更後にも元画素と保持済みの表示版が変わらないことを確認する。
消去は透明度がゼロになった画素のRGBを保持し、公開画素読取り、Undo／Redo、復旧と
保持済みの版を照合する。通常画面の直線消去もCPU画像とGPU画像で同じRGBA値を要求する。
入力試験の画面はドック配置から独立したキャンバス領域、等倍、回転なし、反転なし、中央配置を固定する。
直線ツールは通常画面のポインター列でプレビューの置換、取消し、消去、一筆一件の履歴と
Undo／RedoをCPU画像とGPU画像で照合する。拒否通知中のタブ終了はプレビュー中と
ボタン解放時を検査し、キャンバス終了後の入力仲介と再有効化の寿命を確認する。
`KisEllipseToolDocumentTest`は通常画面のポインター入力で、楕円・矩形・角丸矩形・折れ線・多角形の前景色・背景色の輪郭、消去、取消し、
Undo／RedoとKRA保存をCPU画像とGPU画像で検査する。吸着を無効にし、文書・ブラシ・表示条件を固定する。
保存完了通知と保存中状態の解除を待ち、保存済み履歴位置へのUndoで変更済み状態が解除されることを照合する。
塗りつぶし通知中のタブ終了では、履歴と画素の保持を検査する。
輪郭、既存経路の単色塗り、GPUの単色塗りの画面試験は同じQt試験対象の別のCTestとして登録し、それぞれの時間制限と直列実行を維持する。
単色塗りは通常のポインター列と固定座標で、前景色・背景色、輪郭なし、取消し、保存、消去とUndoを検査する。
固定した塗りは`LIBREPAINT_GPU_RESULT_DIR`へ`filled-<図形>-tool`と`fill-only-rectangle-tool`のKRA・PNGを出力する。
折れ線・多角形はクリックした頂点をダブルクリックで確定する。描画通知中のタブ終了では進行中の操作を取り消す。
`KisToolProxyContractTest`は、継続を辞退したダブルクリックの直後に、次の描画の開始入力が届くことを検査する。
`KisGpuTileRendererTest`は矩形・角丸矩形の回転、閉じた輪郭、取消し、Undo／Redo、未確定操作の復旧を検査する。
二軸の異なる半径、上限への制限、半径ゼロ、不正な半径の拒否と復旧後の半径の保持も検査する。
`KisGpuOperationDocumentTest`は旧形式9〜23の復旧を検査する。
`KisGpuLegacyRecoveryFixture_p.h`は、各形式が扱える操作から旧形式の試験記録を生成する。
形式25の移動と未確定入力、形式26の移動・切り抜きと未確定入力は、旧版の実装が生成した固定記録からも復旧を検査する。
形式27の画像切り抜き・キャンバス変更と未確定入力も固定記録から復旧し、各履歴位置の寸法と位置を照合する。
`TestNode`のレイヤー切り抜き試験は公開`Node.cropNode()`と通常の切り抜きツールを操作し、
既定画素、透明画素のRGB、位置、ロック、逆向き・空・包含範囲、一件のUndo／Redoを照合する。
二重グループ内の描画レイヤーは、親子関係、兄弟の画素、複製、復旧、KRA再読込とPNGを照合する。
通常ツールでも階層内の確定・再編集・取消し・再確定・ロックを検査し、別画像の同じUUIDを持つ対象は変更前に拒否する。
再編集の開始・取消し・再確定、通知中の呼出し元・文書・ツールの破棄、範囲外入力の原子的な拒否も検査する。
複製、復旧、固定した作成・保存時刻でのKRA保存再読込、PNG書出しは画素と位置を保持する。
`LIBREPAINT_GPU_RESULT_DIR`へ`crop-<条件>`のBGRA画素、投影RGBA画素、状態、KRA、PNGを出力する。
環境間では各ファイル全体を比較する。ツール再編集の試験は、終了要求も発行する`waitForDone()`を
範囲の復元後へ挿入せず、編集状態と履歴を観測する。
GPU描画試験は境界だけの追加割当、内側タイルの共有、固定版、容量不足時の保持を検査する。
`KisGpuPaintImageTest::directWritesKeepTheirCropHistoryBlocks`は、未使用・保持・削除・境界の
64×64履歴ブロックへの直接書込みと、連続切り抜き、負座標への移動、描画、既定色の書込みを照合する。
固定版と復旧後のUndo／Redoを検査し、同じGPUタイル内の別ブロックも独立して保持する。
`TestNode::testGpuLayerCropKeepsUnrelatedDirectWritesAcrossUndo`は公開APIと文書の履歴から同じ保持を検査し、
`LIBREPAINT_GPU_RESULT_DIR`へ`crop-independent-write.recovery`を出力する。
`TestDocument::testCanvasResizePreservesPixelsHistoryAndSaving`は、公開APIで画像全体の切り抜きと
キャンバス変更をCPU画像とGPU画像へ適用する。縮小、拡大、同じ範囲、内容外の範囲で、
非表示・ロックしたレイヤーも含む画素、位置、既定画素、寸法、表示の固定点、履歴とKRA再読込を照合する。
`LIBREPAINT_GPU_RESULT_DIR`へ`canvas-resize-<条件>`のレイヤーBGRA画素、投影BGRA画素、状態、
KRAとPNGを出力し、環境間でファイル全体を比較する。
同じ対象の拒否・寿命試験は不正入力の原子的な拒否と、寸法・画像変更通知中の文書終了を検査する。
切り抜きツールの試験は画像・キャンバスモードでも確定・取消し・再編集・通知中の終了を扱い、
GPU使用中の拒否後に同じ範囲を再試行できることを検査する。
GPU所有者の試験は保持した旧版、使用中・読取り容量・座標上限による拒否と、復旧後の寸法・画素・Undo／Redoを照合する。
`KisGpuPaintImageTest::positionedPixelWritesPreservePixelsVersionsAndRecovery`は、移動したレイヤーへ
64画素の履歴境界と256画素のGPUタイル全体にまたがる画素を書き込む。
直接書込みと履歴付き置換について、透明画素のRGB、保持した旧版、変更済み状態、Undo／Redoと復旧を照合する。
固定したレイヤーUUIDと入力から`LIBREPAINT_GPU_RESULT_DIR`へ`positioned-write-<条件>`のRGBA画素と復旧記録を出力し、環境間で全バイトを比較する。
復旧記録の検査は重なる矩形、GPUタイル越境、過大な領域と旧形式の不正な整列を拒否し、現在版を保持する。
`TestNode::testImportedLayerPositionSurvivesPublicCopiesSavingAndRecovery`は、読込後の移動済みレイヤーへ
公開APIで範囲を書き込み、CPU画像と画素を照合してから複製、履歴操作、KRA保存再読込と復旧を検査する。
画像所有者と読取り試験は未確定入力・使用中・読取り容量の拒否と、大きな単色キャンバスの縮小画像を検査する。
形式11は角丸矩形と頂点列を保持し、形式12は描画時の透明度保護を保持する。
形式13は描画開始時とレイヤーのチャンネル保護、属性履歴を保持する。
形式14は描画開始時の左右・上下の対称設定と軸位置を保持する。通常入力では開始後の
設定変更からの独立を検査する。GPU描画では片軸・両軸、移動した軸、キャンバス外、
軸上の重なり、直線、Undo／Redoと未確定操作の復旧を全画素で照合する。
7条件を`LIBREPAINT_GPU_RESULT_DIR`の`symmetry-<条件>.rgba`へ出力する。
形式15は履歴版ごとの複数レイヤー選択を保持する。レイヤー複製は既存経路とGPU経路で、
追加順、操作対象、複数選択、属性、元画素と独立した編集、一件のUndo／Redoを照合する。
操作モデルでは入力と圧縮画素の共有、復旧、重複識別子、選択不整合と容量不足の原子的な拒否を検査する。
同じ固定文書の複製は復旧記録全体の一致とUUIDの衝突解決を照合し、
`LIBREPAINT_GPU_RESULT_DIR`へ`layer-duplication.recovery`を出力する。
形式16は操作対象と選択集合を独立して保持する。通常画面では対象を選択から外し、
複製とUndo、選択解除後の対象への復帰を既存経路とGPU経路で照合する。
操作モデルは復旧と未確定入力の保持を検査し、固定した選択状態を
`LIBREPAINT_GPU_RESULT_DIR`へ`layer-selection.recovery`として出力する。
形式17は輪郭指定と単色塗りを保持し、描線・塗りなしの履歴も復旧する。
描画試験は塗りと輪郭の色、Undo／Redo、復旧後の全画素一致を検査する。
塗り領域全体がタイル予算を超える場合の原子的な拒否と、表示領域だけを一枚のタイルで生成することを照合する。
一直線上の頂点だけを塗る要求は、画素と一回の履歴を保持し、塗り用タイルの割当をゼロに保つ。
`KisGpuPixelReadbackTest`は1024×1024の単色塗りを16枚の完全なレイヤータイルとして読み、
`LIBREPAINT_GPU_RESULT_DIR`へタイル座標順の`solid-fill-layer-interior.rgba`を出力する。
`KisGpuPaintImageTest`は塗りの内側への直接書込みを、塗り後の履歴版へ保持し、Undoで塗り前の画素へ戻す。
形式21は加重補正の固定設定と正規化した入力速度を保持する。
操作試験は固定した設定と元入力、速度と設定の値域、未確定操作の復旧、取消しと履歴を検査する。
描画試験は速度、筆圧補正、末尾の強さと距離ゼロについて固定した補正後の点から生成する全画素を照合する。
逐次更新・一括描画・固定版・停止・Undo／Redo・復旧を検査し、
`weighted-smoothing-{weighted,no-pressure,no-tail,slow,fast,zero-distance}.rgba`を出力する。
通常画面は加重補正で複数点を描き、設定変更からの独立、停止、消去、取消し、入力上限と拒否通知中のタブ終了を検査する。
`KisGpuPaintDocumentTest`はKRA保存・再読込み・同じ追加描画後の画素を照合し、
`weighted-smoothing.kra`と`weighted-smoothing-merged.png`を出力する。
`TestDocument`は同じ加重補正の画素を公開APIと通常の書出し操作からPNGへ渡し、全画素・ICC・解像度、履歴と保存先の保持を検査する。
固定出力は`weighted-api-rgba.png`と`weighted-menu-rgba.png`である。
重み評価の上限超過では、描画・読取り・直接書込みが部分結果を採用せず、入力・履歴・復旧を保持することを検査する。
形式20は基本補正の設定、入力停止位置と確定状態を保持する。
操作試験は元の座標・センサー値、停止位置、未確定の復旧、確定・取消し・履歴を検査する。
停止位置の順序と範囲、設定の組合せ、確定状態と不正な個数は、編集中の文書を変えずに拒否する。
描画試験は曲線、停止後の継続、停止による補間、タップ、2点入力の解放、同じ位置の入力を、
明示した曲線または点の画素と照合し、逐次更新・Undo／Redo・復旧後の一致を検査する。
固定出力は`basic-smoothing-{curve,idle-then-continue,idle-two,tap,tap-after-two,stationary}.rgba`である。
通常画面は停止による補間と継続、描画中の設定変更、停止待ちの取消し、描画・消去・履歴・KRA保存を検査する。
画像所有者は停止の版を通知前にGPUへ要求し、同じ位置の停止を重ねた場合の通知と版の維持を検査する。
派生量を超えた基本補正は、描画・読取り・直接書込みで部分結果を採用せず、入力・履歴・復旧を保持する。
形式19はピクセル補正の設定と元入力のセンサー値を保持する。
通常画面は補正を有効にした描画・消去・取消し・Undo／Redo・KRA保存と、描画中の設定変更を検査する。
描画試験はタップ、角の保留と採用、負座標、開始位置のタイルの解放について、
一点ずつの更新と一括描画、履歴と復旧の一致を検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`pixel-perfect-{tap,held-corner,flushed-corner,negative-floor,initial-tile-removed}.rgba`を出力する。
操作試験は元の座標・筆圧・傾き・回転・時刻の保持と、未確定操作の復旧を検査する。
形式18は移動・直線・三次曲線の命令列、制御点と塗り規則を保持する。
公開APIの経路描画は直線・曲線・複数経路・偶奇と巻き数による穴の塗り・空の経路について、
指定レイヤー、画面の選択、複製レイヤー、Undo／Redoを既存経路とGPU経路で照合する。
描画試験は経路の区切り、制御点から生成した曲線、二つの塗り規則、復旧後の全画素一致を検査し、
`LIBREPAINT_GPU_RESULT_DIR`へ`path-curve.rgba`、`path-subpaths.rgba`、`path-even-odd.rgba`、`path-winding.rgba`を出力する。
`KisGpuPaintDocumentTest`は複数タイルにまたがる曲線と穴のある塗りをKRAへ保存・再読込みし、
全画素一致と埋込PNGを検査する。固定出力は`path-filled.kra`と`path-filled-merged.png`である。
操作モデルは空の経路の履歴と未確定操作の復旧、不完全な命令列と入力予算の拒否を検査する。
派生する境界線分の上限を超えた経路は、描画・保存用読取り・直接書込みが部分結果を採用せず、
操作値・履歴・復旧記録を保持することを検査する。
被覆を外すプレビューは以前の背景へ戻り、保持した描画版の画素を維持する。
選択変更の通知中にタブを終了してウィンドウを破棄する試験は、既存経路とGPU経路で
タブ・画像の解放とGPU接続の解放を検査する。破棄待ちのタブもツール資源の生存中に終了する。
離れた反射先の疎なタイル、入力追加と全再生成の一致、容量不足時の状態保持も検査する。
折れ線・多角形は頂点の保持、閉じる辺、同じ点列の自由描画との全画素一致、履歴と未確定操作の復旧を検査する。
透明度保護は透明・微小アルファ・半透明・不透明な画素への合成、消去の優先、全画素のアルファ保持、
未確定操作の復旧とUndo／Redoを検査する。通常画面の自由描画では、開始後の設定変更からの独立、
消去・取消し・保存済み位置へのUndoも照合する。
透明度保護の6条件は`LIBREPAINT_GPU_RESULT_DIR`へ`alpha-lock-<条件>.rgba`を出力する。
レイヤーのチャンネル保護は、透明・半透明の画素、消去、色だけ・アルファだけの書込みと
全チャンネルの保護、開始後の設定変更、履歴と復旧を検査する。7条件のGPU画素を
`channel-protection-<条件>.rgba`へ出力する。通常パネルでの透明度保護の変更とUndo／Redo、
通常入力の描画・消去・取消し、KRA再読込み後の追加描画を照合する。
`TestKraRasterCodec`は既存のBGRA順の保護属性、保存・再読込み・再保存、保護の解除と元画素の保持を検査する。
固定した5条件を`channel-protection-<条件>.kra`へ出力し、ファイル全体を環境間で照合する。
各ファイルは256×256のタイルを行優先のRGBA8で保持し、環境間でSHA-256を照合する。
`KisDocumentReplaceTest`は、変更済み・保存済みの履歴通知を遅延させ、後続操作後の文書状態を照合する。
現在の履歴位置に対応する表示と、履歴外の変更を保持する条件を検査する。
`kis_image_test`は、部分投影と画像全体の更新処理をセマフォで一時停止し、別の処理から
フィルターを解除する。解除した時点と要求完了後の寿命を照合し、処理中の所有者を保持することを検査する。
固定座標のツール呼出しによる図形編集は、`LIBREPAINT_GPU_RESULT_DIR`へ
`ellipse-tool.{kra,png}`、`rectangle-tool.{kra,png}`、`rounded-rectangle-tool.{kra,png}`、
`polyline-tool.{kra,png}`と`polygon-tool.{kra,png}`を出力する。
これらのSHA-256を環境間で照合し、通常入力の表示座標による差と保存結果を区別する。
円形マーカーの直線の途中の筆圧は、入力の始終点を結ぶ線への位置の投影を維持し、自由描画の同じ
座標・筆圧列とタイル全体の画素を照合する。直線を移動した場合も同じ筆圧分布を照合する。
Gaussian WASHの直線は、保持した入力位置を変更後の始終点へ投影し、元の筆圧・傾き・時刻を保持する。
`KisGpuTileRendererTest::gaussianLineShapeEditsPreserveInputHistoryAndRecovery`は、中間筆圧、単一標本、
同位置の入力からの移動・拡縮、旧タイルの除去、取消し、Undo／Redoと復旧を、明示した直線パスと照合する。
`KisGpuPaintDocumentTest::gaussianLineEditsSaveReopenAndContinueAtTheirNewPosition`は、固定UUIDの文書で
別タイルへ移した直線のKRA保存・再読込みと追記を照合する。`LIBREPAINT_GPU_RESULT_DIR`へ
`gaussian-line-<条件>.rgba`と`gaussian-line/`内のKRA・RGBA画素を出力し、環境間で全バイトを比較する。
Gaussianの複合パスは移動命令の前後で間隔を共有し、個別のパス呼出しはそれぞれの間隔を持つ。
直線とパスの交互呼出しは、直線の間隔を再開し、不透明度の平均を一つのストロークで保持する。
復旧形式39は各パス呼出しの半開入力範囲を保持し、範囲の重複・入力外・命令途中の境界を拒否する。
`KisGpuOperationDocumentTest`は作業中の入力、履歴と旧形式38の固定記録を検査する。
`KisGpuTileRendererTest::gaussianPathCallsKeepDistanceHistoryAndRecovery`は、変更前の通常CPU描画から
固定した全画素、間隔だけを変えるプレビュー、取消し、保持済み版、履歴と復旧を検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`gaussian-path-calls-<条件>.rgba`を出力する。通常文書の
`TestDocument::testDefaultBrushGpuEditingKeepsPixelsHistoryAndFiles`は、新規・読込文書の複合パス、
個別パスと二種類の交互呼出しについて、追記・履歴・KRA保存再読込み・PNG/JPEGを書込みバイトまで照合する。
Gaussianの三次曲線は、開いた曲線、始点へ戻る曲線、複合・個別のパスと直線との混在を固定入力とする。
`libs/image/gpu/tests/KisGpuGaussianBrushFixture_p.h`は、通常の資源取得から保存した形式39の
ブラシ設定を保持する。資源の版、倍精度の筆圧応答、流量と不透明度を同じ値で再生する。
`KisGpuTileRendererTest::gaussianCubicPathsKeepPixelsInputHistoryAndRecovery`は、通常CPU描画の全画素、
制御点を保持したプレビュー、保持済み画素、取消し、履歴と復旧を検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`gaussian-cubic-<条件>.rgba`を出力し、環境間で全バイトを比較する。
通常文書の同じ5操作は、新規・読込文書での追加編集、履歴、KRA再読込とPNG/JPEGの全バイトを照合する。
平滑化なしのGaussianフリーハンドは、通常のマウス入力からタップ、2点、移動、閉経路、
静止、始点の反復と折返しを検査する。押下直後の下地、追加編集、履歴と保存ファイルを
`TestDocument::testDefaultBrushGpuEditingKeepsPixelsHistoryAndFiles`でCPU経路と照合する。
`KisGpuTileRendererTest::gaussianFreehandKeepsInitialDistanceAndFinalTap`は、通常CPUツールの全画素、
入力中・確定後の復旧、保持済み版と取消しを検査する。固定レイヤー識別子の条件で
`LIBREPAINT_GPU_RESULT_DIR`へ`gaussian-freehand-<条件>.rgba`、`.pending.recovery`と`.recovery`を
出力し、画素と入力・ブラシ設定を含む復旧記録の全バイトを環境間で比較する。
静止入力は2・3・8標本を独立した一回のタップと全画素比較し、移動した閉経路と極短移動を区別する。
Gaussian WASHの基本補正は`KisGpuTileRendererTest::gaussianBasicKeepsCpuPixelsFlushesAndRecovery`で、
通常CPU描画の固定全画素、入力停止時の補間、入力中・確定後の復旧、取消しと履歴を照合する。
`libs/image/gpu/tests/KisGpuGaussianBasicSmoothingFixture_p.h`は、元の座標・筆圧・時刻とCPU参照の由来を保持する。
`LIBREPAINT_GPU_RESULT_DIR`へ`gaussian-basic-<条件>`の画素と復旧記録を出力し、環境間で全バイトを比較する。
通常文書の同じ描画・保存契約は、基本補正のタップ、短い入力、閉経路、折返し、停止時の補間と
静止入力を、新規・読込文書、通常合成と二つの消去指定で検査する。
レイヤー識別子を描画前の入力として固定し、KRA、PNG、JPEGを全バイトで照合する。
Gaussian WASHの加重補正は`KisGpuTileRendererTest::gaussianWeightedKeepsCpuPixelsAndRecovery`で、
タップ、2・3・8標本の静止入力、筆圧に応じた可変距離、移動、始点の反復と停止後の継続を扱う。
`libs/image/gpu/tests/KisGpuGaussianWeightedSmoothingFixture_p.h`は元の入力と設定、CPU参照画素の由来を保持する。
入力中・確定後の復旧、保持した版、取消しと履歴を照合し、停止処理の反復による記録の不変を検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`gaussian-weighted-<条件>`の画素と復旧記録を出力する。
`gaussianWeightedOnImportedPixelKeepsCpuMergeByte`は、半透明の読込画素へ続けて描いたときの
CPU保存画素を維持する。被覆の丸め境界を含む入力で、プレビュー、確定と復旧後の値を比較する。
通常文書の描画・保存契約は、新規・読込文書、通常合成と二つの消去指定で、
マウス入力、筆圧変化を持つペン入力、停止処理と追加編集、KRA再読込とPNG/JPEGを全バイトで照合する。
静止入力はGPU文書の一回のタップと比較し、同じ画素と保存ファイルになることを検査する。
Gaussian WASHのピクセルに合わせた補正は`KisGpuTileRendererTest::gaussianPixelPerfectKeepsPixelsInputHistoryAndRecovery`で、
タップ、2・3・8標本の静止入力、保留・確定する折れ点、最初のタイルが除かれる入力と負座標を検査する。
`libs/image/gpu/tests/KisGpuGaussianPixelPerfectFixture_p.h`は、元の座標・時刻と参照画素の由来を保持する。
参照画素はタップと移動を通常CPU描画、補正後が1点となる入力をその画素中心での通常POINT描画から得る。
入力中・確定後の復旧、保持した版、取消しと履歴を照合し、
`LIBREPAINT_GPU_RESULT_DIR`へ`gaussian-pixel-<条件>`の画素と復旧記録を出力して全バイトを環境間で比較する。
通常文書の同じ描画・保存契約は、新規・読込文書で全8入力と追加編集、KRA再読込、PNG/JPEGを照合する。
保留・確定する折れ点は通常合成と二つの消去指定を扱う。座標の移動は画素中心への丸めより先に適用し、
移動によって複数点になる入力は、その生入力を使う通常CPU描画と比較する。
Gaussian WASHの消去は`KisGpuTileRendererTest::gaussianEraseKeepsColorsHistoryAndRecovery`で、
通常CPUの固定画素、透明画素を含むRGB保持、アルファの非増加、入力中・確定後の復旧、
取消しとUndo／Redoを照合する。直線、開いた曲線、閉曲線と混在、流量・不透明度ゼロ、
不透明度の8ビット丸め境界について、`gaussian-erase-<条件>.rgba`へ128×128の全RGBAを出力する。
通常文書の同じ描画・保存契約は、消しゴム切替と消去合成の両方から、新規・読込文書への
図形とフリーハンドの消去、RGB保持・アルファの非増加、追加編集と全保存ファイルを照合する。
同じ固定入力の復旧記録は`gaussian-erase-<条件>.pending.recovery`と`.recovery`へ出力し、
入力列・ブラシ設定・履歴を含む全バイトを環境間で比較する。
`TestDocument::testDefaultBrushMouseInputKeepsFixedTimeThroughRecovery`は、通常マウス入力から
入力構築・自由描画・操作記録を通る経過時間と速度を検査する。入力イベントの時刻と描画の
経過時間をそれぞれ固定し、入力中・確定後の復旧記録まで同じ標本が届くことを照合する。
同じ試験のCPU加重補正条件は、固定した6標本で非ゼロの補正距離、終端の補完、
始点の反復、停止後の継続と停止処理の反復を扱う。描画前後の全画素、
入力時刻とセンサー値、一回の履歴とUndo／Redoを照合し、
`fixed-time/<条件>/`へ`painted.bgra`と入力・制御点の診断記録`input.json`を出力する。
`KisGpuTileRendererTest::basicCurveControlsPreserveClosedSegmentDecisions`は、
閉線分の内側・端点・外側と平行・交線なしの制御点を検査する。
`TestDocument::testDefaultBrushStationaryInputKeepsOneTap`は、CPU・GPUの補正なし・基本補正を
通常マウス入力から検査する。同じ文書でタップ、Undo、反復入力を行い、不均一な下地、履歴と
KRA・PNG・JPEGの全バイトを照合する。同じ保存済み結果の再読込み後にも、タップと反復入力の追加編集を比較する。
保存ごとに既存の文書情報・時刻・設定を固定し、同じファイル名の別ディレクトリーへ出力する。
画素を観測したCPU画像版は、画像の読取ロックで保存開始時の複製まで保持する。
`LIBREPAINT_GPU_RESULT_DIR`の`stationary-tap/<条件>/`は、`tap/`、`repeated/`、`after-tap/`と
`after-repeated/`に`document.{bgra,kra,png,jpg}`を保持する。反復入力の比較前の画素は
`repeated-input.bgra`と`reopened-repeated-input.bgra`に保持する。
`FreehandStrokeContractTest::stationaryCompletionProducesExactlyOneTap`は、未描画の終了、既に発行した印と
縮小版の条件付き完了を検査する。`directLineCompletionPreservesEachPainter`は、ブラシ印を登録せず描く線と
更新処理で消費した変更領域について、主描画先とマスクの補完を個別に検査する。
三次曲線は、始終点が同じ場合も制御点が異なれば経路を分割し、全点が一致する場合は静止点として扱う。
`kis_paintop_test::testCubicTraversalKeepsGeometryAndInput`は、閉曲線と等価な中点分割のブラシ呼出しを照合する。
通常の曲線、同一直線上の閉曲線と静止点について、始終点、連続した筆圧・時刻と曲線点列を検査する。
`LIBREPAINT_GPU_RESULT_DIR`へ`cubic-traversal-<条件>.json`を出力し、ブラシへ渡した線分・入力値と点列を環境間で全バイト比較する。
既存KRA読取側との照合は登録済みKRAプラグインを直接利用し、新しい読込と独立した画素・構造の
検査を維持する。JPEG XLとRGBEの形式試験も、この読取側で比較用KRAの色・画素を取得する。
KRAの付属情報は解像度、ICC、文書情報、注釈と対称軸を照合し、不正な解像度・文書情報と欠落・重複した
注釈による読込失敗を検査する。読込後にファイル所有者と外部の実行参照を解放し、保持した画像の
画素読取りと最後の参照による実行所有者の破棄も確認する。
対称軸は公開文書からの保存と初期化後の書出し、ファイルの読込み、重複要素と不正なXMLの原子的な拒否を検査する。
`TestKraRasterCodec`は固定した設定を`mirror-axis-metadata.kra`へ出力し、環境間でファイル全体を照合する。
公開文書のスナップショットは、独立した画素、空の履歴、UUID、解像度、注釈とノード属性を照合する。
共有実行の検査は外部の所有参照と文書を解放し、複製と公開ノードによる画素読取り、最後の参照の
解放による実行所有者の破棄を確認する。
文書置換は画像参照の維持、旧ノードの画素の寿命、容量不足時の画像・保存先・文書情報の維持を
検査する。通常画面では同じ寸法への置換後に表示、入力、未確定入力のUndo、保存を一巡する。
画像所有者の試験は、複数の履歴位置を一度の通知で選び、不正な位置と編集中の位置移動を
状態変更前に拒否することを検査する。履歴表示の試験は、コマンドを実行せずに状態を反映し、
存続するコマンドの参照と削除するコマンドの所有権を検査する。
Linuxの実行にはVulkanに対応するQt画面プラグインを使う。画面を持たない検証は
独立したXvfbのX11画面を用意して`QT_QPA_PLATFORM=xcb`で実行する。

`KisGpuTilePresentationTest::tileOverBackgroundMatchesEightBitSourceOver`は、RGBA8とBGRA8の表示先で、
各色成分とアルファの8ビット値の組合せを、不透明・半透明・透明・小数精度の背景色へ合成する。
入力は色成分を0〜255、アルファを1〜255として、緑を`(149 * red + 37) & 255`、
青を`(73 * red + 113) & 255`で固定し、アルファ0の行と既知の半透明画素も含める。
Qtの8ビット乗算済みアルファ合成との全画素一致と、元のGPU画素・文書版・復旧記録の保持を要求する。
`LIBREPAINT_GPU_RESULT_DIR`へ`matte-palette-<format>-<background>-actual.rgba`と`-expected.rgba`を保存する。

`KisGpuTilePresentationTest::checkerBackgroundPreservesDocumentPixels`は、空の文書と半透明の画素について、
固定・追従・拡大・90度回転、表示先による切取り、RGBA8とBGRA8を全画素で照合する。
期待値はQtの8ビット前景合成で2色を作り、物理画素の中心から文書境界と模様のセルを直接判定する。
`checker-background-<format>-<condition>-actual.rgba`と`-expected.rgba`を出力し、元画素・文書版・復旧記録の保持を検査する。
不正な模様の色・セル幅・変換・座標範囲は表示先の変更前に拒否する。
`KisGpuDocumentViewTest::canvasBackgroundFollowsPreferencesWithoutEditing`は、通常の空の文書で、
パン・ズーム・23度回転・設定変更後の全画素と文書画素・履歴・変更状態を照合する。
固定・追従と半透明の2条件を組み合わせ、`ordinary-canvas-background-<condition>-<stage>-{actual,expected}.png`を保存する。
最大幅の文書では1画素の模様を使い、末尾と余白を表示した後のパンでも全画素と文書状態を照合する。
試験中はスクロールバーを非表示にし、表示内容とキャンバス境界を比較する。ネイティブ子ウィンドウの
外縁2物理画素に含まれるウィンドウ影は表示契約から除外し、その内側の全画素を照合する。
低水準の固定画素は環境間で照合し、通常画面の取得は各環境の表示座標との接続を検査する。

`KisGpuTilePresentationTest`は、輪郭の移動・線幅・消去と不透明・半透明・透明背景の全RGBA画素を照合する。
不正な座標・線幅・色と頂点数の上限超過では、表示先の全画素が維持されることを検査する。
`KisGpuDocumentViewTest::brushOutlineFollowsTheOrdinaryPointerWithoutEditing`は、通常画面のブラシとポインターから
輪郭の表示・移動・倍率変更・ツール切替・文書の非アクティブ化・退出時の消去を検査し、文書画素・版・履歴・変更状態の維持を照合する。
輪郭径は画像座標からウィジェット座標へ変換し、画面倍率を掛けた物理画素で照合する。
画面取得は起動通知が消えた文書表示との厳密一致から開始し、設定変更による表示窓の再生成を追跡する。
倍率変更後も通知が重ならないフレームの輪郭径・位置・画素数を同じ条件で照合する。
`LIBREPAINT_GPU_RESULT_DIR`へ低水準の`.rgba`と通常画面の`ordinary-brush-outline-<stage>.png`を保存する。
通常画面の画像はネイティブ表示と入力の接続を確認し、固定したGPU表示画素の環境間比較は低水準の全RGBAを使う。

`KisGpuTilePresentationTest::cropPreviewRetainsAntialiasedGeometryAndDocument`は、切り抜きの暗幕、ハンドルと補助線を
アンチエイリアス付きの既存の描画条件で照合する。DPR 1／2／3、回転、反転、太い線、半透明背景、
重なるハンドル、DPR2での縮小・回転と2画素の枠線の組合せ、
ドラッグ途中の空・負の矩形、表示端で切れる補助線と最大文書端の移動を含む。
表示専用の被覆率と丸めの比較上限は各成分2階調とし、文書版と復旧記録は厳密に比較する。
RGBA8／BGRA8の全表示画素を`crop-<format>-case-<condition>-{actual,expected}.rgba`へ保存し、
不正入力では表示先の全画素が維持されることを照合する。
`KisGpuDocumentViewTest::cropPreviewPreservesPixelsHistoryAndCancellation`は、登録済みの切り抜きツールを使い、
5種類の補助線、ドラッグによる移動とサイズ変更、パン・ズーム・回転、取消しを検査する。
自由ドラッグは吸着を無効にして位置を照合し、画像端への吸着を有効にした操作も検査する。
画像を表示領域の中央へ置き、吸着の距離は表示座標で10画素に固定して、DPRごとの画像座標との対応を照合する。
作成と確定、Undo／Redo、KRA再読込とPNG書出しでは文書画素を全バイトで比較し、表示用の暗幕と装飾から分離する。
通常画面の全画素は同じ表示専用の上限で照合し、`crop-<style>-{actual,expected}.png`と
`crop-operation-<stage>-{actual,expected}.png`を保存する。

### GPU表示のVulkan検証

LinuxのGPU表示は、画素試験に加えてVulkan検証層で生成シェーダー、表示画像の配置と
資源の寿命を検査する。独立したXvfbの`DISPLAY`を選択し、固定したNix入力から
検証層を取得して実行する。

```sh
nixpkgs_rev="$(./scripts/run-shared-test-env python3 -c \
  'import json; print(json.load(open("flake.lock"))["nodes"]["nixpkgs"]["locked"]["rev"])')"
validation_layers="$(nix build --no-link --print-out-paths \
  "github:NixOS/nixpkgs/$nixpkgs_rev#vulkan-validation-layers")"
./scripts/run-shared-test-env ./scripts/build-incremental native build KisGpuCanvasWindowTest
./scripts/run-shared-test-env ./scripts/build-incremental native build KisGpuTilePresentationTest
./scripts/run-shared-test-env env QT_QPA_PLATFORM=xcb \
  VK_LAYER_PATH="$validation_layers/share/vulkan/explicit_layer.d" \
  VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation \
  ctest --preset tdd-linux --verbose --tests-regex '^libs-canvas-gpu-' \
  > gpu-native-validation.log 2>&1 &&
  ! rg 'Validation Error:|SYNC-HAZARD-' gpu-native-validation.log
```

検証結果には使用GPU、検証層の版、試験結果と診断ログを記録する。
試験成功とネイティブAPIの診断ゼロを併せて受入れ条件とする。
表示用シェーダーの変更では、透明度、クリップ、移動・拡大・回転の画素契約も照合する。

### GPU連続入力の処理時間

`KisGpuInteractiveFrameBenchmark`は、250筆の背景を持つ2048×2048文書の
512×512表示領域へ、筆圧の変化する入力を追加して処理時間を測る。
対象を明示して構築し、macOSの画面外処理時間を次のように測る。

```sh
./scripts/run-shared-test-env ./scripts/build-incremental native build KisGpuInteractiveFrameBenchmark
./scripts/run-shared-test-env env QT_QPA_PLATFORM=offscreen \
  build/tdd-macos/bin/KisGpuInteractiveFrameBenchmark.app/Contents/MacOS/KisGpuInteractiveFrameBenchmark \
  --offscreen --output gpu-frame.json --maximum-p95-ms 8.3
```

Linuxでは対応する実行ファイル`build/tdd-linux/bin/KisGpuInteractiveFrameBenchmark`を使う。
画面外は`QT_QPA_PLATFORM=offscreen`、ウィンドウ提示は`QT_QPA_PLATFORM=xcb`で実行する。
既定値は予備32フレームと測定240フレームで、
`--warmup`、`--frames`、`--history-strokes`で変更できる。
JSONには実際のGPU、表示面の画素数、CPUの入力処理から描画送信までの各測定値、
入力受付、表示面の取得、描画記録、送信の各区間、
CPU側のP95、タイル保持量と最終合成画像のSHA-256を記録する。
GPU時間の欠測は`completedGpuMs`の空配列と`gpuP95Ms`の`null`で表し、
ウィンドウ表示の上限判定では検証不足として扱う。
同じ設定の改善前後では最終画像の照合値を一致させる。
`--paint-layer`は下から数えた描画レイヤー番号を指定し、既定値は最下位の`0`である。
高負荷の最上位レイヤーは`--paint-layer 23`で測定する。両条件を個別に反復し、
JSONの`paintLayerIndex`とともに保持量、処理時間、最終画像を比較する。
比較する実行ファイルと共有ライブラリーは、それぞれ対応するヘッダーで構築する。
クラスの保持状態が変わる変更では、ヘッダー内で生成される破棄処理を含めて
実行ファイルを再構築し、測定した実行物と依存ライブラリーの照合値を記録する。

通常合成のアルファ・除算を変更する場合は、実際のGPUシェーダーで全ての非ゼロ不透明度と
元アルファ、1から254の下地アルファを組み合わせ、CPUの単精度除算と
合成アルファと商の全ビットを照合する。参照計算は、このRGBA8入力範囲の積和を倍精度で評価して
単精度へ丸める。透明・不透明な下地は既存の投影画素契約で確認する。
色成分の契約は、実際の不透明度とアルファから得る混色率、および半整数の境界と隣接する
単精度値について、全256×256の元色・下地色を積和の単精度丸めと最近接偶数の8ビット丸めで照合する。
描画器は灰色129へ灰色126を不透明度85で重ねた結果128を、一様な既定色と実タイルで照合する。
灰色253・アルファ25へ黒・アルファ38を不透明度1で重ねた結果は、灰色251・アルファ25を保持する。
不透明度100%では、アルファ2同士の正・逆方向の半整数丸め、アルファ2と6の境界、
色成分ごとに境界が異なる入力と通常の混色を、一様な既定色と実タイルで照合する。
`normalProjectionKeepsDefaultAndTileColorsAtRoundingBoundaries`の固定値は、
単精度の積和丸めと最近接偶数の8ビット丸めを持つ投影結果を保持する。
`multiPassGroupsKeepDistinctLayerParameters`は、複数タイルにわたるグループの
不透明度、合成方式、成分指定と途中の透明RGBを一様な既定色の合成と照合する。
属性を変更した後も、変更前の版が同じ画素を保持することを確認する。
`regionalInputKeepsPixelsAcrossViewportAndCapacityChanges`は、表示領域への入力追加、
領域の移動・縮小とタイル容量の変更を、全文書の再生成と照合する。
保持した版とUndo／Redoの画素を維持し、描画器のタイル予算に収まることを確認する。
`longStrokeRebuildKeepsPixelsAndHistory`は、長い一本の筆跡を一括入力と分割入力で照合し、
固定した不透明色、Undo／Redo、保持した版と復旧後の画素を確認する。
`longDocumentRebuildUsesBoundedTileStorage`は、多数の操作を含む文書の再生成を固定画素と照合する。
公開文書の同じ入力は、既存画像との画素比較、KRA統合画像、PNG、両所有者による再読込みを検査する。

```sh
./scripts/run-shared-test-env ./scripts/run-test KisGpuProjectionArithmeticTest
./scripts/run-shared-test-env ./scripts/run-test KisGpuTileRendererTest
```

`deviceName`、`deviceType`と`backend`は描画に使用したアダプターから取得する。
性能比較は、これらの値と実機のGPU構成を照合してから行う。LinuxのNVIDIA GPUでは、
診断用の実行中に`nvidia-smi pmon -c 1`で測定プロセスのPIDとGPU上の処理を照合する。
外部監視を伴う実行は、監視負荷を含む診断として通常の性能測定と分けて記録する。

表示面は垂直同期に従って提示し、イベント処理と最終画像の読取りを測定区間の外へ置く。
表示面取得の待ち時間も記録して比較する。
上限指定時はCPUのP95超過、ウィンドウ提示時のGPU測定値の欠落で終了値2を返す。
`--offscreen`は、同じ表示倍率と画素数のRGBA8テクスチャへ出力し、OSの表示面取得待ちを
含まない処理時間を測る。この方式は`wgpuDevicePoll`でGPU完了までを測定し、
測定中のCPU画素読取りを伴わない。上限判定にはCPU側で観測したGPU完了までの時間を使う。
JSONの`target`は`window`または`texture`を記録し、Qtが報告する表示更新周波数も記録する。
`completionSynchronized`が`true`の場合はCPUの測定区間にGPU完了待ちを含む。
これは処理費用の比較用であり、通常画面の240Hz入力、60Hz／120Hz表示、
入力から実表示までの受入れ検証は、対象機器で別途実施する。
`--crop-preview`は毎フレームの表示へ切り抜きの暗幕、8個のハンドルと8本の補助線を追加し、
JSONの`cropPreview`へ有効状態を記録する。同じ入力・仕事量で有効／無効を交互に測り、
処理時間、保持した入力、最終文書の投影ハッシュを比較する。投影ハッシュは編集用の文書画素から計算する。
試験の画面外フレームは画素の読取り完了を保証する。通常表示は表示フレームの命令列へ計算を連結する。

### 時間を固定した連続入力と高負荷の測定

小負荷の描画速度は120fpsを目標、90fps前後を受入れの下限とし、処理のフレーム予算を
目標約8.3ms、下限約11.1msとして評価する。高負荷は60fpsを目標とし、
240Hzの入力と60Hzの送信条件で、処理時間のP95と1フレーム約16.67ms
（1,000/60ms）の目標との差を記録する。高負荷の60fpsは改善目標として扱い、
機能移行の完了判定とは分けて評価する。実表示までの遅延と入力保持も併せて検証する。

`KisGpuInteractiveFrameBenchmark --input-hz 240`は、入力標本の予定時刻と表示の予定時刻を
独立に進める。予備2秒・測定10秒の全2,880標本を保持し、表示が遅れた場合も
確定前に残りを受理・描画する。`--display-hz`は送信の予定周波数で、画面の実測更新周波数は
`displayRefreshRateHz`へ別に記録する。
描画完了時に次の期限が到来済みなら、最新の描画要求を一つ保持して処理を続ける。
それ以前の要求は集約し、期限前の待機は入力と描画の次の予定時刻から決める。
描画中に到来した期限の数は`missedScheduledFrames`へ計上し、処理の再開と独立して保持する。

`--high-load`は4096×4096文書、24レイヤー、直径512の円形ブラシ、中央1024×1024の
占有領域を使用する。占有領域は固定したRGBA8チェックポイントから初期化し、
初期転送を測定時間の外へ置く。GPUタイル予算は512MiB。小負荷条件は従来の
2048×2048文書と直径16のブラシを使う。両条件の背景筆跡は`--history-strokes`で固定する。

画面外の高負荷測定は次の条件で実行する。

```sh
./scripts/run-shared-test-env env QT_QPA_PLATFORM=offscreen \
  build/tdd-macos/bin/KisGpuInteractiveFrameBenchmark.app/Contents/MacOS/KisGpuInteractiveFrameBenchmark \
  --offscreen --high-load --input-hz 240 --display-hz 60 --warmup-seconds 2 --duration-seconds 10 \
  --maximum-p95-ms 16.666667 --output high-load-60.json
```

同じ条件を3回の独立起動で測る。小負荷は`--high-load`を外し、
`--display-hz 120 --maximum-p95-ms 8.3`で実行する。小負荷の60Hz条件と高負荷の
120Hz条件は比較用に記録する。`--offscreen`を加えるとGPU完了待ちを含む処理時間の
P95を指定した上限で検査し、超過時に終了値2を返す。GPUの時刻を計測しない画面提示では、
同じ上限指定は検証不足として終了値2を返す。

環境間の速度比較では、CPUとGPUの型、GPUの接続、ドライバー、構築設定と描画領域を記録する。
実時間入力では処理速度に応じて一フレームの入力数が変わるため、最終画像の一致と
フレームごとの仕事量をそれぞれ確認する。処理能力の切分けには実時間指定を外し、
`--warmup 120 --frames 600`で同じ入力列を連続実行する。連続実行と間隔付きの実行は、
GPUの動作周波数と性能状態への応答も含めて比較する。CPUの送信区間は
`wgpuCommandEncoderFinish`、`wgpuQueueSubmit`と`wgpuDevicePoll`に分け、
経過時間とCPU実行時間を区別する。監視、追加の休止とAPI追跡を伴う条件は診断として記録し、
通常設定の性能受入れは元の入力・時間予算で判定する。

JSON形式3は受理・送信した入力数、表示予定の欠測数、送信前の未送信標本数、
最新・最古の入力予定時刻から送信までの時間を記録する。CPU時間には表示間に
受理した入力処理の合計を含める。最大タイル保持量はGPU画素の論理保持量を示す。
物理GPUメモリー、割当回数、GPU内コピー量とGPU待ち列は対応する計測で確認する。
実表示時刻は`actualDisplayP95Ms: null`で表し、入力から実表示までの受入れと区別する。
表示頻度を変えた反復でも、最終画像の`projectionSha256`を照合する。
ブラシ径の連続変更と楕円の連続変更は、それぞれの固定入力を用いる測定で受け入れる。

`--gpu-timestamps`は診断用のGPU時刻クエリーを有効にする。対応しないGPUは理由を返す。
測定フレームの描画・合成・表示用描画をGPU側の時刻で囲み、結果は全フレームの測定後に
一括で読み取る。記録用の計算処理を追加するため、通常設定の速度比較と分けて実行する。
最大4,096送信フレームまで記録でき、測定した全フレームの正の経過時間を検査する。
JSONの`diagnosticGpuTimestamps`が計測の有無を示し、`completedGpuMs`と`gpuP95Ms`が
GPU自身の処理時間を示す。画面外ではCPU側の完了観測時間とも比較する。
画面提示ではGPU時刻の読取りを測定後へ置き、フレームごとの完了待ちを追加しない。
このGPU時刻は描画命令の処理時間であり、OSが画面に実表示した時刻は別に検証する。

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

パッケージ構築は`scripts/platform/audit-windows-package.py`を実行し、全PEファイルが
x86_64であること、DLLのベース名が一意であること、各インポートが同梱DLLまたは
WindowsのシステムDLLとして解決することを確認する。同梱DLLは製品の探索先である
`bin`から解決する。Visual C++の実行時DLLも同梱依存として検査する。
主要画像形式、Python／PyQt／SIP、G'MIC、FFmpeg／MLTの実行時部品の収録と、
製品実行時に不要な開発・試験用成果物の除外も同じ検査の契約とする。
`scripts/platform/check-release-assets`はWindows ZIPを展開し、公開前と公開後にこの検査を再実行する。

増分構築したQt TestをWindowsへ配置する場合も、実行ファイルを`bin`に置き、
操作定義、入力設定とICCプロファイルを製品と同じ`share/krita`の探索先へ配置する。
Fontconfigの設定と固定フォント本体は、[Windows同梱定義](../../nix/windows/default.nix)と同じ
相対パスへ対応付ける。DLL、Qtプラグイン、資源と固定試験データを含む配置を検証してから試験を実行する。
Qtの画面外基盤向けにも同じ固定フォントを`lib/fonts`へ配置し、`QT_QPA_FONTDIR`でその場所を指定する。
画面を表示しない機能契約は`QT_QPA_PLATFORM=offscreen`、`CreateNoWindow=true`とセッション0で実行する。
入力と実表示の製品受入れは、対象の対話環境で別途検証する。
資源を登録するQt Testは、`KIS_TEST_PREFIX_PATH`に指定した配置先の`lib/kritaplugins`と
`share`を使う。`CMakeCache.txt`を持つ増分構築先は、CMakeが設定したプラグイン・資源の探索先を使う。
公開ノード試験の固定バンドルは、`libs/resources/tests/data/bundles/test1.bundle`から
配置先の`bin/data/test1.bundle`へコピーする。試験は指定した配置先の固定入力を優先し、
通常のソース反復ではソースツリーの同じ入力を使う。公開ノード試験は、製品の`krita.action`・
`kritamenu.action`を読み、操作の有効化から履歴と保存までを検査する。
通常のレイヤーパネル操作は`kritalayerdocker`を試験の依存へ含め、操作対象と選択の通知も実行する。

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
GPU実行ライブラリーは`nix/android/wgpu-native.nix`が共通ソースと固定NDKから構築する。
Rustの対象標準ライブラリーも固定したソースから構築する。ABI、16KiBページ境界、
配置に依存しない共有ライブラリー名とNDK利用側のリンクを依存物の構築時に検査する。
アプリ用依存集合と追加ライブラリー指定が、製品APKと試験APKの探索先を供給する。
Androidの画面プラグインは`nix/android/qt-native-surface.nix`が固定したQt本体に対して
単独で構築する。製品・試験の包装は`LIBREPAINT_ANDROID_NATIVE_SURFACE_PLUGIN`で
この成果物を選び、C++用依存集合と構成識別子を維持する。Qtの更新時は対応するパッチを
確認し、両ABIで画面プラグイン、キャンバス試験APKと製品APKを検査する。
APKとAABの検査は、選択した画面プラグインと包装済みライブラリーのビルド識別子も照合する。
構築は既定で4並列とし、`CMAKE_BUILD_PARALLEL_LEVEL`で並列数を指定できる。

### APKとAAB

```sh
build-incremental android configure
build-incremental android plan krita
build-incremental android build krita
build-incremental android package-product
build-incremental android-x86_64 build krita
```

Qt 6の`androiddeployqt`が`packaging/android/apk`を使い、
`create-apk-krita`と`create-aab-krita`で包装する。増分構築は構築木の
`product-apk-template`へ製品テンプレート、固定したGradle起動スクリプトとQtの
ライブラリー資源定義を配置し、包装時に更新する。`androiddeployqt`はこの書込み可能な
作業用テンプレートを使う。ABIごとの個別環境を初回に開く場合は
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
ELF検査は見出しの出力を最後まで読み取り、検査器の終了状態とABIの一致を確認する。
`scripts/tests/test_android_package_audit.py`は、見出しを分割して出力する検査器の正常終了、
異なるABIの拒否と、見出し出力後の検査器の失敗を`verify-quick`で検査する。

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
試験対象の構築設定を保ったまま、専用の起動部をDebug構成で包装し、Androidの
開発用署名を含むAPKを作る。包装検査はABI、16KiB整列と画面プラグインに加えて、
導入に必要なAPK署名を検査する。
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
構築は既定で4並列とし、`CMAKE_BUILD_PARALLEL_LEVEL`で並列数を指定できる。

GPU実行基盤の依存定義は`nix/ios/wgpu-native.nix`、Rustのコンパイラー・ホスト標準ライブラリー・
標準ライブラリーソースの固定値は`nix/gpu/rust-toolchain.nix`が所有する。
対象標準ライブラリーは固定SDKと最小OSでソースから構築する。依存定義の変更後は増分環境を
一度更新し、通常のソース編集では記録済み環境を再利用する。依存物の構築時に全静的アーカイブ
要素を検査し、C API利用側の最終リンクまで確認する。

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

### 検証の一時生成物

検証開始時に、ソース版・実行環境・必要な反復と原結果の保存先を決める。
各環境・反復の原結果は一組を保持し、環境間比較は既存原本を逐次読み取って差分と照合結果を記録する。
構築計画と依存調査は対象を指定し、コマンド、直接依存、空構築範囲を記録する。

作業単位の終了時に、転送用の圧縮物・展開した複製・旧生成物を原本と照合し、
利用終了と削除権限を確認した対象を除去する。失敗時の入力・原診断・実際の結果は、
原因と影響を再調査できる一組を保持する。未コミットの変更、利用者の成果物、
利用中の主増分構築木と共有キャッシュを保護し、回収量と保持する原本を会話で報告する。

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
