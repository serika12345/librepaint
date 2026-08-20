# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-08-20 10:56 JST
- 状態: `completed`
- 現在の検査段階: R1-G6e-P1取り消し履歴の文書UI境界
- 関連TODO: `docs/architecture/TODO.md`の「R1: コードパッケージングの改善」
- ブランチ: `r1-g6e-p1-document-undo-ui-boundary`
- 目的: 文書状態の公開依存から取り消し履歴とQt Widgetsを除き、履歴接続、操作、表示を
  具体的な文書UI所有へ集約する。

## 再開環境

- direnvが`test`開発シェルと`build-incremental`、`run-test`、`verify-quick`、
  `verify`を読み込む。
- DarwinホストがmacOSとiOS、`ssh nixos`で接続するx86_64 NixOSホストがLinux、
  Android、Windowsの構成を担当する。
- 全プラットフォームの実構成差分は、両ホストの清浄な作業ツリーを同じコミットへ
  揃え、`scripts/architecture/verify_cmake_graphs.py`から5構成を並行検証する。
- CMake File API `codemodel-v2`の台帳は
  `docs/architecture/cmake-targets-<platform>.json`、全構成の比較結果は
  `docs/architecture/cmake-target-matrix.json`に記録する。

## R1-G1で完了した作業

- 固定File API応答と抽出器が、ターゲット名、種別、リポジトリ相対の定義場所、
  `linkLibraries`による直接CMakeターゲット依存を決定的に整列する。
- 再生成器が各プラットフォームの増分構築入口から永続構築木を取得し、問い合わせ、
  構成同期、台帳更新、バイト単位の差分検査を実行する。
- macOS 627件、Linux 642件、iOS 561件、Android 567件、Windows 597件の
  ターゲットを同じ形式で記録した。
- 差分行列が545件の共通ターゲット、119件の条件付きターゲット、247件の構成差を
  持つターゲットを記録した。
- 絶対パスから名前が変わる翻訳補助ターゲットを固定応答の契約で除外し、異なる
  作業ツリーから同じ台帳を再生成できるようにした。
- 全プラットフォーム同時検証入口が、Darwinとx86_64 Linuxの担当構成、同一コミット、
  清浄な作業ツリーを構成開始前に検査する。遠隔の並行Nix評価は個別の評価処理を使う。
- Windowsのソース準備処理を通常のNix構築と増分構築で共有し、CMakeの試行生成物を
  Ninja構築木へ収める契約を固定した。
- 台帳の範囲、差分行列、ホスト割り当て、更新手順、同時検証コマンドを
  アーキテクチャガイドと開発手順へ記録した。

## R1-G2aで完了した作業

- `docs/architecture/public-surface-inventory.json`に、`kritaui`と`kritaimage`の
  代表公開ヘッダー3件、主要クラス3件、PNG読込プラグイン1件を記録した。
- 公開ヘッダーを所有ターゲット、公開マクロ、別ターゲットからの直接include、
  対応プラットフォーム、責務、根拠へ接続した。
- 主要クラスを宣言、実装、所有ターゲット、公開ヘッダー、利用元へ接続し、
  プラグインをCMakeターゲット、JSONメタデータ、登録マクロ、実行時利用元へ接続した。
- `check_public_surface_inventory.py`が、パス、決定的な整列、5台帳のターゲット実体、
  ソース所属、公開マクロ、include、クラス宣言、メタデータ、登録を高速検査で確認する。
- Nixの独立した運用検査を、Nix提供のBash、明示的なスクリプト処理系、認証局証明書で
  完結させ、macOSとLinuxで同じ検査を構築できるようにした。
- 台帳の目的、構成、検査方法、保守手順をアーキテクチャガイドと開発手順へ記録した。

## R1-G2bで完了した作業

- 公開マクロを使用する製品ヘッダーと、所有元外の製品ソースから直接includeされる
  ヘッダーの和集合を、公開ヘッダーの全件採取規則として固定した。
- `kritaimage`の332件と`kritaui`の307件を所有ターゲット、公開根拠、5構成、
  外部利用ソース5,437箇所へ接続し、`scope.publicHeaders`を`complete`へ更新した。
- `publicHeaderPolicy`が対象拡張子、製品ソースディレクトリー、試験経路の除外、
  公開根拠を固定し、試験専用の共有ヘッダーを製品パッケージ間の公開面から分離した。
- 決定的更新器が公開ヘッダー集合と全利用ソースを再生成し、検査器が欠落、余分な項目、
  所有者、整列、公開マクロ、直接include、利用ソースの差分を診断する。
- R1-G2aの詳細な代表記録を`publicHeaderDetails`として維持し、主要クラスとプラグインの
  所有者、利用元ターゲット、責務との接続を保持した。
- 全件採取の範囲、除外理由、更新コマンド、検査内容、変更時の保守手順を
  アーキテクチャガイドと開発手順へ記録した。

## R1-G2cで完了した作業

- `libs/ui`直下の完全公開ヘッダー集合から、公開マクロを持つクラスと構造体124件を
  決定的に採取した。
- 115件を名前に対応する実装単位へ接続し、インライン、テンプレート、単純データ、
  抽象接続面として宣言側で完結する9件を空の実装単位一覧として記録した。
- 124件をアプリケーション調整7件、キャンバス・表示39件、文書状態21件、入出力14件、
  資源・設定26件、ウィンドウ・作業空間17件へ分類した。
- UI直下クラス責務台帳が、宣言種別、公開ヘッダー、実装単位、所有ターゲット、5構成、
  責務領域を接続し、`KisApplication`、`KisDocument`、`KisImportExportManager`を固定した。
- 決定的更新器が責務分類を保持して宣言と実装単位を更新し、検査器が候補の欠落、
  余分な項目、未知の責務、宣言と実装の差分を診断する。
- 台帳の範囲、6責務領域、更新方法、保守手順、`libs/ui/tool`を分離する理由を
  アーキテクチャガイドと開発手順へ記録した。

## R1-G2dで完了した作業

- `libs/ui/tool`以下の完全公開ヘッダー集合から、公開マクロを持つクラスと構造体50件を
  40ヘッダーから決定的に採取した。
- 48件を名前に対応する実装単位へ接続し、抽象接続面とインライン実装として宣言側で
  完結する2件を空の実装単位一覧として記録した。
- 50件を入力解釈7件、ツール呼出し14件、ストローク生成9件、描画実行12件、
  設定表示8件へ分類した。
- UIツールクラス責務台帳が、宣言種別、公開ヘッダー、実装単位、`kritaui`所有者、
  5構成、責務領域、ツールディレクトリー外の直接利用元102ソースを接続した。
- 決定的更新器が責務分類を保持して構造と利用元を更新し、検査器が候補の欠落、
  余分な項目、未知の責務、宣言、実装、利用元の差分を診断する。
- 台帳の範囲、5責務領域、更新方法、保守手順と、現在のUI・入力・ストローク・描画責務の
  混在をアーキテクチャガイドと開発手順へ記録した。

## R1-G2eで完了した作業

- 試験経路を除く`plugins`以下から、兄弟JSONを指定する登録マクロ172件を全件採取し、
  プラグインID、実装、メタデータ、登録方式を一対一で接続した。
- 実際のCMake所有ターゲットと対応構成を固定し、macOS 167件、Linux 170件、iOS 162件、
  Android 162件、Windows 168件を記録した。
- 14サービス種別を12の機能所有領域と実行時レジストリーへ接続し、入出力42件、
  画像フィルター33件、ドッカー表示30件などの現在の登録境界を固定した。
- 157件はJSONのライブラリー名をCMake所有者の根拠とし、同項目がないか記録済みターゲットと
  一致しない15件は登録実装を含む`CMakeLists.txt`のソース所属で所有者を固定した。
- 決定的更新器と検査器が、登録の欠落と重複、メタデータ、所有者、ターゲット種別、
  対応構成、サービス種別、機能所有者、実行時読込元の差分を診断する。
- `scope.plugins`を`complete`へ更新し、公開ヘッダー、主要クラス、UIツールクラス、
  プラグインのR1-G2完了条件を満たした。

## R1-G3aで完了した作業

- `docs/architecture/package-responsibilities.json`に、アプリケーション調整、キャンバス表示、
  文書寿命、入出力、入力解釈、描画、プラグイン基盤、リソース管理、ツール呼出しの
  9責務を記録した。
- 9責務を現在の所有ソースディレクトリーと15の中核CMakeターゲットへ接続し、
  `kritaui`が7責務を所有する現在の集中を固定した。
- R1-G2の公開ヘッダー、UI直下124クラス、UIツール50クラス、主要クラス3件、
  プラグイン172登録と14サービス種別を責務へ接続した。
- 15ターゲットについて、macOS、Linux、iOS、Android、Windowsに存在するターゲット種別、
  製品ターゲットへの直接依存、製品ターゲットからの利用元を5台帳の和集合として記録した。
- 決定的更新器と検査器が、責務の欠落と未知ID、所有ターゲットの欠落、責務領域の重複、
  台帳参照、生成済み公開面、プラグイン、ターゲット関係の差分を診断する。
- 責務地図の目的、現在の所有者、読み方、更新方法、保守条件をアーキテクチャガイドと
  開発手順へ記録した。

## R1-G3bで完了した作業

- `docs/architecture/allowed-package-dependencies.json`に9責務を第0層から第7層へ配置し、
  上位層から下位層だけへ向かう有向非巡回の許可グラフを定義した。
- アプリケーション寿命、文書セッション、画像モデル、描画実行、プラグイン登録など
  14の公開接続面に、目的、寿命、エラー動作を記録した。
- 各許可依存を利用する公開接続面へ接続し、プラグイン登録のリンク方向と実行時の
  登録制御を区別した。
- 15の中核所有ターゲット間にある27の直接リンクを、共有ターゲットが所有する責務の
  直積として69候補へ射影した。
- 69候補を同一責務内8件、許可方向35件、R1-G4で基準化する26件へ分類し、後者が
  14種類の責務対を持つことを固定した。
- 決定的更新器と検査器が、未知の責務と公開接続面、自己依存、循環、同層と上位層への
  依存、未分類または陳腐化した現在辺を診断する。
- 責務層、公開接続面、現在辺の射影、更新方法をアーキテクチャガイドと開発手順へ記録し、
  R1 TODOの責務・依存方向と一方向グラフ契約を完了へ更新した。

## R1-G4aで完了した作業

- `docs/architecture/dependency-violation-baseline.json`に、直接includeで一意に帰属できる
  8種類、305件の確認済み逆方向依存を記録した。
- アプリケーション調整から描画71件、キャンバス表示から文書寿命73件、描画から
  文書寿命95件などを、元のCMakeターゲット辺と5構成へ接続した。
- 各確認済み違反にR1-G6の所有段階、現在必要な理由、除去条件、現在件数と等しい
  審査済み上限を設定した。
- 直接includeを一意に帰属できない6種類を未確定射影として分離し、4種類は共有ヘッダー
  1件、2種類は帰属済み直接includeが存在しないことを記録した。
- 採取器が製品ソースと依存先ヘッダーをパス末尾または一意な名前で接続し、単一所有者、
  公開クラス分類、最長責務ディレクトリーの順に責務を決定する。
- 決定的更新器と検査器が、新規違反、審査済み上限の拡大、縮小可能な上限、根拠の置換、
  確認済み責務対と未確定射影の欠落を診断する。
- 基準の範囲、8種類の現在境界、採取規則、更新方法、上限の審査手順をアーキテクチャ
  ガイドと開発手順へ記録した。

## R1-G4bで完了した作業

- `KoStrokeConfigWidget`を審査済み公開ヘッダーとしてキャンバス表示責務へ割り当て、
  全件分類範囲外の公開ヘッダーを責務地図で一意に扱う契約を追加した。
- 共有ターゲット由来の6未確定射影を、`kritabasicflakes`から`kritaui`への3includeと、
  `kritaui`から`kritaimpex`への4includeの実責務へ帰属させ、逆方向依存ではないことを
  `docs/architecture/structural-dependency-baseline.json`へ記録した。
- 15中核ターゲットと全製品構築ターゲットの強連結成分を5構成で採取した。全製品範囲は
  macOS 215件、Linux 221件、iOS 207件、Android 207件、Windows 224件で、両範囲の
  循環上限を0件に固定した。
- 公開マクロを持たず、所有元外の直接includeだけを公開根拠とする`kritaimage`の
  29ヘッダー、593参照と、`kritaui`の15ヘッダー、34参照を既存違反基準へ記録した。
- 構造依存基準の決定的更新器と検査器が、新たな未帰属射影、ターゲット循環、内部
  ヘッダー参照の増加、縮小可能な上限、根拠の置換を診断する。
- 構造基準の範囲、現在値、更新方法、上限の保守手順をアーキテクチャガイドと開発手順へ
  記録し、R1の依存・ヘッダー境界検査ツールをNix高速検査へ固定した。

## R1-G5で完了した作業

- `docs/architecture/package-relocation-plan.json`に9責務の現行所有者と、目標
  ディレクトリー、`Krita`名前空間、主CMakeターゲット、許可依存、完了条件を対応付けた。
- プラグイン基盤の現行境界を第0層として保持し、リソース、描画、入出力、キャンバス、
  文書、ツール、入力、アプリケーションの順にR1-G6aからR1-G6hまでの8段階を確定した。
- 各段階に移動元、移動先、作成ターゲット、必要な特性試験、完了条件、中止条件を設定し、
  8種類305件の逆方向includeと44ヘッダー627件の内部参照を最終的にゼロへ縮小する
  段階別上限を固定した。
- UI旧include、`kritaui`、既存の大域C++識別子を含む11の一時互換経路へ、導入段階、
  R1-G7の所有者、最大範囲、削除条件、検証方法を設定した。
- 最初の実装をR1-G6aに決め、`libs/store`を`libs/resources/storage`へ移す保存契約、
  リソース表示と描画設定表示の分離、47件の逆方向include解消を一つの検査単位にした。
- 計画検査器が責務地図、許可依存、依存違反基準、構造依存基準、5構成の現行
  CMakeターゲットへ計画を照合し、移行順、全件被覆、段階別上限、最終ゼロ状態を検査する。
- 再配置計画の目的、構成、実装順、最初の段階、検査方法、保守条件をアーキテクチャ
  ガイドと開発手順へ記録した。

## R1-G6a保存境界で完了した作業

- ZIPとディレクトリーの読書き、不正ZIPの拒否、失敗した読込後の継続、重複書込の拒否と
  既存データ保持を`TestResourceStorageArchiveContract`へ固定した。
- `libs/store`の書庫保存実装、内部ヘッダー、手動試験を`libs/resources/storage`へ移し、
  `kritaresourcestorage`を独立したライブラリーとして構築した。
- `KoXmlNS`と`KoXmlWriter`を`libs/serialization/xml`へ移し、Qt Coreだけに依存する
  `kritaxmlserialization`として書庫保存から分離した。
- `libs/store`、`kritastore`、転送ヘッダー、旧公開マクロ、互換専用試験を除去し、製品中の
  旧include経路を正規ヘッダーへ変更した。
- 書庫保存とXML直列化の実利用ターゲットへ直接リンクを設定し、利用実体のなかった
  `kritaresourcewidgets`とPSD書出しの保存リンクを除去した。
- XML数値属性の15桁および`FLT_DIG`表現、エスケープ、文書構造を`TestXmlWriter`へ固定し、
  保存契約を`TestResourceStorageArchiveContract`へ限定した。
- 5構成で`kritaresourcestorage`と`kritaxmlserialization`を独立構築し、macOS、Linux、
  Android、Windowsでは共有ライブラリー、iOSでは静的ライブラリーになることを
  CMake台帳へ固定した。
- Windows増分構成へ`Release`構成種別を明示し、清浄な構築木から有効なCMake File API
  構成名を生成する契約を固定した。
- タイル試験の共通補助から`KoStore_p.h`と保存内部状態の操作を除去し、CMakeへ登録されて
  いなかった圧縮試験ソースを削除した。製品外を含め、保存内部ヘッダーは所有パッケージの
  実装3ファイルだけが参照する。
- ネイティブCTestのアプリケーション接頭辞とプラグイン探索先を各増分構築木へ固定した。
  macOSで存在しないNix storeのインストール先を参照していた試験環境を解消した。

## R1-G6a表示境界で完了した作業

- `libs/resourcewidgets`を`libs/resources/ui`へ移し、汎用の選択、タグ、一覧、保管場所の
  表示を`kritaresourceui`として独立構築した。型付きの不変リソース記述子と表示契約を
  `TestResourceUiContract`へ固定した。
- `libs/ui/KisPaintopPropertiesBase.*`、`libs/ui/KisPaletteEditor.*`、
  `libs/ui/kis_categories_mapper.*`、`libs/ui/kis_categorized_*`、
  `libs/ui/kis_composite_ops_model.*`、`libs/ui/kis_paint_ops_model.*`、
  `libs/ui/kis_paintop_option*`、`libs/ui/kis_paintop_settings_widget.*`を
  `libs/tools/ui`へ移した。
- `libs/ui/widgets/kis_categorized_list_view.*`、`libs/ui/widgets/kis_cmb_composite.*`、
  `libs/ui/widgets/kis_paintop_list_widget.*`を`libs/tools/ui`へ移し、描画設定表示を
  `kritatoolsui`として独立構築した。設定表示契約を`TestToolSettingsUiContract`へ固定した。
- `libs/ui/tests`にあった分類モデル試験を`libs/tools/ui/tests`へ移し、製品実装と試験の
  所有先を一致させた。
- `libs/ui/KisResourceServerProvider.*`を起点として、描画プリセットとレイヤースタイルの
  提供処理を`libs/tools/ui/KisPaintResourceServerProvider.*`へ分離した。起点ファイルには
  作業空間、ウィンドウ配置、セッションの提供処理を残した。
- `libs/ui/kis_config.*`を起点として、画像入出力設定を`libs/image/kis_image_config.*`へ、
  画面プロファイル選択を`libs/ui/KisDisplayConfig.*`へ分離した。
- パレット編集の文書操作は`libs/tools/ui/KisPaletteEditor.*`の表示から分離し、
  `plugins/dockers/palettedocker/palettedocker_dock.cpp`が現在の文書と操作ダイアログを
  接続する。
- 旧`libs/resourcewidgets`、旧`kritaresourcewidgets`ターゲット、転送ヘッダーを除去した。
  `kritaresourceui`は描画ターゲットへ依存せず、リソース管理から描画への40件の
  逆方向includeは0件になった。
- 共有ターゲット内に残るパレット、レイヤー設定、プリセット編集の7ソースを
  `reviewedSourcePaths`でツール呼出し責務へ帰属させ、未確定射影を0件に保った。
- 確認済み逆方向依存は5責務対257件へ縮小した。全5構成で16中核ターゲットと
  全製品ターゲットの循環は0件である。

## R1-G6b描画実行境界で完了した作業

- `libs/ui/tool/strokes`のストローク生成・実行を`libs/painting/strokes`へ移した。
  同じ起点にあった`KisAsynchronousStrokeUpdateHelper.*`、
  `KisStrokeCompatibilityInfo.*`、`KisStrokeSpeedMonitor.*`、
  `kis_resources_snapshot.*`も`libs/painting`へ移し、`kritapainting`が所有する。
- `libs/command`の画像・キャンバス向け取り消し処理を`libs/painting/undo`へ移し、
  `kritapaintingundo`として分離した。`libs/metadata`の画像メタデータ実装は
  `libs/painting/metadata`へ移し、`kritapaintingmetadata`として分離した。旧ディレクトリー、
  旧メタデータターゲット、転送ヘッダーは残していない。
- 画像層が利用する取り消しとメタデータを画像層より下、画像層を利用するストローク実行を
  画像層より上に分けた。これにより3責務を描画所有へ集約しながらCMakeターゲットの循環を
  発生させない構成にした。
- `libs/ui/tool/kis_resources_snapshot.*`を起点とした資源スナップショットは、UIの具体的な
  資源提供者ではなく`libs/resources`の読出し接続面を保持する。呼出し側が所有する提供者から
  読出し接続面を渡すことで、描画処理からUI資源管理の知識を除去した。
- `libs/ui/tool/KisStrokeSpeedMonitor.*`を起点とした速度計測は、設定の読取りをUI側へ移し、
  描画側は呼出し側から有効状態を受け取る。`libs/ui/tool/kis_tool_utils.*`にあった画像状態だけを
  扱う色採取とノード検索は`libs/painting/kis_painting_utils.*`へ移した。
- 描画から文書寿命への直接includeは95件から0件になった。`kritaimage`の29ヘッダー593参照は、
  `libs/painting/tests/TestPublicImageHeaders.cpp`の構築契約で公開面を確定し、未審査の内部参照を
  0件にした。`kritaui`の内部参照は14ヘッダー32件まで縮小した。
- 5構成のCMake台帳はmacOS 634件、Linux 649件、iOS 568件、Android 574件、Windows 604件、
  共通552件、条件付き119件、構成差252件を記録する。18中核所有ターゲットと全製品ターゲットは
  5構成すべてで循環0件を維持する。

## EXIF構造化メタデータ読込修正で完了した作業

- `plugins/metadata/exif/kis_exif_io.cpp`を起点として、OECFとCFAの列数と行数を
  EXIF形式で定められた16ビット値として読み、ホストの整数幅とExiv2の版に依存しない
  寸法解釈へ統一した。
- OECFは列名領域と有理数領域、CFAは画素配列について、列数と行数から求めた必要量を
  入力長と照合する。ゼロ寸法、積の表現範囲超過、切り詰め、余分なデータを不正値として扱う。
  実機由来データで確認した列名の全省略は、有理数領域の長さが寸法と完全一致する場合だけ
  列数分の空名として受理する。
- 不正な構造化項目はその項目だけを読込対象から外し、同じEXIFデータに含まれるカメラ機種などの
  正常なメタデータを保持する。CFAの各値は符号なし8ビット値として保持する。
- `plugins/metadata/tests/kis_exif_test.cpp`に、実在するOECFの2列129行、切り詰めを誘発する
  異常寸法、CFAの2行2列と値255の往復契約を追加した。EXIF試験の入口は画像・UI資源を
  初期化しない構成へ限定し、macOSとLinuxで同じ入出力契約を実行可能にした。
- `plugins/impex/jpeg/tests/kis_jpeg_test.cpp`の既存JPEG読込契約で、問題を再現した
  `HPIM0760.JPG`を含む入力群が有限メモリーで完了することを確認した。

## R1-G6c入出力境界で完了した作業

- `libs/ui/KisImportExportFilter.*`、`KisImportExportErrorCode.*`、
  `KisImportExportAdditionalChecks.*`を`libs/impex`へ移した。形式探索とMIME選択は
  `KisImportExportFilterRegistry.*`が所有し、結果分類、ファイル事前条件、変換フィルターと
  合わせて`kritaimpex`だけで構築・検査できる。
- `libs/ui/KisImportExportManager.*`と`KisImportUserFeedbackInterface.*`を起点として、
  文書変換の調整、利用者通知、ダイアログ、クリップボード、画像読込補助を
  `libs/impex/ui`へ移した。動画符号化調整は`libs/impex/animation`へ移し、
  `kritaimpexui`が両ディレクトリーを所有する。
- `kritaimpexui`は文書・画面型との現在のABI接続を保つオブジェクト所有単位として
  `kritaui`へ全実装を組み込む。入出力の製品実装は`kritaui`のソース一覧に属さず、
  5構成のCMake台帳が所有ターゲットと依存方向を別々に記録する。
- `KisMimeData`はノード追加・移動に必要な狭い接続面を所有し、UI側のノード挿入実装が
  その接続面を実装する。参照画像のクリップボード読込はQtの画像値を受け取る経路へ変更し、
  キャンバス表示から入出力への逆方向includeを除去した。
- `libs/ui`の旧入出力ファイル、旧ダイアログ、旧媒体符号化ファイルを削除し、利用元を
  正規の`libs/impex`経路へ更新した。転送ヘッダーと互換分岐は追加していない。
- UI直下の入出力14クラスは所有先へ移り、UI直下クラス台帳は92件になった。
  `kritaimpex`と`kritaimpexui`の未宣言パッケージ外参照は0件、`kritaui`の内部参照は
  11ヘッダー28件になった。19中核所有ターゲットと全製品ターゲットは全5構成で循環0件を保つ。
- Android増分構成へ`Release`構成種別を明示し、AndroidのCMake File API台帳を
  他の構成と同じ決定的な構成名から再生成できるようにした。
- `docs/architecture/TODO.md`に、UIから利用事例を呼び出す接続、起動時の実装登録、
  ドメイン計算とI/O副作用の分離を長期ビジョンとして記録した。正式評価はキャンバス表示と
  文書寿命の分離後に行い、専用の移行は保守責任者の明示的な決定後に開始する。

## キャンバス座標境界で完了した作業

- `libs/ui/canvas/kis_coordinates_converter.*`と`KisCanvasState.*`を`libs/canvas`へ
  移し、独立した共有ライブラリー`kritacanvas`が座標変換と画面状態を所有するようにした。
  旧ファイルと転送ヘッダーは残していない。
- 座標変換器から設定読込みを除き、`libs/ui/canvas/kis_canvas2.cpp`が表示設定値を
  明示的に渡すようにした。変換器は構築元画像を保持せず、画像解放後も構築時の幾何情報と
  変換結果を利用できる寿命契約を追加した。
- `libs/ui/tests/kis_coordinates_converter_test.*`を`libs/canvas/tests`へ移し、
  `kritaui`をリンクせずに画像寿命、設定入力、既存の座標変換を検査するようにした。
- 構造検査が`kritacanvas`の実体、旧配置の不在、UI設定・文書・表示型への逆方向includeの
  不在を継続確認する。公開面台帳は`kritacanvas`の2ヘッダーを記録する。
- 5構成のCMake台帳は`kritacanvas`を独立所有ターゲットとして記録し、20の中核所有
  ターゲットと全製品ターゲットは全構成で循環0件を維持する。
- この単位では投影更新、色変換、動画キャッシュおよび長期構造ビジョンの専用移行を
  開始していない。長期構造の専用移行は保守責任者の明示的な判断を開始条件とする。

## 投影更新境界で完了した作業

- `libs/ui/canvas/kis_prescaled_projection.*`を起点として、表示用画像片、投影更新情報、
  投影取得接続面、拡大縮小済みフレームを`libs/canvas`へ移し、`kritacanvas`が所有する
  公開面を8ヘッダーへ拡張した。旧配置、転送ヘッダー、型別名だけを集めたヘッダーは
  残していない。
- 投影フレーム生成からUI設定の読取りと具体的な表示フィルター型を除き、呼出し側が
  更新片の寸法、画面プロファイル、色変換方法、画素フィルターを明示的に渡す構成にした。
  `libs/ui/canvas/kis_qpainter_projection_factory.*`がUI設定と具体的な画像投影実装を
  キャンバス所有の接続面へ結ぶ。
- 共通の更新情報は`libs/canvas/kis_update_info.*`が所有し、QPainter投影向けの更新情報を
  `libs/canvas/kis_projection_update_info.*`、OpenGL固有のタイル更新情報を
  `libs/ui/opengl/kis_opengl_update_info.*`へ分けた。キャンバス側はOpenGLタイル型を
  参照しない。
- `libs/canvas/tests/kis_prescaled_projection_contract_test.*`が、画像の汚れ領域を投影更新へ
  通知すること、画像外の空更新では直前の有効フレームを保持することを、UIライブラリーを
  リンクせずに検査する。
- 構造検査が新しい所有先、旧配置の不在、UI設定、表示型、OpenGLタイル型、文書型への
  逆方向includeの不在を継続確認する。表示色変換と動画キャッシュは後続の独立単位とする。

## 表示色変換境界で完了した作業

- `libs/ui/KisOcioConfiguration.*`と`libs/ui/KisSurfaceColorSpaceWrapper.h`を起点として、
  表示色の設定値とQt画面色空間との変換値を`libs/canvas/color`へ移した。旧配置と
  転送ヘッダーは残さず、プラグインとUIは`kritacanvas`の公開面を直接参照する。
- `libs/ui/canvas/kis_display_color_converter.*`から画素、色、画像の変換処理を
  `libs/canvas/color/kis_display_color_transform.*`へ分離した。変換本体はUI設定、現在ノード、
  資源管理、画面パレットを参照せず、プロファイル、変換方法、表示フィルターを入力として
  色変換を実行する。
- UI側の色変換器は変換本体を保持し、現在ノードと設定通知の監視、前景色の視覚表現維持、
  ハンドル色とシステムパレットの更新、利用元への変更通知を担当する。
- `libs/canvas/tests/kis_display_color_transform_test.*`が標準表示変換、往復変換、
  プロファイル一致時の省略判定、表示フィルター適用、ビット深度別キャッシュをUIなしで
  検査する。`KisSurfaceColorSpaceWrapperTest`も`libs/canvas/tests`へ移した。
- `libs/ui/tests/kis_display_color_converter_contract_test.cpp`が画面設定を一度だけ反映し、
  同じ設定の再入力で通知を重複させず、UI接続後も表示色結果を維持することを検査する。
- 公開面は`kritacanvas`の12ヘッダーへ拡張し、未宣言だった`kritaui`内部参照の基準を
  11ヘッダー28件から9ヘッダー24件へ縮小した。動画キャッシュは後続の独立単位とする。

## アニメーションキャッシュ境界で完了した作業

- `libs/ui/kis_animation_frame_cache.*`を起点として、フレーム範囲の検索、挿入、無効化、
  結合と移動指示を`libs/canvas/animation/kis_animation_frame_cache_index.*`へ分離した。
  UI側のキャッシュはこの状態を保持し、画像の再生状態と保存・復元処理を調整する。
- `libs/ui/KisFrameDataSerializer.*`と`libs/ui/KisFrameCacheStore.*`を起点として、
  タイル差分の直列化とフレーム保存を`libs/canvas/animation`へ移した。保存値はUI型と
  OpenGL型を含まず、UI側の`libs/ui/animation/cache/KisFrameCacheSwapper.*`が
  OpenGL更新情報との相互変換を担当する。
- `libs/ui/opengl/kis_texture_tile_info_pool.*`を、描画方式に依存しないタイル転送領域として
  `libs/canvas/tiles`へ移した。旧クラス名、旧ファイル、転送ヘッダーは残していない。
- `libs/ui/kis_animation_cache_populator.*`とキャッシュ調停を`libs/ui/animation`へ、
  保存変換を`libs/ui/animation/cache`へ配置した。`libs/image/kis_types.h`からUIキャッシュ型の
  別名を除き、UI所有の前方宣言を利用元が明示的に参照する。
- `libs/ui/KisWidgetWithIdleTask.h`を`libs/ui/canvas`へ移し、ドッカーから利用する表示契約を
  `libs/ui/tests/TestCanvasUiPublicHeaders.cpp`の構築で固定した。旧配置は残していない。
- フレーム範囲、保存、直列化をUIライブラリーなしで検査する3契約と、UIの保存変換、
  既存キャッシュ統合、公開表示ヘッダーを検査する3契約を固定した。
- `kritacanvas`の公開面は17ヘッダー、`kritaui`は249ヘッダーになった。未宣言の
  `kritaui`内部参照は9ヘッダー24件から7ヘッダー20件へ縮小し、キャンバス表示から
  文書寿命への逆方向includeは0件を維持する。これによりR1-G6dの完了条件を満たす。

## R1-G6e文書取り消し境界の実装

- R1-G6e開始時にUIの文書状態へ分類した25クラスのうち、最初の起点を
  `libs/ui/kis_document_undo_store.*`とした。文書全体への参照を取り消し履歴の直接借用へ
  狭め、履歴操作と変更通知を`libs/document/undo`の`kritadocument`へ移した。
- `KisDocument`は履歴を先に構築して接続へ渡し、履歴が接続より長く存続する責務を持つ。
  接続は非nullと同一スレッドを検査し、履歴位置の通知を同期転送する。
- 空履歴の現在操作、命令追加、直前命令の取消し、マクロの一括化、やり直し履歴の破棄、
  履歴位置変更の同期通知、同一スレッド、非所有の借用寿命を専用契約へ固定した。
- 旧ファイル、転送ヘッダー、旧クラス名の別名は存在しない。UI直下の文書状態分類は
  24クラスとなり、公開面台帳は文書所有の正規ヘッダーを5構成へ接続する。
- 5構成のCMake台帳は`kritadocument`を記録する。LibrePaint内の直接依存は
  `kritapaintingundo`、直接利用元は`kritaui`であり、中核所有ターゲットと全製品ターゲットの
  循環は全構成で0件を維持する。
- この単位は取り消し履歴の所有境界だけを扱う。文書識別、変更状態、保存、自動保存、
  回復、文書情報、ノードと選択の操作は後続レビュー単位とする。利用事例の登録構造と、
  ドメイン計算からI/Oを分ける専用移行は開始しない。
- 取り消し接続は`KisDocument`と`kritaui`を参照せず、`kritadocument`はLibrePaint内では
  `kritapaintingundo`だけを下位依存として構築できる。既存の文書・画像の取り消し挙動は
  専用契約とUI利用元の構築で維持する。
- `kritapaintingundo`は操作履歴とQt Widgetsのアクション生成を同じライブラリーで提供するため、
  この単位だけでは文書ドメインのQt Widgets依存は完了条件を満たさない。履歴と表示用アクションの
  分離要否はR1-G6eの後続単位で判断し、最初の単位へ新しい接続面を追加しない。

## R1-G6e文書識別境界の実装

- `libs/ui/KisDocument.cpp`の文書パス、実ファイルパス、現在MIME形式、MIME自動判定由来を
  起点として、`libs/document/session/kis_document_identity.{h,cpp}`の
  `Krita::Document::Identity`へ移した。`KisDocument`は既存の公開API、パス変更通知、
  MIME判定とファイルを開く調整を維持する。
- 文書識別はQt Coreの値状態として、表示用パスと入出力用実ファイルパスを独立して保持する。
  パスの実変更判定、MIME形式と自動判定由来、複製をUIなしの契約で固定した。
- `KisDocument`の接続契約は、同一パスの再設定では通知しないこと、パス初期化時の通知、
  パス初期化通知時点では実ファイルパスを保持して通知後に消去する順序、実ファイルパスと
  MIME形式、保存用スナップショットへの識別状態の複製を固定する。
- 旧`KisDocument::Private::outputMimeType`は設定と複製だけが行われ、読み取る利用元が
  存在しなかったため削除した。互換経路、転送ヘッダー、旧名の別名は追加していない。
- 公開面台帳は`kritadocument`の文書識別ヘッダーを、`kritaui`の直接利用と5構成の
  対象へ接続する。文書識別の抽出ではUI直下の`document-state`分類24クラスは変わらない。
- この単位の実装範囲は文書識別だけとし、変更状態は次の独立単位へ分けた。保存、自動保存、
  回復、文書情報、ノードと選択の操作、および利用事例登録とI/O分離の専用移行は含まない。

## R1-G6e文書変更状態境界の実装

- `libs/ui/KisDocument.cpp`の変更済み、自動保存後変更、保存中変更、取り消し履歴に現れない
  画像変更の4状態を起点として、
  `libs/document/session/kis_document_modification_state.{h,cpp}`の
  `Krita::Document::ModificationState`へ移した。
- 文書変更状態は、同じ変更済み値の再設定でも自動保存後と保存中の変更を再記録する。
  未変更への遷移は取り消し不能変更を消去し、失敗した自動保存は現在の変更済み状態から
  次回自動保存の必要性を復元する。
- 保存用の状態複製は変更済みと取り消し不能変更を引き継ぎ、元の文書で進行している保存と
  自動保存の経過を複製先へ持ち込まない。
- `KisDocument`は既存の`isModified()`、`setModified()`、`modified(bool)`を維持し、
  編集時刻、文書情報更新、自動保存タイマー、保存・回復処理、Qt通知を接続する。
- 互換経路、転送ヘッダー、旧名の別名は追加していない。変更状態の実装はQt型を参照せず、
  公開面台帳は`kritadocument`の所有、`kritaui`の直接利用、5構成の対象へ接続する。
- この抽出は`KisDocument`内の埋込み状態を移すため、UI直下の`document-state`分類は
  24クラスを維持する。保存、自動保存、回復、文書情報、ノードと選択の操作は後続単位とする。
  利用事例登録と純粋計算・I/O分離の専用移行は、保守責任者が開始を決定する段階まで
  現行構造を維持する。

## R1-G6e文書自動保存実行状態境界の実装

- `libs/ui/KisDocument.cpp`の自動保存用複製の書出し中状態と連続失敗回数を起点として、
  `libs/document/session/kis_document_autosave_state.{h,cpp}`の
  `Krita::Document::AutoSaveState`へ移した。
- 自動保存用複製の書出し開始と終了、3回の連続失敗後に次の試行で複製経路へ切り替える
  境界値、通常間隔へ戻る際の失敗履歴消去をUIなしの契約で固定した。
- `KisDocument`は自動保存タイマー、有効状態と通常・緊急間隔、文書複製、ファイル出力、
  状態表示、回復用自動保存の調整を維持する。既存の`isAutosaving()`公開APIは新しい状態を
  読み取る。
- 読み書きされていなかった旧`disregardAutosaveFailure`は除去した。互換経路、転送ヘッダー、
  旧名の別名は追加していない。
- この抽出は`KisDocument`内の埋込み状態を移すため、UI直下の`document-state`分類は
  24クラスを維持する。自動保存I/Oと回復処理の分離、利用事例登録、純粋計算・I/O分離の
  専用移行は開始していない。

## R1-G6e文書回復自動保存調停状態境界の実装

- `libs/ui/KisDocument.cpp`の回復用自動保存要求、保存開始中状態、同期完了の延期結果、
  既存保存への合流先を起点として、
  `libs/document/session/kis_document_recovery_autosave_state.{h,cpp}`の
  `Krita::Document::RecoveryAutoSaveState`へ移した。
- 未処理要求の開始と取消し、利用可能な既存保存への合流、保存開始中に届いた同期完了の
  延期、開始失敗時の延期破棄、要求ごとの一度限りの完了をUIなしの契約で固定した。
- `KisDocument`は変更状態、自動保存タイマー、背景保存の開始と継続、ファイルの存在と
  大きさの検証、状態表示、`sigRecoveryAutoSaveFinished`通知を維持する。
- 旧6フィールドは除去した。互換経路、転送ヘッダー、旧名の別名は追加していない。
- この抽出は`KisDocument`内の埋込み状態を移すため、UI直下の`document-state`分類は
  24クラスを維持する。回復I/O、利用事例登録、純粋計算・I/O分離の専用移行は開始していない。

## R1-G6e文書回復状態境界の実装

- `libs/ui/KisDocument.cpp`の回復済み文書状態を起点として、
  `libs/document/session/kis_document_recovery_status.{h,cpp}`の
  `Krita::Document::RecoveryStatus`へ移した。
- 通常文書の初期状態、実際の状態遷移だけを通知対象とする変更判定、文書状態の値コピーを
  UIなしの契約で固定した。既存`KisDocument`契約は同値再設定時の通知抑制と、保存用
  スナップショットが通常文書状態から始まる挙動を固定する。
- `KisDocument`は回復データの探索と読込、保存後の回復ファイル消去、表示、
  `sigRecoveredChanged`通知を維持する。
- 旧`isRecovered`フィールドは除去した。互換経路、転送ヘッダー、旧名の別名は
  追加していない。
- この抽出は`KisDocument`内の埋込み状態を移すため、UI直下の`document-state`分類は
  24クラスを維持する。回復I/O、利用事例登録、純粋計算・I/O分離の専用移行は開始していない。

## R1-G6e-P0文書パッケージ境界計画で完了した作業

- `AGENTS.md`に、依存方向、具体的な命名、現存する関心領域の分割と集約、必要性を確認した
  ロジック再構築と抽象化というリファクタリング順序を固定した。
- YAGNIを優先し、差し替え、値の受渡しでは成立しない試験境界、外部処理の置換、移動だけでは
  解けない循環の根拠がない利用事例層、接続面、アダプター、リポジトリー、サービス探索器、
  共通基底、空ターゲットを追加しない運用規則を固定した。
- `docs/architecture/document-package-boundary-plan.md`に、文書状態の`kritadocument`、
  文書ファイル保存の`kritadocumentfiles`、文書表示の`kritadocumentui`という
  現在確認できる具体所有と一方向の依存を記録した。
- 実装をP1取り消し履歴の文書UI境界、P2文書表示の集約、P3文書ファイル保存の集約、
  P4残る境界評価の4検査段階へ分けた。ロジック再構築とI/O隔離はP4で現在の根拠を
  確認してから独立段階として計画する。
- 最初の実装単位は`libs/document/undo/kis_document_undo_store.{h,cpp}`と
  `libs/command/{kundo2model,kundo2view}.{h,cpp}`を起点とし、文書と取り消し履歴の接続および
  履歴表示を`libs/document/ui/undo`へ移す。`kritadocument`の公開リンク閉包をQt Coreだけへ
  縮小した状態をP1の完了条件とする。

## R1-G6e-P1取り消し履歴の文書UI境界で完了した作業

- `libs/document/undo/kis_document_undo_store.{h,cpp}`を起点として
  `libs/document/ui/undo/kis_document_undo_store.{h,cpp}`へ移し、文書と画像所有の履歴を
  接続する責務を`kritadocumentui`へ移した。
- `libs/command/{kundo2model,kundo2view}.{h,cpp}`を起点として
  `libs/document/ui/undo/{kundo2model,kundo2view}.{h,cpp}`へ移し、履歴の行、選択状態、
  操作名、有効状態を文書UI所有へ集約した。
- `kritaui`は`kritadocument`と`kritadocumentui`を直接利用する。`kritadocumentui`は
  `kritapaintingundo`を利用し、`kritadocument`はQt Core以外の公開リンク依存を持たない。
- 履歴表示だけを所有していた旧`kritacommand`ターゲットと`libs/command`の旧ファイルを
  除去した。旧配置の転送ヘッダー、別名、互換ターゲットは追加していない。
- 既存の履歴操作、マクロ、やり直し破棄、同期通知、非所有の借用寿命を維持し、履歴表示、
  操作名、取消し・やり直しの有効状態、履歴選択による移動を新しい特性契約で固定した。
- 新しい利用事例層、永続化層、接続面、アダプター、サービス探索器、共通基底は追加せず、
  現在存在する実装と依存を具体的な所有へ移した。

## 検証状態

- 初回の`TestXmlWriter`構築は、構築対象外だった旧試験が存在しないAPIと古い構築子を
  参照していることを診断した。現在の公開面に対する数値、エスケープ、構造契約へ置換した。
- `nix develop .#test --command ./scripts/run-test TestResourceStorageArchiveContract`:
  ZIP、ディレクトリー、不正入力、失敗後の継続、重複書込の5契約がmacOSとLinuxで成功した。
- `nix develop .#test --command ./scripts/run-test TestXmlWriter`: doubleとfloatの数値表現、
  属性とテキストのエスケープ、字下げを含む文書構造がmacOSとLinuxで成功した。
- macOSで`kritaflake`、`kritaimage`、`kritaui`、KRA/ORA入出力、画像・パス図形、既定
  ツールを含む1,549工程の主要利用先構築が成功した。
- macOS、iOS、Linux、Android、Windowsで`kritaresourcestorage`と
  `kritaxmlserialization`の実構築が成功した。
- 5構成のCMake台帳と差分行列を再生成した。全構成から`kritastore`と互換専用試験が消え、
  書庫保存はQt Core、KConfig、QuaZip、XML直列化はQt Coreだけへ直接依存する。
- `nix develop .#test --command ./scripts/verify-quick`: 86件の単体試験、責務・依存・構造台帳、
  再配置計画、文書、リンク、D2再生成を含む高速検査が成功した。
- `nix flake check --no-build --all-systems`: 全Nix出力の評価が成功した。
- macOSの全3,563構築工程が成功し、LibrePaint本体、全製品プラグイン、登録済み試験を
  旧保存互換なしでリンクした。
- `ctest --preset tdd-macos`: 書庫保存、XML直列化、タイル保存を含む275件が成功した。
  試験接頭辞修正前の資源経路による125件の異常終了は解消した。残る39件は既存の画像基準、
  Qt 6モデル契約、macOSファイル権限、300秒制限、セグメンテーション違反で失敗する。
  `TestResourceStorageArchiveContract`、`TestXmlWriter`と9件の登録済みタイル試験は成功する。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos`:
  同一コミットのmacOS、iOS、Linux、Android、Windows台帳と差分行列が成功した。
- `nix develop .#test --command ./scripts/run-test TestResourceUiContract`、
  `TestToolSettingsUiContract`、`TestKisPaletteModel`: macOSでリソース記述子、描画設定表示、
  パレットモデルの契約が成功した。
- macOSで`kritaui`、iOSでLibrePaint本体まで構築し、表示境界の分離後もリンクが成功した。
- `ssh nixos`上のx86_64 Linuxで`kritaui`、Android arm64-v8aで
  `libkritaui_arm64-v8a.so`、Windows x86_64で`libkritaui.dll`の構築とリンクが成功した。
- 5構成のCMake台帳を再生成した。macOS 630件、Linux 645件、iOS 564件、Android 570件、
  Windows 600件のターゲット、548件の共通ターゲット、119件の条件付きターゲット、
  250件の構成差を持つターゲットを記録した。
- `nix develop .#test --command ./scripts/verify-quick`: 88件の単体試験、責務・依存・構造台帳、
  再配置計画、文書、リンク、D2再生成を含む高速検査が成功した。
- `nix flake check --no-build --all-systems`: 表示境界分離後の全Nix出力の評価が成功した。
- `nix develop .#test --command ./scripts/verify`: macOSの全2,395構築工程が成功し、
  CTest 316件中279件が成功した。追加した表示境界契約はすべて成功し、残る37件は
  既存の画像基準、Qt 6モデル契約、macOS環境、300秒制限、セグメンテーション違反で失敗する。
- `ctest --preset tdd-macos`で描画境界、公開画像ヘッダー、取り消し、メタデータ、画像、投影、
  トランザクション、ストローク、取り消し付きストローク、ストロークキューの対象10件が成功した。
- `nix develop .#test --command ./scripts/verify-quick`: 88件の単体試験、責務・依存・構造台帳、
  再配置計画、文書、リンク、D2再生成を含む高速検査が成功した。
- `nix flake check --no-build --all-systems`: 描画境界分離後の全Nix出力の評価が成功した。
- 描画境界分離後の`kritaui`をmacOSとx86_64 Linuxで、LibrePaint本体をiOSで構築した。
  Android arm64-v8aの`libkritaui_arm64-v8a.so`とWindows x86_64の`libkritaui.dll`も構築し、
  5構成すべてで最終生成物のリンクが成功した。
- Androidの初回構築は、資源キャッシュの共有ポインターを`QVariant`から取り出す箇所で
  完全型を要求するQt 5の診断を記録した。`libs/painting/kis_resources_snapshot.cpp`が
  資源キャッシュ接続面の定義を直接includeするよう修正し、AndroidとiOSの再構築が成功した。
- `nix develop .#test --command ./scripts/verify`: macOSの全3,201構築工程が成功し、
  CTest 318件中281件が成功した。追加した描画境界と公開画像ヘッダーの2契約は成功し、
  失敗37件は前段階と同数で、既存の画像基準、Qt 6モデル契約、macOS環境、300秒制限、
  セグメンテーション違反に分類される。
- 既存失敗の`plugins-impex-jpeg-kis_jpeg_test`は実メモリが最大約4.2 GiB、システムの
  スワップ使用量が約23.3 GiBへ増加したため293秒時点で終了した。終了後のスワップ使用量は
  約2.8 GiB、空きメモリ指標は88%へ回復した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos`:
  同一コミットのmacOS、iOS、Linux、Android、Windows台帳と差分行列が成功した。
- macOSで`KisExifTest`のOECF、異常OECF、CFA契約を実行し、5件すべて成功した。
  `kis_jpeg_test::testFiles`は`HPIM0760.JPG`を含む3件すべてが成功し、最大常駐メモリーは
  209,027,072バイトだった。修正前の制御実行では最大約13.4 GiBまで増加していた。
- x86_64 Linuxで同じ`KisExifTest`の5件と`kis_jpeg_test::testFiles`の3件が成功した。
  EXIF試験から不要なUI資源初期化を除いたため、画面やフォント資源に依存せず構造化
  メタデータ契約を実行できる。
- `kritaexif`をmacOSとx86_64 Linuxで共有モジュール、iOSで静的ライブラリー、Android
  arm64-v8aで`kritaexif_arm64-v8a.so`、Windows x86_64で`kritaexif.dll`として構築した。
  iOSはLibrePaintアプリケーション本体までリンクし、5構成で利用先との接続が成功した。
- `nix develop .#test --command ./scripts/verify-quick`: 88件の単体試験、責務・依存・構造台帳、
  再配置計画、文書、リンク、D2再生成を含む高速検査が成功した。
- `nix flake check --no-build --all-systems`: EXIF修正後の全Nix出力の評価が成功した。
- `nix develop .#test --command ./scripts/verify`: macOSの全製品と試験のリンクが成功し、
  CTest 318件中281件が成功した。失敗37件は前段階と同数である。JPEG対象は2.67秒で終了し、
  読込契約を含む5件が成功した。残る失敗は既存のmacOS読取専用出力契約1件である。
- `TestImportExportBoundary`、`TestImportExportUiBoundary`、`KisMimeDatabaseTest`が成功した。
  形式探索の依存方向、結果分類、ファイル事前条件、利用者通知の一括処理、
  `.kra`のMIME判定を固定した。
- `nix develop .#test --command ./scripts/verify`: macOSの全製品と試験のリンクが成功し、
  CTest 321件中285件が成功した。追加した入出力境界契約とKRA保存往復は成功した。
  残る36件は既存の画像基準、Qt 6モデル契約、macOS環境、300秒制限、
  セグメンテーション違反に分類される。KRA保存の残る2件はmacOSの読取専用出力と
  利用できない`rec2100 PQ 203 nits`プロファイルによる既存失敗である。
- macOSで`kritaui`、iOSでLibrePaintアプリケーション本体、x86_64 Linuxで
  `kritaui`を構築した。Android arm64-v8aの`libkritaui_arm64-v8a.so`と
  Windows x86_64の`libkritaui.dll`も構築し、5構成すべてで最終生成物のリンクが成功した。
- 5構成のCMake台帳を再生成した。macOS 638件、Linux 653件、iOS 572件、Android 578件、
  Windows 608件のターゲット、556件の共通ターゲット、119件の条件付きターゲット、
  256件の構成差を持つターゲットを記録した。19中核所有ターゲットと全製品ターゲットは
  5構成すべてで循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`: 90件の単体試験、責務・依存・構造台帳、
  再配置計画、文書、リンク、D2再生成を含む高速検査が成功した。
- `nix flake check --no-build --all-systems`: 入出力境界分離後の全Nix出力の評価が成功した。
- `nix develop .#test --command ./scripts/run-test kis_coordinates_converter_test`:
  macOSで座標変換、画像解放後の寿命、明示的な表示設定入力を含む1件が成功した。
- `nix develop --no-eval-cache .#test --command ./scripts/verify`: macOSの全製品と
  試験のリンクが成功し、CTest 321件中285件が成功した。`libs-canvas`所有へ移した
  座標変換試験も成功した。残る36件は直前の入出力境界分離時と同数であり、既存の
  画像基準、Qt 6モデル契約、macOS環境、300秒制限、セグメンテーション違反に分類される。
- `nix develop --no-eval-cache .#test --command ./scripts/build-incremental ios build --allow-large`:
  iOSの`libkritacanvas.a`と`LibrePaint.app/LibrePaint`のリンクが成功した。
- Android arm64-v8aで`libkritacanvas_arm64-v8a.so`と`libkritaui_arm64-v8a.so`の
  リンクが成功した。
- x86_64 Linuxで`libkritacanvas.so`と`libkritaui.so`のリンク、および
  `libs-canvas-kis_coordinates_converter_test`の1件が成功した。
- Windows x86_64で`libkritacanvas.dll`と`libkritaui.dll`のリンクが成功した。
- 5構成のCMake台帳を再生成した。macOS 639件、Linux 654件、iOS 573件、Android 579件、
  Windows 609件のターゲット、557件の共通ターゲット、119件の条件付きターゲット、
  257件の構成差を持つターゲットを記録した。20中核所有ターゲットと全製品ターゲットは
  5構成すべてで循環0件を維持する。
- `nix develop --no-eval-cache .#test --command ./scripts/verify-quick`: 92件の単体試験、
  責務・依存・構造台帳、再配置計画、文書、リンク、D2再生成を含む高速検査が成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: キャンバス座標境界分離後の
  全Nix出力の評価が成功した。
- `nix develop --no-eval-cache .#test --command ./scripts/run-test kis_prescaled_projection_contract_test`:
  macOSで汚れ領域通知と最終有効フレーム保持の2契約が成功した。
- `nix develop --no-eval-cache .#test --command ./scripts/run-test kis_prescaled_projection_test`:
  既存の拡大縮小、移動、回転、更新契約がmacOSで成功した。
- `nix develop --no-eval-cache .#test --command ./scripts/verify`: macOSの全製品と全試験の
  構築・リンクが成功し、CTest 322件中287件が成功した。追加した投影更新契約は成功し、
  残る35件は直前の36件から増加していない。
- `nix develop --no-eval-cache .#test --command ./scripts/build-incremental ios build --allow-large`:
  iOSの`libkritacanvas.a`と`LibrePaint.app/LibrePaint`のリンクが成功した。
- `nix develop --no-eval-cache .#test --command ./scripts/verify-quick`: 94件の単体試験、
  責務・依存・構造台帳、再配置計画、文書、リンク、D2再生成を含む高速検査が成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: 投影更新境界分離後の全Nix出力の
  評価が成功した。
- x86_64 Linuxで`libkritacanvas.so`と`libkritaui.so`のリンク、および
  `libs-canvas-kis_prescaled_projection_contract_test`の2契約が成功した。
- Android arm64-v8aで`libkritacanvas_arm64-v8a.so`と`libkritaui_arm64-v8a.so`、
  Windows x86_64で`libkritacanvas.dll`と`libkritaui.dll`のリンクが成功した。
- 5構成のCMake台帳を再生成した。macOS 640件、Linux 655件、iOS 574件、Android 580件、
  Windows 610件のターゲット、558件の共通ターゲット、119件の条件付きターゲット、
  257件の構成差を持つターゲットを記録した。20中核所有ターゲットと全製品ターゲットは
  5構成すべてで循環0件を維持する。
- `kis_display_color_transform_test`、`KisSurfaceColorSpaceWrapperTest`、
  `kis_display_color_converter_contract_test`、`kis_prescaled_projection_test`がmacOSで
  成功した。表示色変換試験は、実際に登録された標準色空間のプロファイルを入力とし、
  試験構築時だけ色管理プラグインを用意して高ビット深度の変換経路も検査する。
- `nix develop --no-eval-cache .#test --command ./scripts/verify`: macOSの全製品と全試験の
  構築・リンクが成功し、CTest 324件中288件が成功した。追加した表示色変換とUI接続の
  2契約、および直前の投影更新契約は成功した。失敗36件のうち35件は直前から継続し、
  追加の`libs-widgetutils-TestKoProgressUpdater`も単独再実行で失敗する変更範囲外の試験である。
- `nix develop --no-eval-cache .#test --command ./scripts/build-incremental ios build --allow-large`:
  iOSの`libkritacanvas.a`、`libkritaui.a`、`LibrePaint.app/LibrePaint`のリンクが成功した。
- x86_64 Linuxで`QT_QPA_PLATFORM=offscreen`を設定し、
  `libs-canvas-kis_display_color_transform_test`と
  `libs-ui-kis_display_color_converter_contract_test`が成功した。`libkritacanvas.so`、
  `libkritaui.so`と両試験実行ファイルのリンクも成功した。
- Android arm64-v8aで`libkritacanvas_arm64-v8a.so`と`libkritaui_arm64-v8a.so`、
  Windows x86_64で`libkritacanvas.dll`と`libkritaui.dll`のリンクが成功した。
- 5構成のCMake台帳を再生成した。macOS 642件、Linux 657件、iOS 576件、Android 582件、
  Windows 612件のターゲット、560件の共通ターゲット、119件の条件付きターゲット、
  257件の構成差を持つターゲットを記録した。20中核所有ターゲットと全製品ターゲットは
  5構成すべてで循環0件を維持する。
- `nix develop --no-eval-cache .#test --command ./scripts/verify-quick`: 97件の単体試験、
  責務・依存・構造台帳、再配置計画、文書、リンク、D2再生成を含む高速検査が成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: 表示色変換境界分離後の
  全Nix出力の評価が成功した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos`:
  同一コミットのmacOS、iOS、Linux、Android、Windows台帳と差分行列が成功した。
- macOSで`libkritacanvas.dylib`、iOSで`libkritacanvas.a`と
  `LibrePaint.app/LibrePaint`、x86_64 Linuxで`libkritacanvas.so`と
  `libkritaui.so`、Android arm64-v8aで`libkritacanvas_arm64-v8a.so`と
  `libkritaui_arm64-v8a.so`、Windows x86_64で`libkritacanvas.dll`と
  `libkritaui.dll`の構築とリンクが成功した。
- Androidの初回構築は、`libs/canvas/animation/kis_frame_data_serializer.cpp`が
  `QDataStream`を間接includeに依存していたことをQt 5構成で診断した。実装が利用する
  Qt型を直接includeした後、macOS、iOS、Linux、Android、Windowsの全構成で再構築が
  成功した。
- `ctest --preset tdd-macos`: 327件中292件が成功した。今回のフレーム範囲、保存、
  直列化、UI保存変換、既存キャッシュ統合、公開表示ヘッダーの6契約はすべて成功した。
  残る35件は直前から継続する画像基準、Qt 6モデル契約、macOS環境、
  セグメンテーション違反の既知失敗である。
- x86_64 Linuxで`QT_QPA_PLATFORM=offscreen`を設定し、今回の6契約がすべて成功した。
- 5構成のCMake台帳を再生成した。macOS 645件、Linux 660件、iOS 579件、Android
  585件、Windows 615件のターゲット、563件の共通ターゲット、119件の条件付き
  ターゲット、257件の構成差を持つターゲットを記録した。20中核所有ターゲットと
  全製品ターゲットは5構成すべてで循環0件を維持する。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos
  --remote-repository /home/masato/librepaint-r1-g6b-verify`: 清浄な同一コミットから
  macOS、iOS、Linux、Android、Windowsの5台帳と差分行列の一致を確認した。
- `nix develop --no-eval-cache .#test --command ./scripts/verify-quick`: 97件の単体試験、
  責務・依存・構造台帳、再配置計画、文書、リンク、D2再生成を含む高速検査が成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: アニメーションキャッシュ
  境界分離後の全Nix出力の評価が成功した。
- 同一コミット`5eff49b46b5293af6c9c0d38d5546a775159a7b7`で
  `nix develop .#test --command ./scripts/verify`を実行し、macOS 327件と
  x86_64 Linux 329件の全ネイティブ試験が成功した。
- `libs-widgetutils-TestKoProgressUpdater`をmacOSとx86_64 Linuxで各50回連続実行し、
  進捗通知の値、表示、通常副処理と永続副処理の寿命契約がすべて成功した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`で、macOS、iOS、Linux、Android、Windowsの
  5台帳と差分行列の一致を確認した。
- `nix flake check --no-build --all-systems`: 全Nix出力の評価が成功した。
- `nix develop .#test --command ./scripts/verify-quick`: R1-G6e文書取り消し境界の計画、
  現行25クラスとの整合、再配置計画、責務・依存・構造台帳、文書、リンク、D2再生成を含む
  97件の単体試験と高速検査が成功した。
- `nix develop .#test --command ./scripts/run-test kis_document_undo_store_test`:
  履歴操作、マクロ、やり直し破棄、同期通知、同一スレッド、非所有の借用寿命を検査する
  文書取り消し契約がmacOSで成功した。
- macOSとx86_64 Linuxで`kritaui`、iOSでLibrePaint本体、AndroidとWindowsで`kritaui`を
  構築した。新しい`kritadocument`と直接利用元が5対象すべてでコンパイルおよびリンクに成功した。
- 5構成のCMake台帳と差分行列を再生成した。macOS 647件、Linux 662件、iOS 581件、
  Android 587件、Windows 617件を記録し、`kritadocument`は全構成に存在する。
- `nix develop .#test --command ./scripts/verify-quick`: 文書所有の公開面、21中核所有ターゲット、
  57リンクの依存射影、循環0件、再配置計画、文書を含む97件の単体試験と高速検査が成功した。
- 同一コミット`7247a9834a32ca3fef6a164bf37b367be61f0ad0`で
  `nix develop .#test --command ./scripts/verify`を実行し、macOS 328件と
  x86_64 Linux 330件の全ネイティブ試験が成功した。
- `nix flake check --no-build --all-systems`: 文書取り消し境界分離後の全Nix出力の評価が
  成功した。
- `kis_document_identity_test`の初回構築は、新しい文書識別ヘッダーが存在しない診断で
  失敗した。実装後は表示用パスと実ファイルパス、実変更判定、MIME形式、自動判定由来、
  複製の契約がmacOSとx86_64 Linuxで成功した。
- `KisDocumentReplaceTest`は既存公開APIから文書識別へ接続し、同一パスの通知抑制、
  パス初期化通知時の実ファイルパス保持、初期化後の消去、保存用スナップショットへの複製を
  macOSとx86_64 Linuxで固定した。
- macOSの初回全試験では、既存`KisSafeDocumentLoaderTest`の1.5秒待機を使うファイル監視
  通知が時間切れになった。再リンク後の反復では成功と時間切れの両方を再現し、文書を開く
  項目は成功した。最終の全試験では同試験を含む329件がすべて成功した。
- x86_64 Linuxでは331件の全ネイティブ試験が成功した。通知順序契約を追加した最終実装
  コミットでも`KisDocumentReplaceTest`と`kis_document_identity_test`が成功した。
- iOSで`libkritadocument.a`、`libkritaui.a`、`LibrePaint.app/LibrePaint`、Android arm64-v8aで
  `libkritadocument_arm64-v8a.so`と`libkritaui_arm64-v8a.so`、Windows x86_64で
  `libkritadocument.dll`と`libkritaui.dll`の構築とリンクが成功した。
- 5構成のCMake台帳と差分行列を再生成した。macOS 648件、Linux 663件、iOS 582件、
  Android 588件、Windows 618件のターゲット、566件の共通ターゲット、119件の条件付き
  ターゲット、258件の構成差を持つターゲットを記録した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`: 清浄な同一コミットから5構成の台帳と差分行列の一致を
  確認した。21中核所有ターゲットと全製品ターゲットは全構成で循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`: 文書識別の公開面、責務・依存・構造台帳、
  再配置計画、完了文書を含む97件の単体試験と高速検査が成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: 文書識別境界分離後の全Nix出力の
  評価が成功した。
- `kis_document_modification_state_test`の初回構築は、新しい文書変更状態ヘッダーが存在しない
  診断で失敗した。実装後は初期状態、自動保存チェックポイント、保存中変更、取り消し履歴に
  現れない変更、保存用複製の契約がmacOSとx86_64 Linuxで成功した。
- `KisDocumentReplaceTest`は既存公開APIから文書変更状態へ接続し、同じ変更済み値の通知抑制、
  保存用スナップショット、未変更への遷移をmacOSとx86_64 Linuxで固定した。
- 同一コミット`e733cad55cac7792437b1dfbd9192c99451e9621`でx86_64 Linuxの全332試験が
  成功した。macOSは全330件中、今回の変更契約を含む329件が成功し、既存の
  `KisSafeDocumentLoaderTest`だけが1.5秒待機のファイル監視通知で時間切れになった。
  同試験の単独再実行は成功し、変更対象外の環境依存試験として区別する。
- iOSで`libkritadocument.a`、`libkritaui.a`、`LibrePaint.app/LibrePaint`、
  Android arm64-v8aで`libkritadocument_arm64-v8a.so`と`libkritaui_arm64-v8a.so`、
  Windows x86_64で`libkritadocument.dll`と`libkritaui.dll`の構築とリンクが成功した。
- 5構成のCMake台帳と差分行列を再生成した。macOS 649件、Linux 664件、iOS 583件、
  Android 589件、Windows 619件のターゲット、567件の共通ターゲット、119件の条件付き
  ターゲット、258件の構成差を持つターゲットを記録した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`: 清浄な同一コミットから5構成の台帳と差分行列の一致を
  確認した。21中核所有ターゲットと全製品ターゲットは全構成で循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`: 文書変更状態の公開面、責務・依存・構造
  台帳、再配置計画、完了文書を含む97件の単体試験と高速検査が成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: 文書変更状態境界分離後の
  全Nix出力の評価が成功した。
- `kis_document_autosave_state_test`の初回構築は、新しい文書自動保存状態ヘッダーが
  存在しない診断で失敗した。実装後は書出し状態の寿命、3回の連続失敗後に次の試行で
  複製経路へ切り替える境界値、失敗履歴の消去がmacOSで成功した。
- `KisDocumentReplaceTest`は既存の文書接続を含めてmacOSで成功した。
- 同一コミット`7b26fa7960115c81bb6a96da82f90b02c692454d`で
  `nix develop .#test --command ./scripts/verify`を実行し、macOS 331件と
  x86_64 Linux 333件の全ネイティブ試験が成功した。
- iOSで`libkritadocument.a`、`libkritaui.a`、`LibrePaint.app/LibrePaint`、
  Android arm64-v8aで`libkritadocument_arm64-v8a.so`と`libkritaui_arm64-v8a.so`、
  Windows x86_64で`libkritadocument.dll`と`libkritaui.dll`の構築とリンクが成功した。
- 5構成のCMake台帳と差分行列を再生成した。macOS 650件、Linux 665件、iOS 584件、
  Android 590件、Windows 620件のターゲット、568件の共通ターゲット、119件の条件付き
  ターゲット、258件の構成差を持つターゲットを記録した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`: 清浄な同一コミットから5構成の台帳と差分行列の
  一致を確認した。21中核所有ターゲットと全製品ターゲットは全構成で循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`: 文書自動保存実行状態の公開面、
  責務・依存・構造台帳、再配置計画、完了文書を含む97件の単体試験と高速検査が成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: 文書自動保存実行状態境界分離後の
  全Nix出力の評価が成功した。
- `kis_document_recovery_autosave_state_test`の初回構築は、新しい文書回復自動保存状態
  ヘッダーが存在しない診断で失敗した。実装後は要求の開始と取消し、既存保存への合流、
  保存開始中に届いた同期完了の延期、開始失敗時の延期破棄、要求ごとの一度限りの完了が
  macOSとx86_64 Linuxで成功した。
- `KisDocumentReplaceTest`は既存の文書接続を含めてmacOSで成功した。
- 同一コミット`ff7211a66e311505f6d1f23fc65136feb2ab88b0`で
  `nix develop .#test --command ./scripts/verify`を実行し、macOS 332件と
  x86_64 Linux 334件の全ネイティブ試験が成功した。
- iOSで`libkritadocument.a`、`libkritaui.a`、`LibrePaint.app/LibrePaint`、
  Android arm64-v8aで`libkritadocument_arm64-v8a.so`と`libkritaui_arm64-v8a.so`、
  Windows x86_64で`libkritadocument.dll`と`libkritaui.dll`の構築とリンクが成功した。
- 5構成のCMake台帳と差分行列を再生成した。macOS 651件、Linux 666件、iOS 585件、
  Android 591件、Windows 621件のターゲット、569件の共通ターゲット、119件の条件付き
  ターゲット、258件の構成差を持つターゲットを記録した。21中核所有ターゲットと
  全製品ターゲットは全構成で循環0件を維持する。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`: 清浄な同一コミットから5構成の台帳と差分行列の
  一致を確認した。
- `nix develop .#test --command ./scripts/verify-quick`: 文書回復自動保存調停状態の公開面、
  責務・依存・構造台帳、再配置計画、完了文書を含む97件の単体試験と高速検査が成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: 文書回復自動保存調停状態境界
  分離後の全Nix出力の評価が成功した。
- `kis_document_recovery_status_test`の初回構築は、新しい文書回復状態ヘッダーが存在しない
  診断で失敗した。実装後は通常文書の初期状態、実際の遷移だけを通知対象とする変更判定、
  値コピーがmacOSとx86_64 Linuxで成功した。
- `KisDocumentReplaceTest`は同値再設定時の通知抑制、回復状態の解除、保存用スナップショットが
  通常文書状態から始まる既存挙動をmacOSとx86_64 Linuxで固定した。
- 同一コミット`fe791e1aa878d2688b2941dc989ee36795a99dfb`で
  `nix develop .#test --command ./scripts/verify`を実行した。x86_64 Linuxは335件の
  全ネイティブ試験が成功した。macOSは333件中、変更契約を含む332件が成功し、既存の
  `KisSafeDocumentLoaderTest`だけが1.5秒待機のファイル監視通知数で失敗した。
  同試験の単独再実行でも通知数が試行ごとに変動し、変更対象外の環境依存試験として区別する。
- iOSで`libkritadocument.a`、`libkritaui.a`、`LibrePaint.app/LibrePaint`、
  Android arm64-v8aで`libkritadocument_arm64-v8a.so`と`libkritaui_arm64-v8a.so`、
  Windows x86_64で`libkritadocument.dll`と`libkritaui.dll`の構築とリンクが成功した。
- 5構成のCMake台帳と差分行列を再生成した。macOS 652件、Linux 667件、iOS 586件、
  Android 592件、Windows 622件のターゲット、570件の共通ターゲット、119件の条件付き
  ターゲット、258件の構成差を持つターゲットを記録した。21中核所有ターゲットと
  全製品ターゲットは全構成で循環0件を維持する。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`: 清浄な同一コミットから5構成の台帳と差分行列の
  一致を確認した。
- `nix develop .#test --command ./scripts/verify-quick`: 文書回復状態の公開面、責務・依存・
  構造台帳、再配置計画、完了文書を含む97件の単体試験と高速検査が成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: 文書回復状態境界分離後の
  全Nix出力の評価が成功した。
- `nix develop .#test --command ./scripts/verify-quick`: 文書パッケージ境界計画、R1 TODO、
  再配置計画、進捗スナップショットの整合を含む97件の単体試験と高速検査が成功した。
- R1-G6e-P0は製品ソース、CMake、Nix出力を変更しない計画単位である。5構成の製品構築と
  macOS、Linuxの全ネイティブ試験は、直前の文書回復状態境界で記録した結果を維持する。
- `kis_document_undo_ui_test`の初回構築は、新しい文書UIターゲットが存在しないため
  Qt Widgetsの`QAction`を解決できない診断で失敗した。公開面契約も旧文書所有の
  6ヘッダーを検出して失敗した。
- 実装後の`kis_document_undo_store_test`と`kis_document_undo_ui_test`はmacOSで成功した。
  履歴操作、通知、借用寿命に加え、操作名、取消し・やり直しの有効状態、履歴行、
  履歴選択による移動を固定した。
- macOSで`kritadocument`、`kritadocumentui`、`kritaui`の構築とリンクが成功した。
- 5構成のCMake台帳と差分行列を再生成した。macOS 653件、Linux 668件、iOS 587件、
  Android 593件、Windows 623件のターゲット、571件の共通ターゲット、119件の条件付き
  ターゲット、258件の構成差を持つターゲットを記録した。21中核所有ターゲットと
  全製品ターゲットは全構成で循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`: `kritadocument`の依存0件、
  `kritadocumentui`から`kritapaintingundo`への依存、`kritaui`から両文書ターゲットへの依存、
  公開面、責務、再配置計画、循環0件を含む97件の単体試験と高速検査が成功した。
- 同一コミット`600aaf17eb00f34eb42ce3413a50c8b433e3c678`で
  `nix develop .#test --command ./scripts/verify`を実行し、macOS 334件と
  x86_64 Linux 336件の全ネイティブ試験が成功した。
- iOSで`libkritadocument.a`、`libkritadocumentui.a`、`libkritaui.a`、
  `LibrePaint.app/LibrePaint`、Android arm64-v8aで`libkritadocument_arm64-v8a.so`、
  `libkritadocumentui_arm64-v8a.so`、`libkritaui_arm64-v8a.so`、Windows x86_64で
  `libkritadocument.dll`、`libkritadocumentui.dll`、`libkritaui.dll`の構築とリンクが成功した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`: 清浄な同一コミットから5構成の台帳と差分行列の
  一致を確認した。21中核所有ターゲットと全製品ターゲットは全構成で循環0件を維持する。
- `nix flake check --no-build --all-systems --no-eval-cache`: 取り消し履歴の文書UI境界分離後の
  全Nix出力の評価が成功した。

## 次の操作

R1-G6e-P1取り消し履歴の文書UI境界をレビューして統合する。統合後はmasterを同期し、
R1-G6e-P2として`libs/ui/KisDocument.cpp`の保存・読込ダイアログ、状態表示、Qt通知の接続と
`libs/ui/KoDocumentInfoDlg.{h,cpp}`を起点に、文書表示の既存挙動と実依存を調査する。
最小の特性契約で初期診断を確認してから`libs/document/ui`へ集約する。

## R1-G5完了根拠

- 9責務すべてが現行所有者、目標ディレクトリー、名前空間、主ターゲット、許可依存、
  完了条件を持つ。
- 8移行段階が許可依存の下位から上位へ並び、新規ターゲットを一度だけ作成する。
- 11の一時互換経路が導入段階、最大範囲、R1-G7の削除条件、検証方法を持つ。
- 8種類305件の逆方向includeと44ヘッダー627件の内部参照が各段階で一度だけ処理され、
  最終上限がゼロになる。
- 最初のR1-G6aが移動元、移動先、必要な契約、基準縮小、完了条件、中止条件を持つ。
- 計画検査が責務・依存・構造基準と5構成の実体を照合し、DarwinとLinuxの独立検査で
  同じ結果になる。

## R1-G4完了根拠

- 確認済み8責務対が全305件の直接includeと元のCMake辺へ対応付く。
- 各違反が理由、所有段階、審査済み上限、除去条件を持つ。
- 共有ターゲットによる6射影が実責務へ帰属し、未確定射影が0件になる。
- 中核15ターゲットと全製品構築ターゲットが5構成すべてで循環0件を維持する。
- 公開宣言を持たない44ヘッダー、627件のパッケージ外参照が審査済み上限を持つ。
- 違反増加、根拠変更、上限縮小を高速検査で診断できる。
- 5構成の実体、許可依存方針、逆方向依存、射影解決、循環、内部ヘッダー基準の一致を
  同時検証できる。
