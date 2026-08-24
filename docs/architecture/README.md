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

現在は`kritacanvas`の18件、`kritadocument`の5件、`kritadocumentfiles`の3件、
`kritadocumentui`の6件、`kritaimage`の334件、`kritaimpex`の12件、
`kritaimpexui`の23件、`kritapainting`の19件、`kritatools`の15件、
`kritaui`の234件を全件記録し、
`scope.publicHeaders`を`complete`とする。入出力領域は
`libs/impex`直下の形式・検査契約と、`libs/impex/ui`および`libs/impex/animation`の
文書・利用者接続を別の公開集合として採取する。試験と性能測定だけで共有するヘッダーは製品パッケージ間の公開面を
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
宣言された現存クラスと構造体80件を、宣言、実装単位、所有ターゲット、5構成、責務領域へ
接続する。78件は実装単位を持ち、2件は宣言側で完結する。責務領域はアプリケーション調整、
キャンバス・表示、文書状態、ツール呼出し、ウィンドウ・作業空間の5種類である。
描画設定表示と入出力として所有先へ移動したクラスは更新時に台帳から除く。

この台帳は`libs/ui`直下の公開クラスを全件対象とする。
[文書境界評価](document-boundary-assessment.json)は、文書状態に分類された残る20クラスと
`KisDocument.cpp`の129メソッド定義を全件対象とし、現在の関心、具体的な所有先、
後続検査段階へ接続する。宣言と実装の経路はクラス責務台帳を正本とする。検査器は
クラス責務台帳との一致、全メソッドの一度限りの分類、
再配置計画に存在する責務、ディレクトリー、ターゲット、検査段階を確認する。
[UIツールクラス責務台帳](ui-tool-class-responsibilities.json)は、同じ公開ヘッダー集合の
`libs/ui/tool`以下を再帰的に調べ、移設後もUI所有に残る公開クラスと構造体22件を記録する。
対象18ヘッダーの全22件が名前に対応する実装単位を持ち、入力解釈7件、ツール呼出し8件、
ストローク生成3件、描画実行2件、設定表示2件へ分類する。

各ツールクラスは宣言、実装単位、`kritaui`所有者、5構成に加え、`libs/ui/tool`の外から
対象ヘッダーを直接includeする製品ソースへ接続する。利用元は51ソースで、同じヘッダーに
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
あるソースを実責務へ一意に割り当てる。描画設定表示、ツールプロキシー、選択ツール配線を
担う14ソースは`tool-invocation`へ割り当て、共有ターゲットの保守的な射影を実際の
include元で解決する。

| 責務ID | 現在の中核所有ターゲット | 対象 |
| --- | --- | --- |
| `application-orchestration` | `krita`、`kritaui` | 起動、OSライフサイクル、アプリケーション、ウィンドウ、作業空間 |
| `canvas-presentation` | `kritabasicflakes`、`kritacanvas`、`kritaflake`、`kritaui` | 座標変換、キャンバス表示、ベクター表示、ドッカー |
| `document-lifecycle` | `kritadocument`、`kritadocumentfiles`、`kritadocumentui`、`kritaui` | 文書寿命、変更状態、保存用ファイル、取り消し履歴、文書調整 |
| `import-export` | `kritaimpex`、`kritaimpexui` | 形式選択、検証、文書入出力、利用者への結果通知 |
| `input-interpretation` | `kritaui` | ポインター、キーボード、タッチ、タブレット、ショートカット入力 |
| `painting-rendering` | `kritacolor`、`kritaimage`、`kritalibbrush`、`kritapainting`、`kritapaintingmetadata`、`kritapaintingundo`、`kritapigment` | 色、ブラシ、画像、投影、ストローク、描画処理、画像メタデータ、取り消し処理 |
| `plugin-infrastructure` | `kritaplugin` | メタデータ探索、ファクトリーとサービス種別の登録 |
| `resource-management` | `kritaresources`、`kritaresourcestorage`、`kritaresourceui` | リソースの保存、検索、タグ、選択、表示 |
| `tool-invocation` | `kritatools`、`kritatoolsui`、`kritaui` | ツール命令、描画設定表示、キャンバス状態へのツール呼出し |

`targetRelations`は23の中核所有ターゲットについて、5構成に存在する種別と、製品CMake
ターゲット間の直接依存および利用元を和集合で記録する。この地図は現在の所有関係を表し、
R1-G3bで定義する許可依存方向の比較元になる。

`kritaui`は9責務中5責務の現所有ターゲットである。UIクラスの責務分類と組み合わせることで、
文書、キャンバス、入力解釈などを凝集したターゲットへ分割する順序を決められる。
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

`currentTargetEdges`は22の中核所有ターゲット間にある65の直接リンクを責務へ射影する。
`kritaui`のような共有ターゲットは、所有する全責務の直積として保守的に扱う。現在は129候補の
うち21候補が同一責務内、78候補が許可方向、30候補が`requires-r1-g4-baseline`である。
最後の分類は共有ターゲットが作る曖昧な候補を含むため、R1-G4で実際のincludeと利用箇所を
根拠に既存違反基準へ確定する。

### 確認済み逆方向依存の基準

[依存違反基準](dependency-violation-baseline.json)は、許可方向外の責務対を製品ソースの
直接includeへ照合し、一意に責務へ帰属できる現在の3責務対を確認済み違反として記録する。
各項目は元のCMakeターゲット辺と5構成、全include根拠、審査済みの最大件数、所有する
ロードマップ段階、現在必要な理由、除去条件を持つ。

| 依存元 | 依存先 | 直接include上限 | 主な現在境界 |
| --- | --- | ---: | --- |
| アプリケーション調整 | 描画 | 75 | `kritaui`内の起動・共有サービスから画像、ブラシ、色処理 |
| 入力解釈 | 描画 | 17 | 入力処理から描画情報とストローク状態 |
| 入力解釈 | リソース管理 | 4 | 入力処理からプリセットとリソース状態 |

採取器は各対象ターゲットの記録済みソースディレクトリー以下から製品ソースを読み、試験経路を
除外する。includeは依存先ヘッダーのパス末尾、またはリポジトリ内で一意なヘッダー名により
解決する。責務は単一所有ターゲット、分類済み公開クラス、最長一致する責務ディレクトリーの
順に決める。全件クラス台帳の範囲外にある審査済みソースは`reviewedSourcePaths`を根拠に
一意に帰属する。この規則で96件の直接includeが現在の確認済み基準になる。R1-G6aは、
描画から入出力への2件、リソース管理から入出力への5件、リソース管理から描画への40件を
解消した。R1-G6bは描画から文書寿命への95件を解消した。描画処理を3ターゲットへ分離した
結果、同一ターゲット内に隠れていたアプリケーション調整から描画への5件と入力解釈から
描画への2件がパッケージ間参照として可視化され、現在件数と同じ上限へ記録されている。

共有ターゲットの保守的射影から生じた残り9責務対は、構造違反基準が実際の
includeを全件帰属させる。`unresolvedProjections`は空であり、確認済み違反の上限には
根拠のある現在の3責務対だけを含める。

検査では現在件数が審査済み上限を超える変更を基準拡大として診断する。現在件数が減った場合も
上限を同じ変更で縮小するまで診断する。根拠の置換は件数が同じでも生成差分として現れる。

### 構造違反と循環の基準

[構造依存基準](structural-dependency-baseline.json)は、共有ターゲット由来の射影解決、
CMakeターゲット循環、公開宣言を持たないヘッダーのパッケージ外参照を一つの継続検査へ
接続する。

10射影はすべて`disproved-by-direct-include-attribution`として解決済みである。
`kritabasicflakes`から`kritaui`、および`kritaui`から`kritatoolsui`への共有ターゲット辺を、
キャンバス表示とツール呼出しの実際のincludeへ帰属させる。入出力を独立所有ターゲットへ
移した結果、入出力責務をUI共有ターゲットへ射影する候補は存在しない。各解決は元のターゲット辺、5構成、実際の責務対、ソース、include、
ヘッダーを記録し、新たな未帰属候補を診断する。

ターゲット循環は、23の中核所有ターゲットと、試験経路を除く全製品構築ターゲットの
2範囲を検査する。全製品範囲はmacOS 223件、Linux 229件、iOS 215件、Android 215件、
Windows 232件であり、現在の非自明な強連結成分は両範囲、全構成で0件である。
`maximumComponents`を0に固定し、新しい直接リンク循環を基準拡大として診断する。

公開面台帳で`external-include`だけを公開根拠とし、所有元外から参照されるヘッダーを、
宣言済み公開面へ移す対象として基準化する。`kritaimage`の29ヘッダー、593参照は公開
ヘッダー構築契約へ移行して0件となった。`kritaimpex`、`kritaimpexui`、`kritatools`も未宣言の
パッケージ外参照が0件であり、現在は`kritaui`の6ヘッダー、19参照が残る。所有段階、理由、
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
段階別上限は311、264、169、169、96、96、96、75、0と縮小する。内部ヘッダーの
直接参照は、R1-G6bで`kritaimage`の593件を解消し、R1-G6cで`kritaui`を28件へ
縮小した。後続段階では20、20、3、3、0へ縮小する。

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

R1-G6bは、`libs/ui/tool/strokes`を`libs/painting/strokes`へ移し、同じUIツール領域に
置かれていた資源スナップショット、非同期更新、互換性判定、速度計測を`libs/painting`へ
移した。`libs/command`の画像・キャンバス向け取り消し処理は`libs/painting/undo`へ、
`libs/metadata`の画像メタデータ実装は`libs/painting/metadata`へ移した。画像層から利用する
取り消し処理とメタデータをそれぞれ`kritapaintingundo`、`kritapaintingmetadata`とし、画像層を
利用するストローク実行を`kritapainting`とすることで、CMakeターゲットの循環を避けている。
資源スナップショットはUIの具体的な資源提供者ではなく、`libs/resources`が所有する読出し
接続面を保持する。旧ディレクトリーの転送ヘッダーと旧メタデータターゲットは存在しない。

R1-G6cは、`libs/ui/KisImportExportManager.*`、`KisImportExportFilter.*`、
`KisImportExportErrorCode.*`、`KisImportExportAdditionalChecks.*`、
`KisImportUserFeedbackInterface.*`を起点として分割した。形式探索、MIME選択、結果分類、
事前検査、変換フィルターは`libs/impex`の`kritaimpex`が所有する。文書変換の調整、
利用者通知、クリップボード、ダイアログ、画像読込補助は`libs/impex/ui`、動画符号化調整は
`libs/impex/animation`に置き、`kritaimpexui`が所有する。`kritaimpexui`は文書・画面型との
現在のABI接続を保つオブジェクト所有単位として`kritaui`へ組み込む。旧`libs/ui`の入出力ヘッダーと
転送ヘッダーは存在せず、利用元は正規の所有先を直接参照する。

R1-G6dの最初の独立単位は、`libs/ui/canvas/kis_coordinates_converter.*`と
`libs/ui/canvas/KisCanvasState.*`を起点として分割した。座標変換と画面状態は
`libs/canvas`の`kritacanvas`が所有し、`kritaui`は表示設定を明示的に渡して利用する。
座標変換器は構築元の画像を保持せず、構築時に取り込んだ幾何情報と変換結果を画像の
解放後も利用できる。旧配置と転送ヘッダーは存在せず、利用元と試験は新しい所有先を
直接参照する。

同段階の次の独立単位は、`libs/ui/canvas/kis_prescaled_projection.*`を起点として分割した。
表示用画像片、投影更新情報、投影取得接続面、拡大縮小済みフレームは`libs/canvas`の
`kritacanvas`が所有する。更新処理は画面設定やUI固有の表示フィルターを直接取得せず、
呼出し側が更新片の寸法、画面プロファイル、変換方法、画素フィルターを渡す。
`libs/ui/canvas/kis_qpainter_projection_factory.*`はUI設定と具体的な画像投影実装を
この接続面へ結び、`libs/ui/opengl/kis_opengl_update_info.*`はOpenGL固有の更新情報を
UI側に保持する。汚れ領域の更新通知と、空の更新では直前の有効フレームを保持する契約を
`libs/canvas/tests/kis_prescaled_projection_contract_test.*`が検査する。旧配置と転送
ヘッダーは存在しない。

表示色変換の独立単位は、`libs/ui/KisOcioConfiguration.*`、
`libs/ui/KisSurfaceColorSpaceWrapper.h`、`libs/ui/canvas/kis_display_color_converter.*`を
起点として分割した。表示色の設定値、Qt画面色空間との変換値、画素・画像の色変換本体、
表示フィルター接続面は`libs/canvas/color`の`kritacanvas`が所有する。
`libs/ui/canvas/kis_display_color_converter.*`はこの変換本体を保持し、現在ノード、設定通知、
前景色、画面パレットとの接続を担当する。UIをリンクしない色変換・色空間値契約と、
UI設定反映契約が同じ結果と通知回数を検査する。旧値型ファイルと転送ヘッダーは存在しない。

動画キャッシュの独立単位は、`libs/ui/kis_animation_frame_cache.*`、
`libs/ui/kis_animation_cache_populator.*`、`libs/ui/KisFrameDataSerializer.*`、
`libs/ui/KisFrameCacheStore.*`、`libs/ui/KisFrameCacheSwapper.*`を起点として分割した。
フレーム範囲の判定と変更指示、差分保存、ディスク直列化、タイル転送バッファーは
`libs/canvas/animation`と`libs/canvas/tiles`の`kritacanvas`が所有する。
`libs/ui/animation`は現在画像と再生状態、生成時機、設定変更を調整し、
`libs/ui/animation/cache`はOpenGL更新情報と保存値の変換を担当する。範囲管理、保存、
直列化はUI型を参照せずに構築・検査できる。旧配置と転送ヘッダーは存在しない。
`libs/ui/KisWidgetWithIdleTask.h`は表示部品として`libs/ui/canvas`へ移し、別ターゲットの
ドッカーが利用する公開ヘッダーを構築契約で固定した。

R1-G6eの最初の独立単位は、`libs/ui/kis_document_undo_store.*`を起点として文書全体への
参照を取り消し履歴の直接借用へ狭めた。R1-G6e-P1では、その接続を
`libs/document/undo/kis_document_undo_store.*`から
`libs/document/ui/undo/kis_document_undo_store.*`へ移し、履歴表示も
`libs/command/{kundo2model,kundo2view}.*`から同じ所有先へ集約した。
`kritadocumentui`が文書と履歴の接続、Qt Widgets用操作、履歴表示を所有し、`kritaui`が
直接利用する。汎用状態だけを持つ`kritadocument`は`kritapaintingundo`への依存を除去し、
Qt Coreだけで公開リンク閉包を構成する。履歴の現在位置、追加、取消し、マクロ、やり直し破棄、
同一スレッド上の同期通知、非所有の借用寿命に加え、操作名、有効状態、履歴行、選択による
履歴移動を専用契約で固定した。旧`kritacommand`、旧配置、転送ヘッダー、別名は存在しない。
同じ一括移設で`libs/ui/KisAutoSaveRecoveryDialog.*`を
`libs/document/ui/recovery/KisAutoSaveRecoveryDialog.*`へ移し、回復候補の初期選択と一括破棄を
文書UI契約として固定した。`KisDocument`状態へ直接依存してAPI再構築を要する文書情報編集と
保存処理は、機械的なファイル移動とは区別して後続の構造変更で扱う。

R1-G6eの第2の独立単位は、`libs/ui/KisDocument.cpp`に埋め込まれていた文書パス、
入出力実装へ渡す実ファイルパス、現在のMIME形式、自動判定由来を起点とする。
これらの文書識別状態は`libs/document/session/kis_document_identity.*`の
`Krita::Document::Identity`が所有し、`KisDocument`は既存APIとパス変更通知を接続する。
表示用パスと実ファイルパスを独立して保持し、同一パスの再設定では通知せず、文書の
スナップショットには識別状態を複製する。設定後に読み取られていなかった書出しMIME状態は
除去した。文書識別の実装はQt Core型だけを使用し、専用契約で検査できる。保存、
自動保存、回復のI/O調整は`KisDocument`に残る。

R1-G6eの第3の独立単位は、`libs/ui/KisDocument.cpp`に埋め込まれていた変更済み状態、
自動保存チェックポイント後の変更、保存実行中の変更、取り消し履歴に現れない画像変更を
起点とする。これらの文書変更状態は
`libs/document/session/kis_document_modification_state.*`の
`Krita::Document::ModificationState`が所有する。同じ変更済み値の再設定でも保存中と
自動保存後の変更を記録し、未変更への遷移では取り消し不能変更を消去する。保存用複製は
文書の変更状態を引き継ぎ、進行中の保存と自動保存の経過を初期化する。
`KisDocument`は編集時刻、文書情報更新、自動保存タイマー、Qt通知、保存・回復処理の
実行を接続し、既存の公開APIを維持する。変更状態の実装はQt型を使用せず、専用契約で
検査できる。`kritadocument`全体は取り消し履歴ライブラリーを通じてQt Widgetsへ依存するため、
Qt Widgets用アクション生成との分離要否はR1-G6eの後続単位で判定する。

R1-G6eの第4の独立単位は、`libs/ui/KisDocument.cpp`に埋め込まれていた自動保存用複製の
書出し状態と連続失敗回数を起点とする。これらの自動保存実行状態は
`libs/document/session/kis_document_autosave_state.*`の
`Krita::Document::AutoSaveState`が所有する。自動保存用複製の書出し開始から終了までを
明示し、3回の連続失敗後は次の試行で複製経路を選ぶ既存の境界値を保持する。通常間隔へ
戻ると失敗履歴を消去する。`KisDocument`はタイマー、設定、文書複製、ファイル出力、
利用者通知、回復調整を引き続き接続する。実行状態の実装はQt型を使用せず、専用契約で
検査できる。読み書きされていなかった旧失敗無視フラグは除去した。

R1-G6eの第5の独立単位は、`libs/ui/KisDocument.cpp`に埋め込まれていた回復用自動保存要求、
保存開始中に同期完了した場合の延期結果、既存の背景保存へ合流した場合の保存先を起点とする。
これらの要求調停状態は`libs/document/session/kis_document_recovery_autosave_state.*`の
`Krita::Document::RecoveryAutoSaveState`が所有する。未処理の要求だけが一度完了し、保存開始が
戻る前に届いた完了は開始結果が確定するまで延期する。既存の背景保存が利用可能な自動保存を
生成した場合は、その保存先を同じ回復要求へ引き継ぐ。`KisDocument`はタイマー、背景保存の
開始、変更状態、ファイルの存在と大きさの検証、完了通知を引き続き接続する。調停状態は
Qt Coreの値だけを使用し、専用契約で検査できる。

R1-G6eの第6の独立単位は、`libs/ui/KisDocument.cpp`に埋め込まれていた回復済み文書状態を
起点とする。この文書状態は`libs/document/session/kis_document_recovery_status.*`の
`Krita::Document::RecoveryStatus`が所有する。通常文書を初期状態とし、回復データから開いた
文書への遷移と通常保存後の復帰について、実際に値が変わる場合だけ通知が必要であることを
返す。`KisDocument`は回復データの探索と読込、保存後の回復ファイル消去、表示、
`sigRecoveredChanged`通知を引き続き接続する。保存用スナップショットは回復元の作業文書では
ないため、従来どおり通常文書状態から始まる。回復状態の実装はQt型を使用せず、専用契約で
検査できる。

R1-G6e開始時の`document-state`分類は25クラスであり、最初の分割後にUI所有の分類は
24クラスとなった。文書識別の抽出後も`KisDocument`自体はUI分類に残るため件数は24である。
変更状態、自動保存実行状態、回復用自動保存調停状態、回復済み文書状態の抽出も同じ
`KisDocument`内の埋込み状態を移すため、分類件数は24を維持する。
文書表示の集約と2つの画像ノード命令分離により、`KoDocumentInfo`、`KoDocumentInfoDlg`、
`KisNodeCommandsAdapter`、旧`KisNodeJugglerCompressed`（現`KisNodeOperationBatch`）が
UI分類から外れ、現在は20クラスである。
残る分類は文書寿命だけでなくノード操作、選択操作、表示モデルを含む。
後続単位では各クラスの実依存から文書状態、文書表示、別機能の操作接続を
判定し、文書寿命を所有するものだけを文書ターゲットへ移す。

R1-G6e後半は[文書パッケージ境界計画](document-package-boundary-plan.md)に従い、依存経路の
一方向化、具体的な命名、現存する関心領域ごとの分割と集約を行う。`kritadocument`は文書状態、
`kritadocumentfiles`は文書ファイル、バックアップ、自動保存ファイル、回復ファイル、
`kritadocumentui`はダイアログ、状態表示、文書情報編集、取り消し履歴との接続と表示を所有する。
形式処理と表示が共有する直列化対象の文書情報は`kritaimpex`が所有する。依存は文書UIから
文書ファイル保存、文書ファイル保存から文書状態と入出力へ向ける。

最初の検査段階で、文書と取り消し履歴の接続および履歴表示を`kritadocumentui`へ移し、
`kritadocument`の公開リンク閉包をQt Coreだけへ縮小した。第2段階では
`libs/ui/KisDocument.cpp`の保存・読込ダイアログ、状態表示、Qt通知を
`libs/document/ui/io`へ、文書情報編集と名前付き自動保存回復ダイアログを
`libs/document/ui/info`と`libs/document/ui/recovery`へ集約した。`KoDocumentInfo`は
`libs/impex/metadata`へ移し、入出力が上位の文書寿命へ依存する辺を作らずに、文書状態を
明示値として受け取る。第3段階では`libs/ui/KisDocument.cpp`と自動保存回復UIにあった
保存先検査、バックアップ、自動保存名、回復候補の探索と読込、使用可否判定、消去を
`libs/document/files`へ集約した。文書UIは生成済みの回復候補値を表示し、形式選択と形式変換、
直列化、非同期保存は既存の入出力所有と文書調整に維持する。第4段階では残る22クラスを
文書構成、外部ファイル層、操作管理、ノード・選択操作接続、Qtモデルと表示状態へ分け、
`KisDocument.cpp`の129メソッド定義を8関心へ分類した。R1-G6e開始時の25クラスすべてが
具体的な所有先またはR1-G6f、R1-G6hを持つ。現在必要な保存I/O差し替え、複数実装、
利用事例登録、追加の純粋計算はなく、文書境界だけを理由とする抽象は追加しない。

ノード表示モデルは`KisNodeManager`と操作接続へ、外部ファイル層は`KisPart`へ直接依存する。
R1-G6fの最初の単位では、`libs/ui/kis_node_commands_adapter.*`を
`libs/image/commands/kis_node_commands_adapter.*`へ移し、ノード追加、移動、削除、属性変更の
取り消し可能な命令を既存の画像命令へ集約した。命令はUI全体ではなく操作対象画像を弱参照し、
長寿命の表示管理側が画面切替時に画像を明示的に結び直す。旧ファイル、転送ヘッダー、旧名の
別名は存在しない。入出力、文書、アプリケーション、ツールが同じ命令を利用しているため、
上位のツール所有へ置かず、既存の許可方向に従って画像命令を所有先とする。

次の単位では、`libs/ui/kis_node_juggler_compressed.*`を
`libs/image/commands/kis_node_operation_batch.*`へ移し、ノードの連続した追加、移動、複製、
削除と一つの取り消し履歴項目への集約を画像所有へ置いた。画像処理が借用していた
`KisNodeManager`は除去し、選択復元に必要なアクティブノードを呼出し側が値として渡す。
操作バッチの寿命構成と利用者操作の配線は`KisNodeManager`に残る。旧ファイル、転送ヘッダー、
旧名の別名は存在せず、新しい汎用層も追加していない。

`KisNodeManager`から画像ノードを変更する場合、移動可能性と選択マスクのアクティブ状態は
`KisNodeCommandsAdapter`が保証する。ミラー処理の再帰範囲、選択範囲、全フレーム処理、並行
ジョブ、取り消し履歴への登録は`KisMirrorProcessingVisitor::applyToNodes()`が構成する。
`KisNodeManager`は編集可否を利用者へ通知し、これらの画像処理を呼び出し、完了後の画面更新を
通知する。

クイックグループ化と解除の画像グラフ変更は
`libs/image/commands/kis_node_operation_batch.*`と`kis_node_group_operations.*`が所有する。
`KisNodeManager`は操作名、編集可否、選択更新、互換性エラーの表示だけを所有し、同ファイルは
1798行から1739行へ縮小した。

R1-G6fのツール命令単位では、`libs/ui/tool`にあった基底ツール、起動方針、変更追跡、
ファクトリー、平滑化設定、共通値処理を`libs/tools`へ移し、`kritatools`として独立構築する。
キャンバス操作の借用契約は`libs/canvas/KisToolCanvas.h`を`kritacanvas`が所有し、依存方向を
`kritatools`から`kritacanvas`へ固定する。画面固有の浮動メッセージと図形選択は
`libs/ui/tool/kis_tool_canvas_utils.*`、`KisCanvas2`との接続は
`libs/ui/canvas/kis_canvas_tool_support.cpp`が所有する。旧配置の転送ヘッダーは存在しない。

委譲ツールの入力フィルター接続と選択ツールの操作状態は`libs/tools`が所有する。
`libs/ui/tool/kis_delegated_tool.h`は`libs/tools/kis_delegated_tool.h`へ移し、
`libs/ui/tool/kis_tool_select_base.h`は操作を持つ`libs/tools/kis_tool_select_base.h`と、設定表示を
持つ`libs/ui/tool/kis_tool_select_ui_base.h`へ分けた。修飾キー対応は
`plugins/tools/selectiontools/kis_selection_modifier_mapper.*`から
`libs/tools/kis_selection_modifier_mapping.*`へ移し、設定値をキャンバス借用契約から渡す。
旧配置、転送ヘッダー、大域的な設定監視オブジェクトは存在しない。

描画ツールの操作状態は`libs/tools/kis_tool_paint_interaction.{h,cpp}`が所有する。
`libs/ui/tool/kis_tool_paint.{h,cc}`からポインター追跡、ブラシ寸法・回転操作、輪郭状態、
輪郭生成を移し、UI側には色採取、ポップアップ、設定部品、設定に基づく輪郭表示、描画補助線の
更新を残した。`kritatools`から`kritaui`への依存はなく、UI側が操作基盤を継承する方向となる。

図形を描画装置へ反映する実行は`kritapainting`が所有する。
`libs/ui/tool/kis_figure_painting_tool_helper.{h,cpp}`を
`libs/painting/kis_figure_painting_stroke.{h,cpp}`へ移し、一つの描画ストロークの開始、ジョブ追加、
終了を`KisFigurePaintingStroke`の寿命へまとめた。`libs/tools/KisToolShapeUtils.h`の描線・塗り値は
`libs/painting/KisFigurePaintingOptions.h`へ移し、空だった同名実装ファイルを除去した。
列挙値の順序とスクリプトのスタイル名対応は維持し、旧配置、転送ヘッダー、旧名の別名は存在しない。

矩形ツールの制約、修飾キー、ドラッグ座標、回転角と矩形計算は
`libs/tools/kis_rectangle_interaction.{h,cpp}`が所有する。
`libs/ui/tool/kis_tool_rectangle_base.{h,cpp}`にはポインター座標変換、編集可否の通知、寸法と位置の
表示、輪郭描画、キャンバス更新、設定部品を残した。矩形、楕円、矩形選択、矩形囲み塗りは同じ
操作状態を使用し、制約、固定寸法、正方形化、移動、中央拡張、回転の座標契約を
`TestToolCoreContract`が固定する。

自由形状ツールの点列、入力中状態、Controlによる継続入力、継続点の取り消し、完了と取消しは
`libs/tools/kis_outline_interaction.{h,cpp}`が所有する。
`libs/ui/tool/KisToolOutlineBase.{h,cpp}`には入力座標変換、編集可否の通知、輪郭表示、再描画範囲、
入力フィルターと操作アクションの接続を残した。自由選択と囲み塗りは同じ操作状態を使用し、通常入力、
継続入力、点の取り消し、完了、取消しの契約を`TestToolCoreContract`が固定する。

多角線ツールの点列、ドラッグ区間、閉路状態、点の取り消し、完了と取消しは
`libs/tools/kis_polyline_interaction.{h,cpp}`が所有する。
`libs/ui/tool/kis_tool_polyline_base.{h,cpp}`にはポインター座標変換、画面距離による始点スナップ判定、
輪郭表示、再描画範囲、右クリックと操作アクションの接続を残した。多角形、多角線、多角形選択は
同じ操作状態を使用し、単一点終了、複数点、閉路、点の取り消し、全取消しの契約を
`TestToolCoreContract`が固定する。

描画入力値の決定は`kritatools`が所有する。開始元の
`libs/ui/tool/kis_speed_smoother.{h,cpp}`は`libs/tools/kis_speed_smoother.{h,cpp}`へ移した。
`libs/ui/tool/kis_painting_information_builder.{h,cpp}`の圧力曲線、速度、傾き、時刻、キャンバス状態の
組立ては`libs/tools/kis_painting_information_builder.{h,cpp}`へ、座標変換と自由描画ツールへの接続は
`libs/ui/tool/kis_painting_information_builder_adapters.{h,cpp}`へ分けた。UI設定は
`libs/ui/tool/kis_painting_information_builder_config_p.h`の内部接続から値として中核へ渡す。
旧配置と転送ヘッダーは存在せず、設定変更通知、座標変換、圧力曲線、決定論的な速度平滑化、
キャンバス回転・反転、傾き補正の契約を維持する。

選択設定表示は`kritatoolsui`が所有する。`libs/ui/widgets/kis_selection_options.{h,cc}`と
`libs/ui/tool/kis_selection_tool_config_widget_helper.{h,cpp}`を同名の`libs/tools/ui`へ移した。
依存していた`libs/ui/widgets/kis_color_label_button.{h,cpp}`と
`libs/ui/widgets/kis_color_label_selector_widget.{h,cpp}`は汎用表示部品として`libs/widgets`へ移し、
レイヤーツリーの配色所有者を`kritaui`へ残した。選択方式、結合方法、アンチエイリアス、拡張、
境界停止、ぼかし、参照レイヤー、色ラベルの保存と再読込は`TestToolSettingsUiContract`が固定する。

ツール設定ポップアップは`kritatoolsui`が所有する。開始元の
`libs/ui/widgets/kis_tool_options_popup.{h,cpp}`を同名の`libs/tools/ui`へ移し、設定部品の見出し、区切り、
並び替え、退避を下位UIだけで構築できるようにした。ドック用フォントは`libs/ui/kis_paintop_box.cc`が
値として渡し、ポップアップボタン、キャンバス、操作アクションとの接続は`kritaui`に残る。

矩形制約表示は`kritatoolsui`が所有する。開始元の
`libs/ui/tool/kis_rectangle_constraint_widget.{h,cpp}`と`libs/ui/forms/wdgrectangleconstraints.ui`を
同名の`libs/tools/ui`へ移し、ウィジェットは設定グループを受け取って制約値と角丸値を信号で返す。
`libs/ui/tool/kis_tool_rectangle_base.cpp`が矩形状態、設定再読込、制約適用を接続し、下位UIは矩形ツールを
参照しない。必要な汎用比率ロックは`libs/ui/kis_aspect_ratio_locker.{h,cpp}`から同名の`libs/widgets`へ
移し、画像寸法、フィルター、描画設定の既存利用元も`kritawidgets`の公開面を使う。

基本図形生成は`kritaflake`が所有する。開始元の`libs/ui/tool/kis_shape_tool_helper.{h,cpp}`を
`libs/flake/KoBasicShapeFactory.{h,cpp}`へ移し、矩形と楕円は登録済み図形ファクトリーを優先して生成し、
対応するプラグインがない構成では同じ境界矩形を持つパス図形を生成する。基本図形ツールと選択ツールは
`kritaflake`へ直接依存し、旧UI補助クラスと転送ヘッダーは存在しない。

次は`libs/ui/tool/kis_stabilized_events_sampler.{h,cpp}`と
`libs/ui/tool/KisStabilizerDelayedPaintHelper.{h,cpp}`を同名の`libs/tools`へ移し、実時間に基づく入力標本化と
遅延描画キューをツール入力所有へ集約する。`libs/ui/tool/kis_tool_freehand_helper.cpp`は自由描画ストロークの
生成と輪郭更新をコールバックで接続する。

共有ライブラリー記号を宣言しない別名、列挙、テンプレートを含む`kritaimage`の29ヘッダーは、
`libs/painting/tests/TestPublicImageHeaders.cpp`で一つの翻訳単位として構築する。この構築契約を
公開根拠として台帳へ記録し、公開面を宣言せずに利用される内部ヘッダーとは区別する。

残る5の一時互換経路は後続UI再配置用の旧include、`kritaui`、既存の大域C++識別子を含む。
各経路は導入段階、R1-G7の所有者、最大範囲、削除条件、検証方法を持つ。計画検査は
9責務と5構成の現行ターゲット、3種類96件の逆方向依存、4ヘッダー7件のUI内部参照を
正本へ照合し、全基準と一時経路が最終状態でゼロになることを確認する。

責務の中心は次の五つです。

- `krita/`はプロセスの入口、アプリケーション資産、OSライフサイクルとの接続を持ちます。主要機能は`libs/`と`plugins/`が所有します。
- `libs/ui`の`kritaui`はアプリケーション、ウィンドウ、文書、キャンバス、入力、ツール共通部をまとめます。
- `libs/impex`の`kritaimpex`と`kritaimpexui`は形式契約、文書入出力、利用者通知をまとめます。
- `libs/image`の`kritaimage`はレイヤーツリー、ペイントデバイス、タイル、ストロークキュー、投影更新を扱います。
- `plugins/`はツール、ブラシエンジン、フィルター、ドッカー、ファイル形式などの機能をレジストリーへ登録します。

`libs/ui`は画面機能と、`KisDocument`、ツール共通処理などのアプリケーション調整を
扱います。入出力管理は`libs/impex`に置き、`KisImage`は画像内容と非同期処理を中心に扱い、
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

`KisDocument`は文書識別と変更状態の公開API、通知、自動保存、読み込み・保存、
`KisImage`の差し替えを調整します。識別値は`Krita::Document::Identity`、変更状態は
`Krita::Document::ModificationState`が所有します。`KisImage`は次を所有します。

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

`KisDocument`は`libs/impex/ui`の`KisImportExportManager`へ処理を委譲します。
`libs/impex`の`KisImportExportFilterRegistry`が`Krita/FileFilter`プラグインをMIME型で選び、
管理クラスが`KisImportExportFilter::convert()`を呼びます。

- KRAやORAのようなコンテナー形式では`libs/resources/storage`の`KoStore`がZIP／ディレクトリー抽象化を提供します。
- XML名前空間と逐次書出しは`libs/serialization/xml`が提供します。
- 画像形式固有の符号化、設定画面、依存ライブラリー接続は`plugins/impex/<format>/`に置きます。
- 形式探索、結果分類、事前検査は`libs/impex`、非同期エクスポート、警告、原子的保存の調整は`libs/impex/ui`にあります。
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
| ブラシエンジンやプリセット | `plugins/paintops`、`libs/brush` | `libs/painting/strokes`、`libs/resources`、`libs/pigment` |
| レイヤー、マスク、画素、投影 | `libs/image` | `KisNode`、`KisPaintDevice`、`KisUpdateScheduler` |
| アンドゥ、非同期処理 | `libs/painting/undo`、`libs/painting/strokes`、`libs/image/commands*`、`libs/image/kis_strokes_queue.*` | ストローク戦略の順序・排他属性 |
| 色空間、プロファイル、合成 | `libs/pigment`、`libs/color`、`plugins/color` | LittleCMS、OpenColorIO、表示変換 |
| ベクター図形、選択図形 | `libs/flake`、`libs/basicflakes`、`plugins/flake` | `libs/ui/flake`、SVG入出力 |
| ブラシ等のリソース管理 | `libs/resources`、`libs/resources/ui` | リソースDB、ローダーレジストリー、同梱バンドル、選択・タグ表示 |
| 描画設定表示 | `libs/tools/ui` | パレット、合成方法、プリセット、描画設定の表示 |
| KRA内部構造、ZIPストレージ | `plugins/impex/libkra`、`plugins/impex/kra`、`libs/resources/storage` | `KisDocument`、メタデータ、XML直列化 |
| 形式探索、MIME選択、入出力結果と事前検査 | `libs/impex` | `KisImportExportFilterRegistry`、`KisImportExportFilter`、プラグインJSON |
| 文書入出力、通知、クリップボード、動画符号化 | `libs/impex/ui`、`libs/impex/animation` | `KisDocument`、利用者操作、プラットフォーム媒体処理 |
| PNG、PSD、RAW等の形式 | `plugins/impex/<format>` | `libs/impex`の形式契約、プラグインJSON、Nix依存 |
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
| `libs/painting` | 描画ストローク、画像・キャンバス向け取り消し処理、画像メタデータ、描画用資源スナップショット |
| `libs/brush`、`libs/pigment`、`libs/color` | ブラシ資産、色空間、色変換・合成の基盤 |
| `libs/flake`、`libs/basicflakes` | ベクター図形、キャンバス、図形ツールの基盤 |
| `libs/resources`、`libs/resources/ui` | リソース永続化、検索、タグ、バンドルと汎用管理画面 |
| `libs/tools/ui` | 描画ツールの設定、パレット、プリセットの表示 |
| `libs/resources/storage`、`libs/serialization/xml` | コンテナーI/OとXML直列化 |
| `libs/painting/metadata`、`libs/psd*` | 画像メタデータとPSD共通実装 |
| `libs/koplugin` | プラグイン探索とメタデータ照会 |
| `libs/impex` | 形式探索、MIME選択、入出力フィルター、結果分類、書き出し前検査 |
| `libs/impex/ui`、`libs/impex/animation` | 文書入出力の調整、利用者通知、クリップボード、媒体符号化 |
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
