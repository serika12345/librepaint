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
- 9責務の再配置先、移行順、互換経路、依存基準の縮小計画
- シェルスクリプト
- アーキテクチャ文書、リンク、D2、生成済みSVG

### 単一試験

```sh
nix develop .#test --command ./scripts/run-test kis_strokes_queue_test
```

第1引数はCMakeの試験ターゲットである。CTest名を絞る必要がある場合は
第2引数へ正規表現を指定する。

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
選び、構成を同期してから対象だけを構築・実行する。ネイティブ試験は対応する
`build/tdd-<platform>`をアプリケーション接頭辞とし、同じ構築木の`bin`から
製品プラグインを読み込む。

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

macOS、Linux、iOS、Android、Windowsの現在のターゲット、種別、定義場所、直接
リンク依存は`docs/architecture/cmake-targets-<platform>.json`へ同じ形式で記録する。
各再生成コマンドはFile APIの`codemodel-v2`問い合わせを対象の永続構築木へ作成し、
構成を同期して台帳を更新する。macOSとiOSはDarwinホストで実行する。

```sh
nix develop .#test --command \
  ./scripts/architecture/regenerate_cmake_graph.py macos
nix develop .#test --command \
  ./scripts/architecture/regenerate_cmake_graph.py ios
```

Linux、Android、Windowsはx86_64 Linuxホストで実行する。

```sh
for platform in linux android windows; do
  nix develop .#test --command \
    ./scripts/architecture/regenerate_cmake_graph.py "$platform"
done
```

生成した5台帳を一つの変更へ集約した後、共通ターゲット、条件付きターゲット、
構成差を持つターゲットの行列を更新する。

```sh
nix develop .#test --command \
  ./scripts/architecture/regenerate_cmake_graph_matrix.py
```

CMakeターゲットまたは`target_link_libraries`を変更したときは5台帳と差分行列を
同じ変更へ含める。記録済み台帳と各実構成の一致は、同じコミットを指す清浄なDarwin
作業ツリーとx86_64 Linux作業ツリーを用意し、次のコマンドで同時に確認する。

```sh
nix develop .#test --command \
  ./scripts/architecture/verify_cmake_graphs.py \
    --remote-host nixos \
    --remote-repository /path/to/clean/librepaint
```

この入口はmacOSとiOSを手元のDarwinホスト、Linux、Android、WindowsをSSH先の
x86_64 Linuxホストで並行して構成し、5台帳のバイト単位の一致を確認してから差分
行列を確認する。コミットまたは作業ツリーがホスト間で異なる場合は構成開始前に
診断する。

`verify-quick`は固定File API応答を使用し、抽出形式、直接依存の選択、決定的な
整列、差分診断、5台帳と差分行列の形式、同時検証入口のホスト割り当てを検査する。
実構成との一致は上記の全プラットフォーム同時検証で検査する。

## 公開面台帳

`docs/architecture/public-surface-inventory.json`は、公開ヘッダー、主要クラス、
プラグインを所有ターゲットと対応プラットフォームへ接続する。次のコマンドは台帳の
公開ヘッダー集合を製品ソースから再生成する。

```sh
nix develop .#test --command \
  ./scripts/architecture/update_public_header_inventory.py
```

次のコマンドは製品プラグインについて、登録マクロ、兄弟JSONメタデータ、CMake所有者、
5構成の対応状況、サービス種別の機能所有者を再生成する。

```sh
nix develop .#test --command \
  ./scripts/architecture/update_plugin_inventory.py
```

次のコマンドは`libs/ui`直下の公開クラスについて、記録済みの責務分類を保持しながら
宣言と実装単位を更新する。新しい公開クラスには責務分類を追加してから実行する。

```sh
nix develop .#test --command \
  ./scripts/architecture/update_ui_class_responsibilities.py
```

次のコマンドは`libs/ui/tool`以下の公開クラスについて、記録済みの責務分類を保持しながら
宣言、実装単位、ツールディレクトリー外の全利用ソースを更新する。新しい公開クラスには
責務分類を追加してから実行する。

```sh
nix develop .#test --command \
  ./scripts/architecture/update_ui_tool_class_responsibilities.py
```

次のコマンドは台帳の形式と整列に加え、リポジトリおよび5構成のCMakeターゲット台帳を
根拠として内容を検査する。

```sh
nix develop .#test --command \
  ./scripts/architecture/check_public_surface_inventory.py
```

`docs/architecture/package-responsibilities.json`は、記録済みの責務分類を保持しながら、
公開ヘッダー、クラス、プラグインと、20の中核所有ターゲットの直接依存および利用元を
各台帳から再生成する。
全件クラス台帳の範囲外にある共有ターゲット内の実装は、`reviewedSourcePaths`へ
製品ソースの正規パスを記録し、一つの責務へ帰属させる。

```sh
nix develop .#test --command \
  ./scripts/architecture/update_package_responsibility_map.py
```

次のコマンドは、9責務の参照と割当て、5構成の対象ターゲット、生成済みの根拠が正本の
台帳と一致することを検査する。

```sh
nix develop .#test --command \
  ./scripts/architecture/check_package_responsibility_map.py
```

`docs/architecture/allowed-package-dependencies.json`は、9責務の階層、14公開接続面、
接続面単位の許可依存を保持する。次のコマンドは、19の中核所有ターゲット間の直接リンクを
責務候補へ射影し、同一責務内、許可方向、R1-G4で基準化する候補へ分類する。

```sh
nix develop .#test --command \
  ./scripts/architecture/update_allowed_package_dependencies.py
```

次のコマンドは責務参照、公開接続面、階層方向、有向非巡回性と、現在リンクの全分類を
検査する。

```sh
nix develop .#test --command \
  ./scripts/architecture/check_allowed_package_dependencies.py
```

`docs/architecture/dependency-violation-baseline.json`は、許可方向外の現在辺から、直接includeで
一意に帰属できる確認済み違反を記録する。
次のコマンドは、審査済みの最大件数、理由、所有段階、除去条件を保持しながら、CMake辺、
対応構成と直接includeを更新する。

```sh
nix develop .#test --command \
  ./scripts/architecture/update_dependency_violation_baseline.py
```

次のコマンドは、確認済み違反の増加、縮小可能な上限、根拠の置換を
検査する。

```sh
nix develop .#test --command \
  ./scripts/architecture/check_dependency_violation_baseline.py
```

`docs/architecture/structural-dependency-baseline.json`は、共有ターゲット由来の射影解決、
中核および全製品CMakeターゲットの循環、公開宣言を持たないヘッダーのパッケージ外参照を
保持する。次のコマンドは、射影の直接include根拠、5構成の強連結成分、公開面台帳の
外部include根拠を更新する。

```sh
nix develop .#test --command \
  ./scripts/architecture/update_structural_dependency_baseline.py
```

次のコマンドは、新たな未帰属射影、ターゲット循環、内部ヘッダー参照の増加、縮小可能な
上限、根拠の置換を検査する。

```sh
nix develop .#test --command \
  ./scripts/architecture/check_structural_dependency_baseline.py
```

`docs/architecture/package-relocation-plan.json`は、9責務の目標ディレクトリー、名前空間、
CMakeターゲット、8段階の移行順、一時互換経路、各依存基準の縮小上限を保持する。
次のコマンドは、責務地図と許可依存方針、逆方向依存基準、構造依存基準、5構成の
CMakeターゲット台帳へ計画を照合する。

```sh
nix develop .#test --command \
  ./scripts/architecture/check_package_relocation_plan.py
```

検査器は、許可依存の下位側を先に移す順序、現行、新規、実装済みターゲットの区別、
全互換経路のR1-G7削除条件、現在96件の逆方向includeと20件の内部ヘッダー参照の
段階別縮小、最終状態のゼロ上限を確認する。責務、公開接続面、基準、ターゲット台帳を
変更した場合は、先行する台帳を更新してから再配置計画を同じ変更で更新する。

検査は次の関係を確認する。

- `publicHeaderPolicy`が対象ソース、拡張子、試験経路の除外、公開根拠の種類を固定する。
- `kritacanvas`、`kritaimage`、`kritaimpex`、`kritaimpexui`、`kritaui`について、公開マクロまたは
  公開ヘッダー構築契約を持つ製品ヘッダーと、所有元外から直接includeされる製品ヘッダーの
  和集合が、欠落と余分な項目なしで記録されている。
- 所有ターゲットが記録した全プラットフォームに存在し、宣言と実装が所有元の
  ソースディレクトリーに属する。
- 公開根拠が公開マクロの宣言、公開ヘッダー構築契約、または所有元外の全利用ソースによる
  直接includeと一致する。
- 詳細な利用根拠のソースが利用元ターゲットに属し、対象ヘッダーを実際にincludeする。
- 主要クラスの宣言、実装、公開ヘッダー、責務根拠が有効な参照を持つ。
- `libs/ui`直下の公開クラスが欠落なく記録され、宣言種別、対応する実装単位、
  所有ターゲット、6種類の責務領域がソースと一致する。
- `libs/ui/tool`以下の公開クラスが欠落なく記録され、宣言種別、対応する実装単位、
  ディレクトリー外の全利用ソース、所有ターゲット、5種類の責務領域がソースと一致する。
- 製品プラグインが欠落なく記録され、ID、実装、兄弟JSON、登録マクロ、CMake所有者、
  対応構成、サービス種別、機能所有領域、実行時の読込元が一致する。
- JSONのライブラリー名から決まる157件と、CMakeソース所属で所有者を固定する15件が、
  記録済みの所有根拠と一致する。
- パッケージ責務地図の9責務が、UIクラス領域、UIツールクラス領域、主要クラス、
  プラグイン機能所有領域を各一回割り当てる。
- UIクラス全件分類の範囲外にある審査済み公開ヘッダーが、一つの責務へ割り当てられる。
- 20の中核所有ターゲットが5構成のいずれかに存在し、定義場所、種別、製品ターゲットへの
  直接依存、製品ターゲットからの利用元がCMakeターゲット台帳の和集合と一致する。
- 全プラグイン登録を所有する発見機構が一つ存在し、各機能責務のプラグインIDと
  サービス種別が公開面台帳の機能所有領域と一致する。
- 9責務の14公開接続面が目的、寿命、エラー動作を持ち、許可依存が既知の接続面を参照する。
- 許可依存が自己参照を持たず、常に下位層へ向かい、有向非巡回グラフを形成する。
- 20ターゲット間の55リンクが、共有ターゲットの全責務を含む107候補へ欠落なく射影され、
  同一責務内16候補、許可方向67候補、R1-G4で基準化する24候補へ分類される。
- 許可方向外の責務対のうち現在の3責務対が96件の直接includeへ一意に帰属し、元の
  CMakeターゲット辺と5構成へ接続される。
- 確認済み3責務対が理由、所有段階、除去条件、現在件数と等しい審査済み上限を持つ。
- 残る9責務対が直接includeの実責務へ帰属し、逆方向依存ではない射影として解決される。
- 20中核ターゲットと全製品構築ターゲットが、5構成すべてで強連結成分0件を維持する。
- 公開宣言を持たない`kritaui`の7ヘッダー、20件のパッケージ外参照が、所有段階、理由、除去条件、
  ヘッダー数と参照数の審査済み上限を持つ。
- 9責務の目標パッケージと8移行段階が許可依存の下位から上位へ並び、各新規ターゲット、
  互換経路、逆方向依存、内部ヘッダー参照を一度だけ処理して最終上限をゼロにする。

公開ヘッダーの追加、削除、公開マクロ、またはパッケージ外includeを変更した場合は更新器を
実行し、台帳を同じ変更へ含める。`libs/ui`直下の公開クラス、その実装ファイル、または
責務分類を変更した場合はUIクラス責務台帳の更新器も実行する。共有ターゲット内にあり
全件クラス台帳の範囲外となるソースの責務を変更した場合は、`reviewedSourcePaths`も更新する。
`libs/ui/tool`以下の公開
クラス、実装ファイル、ディレクトリー外の直接include、または責務分類を変更した場合は
UIツールクラス責務台帳の更新器も実行する。プラグインの登録実装、JSON、CMake所有者、
サービス種別を変更した場合はプラグイン台帳の更新器も実行する。責務、所有ソース、
所有ターゲット、公開面またはCMake直接依存を変更した場合は、先行する該当台帳に続けて
パッケージ責務地図と許可依存方針の更新器を順に実行する。責務層、公開接続面、許可依存を
変更した場合は方針を編集してから許可依存方針の更新器を実行する。対象ソース、include、
公開ヘッダー、クラス責務またはCMake直接依存を変更した場合は、先行する台帳更新後に依存違反
基準の更新器を実行する。現在件数が変わった場合は`maximumDirectIncludes`を同じ変更で審査し、
縮小時は現在件数まで下げる。続けて構造依存基準の更新器を実行し、内部ヘッダーの
`maximumHeaders`と`maximumDirectReferences`を現在値へ縮小する。責務の目標配置、
移行順、作成ターゲット、互換経路、または段階別上限が変わる場合は再配置計画も更新する。
`verify-quick`は8更新器の`--check`、台帳検査器、再配置計画検査器、欠落、所有者、
公開根拠、利用根拠、責務分類、ターゲット関係、依存方向、循環、未分類候補、基準拡大、
縮小可能な上限の診断例を実行する。
`scope.publicHeaders`、`scope.plugins`、UI直下クラス、UIツールクラスは全件であり、
`scope.majorClasses`は3件の詳細な代表記録を維持する。

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
