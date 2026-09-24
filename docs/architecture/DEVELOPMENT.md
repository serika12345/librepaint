# LibrePaint開発・検証基盤

## 目的

この文書は、LibrePaintのアーキテクチャ改造を、リポジトリに記録した
作業順序、Nix開発環境、検査条件を使って再現するための利用手順を定める。
運用規則の正本はルートの`AGENTS.md`、改造順序と完了条件の正本は
`docs/architecture/TODO.md`、現在の再開地点の正本は
`docs/architecture/PROGRESS.md`とする。

## 文書と状態の構成

| 文書 | 所有する情報 |
| --- | --- |
| `AGENTS.md` | エージェントの作業順序、品質契約、必須検証 |
| `docs/architecture/TODO.md` | 全プラットフォーム共通の段階、検査段階、完了条件 |
| `docs/architecture/PROGRESS.md` | 現在の作業一件だけの状態と次の操作 |
| `docs/architecture/README.md` | 安定した責務、実行経路、調査入口 |
| `docs/<platform>/` | OS固有の設計、構築、実機検証 |

作業完了時は、完了した事実をTODO、試験、成果物へ反映し、`PROGRESS.md`を
次の作業を指す現在状態へ更新する。

## 開発環境

初回にリポジトリルートでdirenvの構成を承認する。

```sh
direnv allow
```

以後はリポジトリへ移動すると`.envrc`がNixの`test`シェルを読み込み、`scripts/`を
`PATH`へ追加する。環境にはCMake、Ninja、CTest、運用検査、`ccache`と次の入口が
含まれる。

```sh
build-incremental native build
run-test kis_strokes_queue_test
verify-quick
```

Nixシェルを最初に直接開く場合は同じ`test`属性を使用する。文書と図だけを扱う周期には
軽量な`docs`シェルを使用する。この評価は作業セッションの開始時に一度行い、以後の
ソース編集、対象構築、試験、検査、対象単位コミットは同じシェル内で実行する。

```sh
nix develop .#test
nix develop .#docs
```

### 評価済みNix環境の再利用

direnvが読み込まれたシェルでは、`build-incremental`、`run-test`、`verify-quick`、`verify`を
直接実行する。自動処理、別プロセス、主作業ツリー、担当作業ツリーのうち、現在のシェル環境を
継承しない実行は、主作業ツリーの安定した`.direnv/flake-profile`を次の入口から読み込む。

```sh
./scripts/run-shared-test-env ./scripts/build-incremental native build <target>
./scripts/run-shared-test-env ./scripts/run-test <target> [ctest-regex]
./scripts/run-shared-test-env ./scripts/verify-quick
```

`run-shared-test-env`は評価済みプロファイルを`nix print-dev-env`で読み込み、現在の作業ツリー、
専用Ninja木、共有コンパイラーキャッシュを維持する。番号付きの
`.direnv/flake-profile-*-link`を呼出し側で選ばず、安定した`.direnv/flake-profile`を入口とする。

`nix develop .#test`、`nix develop .#docs`、`direnv exec .`によるローカルFlake評価は、初回の
プロファイル作成、または開発シェル、Flake入力、`flake.lock`、ソース絞り込みの変更後に限定する。
通常のC++、CMake、試験、文書の反復ごとには実行しない。ローカルFlakeは追跡済み作業ツリーを
不変の`*-source`として保存し、アプリケーション構築は`cleanSourceWith`から追加の
`*-librepaint-source`を生成するため、変更状態ごとの再評価は同じ大きさのソース世代を累積させる。

大きなロードマップ項目で意図的にローカルFlakeを再評価する前後は、死んだソース世代の件数と
回収可能量を比較する。

```sh
nix-store --gc --print-dead \
  | rg -- '-(source|librepaint-source)$' \
  | wc -l

nix-store --gc --print-dead \
  | rg -- '-(source|librepaint-source)$' \
  | xargs nix path-info --json -S \
  | jq 'map(.narSize // 0) | add'
```

通常のソース反復で件数が増えた場合は、追加のローカルFlake評価を停止し、最後に成功した
`.direnv/flake-profile`へ戻す。原因、増加件数、回収可能量、再開入口を`PROGRESS.md`へ記録する。
ごみ収集は環境再利用とは別の保守操作として、対象の死んだパス、利用中のプロファイル、主増分構築木、
共有キャッシュを確認し、削除権限を得てから行う。

### 全プラットフォーム共通の増分構築入口

`build-incremental`はプラットフォーム、操作、任意のCMakeターゲットを受け取る。

```sh
build-incremental <platform> <operation> [target]
```

| 操作 | 成果 |
| --- | --- |
| `path` | 現在の依存構成が選ぶ永続Ninja木を表示する |
| `configure` | ソースとCMake構成をNinja木へ同期する |
| `plan [target]` | Ninjaが実行する処理を表示する |
| `build [target]` | 変更分と必要な依存ターゲットを構築する |
| `bootstrap [target]` | 同じNinja木へ最初の構築基準を作る |
| `cache-stats` | 対象プラットフォームのコンパイラーキャッシュ統計を表示する |

ネイティブの`plan`は最初に`build.ninja`だけを対象としてglob検査と必要なCMake再生成を同期し、
同期済みmanifestの一時ハードリンクを別名で読み込んで対象の実作業を乾式表示する。この処理は
コンパイルとリンクを実行せず、一時manifestを成功・失敗の両方で削除する。

`native`はホストをmacOSまたはLinuxへ対応付ける。iOSはDarwinホスト、Androidと
Windowsクロス構築はx86_64 Linuxホストを使用する。

```sh
build-incremental native plan krita
build-incremental native build krita
build-incremental ios plan
build-incremental ios build
build-incremental android build krita
build-incremental android-x86_64 build KisCurveOptionModelTest
build-incremental windows build krita
```

AndroidのQt TestはABIに対応する共有ライブラリーとして構築し、選択した一対象とその推移的な
実行時ライブラリーを標準APKへ収める。`android`は`arm64-v8a`実機用、
`android-x86_64`はx86_64実機およびWaydroid診断用であり、依存物固定表、Ninja木、
構成指紋、コンパイラーキャッシュ、APKをABIごとに分離する。

```sh
build-incremental android-x86_64 package-test KisCurveOptionModelTest
adb connect <Waydroidまたは実機の接続先>
build-incremental android-x86_64 run-test KisCurveOptionModelTest [adb-serial]
```

`run-test`は標準ADBで接続先のABIを検査し、APKの導入、QtActivity経由の実行、
xUnit XMLとlogcatの回収、強制停止、パッケージ削除を行う。結果は選択されたNinja木の
`test-results/<target>/`へ保存する。WaydroidはAndroid診断のADB接続先として使用し、
APKと実行入口はx86_64 Android実機でも共通である。対象の`data/`ディレクトリーはAPKの
assetsへ収め、ActivityがAndroid標準のアプリ専用外部領域へ展開する。ネイティブ試験はその
領域を作業ディレクトリーとして使うため、構築ホストの絶対パスや端末固有の共有パスを必要としない。

### 実装前の構築範囲監査

コードまたは試験契約を編集する前に、変更対象の増分計画、直接CMake依存、空の構築木で必要になる
コマンド閉包を確認する。既存対象では、変更のない状態の`plan`が対象本体を再構築しないことを確認する。
新規対象、ソース追加、直接依存追加では、同じ責務を扱う最も近い既存契約と閉包を比較する。

```sh
build-incremental native plan <target>
ninja -C "$(build-incremental native path)" -t commands <target> | wc -l
```

CMakeの対象定義とFile API応答で直接依存を照合する。対象にアプリケーション実行形式、全プラグイン集合、
`all`、試験が利用しないUI所有者が入る場合は、挙動実装より先に試験対象または製品責務を分離する。
具体的所有者の公開処理を直接検査するため閉包を縮小できない場合は、直接依存、閉包工程数、縮小に必要な
製品分割を`PROGRESS.md`へ記録する。実装後は対象指定構築で、変更したソース、自動生成、リンク以外の
不要な再構築が発生していないことを確認する。

### 公開ヘッダーの変更波及測定

公開ヘッダーを軽量化する変更では、同期済みのNinja構築木を指定して変更前後を同じ入口で測定する。

```sh
./scripts/architecture/measure_change_impact.py \
    "$(build-incremental native path)" \
    libs/global/KoID.h \
    --platform macos \
    --owner-target kritaglobal \
    --contract-target KoIDContractTest
```

この入口は、ソース上の直接取込み、`ninja -t deps`の推移的なコンパイル工程、
`compile_commands.json`の翻訳単位、CMake File APIの対象と直接辺、契約対象の空構築コマンド閉包を
一つの報告へまとめる。製品、試験、生成ソース、AUTOMOC、直接リンク対象、再リンク閉包を分け、
古い依存記録とコンパイル以外の補助依存記録を集計から除外する。`--json`は同じ内容を機械可読形式で出力する。

構築木は変更なしの`plan`と対象構築が成功した状態を使用する。レビュー可能な一変更について変更前後の
値を`PROGRESS.md`へ記録し、リポジトリ全体の順位表は現在状態の参考値として扱う。構築時間と
コンパイラーキャッシュは、同じ測定の前後で`time`と`build-incremental <platform> cache-stats`から
記録する。

| プラットフォーム | 永続構築木 | コンパイラーキャッシュ |
| --- | --- | --- |
| macOS | `build/tdd-macos` | `.cache/librepaint/ccache/native` |
| Linux | `build/tdd-linux` | `.cache/librepaint/ccache/native` |
| iOS | `build-ios/krita/device-incremental/<構成指紋>` | `.cache/librepaint/ccache/ios` |
| Android ARM64 | `build/android/arm64-v8a/<構成指紋>` | `.cache/librepaint/ccache/android` |
| Android x86_64 | `build/android/x86_64/<構成指紋>` | `.cache/librepaint/ccache/android-x86_64` |
| Windows | `build/windows/x86_64/<構成指紋>/ninja` | `.cache/librepaint/ccache/windows` |

ネイティブの`configure`、`plan`、`build`、`bootstrap`は、ホスト用Ninja木の
`compile_commands.json`をリポジトリ直下の同名シンボリックリンクへ同期する。
VS Codeのclangdはこのリンクから実際のコンパイル条件を読み込む。高速検査は、
`.clang-tidy`をNix開発環境のclangツールで検証する。

依存ライブラリーはNix storeとバイナリーキャッシュから供給する。iOS、Android、
Windowsは、依存定義の指紋ごとに`build/nix-profiles/`または`build-ios/nix-profiles/`
へソース非依存の開発環境を固定する。通常のC++編集は同じNinja木と`ccache`を
再利用し、依存定義の変更は新しい構成指紋の木を選ぶ。

Windowsのクロス構築は`build/windows/.../source`へ変更分を同期し、Nix版と共通の
Windows準備処理を適用する。診断と生成物は同じ構成指紋のNinja木に蓄積する。

アプリケーション束、APK、Windows可搬版、IPAの配布検査は、対応するNix出力を
クリーンな検査地点として構築する。依存出力とバイナリーキャッシュは日常の増分
周期とクリーンな検査地点で共有する。

```sh
nix build .#librepaint-macos
nix build .#librepaint-linux
nix build .#librepaint-ios-ipa
nix build .#librepaint-android
nix build .#librepaint-android-x86_64
nix build .#librepaint-windows-archive
```

## 検証階層

### 高速検査

```sh
nix develop .#test --command ./scripts/verify-quick
```

次を検査する。

- UTF-8、制御文字、双方向書式文字のテキスト契約
- 運用検査スクリプト自身の単体試験
- 10責務の所有と許可依存方針
- 現在の公開ヘッダーとプラグイン登録
- 契約試験への型特性、コンパイル時形状検査、完全署名別名の混入と、互換性試験の根拠記載
- シェルスクリプト
- アーキテクチャ文書、リンク、D2、生成済みSVG

### 単一試験

```sh
nix develop .#test --command ./scripts/run-test kis_strokes_queue_test
```

第1引数はCMakeの試験ターゲットである。CTest名を絞る必要がある場合は
第2引数へ正規表現を指定する。`run-test`は`build-incremental native build`
へ対象名を渡し、構成指紋が有効な間は永続Ninja木の再構成を省略する。

書庫保存境界とXML直列化境界は、次の二つの契約で検査する。

```sh
nix develop .#test --command \
  ./scripts/run-test TestResourceStorageArchiveContract
nix develop .#test --command \
  ./scripts/run-test TestXmlWriter
```

リソース選択表示と描画設定表示は、次の契約で個別に検査する。

```sh
nix develop .#test --command \
  ./scripts/run-test TestResourceUiContract
nix develop .#test --command \
  ./scripts/run-test TestToolSettingsUiContract
```

```sh
nix develop .#test --command \
  ./scripts/run-test kis_strokes_queue_test '^libs-image-kis_strokes_queue_test$'
```

スクリプトはホストOSに対応する`CMakePresets.json`の試験プリセットを
選び、必要な場合に構成を同期してから対象と宣言済み依存だけを構築・実行する。ネイティブ試験は対応する
`build/tdd-<platform>`をアプリケーション接頭辞とし、同じ構築木の`bin`から
製品プラグインを読み込む。試験が動的に探索する製品プラグインは、試験実行形式の
CMake依存として具体的なモジュールターゲットを接続する。試験の対象構築に
アプリケーション実行形式、全プラグイン集合、`all`を接続しない。

Ninjaが`premature end of file; recovering`を報告し、変更のない対象を再コンパイルする場合は、
コンパイラーキャッシュではなく永続構築木の`.ninja_deps`破損を確認する。対象構築を停止し、
`.ninja_deps`と`.ninja_log`を退避してから、Nix開発環境のNinjaで記録を再圧縮する。

```sh
cp -p build/tdd-macos/.ninja_deps build/tdd-macos/.ninja_deps.backup
cp -p build/tdd-macos/.ninja_log build/tdd-macos/.ninja_log.backup
nix develop .#test --command ninja -C build/tdd-macos -t recompact
```

再圧縮直後の対象構築は不足した依存記録を一度再生成する。続く同一対象の構築で
`ninja: no work to do.`になることを確認し、退避物は確認完了後に構築木の保守対象から外す。

### 全ネイティブ検査

```sh
nix develop .#test --command ./scripts/verify
```

高速検査に続いて、ネイティブ試験構成の全ターゲットを構築し、登録済みの
通常CTestを実行する。対象コンポーネントが限定できるレッド・グリーン周期
では単一試験を使用し、統合前または広い共有境界の変更で全検査を使用する。
全体検証も`build-incremental native build`を使用し、有効な構成指紋と永続Ninja木を
再利用してから全CTestを実行する。

### Nix評価

Nix出力、開発シェル、依存関係、ソース絞り込みを変更した場合は、全出力を
構築する前に評価を検査する。

```sh
nix flake check --no-build --all-systems
```

運用検査だけをNixの独立した検査として構築する場合は、ホストに対応する
属性を使用する。

```sh
nix build --no-link .#checks.aarch64-darwin.governance
nix build --no-link .#checks.x86_64-linux.governance
```

この検査は専用の軽量ソースだけで完結し、LibrePaint本体とアプリケーション束の
既存構築結果を再利用する。

## CMake試験プリセット

| ホスト | 構成・構築・CTestプリセット | 構築ディレクトリー |
| --- | --- | --- |
| macOS | `tdd-macos` | `build/tdd-macos` |
| Linux | `tdd-linux` | `build/tdd-linux` |

両構成は`BUILD_TESTING=ON`、`KRITA_ENABLE_BROKEN_TESTS=OFF`、
`LIMIT_LONG_TESTS=ON`、`CRASH_ON_SAFE_ASSERTS=ON`を使用する。各BROKEN試験の
原因、決定性、比較規則を確認し、通常検査へ復旧する。

## パッケージ境界と公開契約

高速検査は、手動で保守する最小の方針と現在の製品ソースを検査する。

```sh
nix develop .#test --command \
  python3 scripts/architecture/check_package_boundaries.py
nix develop .#test --command \
  python3 scripts/architecture/check_public_contracts.py
```

`docs/architecture/package-boundaries.json`は10責務、27の中核所有ターゲット、責務間で
許可する直接リンク方向を保持する。責務や所有ターゲットを変更するときは、この方針を
同じ変更で更新する。高速検査は所有の一意性、参照整合性、許可方向の非循環性を確認する。

公開契約の検査は製品ソースを直接走査し、所有パッケージの外から利用されるヘッダーの
公開マクロまたは公開ヘッダー構築契約を確認する。プラグインについては、登録マクロ、
兄弟JSON、ID、サービス種別、CMake所有者の対応を確認する。公開ヘッダーまたは
プラグイン登録を変更したときに更新する生成台帳はない。

### 既存テストの保守

対象責務の呼び出し側、仕様、試験コード、CMake定義を読み、利用者が観測する保証を特定する。
振る舞いを検証する試験を維持し、互換性検査には明示的な要件の根拠を持たせる。
宣言形状や内部構造だけを固定する検証は、必要な意味論の検証状況を確認して削除する。
不足する振る舞いだけを公開操作と観測結果による試験で補う。
契約試験を追加するときは、利用者、操作、観測結果、失敗時に利用者へ生じる不具合を先に定める。
型特性、コンパイル時形状検査、完全署名別名は振る舞い契約へ追加しない。
ソース、ABI、保存形式、プラグイン、スクリプトの互換性を宣言形状で守る場合は、試験名を
`CompatibilityTest`とし、`// Compatibility requirement:`に利用者と維持対象を記載する。
`scripts/architecture/check_test_contracts.py`はこの区別を高速検査で確認する。

変更前に対象の増分構築計画と直接依存を確認し、新設・拡張対象では空構築閉包も測定する。
変更後は対象試験、影響範囲のCTest、`verify-quick`を実行する。
レビュー説明に、保証する結果、削除する制約の理由、検証結果を記す。
`PROGRESS.md`には現在の対象、次の作業、検証状況を記録する。

### 責務単位の並列実装

並列実装を行うときは、一人の統合担当と最大三つの実装担当を使う。統合担当は
現在の作業範囲から、公開ヘッダー、製品実装、試験ソース、CMake定義が重ならない責務単位を選ぶ。
同じ試験ディレクトリーの単一`CMakeLists.txt`、同じ製品集約対象、同じ公開クラスを必要とする
作業は一つの担当へまとめる。これにより、実装中の差分とCMake再生成を担当単位で判断できる。

統合担当は、各担当を開始する前に次の担当票を確定する。担当識別子は`[a-z0-9-]+`とし、
基準コミットは全担当で同じ値を使用する。

```text
担当識別子:
基準コミット:
作業ツリー絶対パス:
目的と観測する挙動:
対象公開ヘッダー:
対象public API識別子:
変更許可パス:
担当CMakeファイルと対象:
最も近い既存契約:
対象プラットフォーム:
共有コンパイラーキャッシュ:
構築実行許可: waiting | granted
Git操作権限: uncommitted | transport-commit
追加委任: forbidden | authorized
統合順:
固有の停止条件:
```

統合担当は担当票と`preparing`、`implementing`、`ready`、`blocked`、`integrated`の状態を、
実装担当の起動前に`docs/architecture/PROGRESS.md`の現在スナップショットへ記録する。再開時は
Git作業ツリー、担当ブランチ、基準コミット、許可パスを実体と照合してから各担当を継続する。
実装担当は担当票を現在作業の範囲として扱い、統合担当の次の操作を選び直さない。追加の
エージェントへの委任は、担当票の`追加委任`が`authorized`の場合だけ行う。

`AGENTS.md`、`docs/architecture/TODO.md`、`docs/architecture/PROGRESS.md`、
`docs/architecture/README.md`、`docs/architecture/DEVELOPMENT.md`は統合担当が所有する。
運用検査の共通処理も、担当票で明示的に移管した場合だけ実装担当が変更する。
この中央所有により、現在の再開地点を一つの順序で更新できる。

明示的なブランチ作成権限がある場合、統合担当は基準コミットから担当ごとのGit作業ツリーを
作成する。作業ツリーはリポジトリーの兄弟ディレクトリーとし、既存パスがないことを確認して
から追加する。

```sh
task_primary_root="$(pwd -P)"
task_base_commit="$(git rev-parse HEAD)"
task_lane="<担当識別子>"
task_worktree="$(dirname "$task_primary_root")/librepaint-work-$task_lane"
test ! -e "$task_worktree"
git worktree add -b "work/$task_lane" \
  "$task_worktree" "$task_base_commit"
```

各作業ツリーは自身の`build/tdd-macos`または`build/tdd-linux`を使用する。CMakeキャッシュは
ソース絶対パスを保持するため、別の作業ツリーからNinja木を複製して使用する構成にはしない。
コンパイラーキャッシュだけを共有し、対象限定構築の再コンパイルを削減する。共有先の絶対パスは
担当票から受け取る。主作業ツリーで許可済みの`.direnv`開発環境は`run-shared-test-env`で再利用し、
担当作業ツリーのリポジトリールート、Ninja木、コンパイラーキャッシュ基準パスを保つ。

```sh
task_shared_ccache="<共有コンパイラーキャッシュの絶対パス>"
LIBREPAINT_SHARED_CCACHE="$task_shared_ccache" \
  ./scripts/run-shared-test-env \
  ./scripts/run-test <target> [ctest-regex]
```

担当作業ツリーを入力とする`nix develop .#test`は、担当ごとの完全なソース写像をNix storeへ
追加するため、並列の通常実装周期には使用しない。Nix開発環境自体を変更する作業は担当票で
主作業ツリーを所有し、その変更後の環境を検査する。`run-shared-test-env`はGit共通ディレクトリーから
主作業ツリーを決定し、主`.direnv`の評価済み環境と共有コンパイラーキャッシュだけを再利用する。

実装担当は、変更前の計画、直接CMake依存、空構築閉包を測定してから担当範囲を編集する。
構築実行許可が`waiting`の間は、ソース調査、既存試験監査、契約設計を進める。統合担当が
`granted`を通知した後に、担当の作業ツリーで構成、構築、CTest、反復実行、高速検査を行う。
Linux検証はLinux担当票を受けた担当だけが`ssh nixos`の実機で実行し、接続不能時はmacOS結果と
未実施理由を引渡しに記録する。

実装担当は中央所有ファイルを変更せず、完了時に次の情報を統合担当へ返す。

```text
状態: ready | blocked
基準コミットと担当先端:
変更パス:
構造変更の移動元と移動先:
固定した挙動と分類:
利用者から観測する結果と試験の対応:
期待した最初の診断:
変更前後の計画、直接依存、コマンド数、入力数:
対象CTest、反復、影響範囲、高速検査の結果:
未実施プラットフォームと理由:
残る危険と次の操作:
```

`Git操作権限`が`transport-commit`の場合、実装担当は担当票の許可パスだけを一つの引渡しコミットにまとめる。
`uncommitted`の場合は担当作業ツリーを未コミットのまま保持し、`ready`または`blocked`で止める。
引渡しコミットは統合後の文書と検証結果を含まないため、`develop`へ直接入れる完成変更ではない。

統合担当は担当の差分が許可パス内に収まることと、基準コミット以後の統合済み変更との非重複を
確認する。準備済みの担当を一つずつ現在の統合作業ツリーへ取り込み、README、TODO、
PROGRESSの必要な更新を同じ差分へ追加する。その後、対象CTest、必要な隣接CTest、
`verify-quick`を実行する。一つの担当を一つのレビュー可能な変更として
完了してから、次の担当を統合する。

担当は、許可パス外の変更、別担当との重複、新しい公開API、未割当て依存、巨大な構築閉包、
分類できない現行挙動を発見した時点で停止する。統合担当は担当票を分け直すか、構造改善だけの
先行担当を作成する。

輸送コミットを統合した担当作業ツリーは、担当処理の終了、作業木clean、輸送ブランチによる履歴保持を
確認した直後に除去する。`git worktree remove --force <絶対パス>`は担当作業ツリー内の無視対象Ninja木も
除去するため、対象を`git worktree list`の登録済み絶対パスへ限定する。未統合または未コミットの担当は
作業ツリーを保持する。作業ブランチ削除は、履歴の保存要否を確認する別の保守操作として扱う。

統合担当は、統合試験の成功後に不要となった生成物を削除する。
担当構築木をゴミ箱へ移した場合も対象経路を確認して容量を回収する。
`PROGRESS.md`に継続利用する生成物と回収した生成物を記録する。
利用中の主増分構築木、共有キャッシュ、利用者所有の成果物は保持する。

CMake構成を変更したときは、対象プラットフォームの構成入口を実行する。

```sh
build-incremental native configure
build-incremental ios configure
build-incremental linux configure
build-incremental android configure
build-incremental windows configure
```

各入口は、CMake File APIの`codemodel-v2`問い合わせを永続構築木へ作成してから構成し、
生成直後の応答に対して次を検査する。

- 方針にある中核所有ターゲットが存在する。
- 中核所有ターゲットの直接リンクが許可した責務方向へ向かう。
- 試験ターゲットを除く全製品ターゲットが循環を持たない。

`native`はmacOSまたはLinuxの現在ホスト、`ios`はDarwinホスト、
`android`と`windows`はx86_64 Linuxホストで実行する。構成時の応答をその場で検査するため、
ホスト間で生成JSONを同期する保守作業は発生しない。

## テスト駆動開発

観測可能な変更は次の周期で進める。

1. 観測可能な契約を表す最小の試験を追加する。
2. 初回実行で期待する差分と診断を記録する。
3. 契約を満たす最小の製品変更を実装する。
4. 試験が成功したまま責務と名前を整理する。
5. 対象コンポーネントと高速検査を実行する。

画像または描画の特性試験では、キャンバス、色空間、ブラシ、入力点、乱数種、
スレッド条件と比較方法を試験データに記録する。基準画像の受け入れ時は、差分を
維持する契約、既知不具合、設計課題のどれに分類したかを同じ変更で記録する。

## テキスト表現

テキスト契約は、タブ、改行、通常の内容文字、翻訳で使うjoiner、左書き・右書き
mark、ファイル先頭のUTF-8 BOMを扱う。検査はASCII制御文字と、表示順へ作用する
双方向の埋め込み、上書き、分離文字を診断する。

## 作業の再開と保守

新しい作業セッションでは、最初に次を実行する。

```sh
git status --short --branch
sed -n '1,240p' docs/architecture/PROGRESS.md
```

`PROGRESS.md`の`次の操作`が現状と一致する場合は、その一件だけを進める。作業を
開始したら状態を`in_progress`にし、終了時は次の作業を指す`planned`、または
理由と再開条件を持つ`paused`／`blocked`へ更新する。

運用基盤を変更するときは、検査スクリプトの受理例と診断例の単体試験を先に更新し、
この文書のコマンド、Nixシェル、実装を同じ変更で一致させる。

運用文書は目的、責務、入力、出力、実行順、成功状態を肯定形で記述する。移行時の
調査結果は変更報告とリポジトリ履歴が所有し、`PROGRESS.md`は現在の作業と次の操作を
所有する。
