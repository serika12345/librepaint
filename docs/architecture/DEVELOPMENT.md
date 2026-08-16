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

Nixシェルを直接開く場合は同じ`test`属性を使用する。文書と図だけを扱う周期には
軽量な`docs`シェルを使用する。

```sh
nix develop .#test
nix develop .#docs
```

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

`native`はホストをmacOSまたはLinuxへ対応付ける。iOSはDarwinホスト、Androidと
Windowsクロス構築はx86_64 Linuxホストを使用する。

```sh
build-incremental native plan krita
build-incremental native build krita
build-incremental ios plan
build-incremental ios build
build-incremental android build krita
build-incremental windows build krita
```

| プラットフォーム | 永続構築木 | コンパイラーキャッシュ |
| --- | --- | --- |
| macOS | `build/tdd-macos` | `.cache/librepaint/ccache/native` |
| Linux | `build/tdd-linux` | `.cache/librepaint/ccache/native` |
| iOS | `build-ios/krita/device-incremental/<構成指紋>` | `.cache/librepaint/ccache/ios` |
| Android | `build/android/arm64-v8a/<構成指紋>` | `.cache/librepaint/ccache/android` |
| Windows | `build/windows/x86_64/<構成指紋>/ninja` | `.cache/librepaint/ccache/windows` |

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
nix build .#librepaint-windows-archive
```

## 検証階層

### 高速検査

```sh
nix develop .#test --command ./scripts/verify-quick
```

次を検査する。

- 製品ソースの行数契約と例外
- UTF-8、制御文字、双方向書式文字のテキスト契約
- 運用検査スクリプト自身の単体試験
- シェルスクリプト
- アーキテクチャ文書、リンク、D2、生成済みSVG

### 単一試験

```sh
nix develop .#test --command ./scripts/run-test kis_strokes_queue_test
```

第1引数はCMakeの試験ターゲットである。CTest名を絞る必要がある場合は
第2引数へ正規表現を指定する。

```sh
nix develop .#test --command \
  ./scripts/run-test kis_strokes_queue_test '^libs-image-kis_strokes_queue_test$'
```

スクリプトはホストOSに対応する`CMakePresets.json`の試験プリセットを
選び、構成を同期してから対象だけを構築・実行する。

### 全ネイティブ検査

```sh
nix develop .#test --command ./scripts/verify
```

高速検査に続いて、ネイティブ試験構成の全ターゲットを構築し、登録済みの
通常CTestを実行する。対象コンポーネントが限定できるレッド・グリーン周期
では単一試験を使用し、統合前または広い共有境界の変更で全検査を使用する。

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

## CMakeターゲット台帳

macOSの現在のターゲット、種別、定義場所、直接リンク依存は
`docs/architecture/cmake-targets-macos.json`へ記録する。次のコマンドはFile APIの
`codemodel-v2`問い合わせを`build/tdd-macos`へ作成し、構成を同期して台帳を更新する。

```sh
nix develop .#test --command \
  ./scripts/architecture/regenerate_cmake_graph.py macos
```

CMakeターゲットまたは`target_link_libraries`を変更したときは、台帳を再生成して
同じ変更へ含める。記録済み台帳とのバイト単位の一致は次のコマンドで確認する。

```sh
nix develop .#test --command \
  ./scripts/architecture/regenerate_cmake_graph.py macos --check
```

`verify-quick`は固定File API応答を使用し、抽出形式、直接依存の選択、決定的な
整列、差分診断と、記録済みmacOS台帳の主要入口を検査する。実際のmacOS構成との
一致は上記の`--check`で検査する。

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

## ソース行数基準

`docs/architecture/source-size-baseline.json`は、運用基盤導入時点で1,000行を
超えていた製品ソースの最大行数を記録する。高速検査は、各ファイルを1,000行の
標準最大値、記録済み最大値、審査済み例外から該当する契約へ対応付ける。

ファイル縮小時は記録値も縮小し、1,000行以下では基準項目を完了扱いとして
削除する。新しい例外は、理由、対応TODO、削除条件、最大行数を`exceptions`へ
記録する。

現状を意図的に再採取する保守コマンドは次のとおりである。

```sh
nix develop .#test --command \
  ./scripts/check_governance.py --update-source-size-baseline
```

R1またはR6で責務分割の基準を再設定する審査時に、このコマンドで現状を採取する。

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
