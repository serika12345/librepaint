# LibrePaintアーキテクチャガイド

## 目的

この文書は、変更内容から調査対象を絞り、LibrePaintの主要な設計境界と実行経路を把握するための入口です。設計判断に使う責務、経路、識別子を中心にまとめています。

全プラットフォーム共通の長期改造計画は[LibrePaint全面改造TODO](TODO.md)で管理します。
開発・検証コマンドは[LibrePaint開発・検証基盤](DEVELOPMENT.md)、現在の再開地点は
[LibrePaintアーキテクチャ作業状況](PROGRESS.md)を正本とします。

- 変更を共通コード、プラグイン、プラットフォーム統合、配布定義のどこへ置くか
- 起動、描画、ファイル入出力がどの境界を通るか
- CMakeターゲット、実行時プラグイン、Nix出力の違い
- 調査時に最初に読むファイルと、その次に追う識別子

図は責務と主要な実行経路を示します。実際のリンク境界は各ディレクトリーの
`CMakeLists.txt`を正本とします。

## 最初の30分で読む順序

1. [ルートのCMakeLists.txt](../../CMakeLists.txt)末尾で、`libs`、`qmlmodules`、`plugins`、`krita`の構成順とiOS条件を確認します。
2. [libs/CMakeLists.txt](../../libs/CMakeLists.txt)と[plugins/CMakeLists.txt](../../plugins/CMakeLists.txt)で、常時リンクするライブラリーと機能単位のプラグインを分けます。
3. [krita/CMakeLists.txt](../../krita/CMakeLists.txt)で実行形式、Qtリソース、OS別ソース、静的プラグインの最終リンクを確認します。
4. [krita/main.cc](../../krita/main.cc)から`KisApplication::start()`を追い、[KisApplication.cpp](../../libs/ui/KisApplication.cpp)でグローバル状態、プラグイン、リソース、メインウィンドウの初期化順を確認します。
5. [KisDocument.h](../../libs/ui/KisDocument.h)と[kis_image.h](../../libs/image/kis_image.h)を読み、文書の寿命・入出力と、画像モデル・描画スケジューラーを分けて捉えます。
6. 対象機能を[変更内容から見る場所](#変更内容から見る場所)で引き、近傍の`CMakeLists.txt`、プラグインJSON、テストまで範囲を広げます。
7. 配布や依存関係の変更では、[flake.nix](../../flake.nix)を入口に、該当する`nix/<platform>/`と`packaging/<platform>/`を読みます。

## 全体構造

![LibrePaintのコードアーキテクチャ](code-architecture.svg)

図の編集元は[code-architecture.d2](code-architecture.d2)です。

各プラットフォームのCMake構成で有効な明示的ビルドターゲットは、次の台帳に
同じ形式で記録する。

| プラットフォーム | CMakeターゲット台帳 | 構築プロファイル |
| --- | --- | --- |
| macOS | [cmake-targets-macos.json](cmake-targets-macos.json) | `tdd-macos` |
| Linux | [cmake-targets-linux.json](cmake-targets-linux.json) | `tdd-linux` |
| iOS | [cmake-targets-ios.json](cmake-targets-ios.json) | `ios-device-incremental` |
| Android | [cmake-targets-android.json](cmake-targets-android.json) | `android-arm64-v8a-incremental` |
| Windows | [cmake-targets-windows.json](cmake-targets-windows.json) | `windows-x86_64-incremental` |

各台帳はターゲット名、種別、定義元のソースディレクトリー、CMakeターゲットへの
直接リンク依存を持つ。[全プラットフォーム差分行列](cmake-target-matrix.json)は、
5構成のターゲット数、共通ターゲット、条件付きターゲット、構成ごとに定義が異なる
ターゲットを一つの決定的なJSONへまとめる。CMakeが生成する補助ターゲットは除外し、
外部ライブラリーのパスや構築ディレクトリーを記録しないため、同じFile API応答から
同じJSONを再生成できる。翻訳処理が絶対パスから生成する`pofiles-<hash>`と
`tsfiles-<hash>`も補助ターゲットとして除外する。

台帳の`dependencies`はCMake codemodelの`linkLibraries`が示す直接リンク対象である。
リンクコマンドへファイルパスやフラグとして入る項目と、推移的な構築順依存は含まない。
この範囲により、R1の責務地図と許可依存規則は明示されたターゲット間リンクを入力に
できる。

### 公開面台帳

[公開面台帳](public-surface-inventory.json)は、公開ヘッダー、主要クラス、プラグインを
5構成のCMakeターゲットへ接続する。`publicHeaderSets`は、公開マクロを使用するヘッダーと、
所有元の外にある製品ソースから直接includeされるヘッダーの和集合を記録する。各集合は
所有ターゲット、公開マクロ、対応プラットフォーム、全利用ソースを持つ。

R1-G2bでは`kritaimage`の332件と`kritaui`の307件を全件記録し、`scope.publicHeaders`を
`complete`とする。試験と性能測定だけで共有するヘッダーは製品パッケージ間の公開面を
表さないため、候補と利用ソースの採取から除外する。`publicHeaderPolicy`が対象拡張子、
製品ソースディレクトリー、除外する試験経路、公開根拠の種類を固定する。

`publicHeaderDetails`は`KisDocument`、`KisImage`、`KisImportExportManager`について、
利用元CMakeターゲットと責務の代表記録を持つ。`scope`は公開ヘッダー、主要クラス、
プラグインの完了状態を個別に示す。

R1-G2eでは、試験経路を除く`plugins`以下で兄弟JSONを指定する登録マクロ172件を全件記録し、
`scope.plugins`を`complete`とする。各項目はプラグインID、実装、メタデータ、登録マクロ、
CMake所有ターゲット、対応構成、サービス種別、機能所有領域、実行時レジストリーを持つ。
構成別の対象数はmacOS 167件、Linux 170件、iOS 162件、Android 162件、Windows 168件である。

`pluginServiceTypeOwners`は14サービス種別を、入出力、画像フィルター、ドッカー表示、
表示拡張、ツール呼出し、描画実行、画像生成、文書メタデータ、色処理、ベクター図形、
プラットフォーム接続、アプリケーション拡張の12領域と実行時の読込元へ対応付ける。
157件はJSONの`X-KDE-Library`をCMake所有者の根拠とする。残る15件は同項目がないか、
5構成のターゲット名と一致しないため、`pluginPolicy.ownerTargetOverrides`が登録実装を含む
`CMakeLists.txt`と実際のターゲットを記録する。

[UI直下クラス責務台帳](ui-class-responsibilities.json)は、`libs/ui`直下の公開ヘッダーに
宣言された現存クラスと構造体106件を、宣言、実装単位、所有ターゲット、5構成、責務領域へ
接続する。104件は実装単位を持ち、2件は宣言側で完結する。責務領域はアプリケーション調整、
キャンバス・表示、文書状態、入出力、ツール呼出し、ウィンドウ・作業空間の6種類である。
描画設定表示として`libs/tools/ui`へ移動したクラスは更新時に台帳から除く。

この台帳は`libs/ui`直下の公開クラスを全件対象とする。
[UIツールクラス責務台帳](ui-tool-class-responsibilities.json)は、同じ公開ヘッダー集合の
`libs/ui/tool`以下を再帰的に調べ、公開クラスと構造体50件を記録する。対象40ヘッダー、
名前に対応する実装単位を持つ48件、宣言側で完結する2件を、入力解釈7件、ツール呼出し
14件、ストローク生成9件、描画実行12件、設定表示8件へ分類する。

各ツールクラスは宣言、実装単位、`kritaui`所有者、5構成に加え、`libs/ui/tool`の外から
対象ヘッダーを直接includeする製品ソースへ接続する。利用元は102ソースで、同じヘッダーに
複数の公開クラスがある場合は各クラスが同じ利用元集合を持つ。この分類は画面表示、入力、
ストローク作成、描画実行が一つのCMakeターゲットに混在する現在の境界を示し、R1-G3の
責務地図と依存方針の入力になる。

### 現在のパッケージ責務地図

[パッケージ責務地図](package-responsibilities.json)は、R1-G1の5構成のCMakeターゲット台帳と
R1-G2の公開面台帳、UIクラス責務台帳、UIツールクラス責務台帳を、現在の9責務へ接続する。
各責務は説明、所有ソースディレクトリー、所有ターゲット、公開ヘッダー、公開クラス、
プラグインID、サービス種別を持つ。

`reviewedPublicHeaderPaths`は、全件分類済みのUI直下またはUIツールクラスの範囲外にある
公開ヘッダーを、根拠を確認した責務へ一意に割り当てる。現在は
`libs/ui/widgets/KoStrokeConfigWidget.h`を、図形線の設定表示とキャンバス選択状態を扱う
`canvas-presentation`へ割り当てる。

`reviewedSourcePaths`は、共有ターゲット内に残る表示実装のうち、全件クラス台帳の範囲外に
あるソースを実責務へ一意に割り当てる。描画設定表示を利用するパレット、レイヤー設定、
プリセット編集の7ソースは`tool-invocation`へ割り当て、共有ターゲットの保守的な射影を
実際のinclude元で解決する。

| 責務ID | 現在の中核所有ターゲット | 対象 |
| --- | --- | --- |
| `application-orchestration` | `krita`、`kritaui` | 起動、OSライフサイクル、アプリケーション、ウィンドウ、作業空間 |
| `canvas-presentation` | `kritabasicflakes`、`kritaflake`、`kritaui` | キャンバス表示、ベクター表示、ドッカー |
| `document-lifecycle` | `kritacommand`、`kritametadata`、`kritaui` | 文書寿命、変更状態、アンドゥ、メタデータ |
| `import-export` | `kritaimpex`、`kritaui` | 形式選択、検証、文書入出力 |
| `input-interpretation` | `kritaui` | ポインター、キーボード、タッチ、タブレット、ショートカット入力 |
| `painting-rendering` | `kritacolor`、`kritaimage`、`kritalibbrush`、`kritapigment` | 色、ブラシ、画像、投影、ストローク、描画処理 |
| `plugin-infrastructure` | `kritaplugin` | メタデータ探索、ファクトリーとサービス種別の登録 |
| `resource-management` | `kritaresources`、`kritaresourcestorage`、`kritaresourceui` | リソースの保存、検索、タグ、選択、表示 |
| `tool-invocation` | `kritatoolsui`、`kritaui` | 描画設定表示とキャンバス状態へのツール呼出し |

`targetRelations`は16の中核所有ターゲットについて、5構成に存在する種別と、製品CMake
ターゲット間の直接依存および利用元を和集合で記録する。この地図は現在の所有関係を表し、
R1-G3bで定義する許可依存方向の比較元になる。

`kritaui`は9責務中6責務の現所有ターゲットである。UIクラスの責務分類と組み合わせることで、
文書、入出力、入力解釈、描画実行などを凝集したターゲットへ分割する順序を決められる。
`plugin-infrastructure`は全172登録の発見機構を所有し、各機能責務は同じ登録を機能領域として
参照する。機構の所有と機能の所有を、この二つの軸で表現する。

### 許可依存方向

[許可依存方針](allowed-package-dependencies.json)は、9責務を8層へ配置し、上位層から下位層の
公開接続面へ向かう直接依存だけを許可する。各公開接続面は目的、寿命、エラー動作を持ち、
依存元は利用する接続面IDを指定する。同層または上位層への辺を認めない階層規則により、
許可グラフは有向非巡回になる。

| 層 | 責務 | 許可する下位責務 |
| --- | --- | --- |
| 7 | `application-orchestration` | キャンバス、文書、入出力、入力、プラグイン基盤、リソース、ツール |
| 6 | `input-interpretation` | キャンバス、ツール |
| 5 | `tool-invocation` | キャンバス、文書、描画、リソース |
| 4 | `document-lifecycle` | 入出力、描画、プラグイン基盤、リソース |
| 3 | `canvas-presentation` | 描画、リソース |
| 3 | `import-export` | 描画、プラグイン基盤、リソース |
| 2 | `painting-rendering` | プラグイン基盤、リソース |
| 1 | `resource-management` | プラグイン基盤 |
| 0 | `plugin-infrastructure` | — |

公開接続面はアプリケーション寿命、キャンバス表示、文書セッションと永続化、形式選択と
直列化、入力動作列、画像モデルと描画実行、プラグイン登録、リソース台帳と保存、
ツール命令と設定表示の14種類である。プラグイン機能は`plugin-registration`へ依存し、
検証済みファクトリーを明示された機能レジストリーへ登録する。このリンク方向と、登録時に
機能レジストリーへ渡る制御を区別する。

`currentTargetEdges`は16の中核所有ターゲット間にある35の直接リンクを責務へ射影する。
`kritaui`のような共有ターゲットは、所有する全責務の直積として保守的に扱う。現在は85候補の
うち9候補が同一責務内、52候補が許可方向、24候補が`requires-r1-g4-baseline`である。
最後の分類は共有ターゲットが作る曖昧な候補を含むため、R1-G4で実際のincludeと利用箇所を
根拠に既存違反基準へ確定する。

### 確認済み逆方向依存の基準

[依存違反基準](dependency-violation-baseline.json)は、許可方向外の責務対を製品ソースの
直接includeへ照合し、一意に責務へ帰属できる現在の5責務対を確認済み違反として記録する。
各項目は元のCMakeターゲット辺と5構成、全include根拠、審査済みの最大件数、所有する
ロードマップ段階、現在必要な理由、除去条件を持つ。

| 依存元 | 依存先 | 直接include上限 | 主な現在境界 |
| --- | --- | ---: | --- |
| アプリケーション調整 | 描画 | 70 | `kritaui`内の起動・共有サービスから画像、ブラシ、色処理 |
| キャンバス表示 | 文書寿命 | 73 | `kritaflake`から文書側に分類した共通アンドゥ命令 |
| 入力解釈 | 描画 | 15 | 入力処理から描画情報とストローク状態 |
| 入力解釈 | リソース管理 | 4 | 入力処理からプリセットとリソース状態 |
| 描画 | 文書寿命 | 95 | 画像・色処理からアンドゥ命令とメタデータ |

採取器は各対象ターゲットの記録済みソースディレクトリー以下から製品ソースを読み、試験経路を
除外する。includeは依存先ヘッダーのパス末尾、またはリポジトリ内で一意なヘッダー名により
解決する。責務は単一所有ターゲット、分類済み公開クラス、最長一致する責務ディレクトリーの
順に決める。全件クラス台帳の範囲外にある審査済みソースは`reviewedSourcePaths`を根拠に
一意に帰属する。この規則で257件の直接includeが現在の確認済み基準になる。R1-G6aは、
描画から入出力への2件、リソース管理から入出力への5件、リソース管理から描画への40件を
解消し、アプリケーション調整から描画への上限を70件へ縮小した。

共有ターゲットの保守的射影から生じた残り8責務対は、構造違反基準が実際の
includeを全件帰属させる。`unresolvedProjections`は空であり、確認済み違反の上限には
根拠のある現在の5責務対だけを含める。

検査では現在件数が審査済み上限を超える変更を基準拡大として診断する。現在件数が減った場合も
上限を同じ変更で縮小するまで診断する。根拠の置換は件数が同じでも生成差分として現れる。

### 構造違反と循環の基準

[構造依存基準](structural-dependency-baseline.json)は、共有ターゲット由来の射影解決、
CMakeターゲット循環、公開宣言を持たないヘッダーのパッケージ外参照を一つの継続検査へ
接続する。

8射影はすべて`disproved-by-direct-include-attribution`として解決済みである。
`kritabasicflakes`から`kritaui`への3includeは、キャンバス表示内または許可済みの
リソース管理への依存である。`kritaui`から`kritaimpex`への4includeは、入出力責務内の
依存である。保存実装の移動後に残るリソース管理から入出力への保守的射影も、実際の
直接includeがないことを固定する。各解決は元のターゲット辺、5構成、実際の責務対、ソース、include、
ヘッダーを記録し、新たな未帰属候補を診断する。

ターゲット循環は、16の中核所有ターゲットと、試験経路を除く全製品構築ターゲットの
2範囲を検査する。全製品範囲はmacOS 216件、Linux 222件、iOS 208件、Android 208件、
Windows 225件であり、現在の非自明な強連結成分は両範囲、全構成で0件である。
`maximumComponents`を0に固定し、新しい直接リンク循環を基準拡大として診断する。

公開面台帳で`external-include`だけを公開根拠とし、所有元外から参照されるヘッダーを、
宣言済み公開面へ移す対象として基準化する。現在は`kritaimage`が29ヘッダー、593参照、
`kritaui`が15ヘッダー、34参照で、合計44ヘッダー、627参照である。所有段階、理由、
除去条件、ヘッダー数と参照数の審査済み上限を保持し、増加と縮小可能な上限の両方を
診断する。

### パッケージ再配置計画

[パッケージ再配置計画](package-relocation-plan.json)は、現在の9責務を目標ディレクトリー、
C++名前空間、主CMakeターゲット、許可依存、移行段階へ対応付ける。既存の凝集した
実装ターゲットは保持し、新しい責務接続面と表示部分だけを独立ターゲットとして追加する。

| 責務 | 目標ディレクトリー | 新しいAPIの名前空間 | 主ターゲット | 移行段階 |
| --- | --- | --- | --- | --- |
| プラグイン基盤 | `libs/koplugin` | `Krita::Plugin` | `kritaplugin` | 現行境界を保持 |
| リソース管理 | `libs/resources` | `Krita::Resources` | `kritaresources` | R1-G6a |
| 描画 | `libs/painting` | `Krita::Painting` | `kritapainting` | R1-G6b |
| 入出力 | `libs/impex` | `Krita::ImportExport` | `kritaimpex` | R1-G6c |
| キャンバス表示 | `libs/canvas` | `Krita::Canvas` | `kritacanvas` | R1-G6d |
| 文書寿命 | `libs/document` | `Krita::Document` | `kritadocument` | R1-G6e |
| ツール呼出し | `libs/tools` | `Krita::Tools` | `kritatools` | R1-G6f |
| 入力解釈 | `libs/input` | `Krita::Input` | `kritainput` | R1-G6g |
| アプリケーション調整 | `libs/application` | `Krita::Application` | `kritaapplication` | R1-G6h |

移行は許可依存の下位から上位へ進める。各段階は必要な特性試験、移動元と移動先、
作成ターゲット、一時互換経路、完了条件、中止条件を持つ。確認済み逆方向includeの
段階別上限は304、257、162、162、89、89、89、70、0と縮小する。内部ヘッダーの
直接参照は、R1-G6bで`kritaimage`の593件を解消し、R1-G6bからR1-G6hで
`kritaui`の34件を32、28、20、20、3、3、0へ縮小する。

最初の実装段階R1-G6aは、`libs/store`の書庫保存を`libs/resources/storage`の
`kritaresourcestorage`へ、XML直列化を`libs/serialization/xml`の
`kritaxmlserialization`へ分け、`libs/resourcewidgets`を`libs/resources/ui`へ移す。
同時に`libs/ui`と`libs/ui/widgets`の描画設定表示を`libs/tools/ui`へ分離し、描画から
入出力2件、リソースから入出力5件、リソースから描画40件の逆方向includeをゼロにする。
保存領域の読込、書込、取消し、不正アーカイブ、リソース検索、表示接続の契約が、
この段階の着手条件と完了判定になる。

保存境界と表示境界は実装済みである。`kritaresourcestorage`がZIPとディレクトリーの保存契約を所有し、
`kritaxmlserialization`がXML名前空間と逐次書出しを所有する。実利用元は必要なターゲットへ
直接リンクする。`libs/store`、`kritastore`、転送ヘッダーは存在しない。移行計画の
`implemented`状態と5構成のCMake台帳が保存ターゲットの存在を継続検査する。両ターゲットは
LibrePaint内の上位製品ターゲットへ依存せず、保存側はQt Core、KConfig、QuaZip、XML側はQt Coreを
利用する。`kritaresourceui`は型付きリソース記述子と汎用の選択・タグ表示を所有し、
`kritatoolsui`はパレット、合成方法、プリセット、描画設定の表示を所有する。旧
`libs/resourcewidgets`、旧ターゲット、転送ヘッダーは存在しない。

残る10の一時互換経路は後続UI再配置用の旧include、`kritaui`、既存の大域C++識別子を含む。
各経路は導入段階、R1-G7の所有者、最大範囲、削除条件、検証方法を持つ。計画検査は
9責務と5構成の現行ターゲット、5種類257件の逆方向依存、44ヘッダー627件の内部参照を
正本へ照合し、全基準と一時経路が最終状態でゼロになることを確認する。

責務の中心は次の四つです。

- `krita/`はプロセスの入口、アプリケーション資産、OSライフサイクルとの接続を持ちます。主要機能は`libs/`と`plugins/`が所有します。
- `libs/ui`の`kritaui`はアプリケーション、ウィンドウ、文書、キャンバス、入力、ツール共通部をまとめます。
- `libs/image`の`kritaimage`はレイヤーツリー、ペイントデバイス、タイル、ストロークキュー、投影更新を扱います。
- `plugins/`はツール、ブラシエンジン、フィルター、ドッカー、ファイル形式などの機能をレジストリーへ登録します。

`libs/ui`は画面機能と、`KisDocument`、入出力管理、ツール共通処理などの
アプリケーション調整を扱います。`KisImage`は画像内容と非同期処理を中心に扱い、
ウィンドウとファイル名は`libs/ui`側が所有します。

## 主要な設計境界

### プロセス入口とアプリケーション初期化

通常のOSでは[krita/main.cc](../../krita/main.cc)の`main`が入口です。Windowsでは[krita/windows_stub_main.cpp](../../krita/windows_stub_main.cpp)の小さな実行形式が、共有ライブラリー側の`krita_main`を呼びます。`krita_main`の実装本体はどちらも`main.cc`です。

`KisApplication::start()`は、おおむね次の順で初期化します。

1. グローバルなファクトリーと設定
2. リソース型
3. プラグインが登録する各レジストリー
4. リソースデータベースと同梱リソース
5. `KisPart`、セッション、`KisMainWindow`
6. 自動保存の復旧と起動引数の文書

iOSのライフサイクル、メモリー警告、Pencilダブルタップは`KisIOS*.mm`から`main.cc`へ通知されます。タッチ向け画面は[plugins/extensions/iostouchui](../../plugins/extensions/iostouchui)にあり、OS通知の橋渡しと画面機能を分離しています。

### 文書と画像モデル

`KisDocument`はファイルパス、変更状態、自動保存、読み込み・保存、`KisImage`の差し替えを管理します。`KisImage`は次を所有します。

- `KisNode`を基底とするレイヤー・マスクのツリー
- `KisPaintDevice`と`tiles3/`の画素タイル
- 合成結果である投影
- `KisUpdateScheduler`、`KisStrokesQueue`、更新キュー
- アンドゥ可能なストロークと画像変更通知

画像状態だけで完結する処理は`libs/image`側、ファイル名やダイアログ、
ウィンドウと連携する処理は`libs/ui`側から検討します。

### プラグインとレジストリー

プラグインはJSONメタデータのサービス種別、ID、対応MIME型などで発見され、コンストラクターからレジストリーへファクトリーを登録します。[KoJsonTrader.cpp](../../libs/koplugin/KoJsonTrader.cpp)が候補を列挙し、[KoPluginLoader.cpp](../../libs/koplugin/KoPluginLoader.cpp)が重複版と無効化設定を処理します。

デスクトップでは`lib/kritaplugins`などから動的に読み込みます。iOSでは
`kis_add_library`が`MODULE`を静的ライブラリーへ変換し、
`krita_ios_target_static_plugins`が実行形式へ登録・リンクします。組み込む対象の
正本は[initial-plugin-profile.json](../../packaging/ios/manifests/initial-plugin-profile.json)です。

機能を追加するときは、C++クラスと次の識別子を一組として確認します。

- 近傍の`CMakeLists.txt`にあるターゲット名
- `K_PLUGIN_FACTORY_WITH_JSON`などが参照するJSON
- `Id`、`X-KDE-ServiceTypes`、MIME型
- アクションIDと`*.action`／XMLGUI定義
- iOSへ含める場合は静的プラグインプロファイル

### Qtリソースとインストール資産

[krita/krita.qrc](../../krita/krita.qrc)は、`kritarc`と`krita5.xmlgui`をQtリソースへ割り当てる小さな目録です。アプリ全体のQtリソース一覧は[krita/CMakeLists.txt](../../krita/CMakeLists.txt)の`krita_QRCS`にあります。アイコン、シェーダー、カーソル、スプラッシュ、既定プリセットなどはそこから実行形式へ組み込まれます。

`install(FILES|DIRECTORY ...)`で配置する資産はQtリソースとは別です。特に`krita/data`、`pics`、`po`、プラグインJSON、バンドル資産を変更するときは、実行時参照方法がリソースURLかインストール先パスかを先に確認します。

## 実行時の主要経路

![描画とファイル入出力の実行経路](runtime-paths.svg)

図の編集元は[runtime-paths.d2](runtime-paths.d2)です。

### 描画

自由描画を追う場合の基準経路は次のとおりです。

1. Qtのポインター／タブレット／タッチイベントを`KisInputManager`がショートカットと入力アクションへ振り分けます。
2. `KisToolInvocationAction`と`KoToolManager`が現在のツールへイベントを渡します。
3. `KisToolFreehand`と`KisToolFreehandHelper`が入力点、筆圧、傾き、プリセットの状態を`FreehandStrokeStrategy`のジョブへ変換します。
4. `KisImage::startStroke/addJob/endStroke`が`KisUpdateScheduler`と`KisStrokesQueue`へ処理を渡します。
5. 選択中の`KisPaintOp`が`KisPaintDevice`のタイルを更新します。
6. dirty領域から投影更新が計画され、`KisCanvas2`へ更新通知が戻ります。

入力の不具合はイベント受信から、ブラシ結果の不具合は`KisPaintOp`から、並列実行・アンドゥ・再描画の不具合はストローク戦略とスケジューラーから調べます。

### ファイル入出力

`KisDocument`は`KisImportExportManager`へ処理を委譲します。管理クラスは`Krita/FileFilter`プラグインをMIME型で選び、`KisImportExportFilter::convert()`を呼びます。

- KRAやORAのようなコンテナー形式では`libs/resources/storage`の`KoStore`がZIP／ディレクトリー抽象化を提供します。
- XML名前空間と逐次書出しは`libs/serialization/xml`が提供します。
- 画像形式固有の符号化、設定画面、依存ライブラリー接続は`plugins/impex/<format>/`に置きます。
- インポート後の共通検査、非同期エクスポート、警告、原子的保存の扱いは`KisImportExportManager`側にあります。
- iOS／Androidの文書選択や内容URIの差は、Qtのファイル機構とプラットフォーム条件を通して共通の`KisDocument`経路へ合流します。

## 変更内容から見る場所

| 変更内容 | 最初に見る場所 | 次に確認する境界 |
| --- | --- | --- |
| 起動順、引数、単一起動 | `krita/main.cc`、`libs/ui/KisApplication.*` | `KisPart`、`KisMainWindow`、OS条件 |
| Windowsの実行形式だけに関係する起動 | `krita/windows_stub_main.cpp`、`krita/CMakeLists.txt` | DLLの`krita_main`、配布ツリー |
| iOSライフサイクル、Pencil、メモリー警告 | `krita/KisIOS*.mm`、`krita/main.cc` | `plugins/extensions/iostouchui`、iOS検証文書 |
| メニュー、ショートカット、アクション | `krita/krita.action`、`krita/krita5.xmlgui`、対象`KisViewManager`機能 | アクションID、プラグイン`*.action` |
| Qtリソースの追加 | `krita/krita.qrc`、`krita/CMakeLists.txt`の`krita_QRCS` | リソースURL、`Q_INIT_RESOURCE`、iOS静的資産 |
| ウィンドウ、ドッカー、キャンバス画面 | `libs/ui`、`plugins/dockers` | `KisMainWindow`、`KisViewManager`、`KisCanvas2` |
| 入力割り当て、ジェスチャー | `libs/ui/input` | 現在ツール、Qtプラットフォームイベント、OS統合 |
| ツールの操作 | `plugins/tools` | `libs/ui/tool`、`KoToolRegistry`、アクション |
| ブラシエンジンやプリセット | `plugins/paintops`、`libs/brush` | `libs/ui/tool/strokes`、`libs/resources`、`libs/pigment` |
| レイヤー、マスク、画素、投影 | `libs/image` | `KisNode`、`KisPaintDevice`、`KisUpdateScheduler` |
| アンドゥ、非同期処理 | `libs/command`、`libs/image/commands*`、`libs/image/kis_strokes_queue.*` | ストローク戦略の順序・排他属性 |
| 色空間、プロファイル、合成 | `libs/pigment`、`libs/color`、`plugins/color` | LittleCMS、OpenColorIO、表示変換 |
| ベクター図形、選択図形 | `libs/flake`、`libs/basicflakes`、`plugins/flake` | `libs/ui/flake`、SVG入出力 |
| ブラシ等のリソース管理 | `libs/resources`、`libs/resources/ui` | リソースDB、ローダーレジストリー、同梱バンドル、選択・タグ表示 |
| 描画設定表示 | `libs/tools/ui` | パレット、合成方法、プリセット、描画設定の表示 |
| KRA内部構造、ZIPストレージ | `plugins/impex/libkra`、`plugins/impex/kra`、`libs/resources/storage` | `KisDocument`、メタデータ、XML直列化 |
| PNG、PSD、RAW等の形式 | `plugins/impex/<format>` | `KisImportExportManager`、プラグインJSON、Nix依存 |
| 外部操作API、スクリプト公開面 | `libs/libkis`、`plugins/python` | ABI/API互換性、Python/PyQtを含む配布対象 |
| QML部品 | `qmlmodules` | Qt Quickの有効条件、iOSプロファイル |
| 共通ビルド条件 | ルート`CMakeLists.txt`、対象ディレクトリーの`CMakeLists.txt` | CMakeオプション、ターゲットの公開依存 |
| OS別依存関係とアプリビルド | `flake.nix`、`nix/<platform>/` | 依存関係出力、ソースビルド、ランタイム組立 |
| 署名、アーカイブ、端末配備 | `packaging/<platform>/` | Nix出力との受け渡し、認証情報を使う外部段階 |
| ブランド、アイコン、配布メタデータ | `krita/pics/branding`、`krita/CMakeLists.txt`、`packaging` | 安定識別子、MIME／UTI、各OSのバンドル情報 |

## ディレクトリーの責務

| パス | 主な責務 |
| --- | --- |
| `krita/` | 実行形式、起動、アプリ資産、OS別のプロセス統合 |
| `libs/global`、`libs/widgetutils`、`libs/widgets` | 共通基盤、Qt補助部品、再利用画面部品 |
| `libs/ui` | アプリケーション調整、文書、ウィンドウ、キャンバス、入力、ツール共通部 |
| `libs/image` | 画像・ノード・画素タイル・投影・ストローク・更新処理 |
| `libs/brush`、`libs/pigment`、`libs/color` | ブラシ資産、色空間、色変換・合成の基盤 |
| `libs/flake`、`libs/basicflakes` | ベクター図形、キャンバス、図形ツールの基盤 |
| `libs/resources`、`libs/resources/ui` | リソース永続化、検索、タグ、バンドルと汎用管理画面 |
| `libs/tools/ui` | 描画ツールの設定、パレット、プリセットの表示 |
| `libs/resources/storage`、`libs/serialization/xml` | コンテナーI/OとXML直列化 |
| `libs/metadata`、`libs/psd*` | メタデータとPSD共通実装 |
| `libs/koplugin` | プラグイン探索とメタデータ照会 |
| `libs/impex` | 入出力フィルターの共通契約と書き出し前検査 |
| `libs/libkis` | 外部APIとスクリプト向けの公開ラッパー |
| `plugins/` | 実行時に登録する機能実装 |
| `qmlmodules/` | Qt Quick向けの再利用部品 |
| `nix/` | 再現可能な依存関係、アプリビルド、ランタイム組立 |
| `packaging/` | アーカイブ、署名、配布物、端末配備 |
| `cmake/` | 検出モジュール、構成マクロ、プラットフォーム検査 |

## ビルドと配布の構造

![Nixビルドと配布の構造](build-architecture.svg)

図の編集元は[build-architecture.d2](build-architecture.d2)です。

[flake.nix](../../flake.nix)は出力名とパッケージ集合を接続し、具体的なレシピを`nix/<platform>/`へ委譲します。保守時は次の三段階を分けます。

1. 外部依存関係
2. LibrePaintソースをコンパイルするアプリケーション
3. ランタイム組立、アーカイブ、署名、配備

LinuxとWindowsでは依存関係出力をソースビルドから分離しています。LinuxのAppImage、WindowsのZIP、iOSのIPAは完成済みアプリケーションへ重ねる最終段階です。iOSはさらに、外部ライブラリーを個別のNix派生物として構築し、固定したXcode／SDK契約を検査します。Appleの署名、AltStoreへのインストール、端末操作は認証情報と外部状態を扱うため`packaging/ios`側に残ります。

## 調査と設計判断の手順

### 1. 実行時の所有者を決める

現象を「プロセス」「文書」「画像」「プラグイン機能」「資産」「配布物」のどれが所有するか分類します。所有者が不明な場合は、公開クラス名より先に呼び出し経路を`rg`で追います。

### 2. 構築時と実行時の境界を分ける

`CMakeLists.txt`はコンパイル・リンク・インストールの関係を決めます。プラグインJSONとレジストリーは実行時の発見と選択を決めます。Nixはそのターゲットへ与える外部依存関係と成果物の組立を決めます。同じ機能でも三つすべてに変更が必要な場合があります。

### 3. 共通実装を先に検討する

描画、文書、ファイル形式、画面動作の共通処理は`libs`または`plugins`を所有者に
します。OSのライフサイクル、ネイティブファイル選択、入力API、署名・配備との
接続をプラットフォーム境界へ置くと、デスクトップとモバイルで同じ処理経路を
検証できます。

### 4. 安定識別子を確認する

KRA MIME／UTI、設定ディレクトリー、CMakeターゲット、プラグインID、
アクションID、デスクトップIDには互換性上の意味があります。変更時は参照元、
移行方法、互換性試験を一組で扱います。

### 5. 影響に比例した検証を選ぶ

| 変更範囲 | 最低限の検証 |
| --- | --- |
| 文書と図のみ | `nix develop .#docs --command scripts/docs/check-architecture.sh` |
| CMake／Nix評価 | `nix flake check --no-build --all-systems` |
| ライブラリー内部 | 対象ディレクトリーの単体試験と該当プラットフォームの開発シェル |
| プラグイン | 登録確認、対象機能の操作、該当形式なら往復試験 |
| 入力・描画 | 押下・移動・解放、アンドゥ、投影更新、対象デバイス |
| 配布定義 | 名前付き`nix build`出力、成果物検査、対象OSでの起動 |
| iOS静的プロファイル | プラグイン目録、最終リンク、IPA検査、実機の対象操作 |

## この文書と図の保守

文書用の全ツールは`nix develop .#docs`にあります。図の生成元は
`docs/architecture/*.d2`で、SVGはレビューと通常のMarkdown表示のために
追跡します。図の変更はD2の生成元へ加え、SVGを再生成します。

`nix develop .#docs --command scripts/docs/render-architecture.sh`

文書、リンク、D2構文、生成済みSVGの一致をまとめて確認します。

`nix develop .#docs --command scripts/docs/check-architecture.sh`

新しい主要境界を追加した場合は、全体構造、変更内容から見る場所、該当する
実行経路の三か所が整合するように更新します。プラットフォーム固有の詳細手順は
`docs/<platform>/`または`packaging/<platform>/`の文書を正本にします。
