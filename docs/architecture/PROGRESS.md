# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-08-27 21:20 JST
- 状態: `in_progress`
- 現在の検査段階: R2-G19b 全public API挙動契約の充足
- 関連TODO: `docs/architecture/TODO.md`の「R2: 現行挙動のテスト固定」
- ブランチ: `develop`
- 目的: 全public APIを具体的な挙動試験へ対応付け、大規模リファクタリングの判定基盤を完成する。

## 再開環境

- direnvが`test`開発シェルと`build-incremental`、`run-test`、`verify-quick`、
  `verify`を読み込む。
- DarwinホストがmacOSとiOS、`ssh nixos`で接続するx86_64 NixOSホストがLinux、
  Android、Windowsの構成を担当する。
- `docs/architecture/package-boundaries.json`が10責務、27中核所有ターゲット、許可する
  責務間依存だけを保持する。
- 各プラットフォームの`build-incremental <platform> configure`は、CMake File APIの
  問い合わせを作成し、構成直後の実グラフで所有ターゲット、依存方向、製品循環を検査する。
- `verify-quick`は境界方針と、現在の公開ヘッダー、プラグイン登録、テキスト、スクリプト、
  文書を検査する。

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

- 直接includeで一意に帰属できる8種類、305件の確認済み逆方向依存を採取した。
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
  構造依存検査へ接続した。
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

- R1-G5再配置計画台帳に9責務の現行所有者と、目標
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
- 実装をP1独立した文書UIの一括移設、P2文書表示の構造分離、P3文書ファイル保存の構造分離、
  P4残る境界評価の4検査段階へ分けた。ロジック再構築とI/O隔離はP4で現在の根拠を
  確認してから独立段階として計画する。
- 最初の実装単位は`libs/document/undo/kis_document_undo_store.{h,cpp}`と
  `libs/command/{kundo2model,kundo2view}.{h,cpp}`を起点とし、文書と取り消し履歴の接続および
  履歴表示を`libs/document/ui/undo`へ移す。`kritadocument`の公開リンク閉包をQt Coreだけへ
  縮小した状態をP1の完了条件とする。

## R1-G6e-P1独立した文書UIの一括移設で完了した作業

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
- `libs/ui/KisAutoSaveRecoveryDialog.{h,cpp}`を起点として
  `libs/document/ui/recovery/KisAutoSaveRecoveryDialog.{h,cpp}`へ移し、回復候補の一覧、選択、
  一括破棄、プラットフォーム別回復場所を`kritadocumentui`へ集約した。
- 独立ファイルの移設は同じPRへまとめた。`KoDocumentInfo`、`KoDocumentInfoDlg`、
  `KisDocument.cpp`内の保存処理は上位状態へ直接依存し、別ライブラリー化にAPIと責務の
  再構築が必要なため、機械的移設ではなくP2とP3の構造変更として扱う。

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
- `kis_document_autosave_recovery_dialog_test`の初回構築は、新しい文書UI所有の回復ダイアログ
  ヘッダーが存在しない診断で失敗した。実装後は回復候補の初期選択と一括破棄がmacOSで
  成功し、`kritaui`も新しい所有先を直接利用してリンクした。
- 5構成のCMake台帳と差分行列を再生成した。macOS 654件、Linux 669件、iOS 588件、
  Android 594件、Windows 624件のターゲット、572件の共通ターゲット、119件の条件付き
  ターゲット、258件の構成差を持つターゲットを記録した。
- 同一コミット`e0f24b17fc54ba482e7ed7b46bd614a87700b702`で
  `nix develop .#test --command ./scripts/verify`を実行し、macOS 335件と
  x86_64 Linux 337件の全ネイティブ試験が成功した。
- `./scripts/build-incremental ios build --allow-large`でiOSの`libkritadocumentui.a`、
  `libkritaui.a`、`LibrePaint.app/LibrePaint`まで構築成功した。
  Android arm64-v8aとWindows x86_64では`./scripts/build-incremental <platform> build
  kritaui`を実行し、それぞれ`libkritadocumentui_arm64-v8a.so`と
  `libkritaui_arm64-v8a.so`、`libkritadocumentui.dll`と`libkritaui.dll`の構築に成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`: 自動保存回復UIの移設後も
  全Nix出力の評価が成功した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`: DarwinとNixOSの清浄な同一コミットから5構成の
  台帳と差分行列の一致を確認し、21中核所有ターゲットと全製品ターゲットは循環0件を
  維持した。

## R1-G6e-P2で完了した作業

- `libs/ui/KisDocument.cpp`の保存・読込ダイアログ、状態表示、Qt通知の接続を起点として、
  `libs/document/ui/io/kis_document_io_presentation.{h,cpp}`へ移した。保存成功時の
  `completed`、`sigSavingFinished`、状態表示の順序、取消し時の非表示、バッチ失敗時の
  状態表示、自動保存の状態表示を専用契約へ固定した。
- `libs/ui/KoDocumentInfoDlg.{h,cpp}`と`libs/ui/forms/koDocumentInfo{About,Author}Widget.ui`を
  `libs/document/ui/info`へ移した。ダイアログは`KisDocument`の親型から値を推測せず、
  表示するパスとMIME形式を値として受け取る。
- `libs/ui/dialogs/KisRecoverNamedAutosaveDialog.{h,cpp,ui}`を
  `libs/document/ui/recovery`へ移した。ファイルからのプレビュー生成は既存の
  `KisFileIconCreator`利用元に維持し、ダイアログは生成済み`QIcon`値の表示だけを所有する。
- `libs/ui/KoDocumentInfo.{h,cpp}`を`libs/impex/metadata`へ移した。文書情報は形式処理が
  直接直列化する実依存に従って`kritaimpex`が所有し、`kritaimpexui`から文書寿命への
  新しい逆方向依存を回避した。自動保存中と変更済み状態は呼出元が明示する。
- 旧配置、転送ヘッダー、別名は残していない。既存の入出力エラー型、文書状態、`QIcon`値を
  直接使用し、利用事例層、汎用永続化層、接続面、アダプター、共通基底クラスは追加していない。
- 文書情報試験の初回構築は新しい所有先のヘッダーが存在しない診断で失敗した。
  名前付き自動保存回復試験も同じく新しい回復ヘッダーが存在しない診断で失敗した。
  実装後は文書情報、文書情報編集、入出力表示、名前付き自動保存回復の4試験がmacOSで成功し、
  `kritaimpex`、`kritadocumentui`、`kritaui`の構築とリンクが成功した。
- 5構成のCMake台帳と差分行列を再生成した。macOS 658件、Linux 673件、iOS 592件、
  Android 598件、Windows 628件のターゲット、576件の共通ターゲット、119件の条件付き
  ターゲット、259件の構成差を持つターゲットを記録した。21中核所有ターゲットと
  全製品ターゲットは全構成で循環0件を維持する。
- 確認済み逆方向includeは3種類96件、`kritaui`の内部ヘッダー参照は7ヘッダー20件を維持し、
  新しい逆方向依存と内部ヘッダー参照を追加していない。
- 同一コミット`8145698206b2185e2b4502985d8dab32e8e56e47`の清浄な作業ツリーで
  `nix develop .#test --command ./scripts/verify`を実行し、macOS 339件と
  x86_64 Linux 341件の全ネイティブ試験が成功した。追加した4試験も両構成で成功した。
- `nix develop .#test --command ./scripts/build-incremental ios build --allow-large`で
  `LibrePaint.app/LibrePaint`まで構築した。Android arm64-v8aとWindows x86_64では
  `./scripts/build-incremental <platform> build kritaui`を実行し、
  `libkritaui_arm64-v8a.so`と`libkritaui.dll`のリンクに成功した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`は、清浄な同一コミットから5構成の台帳と差分行列を
  検証し、21中核所有ターゲットと全製品ターゲットの循環0件を確認した。
- `nix develop .#test --command ./scripts/verify-quick`はmacOSとLinuxで97件と全統治検査に
  成功した。`nix flake check --no-build --all-systems --no-eval-cache`も全Nix出力の評価に
  成功した。

## R1-G6e-P3で完了した作業

- `libs/ui/KisDocument.cpp`の保存先検査、バックアップ、自動保存名、回復ファイル消去を起点に、
  `libs/document/files/kis_document_save_target.{h,cpp}`、
  `kis_document_backup_file.{h,cpp}`、`kis_document_autosave_files.{h,cpp}`へ集約した。
  `libs/document/files`を具体的な文書ファイル処理の所有先とする`kritadocumentfiles`を構築した。
- `libs/document/ui/recovery/KisAutoSaveRecoveryDialog.{h,cpp}`の回復ファイル読込を起点に、
  回復候補の探索、更新時刻、プレビュー読込を`kis_document_autosave_files`へ移した。
  ダイアログは生成済みの回復候補値を表示し、選択結果を返す。
- `libs/ui/KisApplication.cpp`と`libs/ui/KisView.cpp`は、起動時の回復調整と破棄操作を維持し、
  自動保存ファイルの探索、パス組立て、使用可否判定、消去を`kritadocumentfiles`へ委ねる。
- 旧配置の公開メソッド、転送ヘッダー、別名は残していない。既存の形式選択、形式変換、
  直列化、非同期保存は既存`kritaimpex`と`KisDocument`に維持し、新しい利用事例層、
  汎用永続化層、接続面、アダプター、サービス、リポジトリーは追加していない。
- 文書ファイル契約の初回構築は、`files/kis_document_autosave_files.h`が存在しない診断で
  失敗した。実装後は保存先の存在と書込可否、単純および世代付きバックアップ、自動保存名、
  回復候補の探索、使用可否、消去を検証する`kis_document_files_test`が成功した。
  自動保存回復ダイアログ試験と`kritaui`の構築・リンクもmacOSで成功した。
- 5構成のCMake台帳と差分行列を再生成した。macOS 660件、Linux 675件、iOS 594件、
  Android 600件、Windows 630件のターゲット、578件の共通ターゲット、119件の条件付き
  ターゲット、260件の構成差を持つターゲットを記録した。22中核所有ターゲットと
  全製品ターゲットは全構成で循環0件を維持する。
- 確認済み逆方向includeは3種類96件、`kritaui`の内部ヘッダー参照は7ヘッダー20件を維持し、
  新しい逆方向依存と内部ヘッダー参照を追加していない。
- 同一コミット`19be6382d82d6124eced7172f7bf4a887323180e`の清浄な作業ツリーで
  `nix develop .#test --command ./scripts/verify`を実行し、macOS 340件と
  x86_64 Linux 342件の全ネイティブ試験が成功した。追加した文書ファイル契約も
  両構成で成功した。
- `nix develop .#test --command ./scripts/build-incremental ios build --allow-large`で
  `libkritadocumentfiles.a`、`libkritadocumentui.a`、`libkritaui.a`と
  `LibrePaint.app/LibrePaint`を構築した。Android arm64-v8aとWindows x86_64では
  `./scripts/build-incremental <platform> build kritaui`を実行し、
  `libkritadocumentfiles`、`libkritadocumentui`、`libkritaui`のリンクに成功した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos --remote-repository
  /home/masato/librepaint-r1-g6b-verify`は、清浄な同一コミットから5構成の台帳と差分行列を
  検証し、22中核所有ターゲットと全製品ターゲットの循環0件を確認した。
- `nix develop .#test --command ./scripts/verify-quick`はmacOSとLinuxで97件と全統治検査に
  成功した。`nix flake check --no-build --all-systems --no-eval-cache`も全Nix出力の評価に
  成功した。

## R1-G6e-P4で完了した作業

- `docs/architecture/ui-class-responsibilities.json`の`document-state`分類22クラスを起点に、
  現在の関心、具体的な所有先、後続検査段階を評価した。P1とP2で所有を移した3クラスと
  合わせ、R1-G6e開始時の25クラスすべてを被覆した。
- 残る22クラスを`KisDocument`1件、外部ファイル層1件、操作管理5件、ノード・選択操作接続
  4件、Qtモデルと表示状態11件へ再分類した。ノード表示モデルから`KisNodeManager`への依存と、
  `KisFileLayer`から`KisPart`への依存を、ファイル移動を先行できない根拠として記録した。
- `libs/ui/KisDocument.cpp`の130の一意なメソッド定義を、アプリケーション構成、文書付随状態、
  ファイル読込、ファイル保存と回復、メタデータと資源、文書セッション、画像と取り消し、
  表示と通知の8関心へ一度ずつ割り当てた。
- 保存I/O差し替え、保存計算、利用事例登録、外部ファイル層の抽象接続面について、現在の
  複数実装、差し替え要求、重複、決定的試験の阻害がないことを確認し、抽象を追加しない
  判断を記録した。
- 評価結果を文書パッケージ境界計画へ反映し、後続段階の具体的な所有判断へ接続した。
- `nix develop .#test --command ./scripts/verify-quick`は、新規6件を含む103件の単体試験、
  文書境界評価、既存の公開面、責務、依存、構造、再配置計画、文書、リンク、D2再生成の
  全検査に成功した。製品ソースとCMake境界を変更していないため、構成別構築は実行しない。
- 次の検査段階をR1-G6fとし、`libs/ui/tool`、`KisFilterManager`、`KisImageManager`、
  `KisNodeCommandsAdapter`、`KisNodeManager`、`KisSelectionManager`を調査起点に記録した。
  具体的な命令と、アクション、ダイアログ、キャンバス、ノード、選択の表示配線を分ける。

## R1-G6f画像ノード命令境界で完了した作業

- `libs/ui/kis_node_commands_adapter.{h,cpp}`を起点として、
  `libs/image/commands/kis_node_commands_adapter.{h,cpp}`へ移した。ノード追加、移動、削除、
  不透明度、合成方法、名前変更と、それらの取り消し履歴への登録を既存の画像命令へ集約した。
- `KisViewManager`への参照を除去し、命令側は操作対象画像を弱参照する。長寿命の
  `KisLayerManager`、`KisMaskManager`、`KisNodeManager`、`KisSelectionManager`は、
  ビュー切替時に画像を明示的に結び直す。
- `KisApplication`と`KisView`は画像命令を直接参照せず、既存の`KisNodeManager`へ
  単一ノード追加を委ねる。文書、入出力、ツールの既存利用元は画像命令を直接利用し、
  新しい逆方向依存を作らない。確認済み逆方向includeは75件、17件、4件を維持する。
- 専用契約の初回構築は、画像を直接受ける構築子と画像の再設定操作が存在しない診断で
  失敗した。実装後はノード追加が取り消し可能であることと、操作対象画像を再設定できることを
  `KisNodeCommandsAdapterTest`で確認した。
- 新規の汎用接続面、利用事例層、サービス、リポジトリーは追加していない。ツール専用ではない
  実利用が判明したため、`kritatools`の作成を先行せず既存の画像命令所有へ配置した。
- 5構成のCMake台帳はmacOS 661件、Linux 676件、iOS 595件、Android 601件、
  Windows 631件を記録する。共通579件、条件付き119件、構成差260件であり、
  22中核所有ターゲットと全製品ターゲットは全構成で循環0件を維持する。
- 実装コミット`74f0d0c360bfcadc4cbf207b320297eded34cb59`を両ホストの清浄な作業ツリーへ
  揃え、macOSの全341試験とx86_64 Linuxの全343試験が成功した。iOSはLibrePaint本体、
  Android arm64-v8aとWindows x86_64はUIライブラリーまで構築に成功した。
- 同じ実装コミットで5構成台帳の完全一致検査、103件の方針・台帳試験を含む
  `verify-quick`、`nix flake check --no-build --all-systems --no-eval-cache`が成功した。

## R1-G6f画像ノード操作バッチ境界で完了した作業

- `libs/ui/kis_node_juggler_compressed.{h,cpp}`を起点として、
  `libs/image/commands/kis_node_operation_batch.{h,cpp}`へ移した。連続するノードの追加、移動、
  複製、削除、投影更新と一つの非同期取り消し履歴項目への集約を`kritaimage`が所有する。
- 画像処理が借用していた`KisNodeManager`を除去した。選択復元に必要なアクティブノードは、
  `KisNodeManager`が各操作の呼出し時に値として渡す。操作バッチの寿命構成と利用者操作の
  配線はUI所有に残る。
- `libs/ui/tests/kis_node_juggler_compressed_test.{h,cpp}`を起点として、
  `libs/image/tests/kis_node_operation_batch_test.{h,cpp}`へ移した。新しい公開ヘッダーがない
  初回構築の失敗を確認し、既存の移動、複製、コピー、取り消しに加え、渡したアクティブ
  ノードが取り消し時に復元される契約を固定した。
- `KisNodeOperationBatchTest`の全試験と`kritaui`の増分構築が成功した。旧ファイル、転送
  ヘッダー、旧名の別名、新しい汎用接続面は追加していない。
- 公開面台帳は`kritaimage`334ヘッダー、`kritaui`244ヘッダーを記録する。UI直下の
  公開クラスは80件、文書状態分類は20件となり、R1-G6e開始時の25クラスのうち5クラスが
  具体的な所有先へ移った。
- 実装と台帳を含むコミット`8a92e2bf63b6bd5e3cfaabf82d5a5fb82499c4fd`をDarwinと
  x86_64 Linuxの清浄な作業ツリーへ揃えた。macOSの全341試験とx86_64 Linuxの
  全343試験が成功し、iOSはLibrePaint本体、Android arm64-v8aとWindows x86_64は
  UIライブラリーまで構築に成功した。
- 同じコミットで5構成台帳の完全一致検査と循環0件を確認した。103件の方針・台帳試験を
  含む`verify-quick`と`nix flake check --no-build --all-systems --no-eval-cache`が成功した。
- AndroidとWindowsは構築契約までを確認し、実行時の利用者操作はmacOSとLinuxの
  `KisNodeOperationBatchTest`が固定する。移動対象外のUI管理器と各プラットフォームの
  既存警告は、この境界の残存リスクとして追跡する。

## R1-G6f画像ノード変更実行境界で完了した作業

- `libs/ui/kis_node_manager.cpp`の`moveNodeAt()`にあった移動可能性の判定と、移動先レイヤーで
  既存の選択マスクを非アクティブ化する不変条件を、既存の
  `libs/image/commands/kis_node_commands_adapter.cpp`へ移した。二つの移動入口が同じ条件を
  適用し、UI以外の命令利用元でも画像ノードの規則が成立する。
- 同じ起点の`mirrorNodes()`にあった処理適用器、再帰実行、全フレーム処理、並行ジョブ、
  取り消し履歴項目の構成を、既存の
  `libs/image/processing/kis_mirror_processing_visitor.{h,cpp}`へ移した。UI管理器には編集可否の
  警告、画像処理の呼出し、`nodesUpdated()`による表示更新通知を残した。
- `libs/image/tests/kis_node_commands_adapter_test.cpp`は、アクティブな選択マスクを移すと移動先の
  既存マスクが非アクティブになる契約を固定する。初回実行は既存マスクがアクティブなままの
  診断で失敗し、実装後に成功した。
- `libs/image/tests/kis_processings_test.{h,cpp}`は、UI管理器を使わずに画像、対象ノード、方向、
  選択、操作名だけでミラー処理を実行し、取り消せる契約を固定する。初回構築は画像処理側に
  実行入口が存在しない診断で失敗し、実装後に成功した。
- 新しいファイル、CMakeターゲット、汎用層、利用事例、サービス、リポジトリーは追加して
  いない。現存する二つの具体的な画像所有者へ処理を集約した。
- `KisNodeManager`は`kis_processing_applicator.h`の直接利用元から外れ、公開面台帳を同期した。
  `libs/ui/kis_node_manager.cpp`は1827行から1798行へ縮小し、ソース行数基準を更新した。
- 実装と台帳を含むコミット`a6c74853d96bd17c18eb2144cd719111bd0f2611`をDarwinと
  x86_64 Linuxの清浄な作業ツリーへ揃えた。macOSの全341試験とx86_64 Linuxの全343試験が
  成功し、iOSは`LibrePaint.app`、Android arm64-v8aとWindows x86_64は`kritaui`まで構築に
  成功した。
- 同じコミットで5構成のCMake台帳と差分行列の完全一致を確認した。ターゲット数はmacOS
  661件、Linux 676件、iOS 595件、Android 601件、Windows 631件を維持し、22中核所有
  ターゲットと全製品ターゲットは全構成で循環0件を維持する。
- 103件の方針・台帳試験を含む`verify-quick`、画像側の二契約、`kritaui`の増分構築、
  `nix flake check --no-build --all-systems --no-eval-cache`が成功した。既存の
  `KisNodeManagerTest`は通常構成でbroken試験として登録されており、実行対象外である。
- AndroidとWindowsは構築契約までを確認し、実行時契約はmacOSとLinuxの画像側単体試験で
  固定する。移動対象外のノード管理処理、broken試験のUI統合範囲、各構成の既存警告は
  残存リスクとして追跡する。

## R1-G6fツール命令所有境界で進行中の作業

- `libs/ui/kis_node_manager.cpp`の`createQuickGroupImpl()`と`quickUngroup()`を起点として、
  画像グラフの検証と変更を`libs/image/commands/kis_node_operation_batch.{h,cpp}`へ移した。
  グループ作成、移動先適合性、子ノード移動、空になったグループの除去は新しい具体実装
  `libs/image/commands/kis_node_group_operations.{h,cpp}`が所有する。UI管理器には操作名、編集可否、
  選択更新、互換性エラーの表示を残した。
- `libs/ui/tool/KisSelectionToolFactoryBase.*`、`KisToolPaintFactoryBase.*`、
  `KisToolChangesTracker*`、`KisToolShapeUtils.*`、`kis_delegated_tool_policies.*`、
  `kis_smoothing_options.*`、`kis_tool.{h,cc}`を`libs/tools/`の同名ファイルへ移した。
  `libs/tools/CMakeLists.txt`はこれらを新しい共有ライブラリー`kritatools`として構築し、
  `libs/tools/tests/TestToolCoreContract.cpp`が平滑化設定、ブラシ寸法、アクションID、起動方針を
  固定する。旧配置、転送ヘッダー、旧名の別名は残していない。
- `libs/ui/tool/kis_tool_utils.{h,cpp}`を起点として、画像消去、色採取設定、編集可否の文言、
  カーソル位置、標準ブラシ寸法を`libs/tools/kis_tool_utils.{h,cpp}`へ移した。浮動メッセージと
  複数レイヤー上の図形選択は`libs/ui/tool/kis_tool_canvas_utils.{h,cpp}`へ分離した。
- `libs/ui/kis_config.{h,cc}`の線平滑化設定操作を除去し、既存の設定キーと既定値の所有を
  `libs/tools/kis_smoothing_options.{h,cpp}`へ移した。設定保存は従来と同じ圧縮時機と
  `KSharedConfig`を使用し、UIの汎用設定所有を経由しない。
- ツールが借用する座標変換、画像、ノード選択、処理待機、編集可否、表示通知、輪郭描画、
  設定通知を`libs/canvas/KisToolCanvas.h`へ固定した。`KisCanvas2`の実装は
  `libs/ui/canvas/kis_canvas_tool_support.cpp`へ分割した。`kritatools`が`kritacanvas`へ依存し、
  キャンバスからツールへの逆向き依存と新しい循環は存在しない。
- UIツール責務台帳の33クラスから、`KisTool`、二つのファクトリー、二つの起動方針、
  変更追跡、平滑化設定、共通値型を含む11クラスを`kritatools`へ移した。UI側の分類対象は
  33件から22件、`kritaui`公開ヘッダーは244件から236件となり、`kritatools`の9公開
  ヘッダーと`kritacanvas`の新しい借用契約を公開面台帳へ追加した。
- `libs/ui/canvas/kis_canvas2.cpp`のツール接続実装を
  `libs/ui/canvas/kis_canvas_tool_support.cpp`へ分け、行数上限1726を維持した。
  `libs/image/commands/kis_node_operation_batch.cpp`はグループ処理を具体ファイルへ分けて
  1041行から950行、`libs/ui/kis_node_manager.cpp`は1798行から1739行へ縮小した。
- CMake台帳へ`kritatools`と`TestToolCoreContract`を追加した。現在の記録はmacOS 663件、
  Linux 678件、iOS 597件、Android 603件、Windows 633件、共通581件、条件付き119件、
  構成差262件である。23中核所有ターゲットと全製品ターゲットは5構成で循環0件を維持し、
  `kritatools`の未宣言内部ヘッダー参照は0件である。`kritaui`の内部参照は7ヘッダー20件から
  6ヘッダー19件へ縮小した。
- `KisNodeOperationBatchTest`と`TestToolCoreContract`はmacOSで成功した。`kritatools`、
  `kritaui`、既定描画ツール、選択ツールの増分構築も成功した。`kritaui`の初回再構築では、
  `KisNodeManager`が別用途で必要とする`KisGroupLayer`の完全型include不足を検出し、明示的な
  includeを復旧した後にリンクまで成功した。既存の非推奨API警告は基準内である。
- `nix develop .#test --command ./scripts/verify`は、macOSの全1853構築工程と342件の
  ネイティブ試験に成功した。`verify-quick`の103件、公開面、責務、依存、構造、再配置計画、
  文書、リンク、D2再生成の全検査も成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`は、macOS、iOS、Linux、Android、
  Windowsを含む全出力の式評価に成功した。
- macOSとiOSのCMake台帳は現在の作業ツリーから再生成した。Linux、Android、Windowsの台帳は、
  共通ターゲットの構成差を既存のQt/KF版とライブラリー種別へ反映した。清浄な同一コミットを
  必要とする5構成の完全一致検査、Linuxの全ネイティブ試験、iOS本体、Android、Windowsの
  再構築は、この進行中単位の残存検証である。
- `libs/ui/tool/kis_delegated_tool.h`を`libs/tools/kis_delegated_tool.h`へ移し、委譲ツールが
  `KisCanvas2`と入力管理器を直接参照する経路を除去した。優先入力フィルターの登録と解除は
  `KisToolCanvas`の借用契約を通り、委譲先の入力転送、起動方針、設定部品の集約は維持する。
- `libs/ui/tool/kis_tool_select_base.h`を、選択操作状態、修飾キー、選択境界移動を所有する
  `libs/tools/kis_tool_select_base.h`と、設定部品、アクション接続、ショートカット表示、選択用
  メニューを所有する`libs/ui/tool/kis_tool_select_ui_base.h`へ分けた。8種類の選択ツールは
  新しい表示基底を使用し、旧配置と転送ヘッダーは残していない。
- `plugins/tools/selectiontools/kis_selection_modifier_mapper.{h,cc}`を
  `libs/tools/kis_selection_modifier_mapping.{h,cpp}`へ移し、大域オブジェクトと設定変更通知を
  除去した。現在の交換設定を`KisToolCanvas`から値で渡し、修飾キー組合せとmacOSの
  Shift+Meta補正を`TestToolCoreContract`で固定した。契約追加前はヘッダー不在でコンパイルが
  失敗し、実装後は1件のCTestが成功した。
- 選択取得、修飾キー交換設定、移動カーソル、優先入力フィルターを
  `libs/canvas/KisToolCanvas.h`へ追加し、`libs/ui/canvas/kis_canvas_tool_support.cpp`が既存の
  UI所有者へ接続する。選択メニューは`KisSelectionToolHelper`の表示責務に維持し、選択プラグイン
  から`KisViewManager`を経由する選択取得も除去した。
- 公開面は`kritatools`の9ヘッダーから12ヘッダーへ、`kritaui`の236ヘッダーから235ヘッダーへ
  更新した。UIツール責務台帳は設定表示を所有する`KisToolSelectUiBase`を加えて23クラスとなり、
  `kritaui`の未公開内部参照は6ヘッダー19件から4ヘッダー7件へ縮小した。
- `kritaselectiontools`、`kritadefaulttools_static`、`kritatoolencloseandfill`はmacOSでリンクまで
  成功した。macOSとiOSのCMake台帳は実構成から更新し、Linux、Android、Windowsは同じ三つの
  直接`kritatools`依存を記録した。ターゲット数と循環上限は変わらない。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験、公開面、責務、依存、
  構造、再配置計画、文書、リンク、D2再生成を含めて成功した。
- `nix develop .#test --command ./scripts/verify`はmacOSの245増分構築工程を完了し、342件中
  341件のネイティブ試験が成功した。変更範囲外の`KisSafeDocumentLoaderTest::testFileLost()`は、
  再作成した一時ファイルへの後続書込み後に`loadingFinished`が1500ミリ秒以内に届かず失敗した。
  単独再実行でも同じ行で再現し、同試験、文書読込実装、入出力ターゲットには変更がない。
- `nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、
  Windowsを含む全出力の式評価に成功した。
- `libs/ui/tool/kis_tool_paint.{h,cc}`を起点として、ポインター追跡、ブラシ寸法・回転操作、
  輪郭状態、輪郭生成を`libs/tools/kis_tool_paint_interaction.{h,cpp}`へ移した。UI側の
  `KisToolPaint`はこの操作基盤を継承し、色採取、ポップアップ、設定部品、設定に基づく
  輪郭表示、描画補助線の更新を所有する。`kritatools`から`kritaui`への逆向き依存はない。
- 未使用だったプリセット、透過率、変更状態、版番号のスナップショットと復旧入口を除去した。
  分離で明らかになった旧`kis_tool_paint.h`への暗黙依存は、図形基底と既定描画、動的、
  多角形、多角線、囲み塗り、色塗りマスクの各プラグインで必要なQtまたは画像型を直接includeして
  解消した。
- `libs/ui/tool/kis_tool_paint.cc`は757行から465行、同ヘッダーは169行から87行へ縮小した。
  新しい操作基盤は実装296行、公開ヘッダー92行である。公開面台帳は`kritatools`を12ヘッダー
  から13ヘッダーへ更新し、`kritaui`の235ヘッダーとUIツール責務台帳23クラスを維持した。
- `TestToolCoreContract`へ操作基盤の公開ヘッダー構築契約を追加した。初回はヘッダー不在で
  コンパイルが失敗し、実装後は1件のCTestが成功した。`kritaui`、既定描画ツール、選択ツール、
  囲み塗り、スマート補修、動的、多角形、多角線の各ターゲットはmacOSでリンクまで成功した。
- `nix develop .#test --command cmake --build build/tdd-macos`は全製品と試験ターゲットのリンクまで
  成功した。`nix develop .#test --command ./scripts/verify`は103件の運用検査と342件のネイティブ
  試験をすべて完了した。前の単位で失敗した`KisSafeDocumentLoaderTest::testFileLost()`も成功し、
  今回の単位に残るmacOS検証失敗はない。
- `libs/ui/tool/kis_figure_painting_tool_helper.{h,cpp}`を
  `libs/painting/kis_figure_painting_stroke.{h,cpp}`へ移し、図形描画ストロークの開始、描画ジョブ、
  終了を`kritapainting`へ集約した。新しい`KisFigurePaintingStroke`は複製不能であり、構築から
  破棄まで一つのストロークを所有する。UI側とlibkisは描画実行を所有せず、この具体契約を直接使う。
- `libs/tools/KisToolShapeUtils.h`を`libs/painting/KisFigurePaintingOptions.h`へ移し、描線と塗りの
  安定値を実際の描画所有者へ置いた。空だった`libs/tools/KisToolShapeUtils.cpp`、旧配置、旧名、
  転送ヘッダーは除去した。`TestPaintingBoundary`は列挙値の順序とストローク所有契約を固定する。
- ヘッダー縮小で明らかになった暗黙依存は、`libs/ui/tool/kis_tool_shape.cc`の`KisSelection`、
  `plugins/tools/basictools/kis_tool_line.cc`の`QPainterPath`と`KisResourcesSnapshot`を利用元で
  明示した。`libs/libkis/Notifier.*`は不要な`KisView`と`KisApplication`のヘッダー依存を除去し、
  Qtアプリケーションと必要な画像ノード型を直接参照する。
- 公開面台帳へ`kritapainting`の19ヘッダーを追加した。`kritatools`は13件から12件、`kritaui`は
  235件から234件、UIツール責務台帳は23クラスから22クラスとなった。painting、toolsの未公開
  内部参照は0件であり、UIの4ヘッダー7参照と全製品ターゲットの循環0件を維持する。
- 多角形と多角線プラグインは`kritapainting`を直接リンクする。macOSとiOSのCMake台帳は実構成から
  再生成し、Linux、Android、Windowsにも同じ2辺を同期した。差分行列は共通581件、条件付き119件、
  構成差262件を維持する。
- `TestPaintingBoundary`の初回構築は新しい公開ヘッダーが存在せず失敗し、実装後は1件のCTestが
  成功した。`kritaui`、`kritalibkis`、既定描画ツール、多角形、多角線はmacOSでリンクまで成功した。
- `TestNotifier`は公開ヘッダー縮小で不足が明らかになった画像ノード型を試験側で明示した後、
  1件のCTestが成功した。`verify-quick`は103件の運用試験、公開面、責務、依存、構造、再配置計画、
  文書、リンク、D2再生成を含めて成功した。
- `nix develop .#test --command ./scripts/verify`はmacOSの全製品と試験ターゲットを構築し、342件中
  341件のネイティブ試験が成功した。変更範囲外の`KisSafeDocumentLoaderTest::test()`は、監視対象を
  再書込みした後の`loadingFinished`が1500ミリ秒以内に届かず、
  `libs/ui/tests/KisSafeDocumentLoaderTest.cpp:44`で実測1件、期待2件として失敗した。単独再実行でも
  同じ行で再現し、同じ試験の`testFileLost()`は成功した。同試験、文書監視実装、文書読込、入出力
  ターゲットには変更がない。
- `libs/ui/tool/kis_tool_rectangle_base.{h,cpp}`にあった矩形制約、修飾キー、ドラッグ座標、回転角、
  矩形計算を`libs/tools/kis_rectangle_interaction.{h,cpp}`へ移した。具体的な値オブジェクトが画像画素
  座標の操作状態を所有し、UI基底にはポインター座標変換、編集可否の警告、寸法と位置の表示、
  輪郭描画、キャンバス更新、設定部品を残した。
- `KisToolRectangleBase`の実装は430行から311行、ヘッダーは86行から75行へ縮小した。新しい
  `KisRectangleInteraction`は実装191行、公開ヘッダー64行であり、UI、キャンバス、文書、描画実行を
  参照しない。矩形、楕円、矩形選択、矩形囲み塗りは既存のUI基底から同じ操作状態を使用する。
- `TestToolCoreContract`は比率制約、固定寸法、Shiftによる正方形化、Altによる移動、Controlによる
  中央拡張、ControlとAltによる回転を固定する。初回構築は新しい公開ヘッダーが存在せず失敗した。
  実装後の初回実行は、移設時に0寸法を無効寸法として既定構築した差を中央拡張契約で検出し、
  明示的な`QSizeF(0, 0)`へ戻した後に1件のCTestが成功した。
- `kritaui`、既定描画ツール、選択ツール、囲み塗りはmacOSでリンクまで成功した。公開面台帳は
  `kritatools`を12ヘッダーから13ヘッダーへ更新し、`kritaui`の234ヘッダーを維持する。toolsの
  未公開内部参照は0件、UIは4ヘッダー7参照、全製品ターゲットの循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験、公開面、責務、依存、構造、
  再配置計画、文書、リンク、D2再生成を含めて成功した。`nix develop .#test --command ./scripts/verify`は
  43増分構築工程とmacOSの全342ネイティブ試験に成功した。前の単位で失敗した変更範囲外の
  `KisSafeDocumentLoaderTest::test()`も成功し、今回の単位に残るmacOS検証失敗はない。
- `libs/ui/tool/kis_tool_polyline_base.{h,cpp}`にあった点列、ドラッグ区間、閉路状態、点の取り消し、
  完了と取消しを`libs/tools/kis_polyline_interaction.{h,cpp}`へ移した。具体的な値オブジェクトが画像
  画素座標の多段階操作を所有し、UI基底にはポインター座標変換、画面距離による始点スナップ判定、
  輪郭表示、再描画範囲、右クリックと操作アクションの接続を残した。
- `KisToolPolylineBase`のヘッダーは63行から59行となり、点列と状態フラグを公開UIクラスから除去した。
  新しい`KisPolylineInteraction`は実装105行、公開ヘッダー48行であり、UI、キャンバス、文書、描画実行を
  参照しない。多角形、多角線、多角形選択は既存のUI基底から同じ操作状態を使用する。
- `TestToolCoreContract`は単一点終了、複数点、カーソル区間、閉路、点の取り消し、全取消しを固定する。
  初回構築は新しい公開ヘッダーが存在せず失敗し、実装後は1件のCTestが成功した。`kritaui`、多角形、
  多角線、選択ツールはmacOSでリンクまで成功した。
- 公開面台帳は`kritatools`を13ヘッダーから14ヘッダーへ更新し、`kritaui`の234ヘッダーを維持する。
  toolsの未公開内部参照は0件、UIは4ヘッダー7参照、全製品ターゲットの循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験、公開面、責務、依存、構造、
  再配置計画、文書、リンク、D2再生成を含めて成功した。`nix develop .#test --command ./scripts/verify`は
  増分構築を完了し、macOSの342件中341件のネイティブ試験が成功した。変更範囲外の
  `KisShapeSelectionTest::testHistoryOnFlattening()`は選択矩形が空となり
  `libs/ui/tests/kis_shape_selection_test.cpp:260`で失敗したが、同試験ターゲットの単独再実行は
  1件中1件成功した。図形選択履歴の製品実装と試験実装には変更がなく、直前の矩形操作状態分離では
  全342件が成功しているため、一時的な既存試験失敗として区別する。
- `libs/ui/tool/KisToolOutlineBase.{h,cpp}`にあった点列、入力中状態、Controlによる継続入力、
  継続点の取り消し、完了と取消しを`libs/tools/kis_outline_interaction.{h,cpp}`へ移した。具体的な
  値オブジェクトが画像画素座標の自由形状操作を所有し、UI基底にはポインター座標変換、編集可否の
  通知、輪郭表示、再描画範囲、入力フィルターと操作アクションの接続を残した。
- `KisToolOutlineBase`のヘッダーは82行から76行、実装は319行から313行となり、未使用だった
  `m_paintPath`も除去した。新しい`KisOutlineInteraction`は実装120行、公開ヘッダー52行であり、
  UI、キャンバス、文書、描画実行を参照しない。自由選択と囲み塗りは既存のUI基底から同じ操作状態を
  使用する。
- `TestToolCoreContract`は通常入力、カーソル位置、完了、取消し、継続入力、継続点の取り消しを
  固定する。初回構築は新しい公開ヘッダーが存在せず失敗し、実装後は1件のCTestが成功した。
  `kritaui`、選択ツール、囲み塗りはmacOSでリンクまで成功した。
- 公開面台帳は`kritatools`を14ヘッダーから15ヘッダーへ更新し、`kritaui`の234ヘッダーを維持する。
  toolsの未公開内部参照は0件、UIは4ヘッダー7参照、全製品ターゲットの循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験、公開面、責務、依存、構造、
  再配置計画、文書、リンク、D2再生成を含めて成功した。`nix develop .#test --command ./scripts/verify`は
  増分構築を完了し、macOSの342件中341件のネイティブ試験が成功した。変更範囲外の
  `KisSafeDocumentLoaderTest`はファイル監視通知が1500ミリ秒以内に届かず、`test()`と
  `testFileLost()`が失敗したが、同試験ターゲットの単独再実行は1件中1件成功した。同試験、文書監視、
  文書読込、入出力の製品実装には変更がなく、既知の時間依存試験失敗として区別する。
- `libs/ui/tool/kis_speed_smoother.{h,cpp}`を`libs/tools/kis_speed_smoother.{h,cpp}`へ移し、
  時刻源と平滑化標本数を設定値として受け取る決定論的な速度計算にした。タブレット試験器は
  UI設定を読んで二つの平滑化器へ明示的に渡し、製品の既存設定を維持する。
- `libs/ui/tool/kis_painting_information_builder.{h,cpp}`の入力値決定部分を
  `libs/tools/kis_painting_information_builder.{h,cpp}`へ、座標変換器と自由描画ツールへの接続を
  `libs/ui/tool/kis_painting_information_builder_adapters.{h,cpp}`へ分けた。設定読込と変更通知は新しい
  `libs/ui/tool/kis_painting_information_builder_config_p.h`から値として渡す。スクラッチパッド、直線、
  自由描画、変形リキファイは同じUI接続を使用し、旧配置と転送ヘッダーは残していない。
- 新しい中核は公開ヘッダー118行、実装331行、UI接続は公開ヘッダー62行、実装147行、内部設定接続
  14行である。旧UI配置の公開ヘッダー154行と実装454行を除去した。`TestToolCoreContract`は固定した
  圧力曲線、座標変換、時刻、遠近係数、回転、反転、負の傾き補正と、固定時刻列による速度平滑化を
  検査する。契約追加直後は`libs/tools`に公開ヘッダーがなくコンパイル段階で失敗し、実装後は1件の
  CTestが成功した。
- 公開面台帳は`kritatools`を15ヘッダーから17ヘッダーへ更新し、`kritaui`の234ヘッダーを維持する。
  UIツール責務台帳は22クラスから21クラスとなった。UIから画像描画への確認済み逆方向includeを
  3件削減し、toolsの未公開内部参照0件、UIの4ヘッダー7参照、23中核所有ターゲットと全製品
  ターゲットの5構成における循環0件を維持する。
- 変形ツールは描画入力値の所有者`kritatools`を直接リンクする。macOSとiOSのCMake台帳を実構成から
  再生成し、Linux、Android、Windowsへ同じ直接辺を同期した。ターゲット数はmacOS 663件、
  Linux 678件、iOS 597件、Android 603件、Windows 633件、共通581件、条件付き119件、構成差262件を
  維持する。`kritaui`、既定描画ツール、変形ツールはmacOSでリンクまで成功した。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験、公開面、責務、依存、構造、
  再配置計画、文書、リンク、D2再生成を含めて成功した。`nix develop .#test --command ./scripts/verify`は
  増分構築とリンクを完了し、macOSの342件中341件のネイティブ試験が成功した。変更範囲外の
  `KisSafeDocumentLoaderTest::testFileLost()`はファイル監視通知が1500ミリ秒以内に届かず
  `libs/ui/tests/KisSafeDocumentLoaderTest.cpp:109`で実測0件、期待1件として失敗した。単独再実行では
  同副試験が成功し、同じ時間依存通知を検査する`test()`が
  `libs/ui/tests/KisSafeDocumentLoaderTest.cpp:44`で実測1件、期待2件として失敗した。同試験、文書監視、
  文書読込、入出力の製品実装には変更がなく、失敗箇所の交替から既知の時間依存試験失敗として
  区別する。`nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、
  Windowsを含む全Nix出力の評価に成功した。
- `libs/ui/widgets/kis_selection_options.{h,cc}`と
  `libs/ui/tool/kis_selection_tool_config_widget_helper.{h,cpp}`を同名の`libs/tools/ui`へ移し、選択方式、
  結合方法、アンチエイリアス、拡張、境界停止、ぼかし、参照レイヤー、色ラベルの表示と保存を
  `kritatoolsui`へ集約した。旧配置と転送ヘッダーは残していない。
- 選択設定が依存する`libs/ui/widgets/kis_color_label_button.{h,cpp}`と
  `libs/ui/widgets/kis_color_label_selector_widget.{h,cpp}`は同名の`libs/widgets`へ移した。固定9色と
  現在の強調色から表示色を作る契約を汎用ウィジェット側へ置き、レイヤーツリーの寸法と配色を持つ
  `libs/ui/kis_node_view_color_scheme.{h,cpp}`はUI所有に維持した。
- `TestToolSettingsUiContract`は全8設定の保存と別ウィジェットへの再読込を固定する。契約追加直後は
  `kritatoolsui`に公開ヘッダーがなくコンパイル段階で失敗し、移設後は1件のCTestが成功した。
  `kritatoolsui`、`kritaui`、選択ツールはmacOSでリンクまで成功した。移設で露出したinclude順依存は、
  `libs/ui/kis_layer_manager.h`の選択型と`libs/ui/tool/kis_tool_select_ui_base.h`の図形型を直接includeして
  解消した。
- 公開面台帳は`kritaui`を234ヘッダーから230ヘッダーへ、UIツール責務台帳を21クラスから20クラスへ
  縮小した。未解決の責務射影0件、構造射影10件、全製品ターゲットの循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験、公開面、責務、依存、構造、
  再配置計画、文書、リンク、D2再生成を含めて成功した。
- `nix develop .#test --command ./scripts/verify`は移動した汎用ウィジェットを使うレイヤードッカー、
  基本塗りつぶし、囲み塗りつぶしを含めて再構築し、macOSの全342件のネイティブ試験に成功した。
  `nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、Windowsを
  含む全Nix出力の評価に成功した。
- `libs/ui/widgets/kis_tool_options_popup.{h,cpp}`を同名の`libs/tools/ui`へ移し、設定部品の見出し、区切り、
  並び替え、非表示部品への退避を`kritatoolsui`へ移した。旧配置と転送ヘッダーは残していない。
  ドック用フォントは`libs/ui/kis_paintop_box.cc`が値として渡し、ポップアップボタン、キャンバス、
  操作アクションとの接続をUI所有に維持した。未使用だったドック登録、設定、翻訳依存も除去した。
- `TestToolSettingsUiContract`は二つの設定部品の見出しと親子関係、単一部品への更新後に旧部品を
  退避する状態を固定する。契約追加直後は`kritatoolsui`に公開ヘッダーがなくコンパイル段階で失敗し、
  移設後は1件のCTestが成功した。`kritatoolsui`と`kritaui`はmacOSでリンクまで成功し、未解決の
  責務射影0件、構造射影10件、全製品ターゲットの循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。
  `nix develop .#test --command ./scripts/verify`はmacOSの全342件のネイティブ試験に成功した。
  `nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、Windowsを
  含む全Nix出力の評価に成功した。
- `libs/ui/tool/kis_rectangle_constraint_widget.{h,cpp}`と
  `libs/ui/forms/wdgrectangleconstraints.ui`を同名の`libs/tools/ui`へ移し、矩形の寸法・比率制約、角丸表示、
  角丸設定の保存と再読込を`kritatoolsui`へ集約した。ウィジェットは矩形ツールを保持せず、設定グループを
  受け取って制約値と角丸値を信号で返す。`libs/ui/tool/kis_tool_rectangle_base.cpp`が矩形状態、設定再読込、
  制約適用を接続し、旧配置と転送ヘッダーは残していない。
- 移設に必要な汎用比率ロックを`libs/ui/kis_aspect_ratio_locker.{h,cpp}`から同名の`libs/widgets`へ移した。
  画像寸法、複数整数フィルター、グリッド、ブラシ、スプレー、基本図形の各利用元は同じ公開型を使い、
  直接利用する4製品ターゲットへ`kritawidgets`リンクを明示した。macOSとiOSのCMake台帳は実構成から
  再生成し、Linux、Android、Windowsへ同じ無条件の4辺を同期した。
- `TestToolSettingsUiContract`は角丸X/Y値、角丸比率ロック、角丸UIの表示可否、別ウィジェットへの
  設定再読込を固定する。契約追加直後は`libs/tools/ui`に公開ヘッダーがなくコンパイル段階で失敗し、
  実装後は1件のCTestが成功した。`kritatoolsui`と`kritaui`はmacOSでリンクまで成功した。
- 公開面台帳は`kritaui`を230ヘッダーから228ヘッダーへ、UIツール責務台帳を20クラスから19クラスへ、
  UI直下責務台帳を80クラスから79クラスへ縮小した。未解決の責務射影0件、構造射影10件、全製品
  ターゲットの循環0件を維持する。`nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と
  全統治検査に成功した。
- `nix develop .#test --command ./scripts/verify`は44増分構築工程を完了し、macOSの342件中341件の
  ネイティブ試験が成功した。変更範囲外の`KisSafeDocumentLoaderTest::test()`はファイル監視通知が
  1500ミリ秒以内に届かず、`libs/ui/tests/KisSafeDocumentLoaderTest.cpp:44`で実測1件、期待2件として
  失敗した。同試験ターゲットの単独再実行は1件中1件成功し、同試験、文書監視、文書読込、入出力の
  製品実装には変更がないため、既知の時間依存試験失敗として区別する。
- `nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、Windowsを
  含む全Nix出力の評価に成功した。
- `libs/ui/tool/kis_shape_tool_helper.{h,cpp}`を`libs/flake/KoBasicShapeFactory.{h,cpp}`へ移し、矩形と
  楕円の生成、登録済み図形ファクトリーの検索、プラグイン不在時のパス代替生成を`kritaflake`へ
  集約した。基本図形ツールと選択ツールは`kritaflake`へ直接依存し、旧UI補助クラスと転送ヘッダーは
  残していない。未使用だった多角形選択側の旧includeも除去した。
- `TestKoShapeFactory`は矩形の位置、寸法、角丸比率を登録済みファクトリーへ渡す契約、楕円の位置と
  寸法を設定する契約、両プラグインがない場合に入力境界矩形と一致するパス図形を返す契約を固定する。
  契約追加直後は`KoBasicShapeFactory.h`がなくコンパイル段階で失敗し、実装後は1件のCTestが成功した。
  `kritaflake`、`kritadefaulttools_static`、`kritaselectiontools`はmacOSでリンクまで成功した。
- 公開面台帳は`kritaui`を228ヘッダーから227ヘッダーへ、UIツール責務台帳を19クラスから18クラスへ
  縮小した。macOSとiOSのCMake台帳は実構成から再生成し、基本図形ツールと選択ツールから
  `kritaflake`への2辺をLinux、Android、Windowsへ同期した。未解決の責務射影0件、構造射影10件、
  全製品ターゲットの循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。
  `nix develop .#test --command ./scripts/verify`は18増分構築工程を完了し、macOSの342件中341件の
  ネイティブ試験が成功した。変更範囲外の`KisSafeDocumentLoaderTest::test()`は
  `libs/ui/tests/KisSafeDocumentLoaderTest.cpp:44`で実測1件、期待2件として失敗し、同試験ターゲットの
  単独再実行は1件中1件成功した。文書監視、文書読込、入出力の製品実装には変更がないため、既知の
  時間依存試験失敗として区別する。
- `nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、Windowsを
  含む全Nix出力の評価に成功した。
- `libs/ui/tool/kis_stabilized_events_sampler.{h,cpp}`と
  `libs/ui/tool/KisStabilizerDelayedPaintHelper.{h,cpp}`を同名の`libs/tools`へ移し、入力イベントの実時間
  標本化、描画点の遅延キュー、タイマー駆動を`kritatools`へ集約した。旧UI配置と転送ヘッダーは残して
  いない。`libs/ui/tool/kis_tool_freehand_helper.cpp`は既存の自由描画線生成と輪郭更新をコールバックで
  接続する。
- `libs/ui/tests/kis_stabilized_events_sampler_test.{h,cpp}`を同名の`libs/tools/tests`へ移し、標本化の
  時間分配に加えて、遅延描画キューが3入力点を2線分として順序どおり完了し、取消し時に未描画点を
  破棄する契約を固定した。契約移設直後は`KisStabilizerDelayedPaintHelper.h`が`kritatools`の公開面に
  なくコンパイル段階で失敗し、実装後は1件のCTestが成功した。`kritatools`と`kritaui`はmacOSで
  リンクまで成功した。
- 公開面台帳は`kritaui`を227ヘッダーから225ヘッダーへ縮小し、`kritatools`を17ヘッダーから
  19ヘッダーへ拡張した。UIツール責務台帳は18クラスから15クラスへ縮小した。入力解釈責務の所有者に
  `kritatools`を記録し、移設した4製品ファイルとUI接続元を明示分類した。未解決の責務射影0件、
  構造射影12件、内部ヘッダー基準10件、全製品ターゲットの循環0件を維持する。
- 入力解釈から描画への直接includeは、移設先に残る3件と、入力とストローク生成が混在するUI接続元の
  明示分類で11件が可視化され、14件から25件になった。製品includeは追加していない。確認済み逆方向
  includeは3責務対104件、移設計画の初期縮小量は319件となり、未解決射影0件を維持する。
- 標本化試験ターゲットは`libs/ui/tests`から`libs/tools/tests`へ移り、直接依存を`kritaui`と
  `kritalibkis`から`kritatools`へ縮小した。macOSとiOSのCMake台帳は実構成から再生成し、Linux、
  Android、Windowsへ同じ無条件差分を同期した。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。
  `nix develop .#test --command ./scripts/verify`は全製品と試験ターゲットをリンクし、macOSの342件中
  341件が成功した。新しい`libs-tools-kis_stabilized_events_sampler_test`も成功した。
- 変更範囲外の`KisSafeDocumentLoaderTest::test()`は
  `libs/ui/tests/KisSafeDocumentLoaderTest.cpp:44`で実測1件、期待2件として失敗した。単独再実行2回も
  不安定であり、1回目は同じ箇所、2回目は`test()`が成功した後に`testFileLost()`が109行で実測0件、
  期待1件として失敗した。文書監視、文書読込、入出力の製品実装には変更がなく、通知待機の既知の
  基線不安定性として区別する。
- `nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、Windowsを
  含む全Nix出力の評価に成功した。
- `libs/ui/tool/KisAsyncColorSamplerHelper.cpp`にあったストローク開始、採取ジョブ投入、完了ジョブ、
  終了処理を、新しい`libs/painting/KisColorSamplerStroke.{h,cpp}`へ分離した。UI側の
  `libs/ui/tool/KisAsyncColorSamplerHelper.{h,cpp}`は採取対象と参照画像の解決、キャンバス色資源、
  カーソル、プレビュー配置と描画を維持し、画像ストローク接続面と内部ストローク戦略を直接扱わない。
  `libs/painting/strokes/kis_color_sampler_stroke_strategy.h`は公開記号を除去し、描画パッケージ内部へ
  閉じた。
- `libs/painting/tests/TestPaintingBoundary.cpp`は、二つの採取ジョブ、完了ジョブ、ストローク終了の順序と、
  最後の採取色を一度だけ確定通知する契約を固定する。契約追加直後は
  `KisColorSamplerStroke.h`が存在せずコンパイル段階で失敗し、実装後は1件のCTestが成功した。
  `kritaui`と`kritadefaulttools`もmacOSでリンクまで成功した。
- 公開面台帳は旧ストローク戦略を新しい実行所有クラスへ置き換えて`kritapainting`の19ヘッダー、
  `kritaui`の225ヘッダー、UIツール責務台帳の15クラスを維持した。入力解釈から描画への直接includeは
  25件から24件、
  確認済み逆方向includeは3責務対104件から103件へ縮小した。未解決の責務射影0件、構造射影12件、
  内部ヘッダー基準10件、全製品ターゲットの循環0件を維持する。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。
  `nix develop .#test --command ./scripts/verify`は全製品と試験ターゲットをリンクし、macOSの
  全342件のネイティブ試験に成功した。`nix flake check --no-build --all-systems --no-eval-cache`は
  macOS、iOS、Linux、Android、Windowsを含む全Nix出力の式評価に成功した。

## R1-G6gショートカット照合境界で進行中の作業

- R1-G6fの完了監査で、分類済み22クラスが`kritatools`、`kritatoolsui`、`kritapainting`、
  `kritaflake`へ移設済みであり、ツール所有に割り当てた17件の`kritaui`内部ヘッダー参照が
  除去済みであることを照合した。残るUIツール責務台帳15クラスは、入力解釈、ストローク生成、
  描画実行、表示接続のR1-G6g以降の所有へ分類され、R1-G6fの二つの完了条件を満たした。
- 次の開始ファイルと移設先を対応させ、旧配置と転送ヘッダーを残さず`kritainput`へ移した。
  - `libs/ui/input/KisInputActionGroup.{h,cpp}`から`libs/input/KisInputActionGroup.{h,cpp}`。
  - `libs/ui/input/kis_abstract_shortcut.{h,cpp}`から`libs/input/kis_abstract_shortcut.{h,cpp}`。
  - `libs/ui/input/kis_single_action_shortcut.{h,cpp}`から`libs/input/kis_single_action_shortcut.{h,cpp}`。
  - `libs/ui/input/kis_stroke_shortcut.{h,cpp}`から`libs/input/kis_stroke_shortcut.{h,cpp}`。
  - `libs/ui/input/kis_touch_shortcut.{h,cpp}`から`libs/input/kis_touch_shortcut.{h,cpp}`。
  - `libs/ui/input/kis_native_gesture_shortcut.{h,cpp}`から
    `libs/input/kis_native_gesture_shortcut.{h,cpp}`。
  - `libs/ui/input/kis_shortcut_matcher.{h,cpp}`から`libs/input/kis_shortcut_matcher.{h,cpp}`。
- `libs/input/KisInputAction.h`は照合器が借用する命令の開始、入力、終了、候補切替、優先度、
  利用可否を定義する。`libs/ui/input/kis_input_manager_p.cpp`の内部委譲オブジェクトが既存の
  `KisAbstractInputAction`へ通知し、入力管理器がプロファイルごとの寿命を所有する。公開UI基底の
  継承構造と仮想関数表を維持しながら、入力ターゲットから`kritaui`への逆向き依存を作らず、
  `kritaui`から`kritainput`へ一方向に接続する。
- タッチ設定列挙から`KisTouchGestureType`、接触点数、タッチ描画中の無効化条件への変換は
  `libs/ui/input/kis_input_manager_p.cpp`へ残した。`KisTouchShortcut`は正規化済み値と動的な状態問い合わせを
  受け取り、設定実装や描画・リソースヘッダーを参照しない。
- `libs/input/tests/TestInputShortcutMatcher.cpp`はShiftと左ボタンによるマウス列を決定的に再生し、
  候補選択、開始、移動、終了、再候補化の通知順を固定する。フォーカス喪失による未完了列の終了と
  入力アクション群マスクのスコープ復旧も同じ独立ターゲットで固定した。契約追加時は
  `KisInputAction.h`不在でコンパイルに失敗し、実装後は1件のCTestが成功した。
- `nix develop .#test --command ./scripts/run-test TestInputShortcutMatcher`、
  `nix develop .#test --command ./scripts/run-test KisInputManagerTest`、
  `nix develop .#test --command ./scripts/build-incremental native build kritaui`はmacOSで成功した。
  既存の非推奨API警告以外に構築失敗はない。
- `nix develop .#test --command ./scripts/verify`はmacOSの全1827構築工程を完了し、
  新しい入力照合契約を含む全343件のネイティブ試験に成功した。既知の時間依存試験
  `KisSafeDocumentLoaderTest`も成功し、この単位に残るmacOS検証失敗はない。
- `nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、
  Windowsを含む全Nix出力の式評価に成功した。
- macOSとiOSのCMake台帳を実構成から再生成し、Linux、Android、Windowsへ共通ターゲットと依存を
  同期した。現在はmacOS 665件、Linux 680件、iOS 599件、Android 605件、Windows 635件、
  共通583件、条件付き119件、構成差266件である。
- 公開面台帳は`kritainput`の9ヘッダーを追加し、`kritaui`を225ヘッダーから221ヘッダーへ縮小した。
  中核所有ターゲットは24件、未公開内部参照は従来の4ヘッダー7件を維持し、全製品ターゲットの
  循環は0件である。移設で可視化されたキャンバス表示とツール呼出しから入力解釈への各1件は、
  R1-G6g所有、上限1件、除去条件を持つ一時逆方向参照として記録した。確認済み逆方向includeは
  5責務対105件となり、R1-G6g完了時に両参照を0件へ除去する。
- 入力プロファイル値について、次の開始ファイルと移設先を対応させ、旧配置と転送ヘッダーを
  残さず所有を移した。
  - `libs/ui/input/kis_input_profile.{h,cpp}`から`libs/input/kis_input_profile.{h,cpp}`。
  - `libs/ui/input/kis_shortcut_configuration.{h,cpp}`の永続値と直列化から
    `libs/input/kis_shortcut_configuration.{h,cpp}`。
  - `libs/ui/input/kis_shortcut_configuration.{h,cpp}`の翻訳済み表示文字列生成から
    `libs/ui/input/kis_shortcut_configuration_text.{h,cpp}`。
- `KisShortcutConfiguration`はUIアクションの借用ポインターに代えて安定識別子を保持し、
  `KisInputProfile`は同じ識別子でショートカットを索引する。プロファイル管理器は読込時に識別子を
  設定し、入力管理器と設定画面だけが識別子をUIアクションへ解決する。既存の
  `{mode;type;[key,key];buttons;wheel;gesture}`保存形式と表示文言を維持する。
- `libs/input/tests/TestInputProfile.cpp`は保存列の固定値、直列化往復、識別子の保持、識別子ごとの
  索引を固定する。契約追加時は`kis_input_profile.h`が入力ターゲットに存在せずコンパイルで失敗し、
  実装後は1件のCTestが成功した。`KisInputManagerTest`は登録済み識別子の解決と未知識別子の拒否を
  固定し、1件のCTestが成功した。`kritaui`もmacOSでリンクまで成功した。
- CMake台帳は`TestInputProfile`を5構成の共通ターゲットとして追加した。現在はmacOS 666件、
  Linux 681件、iOS 600件、Android 606件、Windows 636件、共通584件、条件付き119件、
  構成差267件である。公開面台帳は`kritainput`を11ヘッダーへ増やし、`kritaui`は221ヘッダーを
  維持する。全製品ターゲットの循環は0件である。
- プロファイルから照合器への登録は`KisInputManager::Private`へ集約し、公開入力管理器の実装を
  1148行から1129行へ縮小した。未知のアクション識別子は警告して登録を省略し、既知の識別子だけを
  UIアクション実体へ解決する。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。
  `nix develop .#test --command ./scripts/verify`は全893構築工程を完了し、新しいプロファイル契約を
  含むmacOSの全344件のネイティブ試験に成功した。
  `nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、Windowsを
  含む全Nix出力の式評価に成功した。
- 合成入力抑止について、次の開始ファイルと移設先を対応させ、UI内部の状態所有を除去した。
  - `libs/ui/input/kis_input_manager_p.{h,cpp}`の内部`EventEater`にあった連続マウス抑止、遅延した
    左クリック1回、合成事象、右・中ボタン代替、タッチ開始の状態と判定から、
    `libs/input/KisInputEventSuppressor.{h,cpp}`。
  - 同じ開始ファイルの設定読込、Qt事象から正規化値への変換、タブレット診断表示、`TouchBegin`の
    無視、Qt事象フィルター接続は`libs/ui/input/kis_input_manager_p.{h,cpp}`に維持した。
- `KisInputEventSuppressor`は正規化済みの事象種別、ボタン種別、合成元情報を受け取り、抑止理由を
  返す。右・中ボタン代替設定とプラットフォームの合成事象対応は構築時の値として固定し、macOSの
  合成事象条件、Windowsで実行中ストロークを保護する解除条件、既存のQt事象伝播を維持する。
- `libs/input/tests/TestInputEventSuppressor.cpp`はマウス、タブレット、タッチ列を再生し、連続抑止、
  遅延左クリック1回、合成事象、右・中ボタン代替、タッチ開始の抑止理由を固定する。契約追加時は
  `KisInputEventSuppressor.h`が存在せずコンパイルで失敗し、実装後は1件のCTestが成功した。
  `KisInputManagerTest`も1件のCTestに成功し、`kritaui`はmacOSでリンクまで成功した。
- CMake台帳は`TestInputEventSuppressor`を5構成の共通ターゲットとして追加した。現在はmacOS
  667件、Linux 682件、iOS 601件、Android 607件、Windows 637件、共通585件、条件付き119件、
  構成差267件である。公開面台帳は`kritainput`を12ヘッダーへ増やし、全製品ターゲットの循環0件を
  維持する。`libs/ui/input/kis_input_manager_p.cpp`は1011行から990行へ縮小し、大規模ファイルの
  ソース寸法基準から除去した。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。
  `nix develop .#test --command ./scripts/verify`は44件の増分構築工程で全製品と試験ターゲットを
  リンクし、新しい合成入力抑止契約を含むmacOSの全345件のネイティブ試験に成功した。
  `nix flake check --no-build --all-systems --no-eval-cache`はmacOS、iOS、Linux、Android、Windowsを
  含む全Nix出力の式評価に成功した。
- `libs/ui/input`の87ファイルを同名構造の`libs/input/ui`へ、
  `libs/ui/tests/kis_input_manager_test.{h,cpp}`を`libs/input/ui/tests`へ移した。製品実装と試験は
  `libs/input`の責務ルートに集約され、利用元は`input/ui/...`のinclude経路を使う。
- `kritainputui`オブジェクトターゲットが入力UIを一単位として構築し、`kritaui`が既存ABIへ
  組み込む。公開面台帳は`kritainput`の12ヘッダー、`kritainputui`の9ヘッダー、`kritaui`の
  217ヘッダーを記録する。
- CMake台帳はmacOS 668件、Linux 683件、iOS 602件、Android 608件、Windows 638件、共通586件、
  条件付き119件、構成差268件である。入力UI移設でパッケージ間参照となった内部ヘッダー9件16参照は、
  所有段階と解消先を構造基準と再配置計画に記録する。
- `kritainputui`と`kritaui`のmacOS構築、`KisInputManagerTest`は成功した。clangdの厳格な
  include-cleaner診断は移設した全翻訳単位で不要includeと不足includeを報告していない。
  `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。

## R1-G6g入力境界の第2実装単位で完了した作業

- 次の開始ファイルと移設先を対応させ、入力列、キャンバス状態、画像操作の具体所有を分けた。
  - `libs/input/KisInputActionGroup.{h,cpp}`から`libs/canvas/KisInputActionGroup.{h,cpp}`。
  - `libs/image/kis_timed_signal_threshold.{h,cpp}`から
    `libs/input/KisTimedSignalThreshold.{h,cpp}`。
  - `libs/input/ui/kis_select_layer_action.cpp`のレイヤー探索、選択変更、選択メニューから
    `libs/ui/actions/KisLayerSelectionAction.{h,cpp}`。
  - `libs/ui/dialogs/kis_dlg_preferences.cc`の入力設定ページ追加とタブレット診断スロットから
    `libs/ui/dialogs/kis_dlg_preferences_input.cpp`。
- `KisCanvas2`はキャンバス単位の入力アクション群マスクとスコープガードを維持し、登録中の
  `KisInputManager`から優先事象フィルター接続とキャンバス部品変更通知を受け取る。
  ガイド、無限キャンバス、鏡軸、輪郭、折線は`KisCanvas2`または`KisToolCanvas`の具体操作面を使う。
- 入力アクションはストローク終了と取消しを`KisToolCanvas`、活動ノードのアニメーション判定を
  `KisViewManager`、レイヤー選択をUIアクションへ委譲する。矩形修飾キーは入力写像器と同じ
  ShiftとMetaの正規化結果を局所値として使う。
- 遅延描画、安定化標本化、自由描画ストローク生成、非同期色採取表示を実装する翻訳単位は
  ツール呼出しへ帰属する。入力責務の既定ディレクトリーは`libs/input`と`libs/input/ui`であり、
  個別のUI接続は分類済み公開クラスと審査済みソースで帰属する。
- 確認済み逆方向includeは、キャンバス表示から入力解釈13件、入力解釈から描画24件、
  ツール呼出しから入力解釈7件が各0件になった。入力解釈からリソース管理は3件へ縮小し、
  アプリケーション調整から描画75件と合わせた確認済み基準は2責務対78件、未確定射影は0件である。
- `nix develop .#test --command ./scripts/build-incremental native build kritaui`、
  `nix develop .#test --command ./scripts/run-test KisInputManagerTest`、
  `nix develop .#test --command ./scripts/run-test TestInputShortcutMatcher`はmacOSで成功した。
  clangdのinclude-cleaner検査は変更した21翻訳単位で不要includeと不足includeを報告していない。
  `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。

## R1-G6g入力プロファイル配置境界で完了した作業

- `libs/ui/KisViewManager.cpp`がプロファイルの優先順付きファイル一覧と利用者用保存ディレクトリーを
  解決し、既存の`KisInputProfileManager`へ値として渡す。プロファイル管理器は同じ値で読込、保存、
  削除、再読込、既定値復旧を行い、移行器は一覧から選択済みの既定プロファイルを受け取る。
- `libs/input/ui/KisInputProfileMigrator.cpp`、
  `libs/input/ui/config/kis_input_configuration_page.cpp`、
  `libs/input/ui/kis_input_profile_manager.cpp`から`KoResourcePaths.h`参照を除去した。
  `kritainputui`から`kritaresources`と`kritaresourceui`への直接CMake依存も除去した。
- `KisInputManagerTest::testProfileStorageLifecycle()`は、利用者用プロファイル優先、version 5から6への
  移行と互換保存、編集後の保存、同梱既定プロファイルへの復旧を一時配置上で固定する。契約追加時は
  `setProfileLocations()`が存在せずコンパイルで失敗し、実装後は1件のCTestが成功した。
- 確認済み逆方向includeは入力解釈からリソース管理3件が0件となり、アプリケーション調整から
  描画75件だけを残す1責務対75件へ縮小した。未確定射影0件、解決済み構造射影10件、
  25中核所有ターゲットと全製品ターゲットの循環0件を維持する。
- CMake台帳はmacOS 668件、Linux 683件、iOS 602件、Android 608件、Windows 638件、共通586件、
  条件付き119件、構成差270件を維持する。macOSとiOSを実構成から再生成し、Linux、Android、
  Windowsへ同じ無条件依存削除を同期した。
- `kritainputui`と`kritaui`のmacOS構築、`KisInputManagerTest`は成功した。clangdのinclude-cleaner検査は
  変更した5翻訳単位で不要includeと不足includeを報告していない。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。

## R1-G6gキャンバス表示配置境界で完了した作業

- `libs/ui`直下のアニメーション再生と非同期フレーム描画14ファイルを同名の
  `libs/ui/animation`へ、キャンバス表示状態、装飾、座標変換、表示接続55ファイルを同名の
  `libs/ui/canvas`へ移した。
- `KisAsyncAnimation*`、`KisPlaybackEngine*`、`KisMLTProducerKrita*`はアニメーション再生配置を
  所有する。`KisAsyncAnimationFramesSavingRenderer.cpp`は動画書出し調整として
  `document-lifecycle`の審査済み帰属を維持する。残る移設ファイルはキャンバス表示配置を所有する。
- `KisDecorationsManager`と`KisDecorationsWrapperLayer`、`KisMultiSurfaceStateManager`、
  `kcanvaspreview`、`kis_fps_decoration`、`KisClonableViewConverter`、`kis_mirror_manager`は、
  同じ具象表示関心の実装と対になるヘッダーとしてキャンバス表示配置へ集約した。
- `kritaui`のCMake所有、クラス名、公開記号、所有寿命を維持し、製品と試験のincludeを正規の
  `animation/...`または`canvas/...`経路へ更新した。公開ヘッダー試験は旧経路の移設前に
  `animation/KisAsyncAnimationRendererBase.h`不在で失敗し、移設後に成功した。
- UIクラス責務台帳は79クラス、実装単位を持つ77クラス、宣言側で完結する2クラスを維持する。
  キャンバス・表示31クラスは分類済み入れ子ヘッダーとして継続追跡され、台帳上の
  `libs/ui`直下キャンバス・表示ヘッダーは0件となった。5構成のCMakeターゲット台帳は
  ターゲット、構成差、直接リンク依存を維持する。
- `nix develop .#test --command ./scripts/build-incremental native build kritaui`、
  `TestCanvasUiPublicHeaders`、`kis_image_view_converter_test`、`KisFrameCacheSwapperTest`、
  `kis_display_color_converter_contract_test`はmacOSで成功した。clangdのinclude-cleaner検査は
  移設した全33翻訳単位で不要includeと直接include不足を報告していない。
- `nix develop .#test --command ./scripts/verify-quick`は103件の運用試験と全統治検査に成功した。

## R1-G6g文書状態UI配置境界で完了した作業

- `libs/ui`直下の文書構成と文書調整15ファイルを`libs/ui/document`、ノード表示モデルと
  ノード操作接続26ファイルを`libs/ui/nodes`、選択操作接続と選択表示部品10ファイルを
  `libs/ui/selection`へ移した。
- `KisDocument`、外部ファイル層、画像・フィルター管理、文書テキストと絵コンテ項目は
  文書構成配置を所有する。ノード表示モデル、レイヤーとマスクの管理、ノード挿入接続は
  ノード配置を所有し、選択アクション接続と選択パネル部品は選択配置を所有する。
- `kritaui`のCMake所有、クラス名、公開記号、公開挙動、所有寿命を維持し、製品と試験の
  includeを`document/...`、`nodes/...`、`selection/...`の正規経路へ更新した。
  公開ヘッダー試験は移設前に`document/KisDocument.h`不在で失敗し、移設後に成功した。
- UIクラス責務台帳は79クラスを維持し、文書状態20クラスを18の分類済み入れ子ヘッダーで
  継続追跡する。責務ディレクトリー、翻訳単位のCMake登録、分類済みヘッダーとの対応は
  公開面検査が継続確認する。
- `kritaui`、`TestDocumentStateUiPublicHeaders`、`kis_multinode_property_test`、
  `kis_shape_selection_test`、`KisDocumentReplaceTest`はmacOSで成功した。clangdの
  include-cleaner検査は移設した全25翻訳単位で不要includeと直接include不足を報告していない。
- `nix develop .#test --command ./scripts/verify-quick`は104件の運用試験と全統治検査に成功した。
- `KisSelectedShapesProxy.*`、`KisUndoActionsUpdateManager.*`、`kis_filters_model.*`、
  `kis_bookmarked_filter_configurations_model.*`はベクター選択、履歴アクション、フィルター一覧の
  画面接続を含む`libs/ui`直下の候補であり、公開文書状態クラス集合の外側で責務分類を行う。

## R1-G6gアプリケーション・作業空間・ツールUI配置境界で完了した作業

- `libs/ui`直下のアプリケーション調整20ファイルを`libs/ui/application`、ウィンドウ・
  作業空間42ファイルを`libs/ui/workspace`、ツール呼出し8ファイルを既存の`libs/ui/tool`へ
  同じ基底名で移した。
- アクション、起動、設定、プラグイン、資源提供、Androidファイル接続はアプリケーション調整配置を
  所有する。ウィンドウ、ビュー、セッション、作業空間、テンプレート、環境設定、
  起動画面は作業空間配置を所有する。ブックマーク済みツール・フィルター設定と描画ツール箱は
  ツール呼出し配置を所有する。
- `kritaui`のCMake所有、クラス名、公開記号、公開挙動、所有寿命を維持し、製品と試験のinclude、
  CMakeソース、`wdgsplash.ui`の参照を`application/...`、`workspace/...`、`tool/...`の正規経路へ
  更新した。公開includeは正規経路へ直接接続し、公開ヘッダー試験は移設前に
  `application/KisActionPlugin.h`不在で失敗し、移設後に成功した。
- UIクラス責務台帳は79クラス、69ヘッダー、実装単位を持つ77クラス、宣言側で完結する2クラスを
  維持する。対象の27クラスを25の分類済み入れ子ヘッダーで継続追跡し、分類済み公開クラスの
  `libs/ui`直下配置を0件に固定した。UIツールクラス責務台帳は18クラス、16ヘッダー、利用元56ソースを
  記録する。
- `kritaui`、`TestApplicationWorkspaceToolUiPublicHeaders`、`kis_derived_resources_test`、
  `kis_view_signals_test`はmacOSで成功した。clangdのinclude-cleaner検査は移設した全36翻訳単位で
  不要includeと直接include不足を報告していない。
- `nix develop .#test --command ./scripts/verify-quick`は104件の運用試験と全統治検査に成功した。
- `libs/ui`直下の台帳外候補は、入出力表示の`KisImportExport*`、事象接続の
  `KisLongPressEventFilter.*`と`KisMouseClickEater.*`、表示構成とOS接続の`KisUiFont.*`、
  `thememanager.*`、`osx.*`、ツール・資源表示の`KisPresetShadowUpdater.*`、`kis_control_frame.*`、
  `kis_custom_pattern.*`、`kis_derived_resources.*`、`kis_favorite_resource_manager.*`、
  `kis_filters_model.*`、`kis_popup_palette.*`、文書・図形接続の`KisSelectedShapesProxy.*`と
  `KisUndoActionsUpdateManager.*`、共有UI接続面と補助処理の残りで構成される。

## R1-G6g UI root残存配置境界で完了した作業

- `libs/ui`直下の台帳外43ファイルを現在責務へ配置した。入出力表示2ファイルは
  `libs/ui/impex`、事象接続13ファイルは`libs/ui/events`、テーマ表示4ファイルは
  `libs/ui/theme`、macOS接続3ファイルは`libs/ui/platform`、資源表示17ファイルは
  `libs/ui/resources`、図形選択接続2ファイルは`libs/ui/flake`、履歴アクション接続2ファイルは
  `libs/ui/actions`が所有する。
- `libs/ui`直下には`CMakeLists.txt`と`kritaui_export_instance.h`の2ファイルが存在する。
  前者は`kritaui`の構築定義を、後者はターゲット公開テンプレート記号の設定を所有する。
  再配置検査は旧開始パスの消滅、全宛先の実在、責務ディレクトリー、CMake登録、rootの
  正確な2ファイルを継続確認する。
- `KisPopupWidgetInterface.h`、`KisUiFont.h`、`kis_cursor_override_hijacker.h`、
  `kis_favorite_resource_manager.h`、`kis_popup_palette.h`の公開面は責務別の正規入れ子経路を持つ。
  `TestRemainingUiRootPublicHeaders`が5ヘッダーを一つの利用翻訳単位として構築する。
- `kritaui`、`TestRemainingUiRootPublicHeaders`、`kis_derived_resources_test`はmacOSで成功した。
  clangdのinclude-cleaner検査はCMake登録された移設先20翻訳単位で不要includeと直接include不足を
  報告していない。
- `nix develop .#test --command ./scripts/verify-quick`は全運用試験と統治検査に成功した。

## R1-G6gアプリケーション・作業空間include境界で完了した作業

- `libs/ui/application/KisApplication.cpp`、`KisPart.cpp`、`kis_action_manager.cpp`と、
  `libs/ui/workspace/KisMainWindow.cpp`、`KisView.cpp`、`KisViewManager.cpp`、
  `kis_statusbar.cc`から、記号利用を持たない35件のincludeを除去した。
- `KisApplication.cpp`は`KoGamutMask.h`と`KisSeExprScript.h`を直接参照する。
  完全型を必要とする`KisPlaybackEngine.h`、`KisToolBarStateModel.h`、`kis_selection.h`も
  各利用翻訳単位から直接参照する。
- アプリケーション調整から描画への確認済み逆方向includeは、`filter/kis_filter.h`、
  `generator/kis_generator.h`、`brushengine/kis_paintop_settings.h`の3件を解消し、
  1責務対75件から72件へ縮小した。未確定射影0件とターゲット循環0件を維持する。
- `kritaui`、`TestApplicationWorkspaceToolUiPublicHeaders`、`kis_view_signals_test`は
  macOSで成功した。変更した7翻訳単位の差分はinclude行に限定される。
  `nix develop .#test --command ./scripts/verify-quick`は104件の運用試験と全統治検査に成功した。

## R1-G6gアプリケーション・作業空間include境界の第2単位で完了した作業

- アプリケーション調整から描画への72件を、公開ヘッダーの画像型参照7件、
  `KisApplication.cpp`の起動時レジストリー登録13件、文書・表示操作52件へ分類した。
- `libs/ui/application/KisPart.h`、`libs/ui/workspace/KisView.h`、`KisViewManager.h`、
  `kis_statusbar.h`は、公開関数で使用する画像共有ポインター型を前方宣言する。
  `libs/ui/workspace/kis_preference_set_registry.h`はUI設定画面の登録型だけを宣言する。
- `libs/impex/animation/KisFFMpegWrapper.cpp`はファイル入出力診断を宣言する
  `kis_debug.h`を直接includeする。公開ヘッダーからの推移的な宣言に依存しない。
- 確認済み逆方向includeは1責務対67件となり、公開ヘッダー型参照2件、起動時
  レジストリー登録13件、文書・表示操作52件を持つ。公開ヘッダーの2件は、ノード追加
  フラグ値と作業空間資源の設定基底型を宣言する。未確定射影0件とターゲット循環0件を維持する。
- `kritaui`、`krita`、`TestApplicationWorkspaceToolUiPublicHeaders`、
  `kis_view_signals_test`はmacOSで成功した。公開面台帳は`kis_types.h`の利用元を同期し、
  変更した5公開ヘッダーと1翻訳単位のclangd include-cleaner検査は不要includeと直接include不足を
  報告していない。`nix develop .#test --command ./scripts/verify-quick`は104件の運用試験と
  全統治検査に成功した。

## R1-G6g起動時描画登録所有境界で完了した作業

- 起動時の組込み描画資源登録を次の開始ファイルと所有先へ対応させた。
  - `libs/ui/application/KisApplication.cpp`のペイントプリセットローダー定義から
    `libs/image/brushengine/kis_paintop_registry.{h,cc}`の`registerResourceLoader()`。
  - 同ファイルのGBR、GIH、SVG、PNGブラシローダーと優先度10のブラシメタデータ修復登録から
    `libs/brush/kis_brush_registry.{h,cpp}`の`registerResourceLoaders()`と
    `registerResourceCacheFixup()`。
  - 同ファイルのPSDレイヤースタイルローダー定義から
    `libs/image/kis_psd_layer_style.{h,cpp}`の`registerResourceLoader()`。
- `libs/ui/application/KisApplication.cpp`のフィルター、生成器、ペイント操作、メタデータの
  レジストリー起動は、`libs/koplugin/KoPluginLoader.{h,cpp}`が所有する既存サービス読込面を使う。
  サービス順序、プラグインの具体レジストリー、プロセス寿命、診断経路を維持する。
- 資源ローダーの副種別、資源種別、表示名、MIME型、ブラシ修復優先度と登録順序を維持した。
  `libs/image/tests/TestBuiltInResourceLoaderRegistration.cpp`はペイントプリセット、4ブラシ形式、
  PSDレイヤースタイルを各MIME型から解決する契約を固定する。
- `KisApplication.cpp`の描画所有ヘッダーは13件から4件となった。確認済み逆方向includeは
  1責務対67件から58件へ縮小し、未確定射影0件、製品ターゲット循環0件を維持する。
- `filter/kis_filter_configuration.h`と`kis_meta_data_io_backend.h`の直接利用を持たないincludeを
  除去した。変更した4製品翻訳単位と契約試験のinclude-cleaner検査は、直接利用する宣言を各所有者の
  正規ヘッダーから解決する。
- macOSで`krita`、`TestBuiltInResourceLoaderRegistration`、
  `TestApplicationWorkspaceToolUiPublicHeaders`、`kis_filter_registry_test`の構築と試験に成功した。
  `nix develop .#test --command ./scripts/verify-quick`は104件の運用試験と全統治検査に成功した。

## R1-G6gアクション有効状態所有境界で完了した作業

- `libs/ui/application/kis_action_manager.cpp`にあった画像アニメーション有無の取得を
  `libs/ui/document/KisDocument.{h,cpp}`の`hasAnimation()`へ配置した。
- 同じ開始ファイルにあった活動ノードの存在、レイヤー型、継承型、編集可否、編集可能な
  ペイントデバイス有無の取得を`libs/ui/nodes/kis_node_manager.{h,cpp}`へ配置した。
  アクション管理は所有者から得た値を既存の順序で起動フラグと起動条件へ合成する。
- `libs/ui/application/kis_action_manager.cpp`から`kis_layer.h`と
  `kis_image_animation_interface.h`の直接includeを除去し、`nodes/kis_node_manager.h`を追加した。
  確認済み逆方向includeは58件から56件へ縮小し、未確定射影0件、製品ターゲット循環0件を維持する。
- `libs/ui/tests/kis_action_manager_test.cpp`は活動ノード未設定時とペイントレイヤー起動後の
  `ACTIVE_LAYER`契約を検査する。macOSで`KisActionManagerTest`の構築に成功した。
  CTest登録はbroken指定で除外され、直接実行は表示初期化中の既存SIGSEGVを再現する。
- 変更した4翻訳単位のclangd include-cleaner検査は不要includeと直接include不足を報告していない。
- `KisDocument.cpp`は最大3027行、`kis_node_manager.cpp`は最大1763行の審査済みソース行数例外を持つ。
  R1-G6hの文書・画像構成とノード・画像調整の所有分割が各ファイルを標準最大値へ縮小し、例外を完了する。

## R1-G6gアプリケーション共有サービス所有境界で完了した作業

- アプリケーション調整にあった描画状態の取得と共有サービス接続を、次の開始ファイルと所有先へ
  対応させた。
  - `libs/ui/application/KisApplication.cpp`のアニメーション書出し範囲取得から
    `libs/ui/dialogs/KisAsyncAnimationFramesSaveDialog.{h,cpp}`の文書再生範囲を使う生成経路。
  - `libs/ui/application/KisPart.cpp`のテンプレート層名変換から
    `libs/ui/document/KisDocument.h`と`libs/ui/document/KisDocumentTemplate.cpp`の
    `translateTemplateRootLayerName()`。
  - 同じ開始ファイルの優先キャッシュ範囲判定から
    `libs/ui/animation/kis_animation_cache_populator.cpp`の優先要求受付。
  - 同じ開始ファイルの画像待機表示コールバック登録から
    `libs/ui/dialogs/kis_delayed_save_dialog.{h,cpp}`の`registerBusyWaitFeedback()`。
  - 同じ開始ファイルのアイドル時メモリー統計接続から
    `libs/image/kis_idle_watcher.{h,cpp}`の`connectMemoryStatisticsUpdates()`。
- アニメーション書出し範囲、優先キャッシュの再生範囲、テンプレートのルートレイヤー名、強制待機表示、
  アイドル時メモリー統計更新の順序と寿命を維持する。アプリケーション調整から描画への
  確認済み逆方向includeは56件から48件へ縮小し、未確定射影0件、製品ターゲット循環0件を維持する。
- `libs/ui/tests/kis_animation_exporter_test.cpp`は書出しダイアログが画像の文書再生範囲を使う経路を
  検査する。`libs/ui/tests/KisDocumentReplaceTest.cpp`はテンプレート辞書によるルートレイヤー名変換を検査する。
- 変更した9翻訳単位はmacOSの製品コンパイル条件でコード生成に成功した。include-cleaner監査は
  未使用include 21件を除去し、`QMap`、標準アルゴリズム、排他制御、所有権、関数、キューの
  直接includeを追加した。更新した実装オブジェクトを既存ライブラリーへ個別リンクした
  `KisDocumentReplaceTest::testTemplateRootLayerNameTranslation`と
  `KisAnimationExporterTest::testAnimationExport`は成功した。`verify-quick`は方針試験104件、
  生成台帳、依存方向、製品ターゲット循環、ソース行数、文書と図の検査に成功した。

## R1-G6gキャンバス状態表示所有境界で完了した作業

- `libs/ui/workspace/kis_statusbar.h`を`libs/ui/canvas/kis_statusbar.h`、
  `libs/ui/workspace/kis_statusbar.cc`を`libs/ui/canvas/kis_statusbar.cc`へ移した。
  `KisStatusBar`は画像寸法、選択範囲、色プロファイル、メモリー使用量、キャンバス回転の
  表示と利用者操作との接続をキャンバス表示配置で所有する。
- `kritaui`のCMake所有、公開クラス名、公開記号、所有寿命を維持し、製品と公開ヘッダー試験の
  10 include経路を`canvas/kis_statusbar.h`へ同期した。公開面検査はキャンバス表示、
  アプリケーション、作業空間、ツールの現在配置を継続確認する。
- UIクラス責務台帳は`KisStatusBar`をキャンバス・表示へ分類し、キャンバス・表示31クラス、
  ウィンドウ・作業空間16クラスを記録する。アプリケーション調整から描画への確認済み
  逆方向includeは48件から42件へ縮小し、未確定射影0件、製品ターゲット循環0件を維持する。
- 変更した11翻訳単位はmacOSの製品コンパイル条件でコード生成に成功した。
  clangd include-cleaner監査は未使用includeと直接include不足が0件であることを確認した。
  更新した公開ヘッダー試験オブジェクトを既存ライブラリーへ個別リンクした
  `TestCanvasUiPublicHeaders`は3件すべて成功した。移設後の実装は474行、公開ヘッダーは135行で、
  ソース行数検査の標準最大値内にある。
- `verify-quick`は方針試験104件、生成台帳、依存方向、製品ターゲット循環、ソース行数、
  文書と図の検査に成功した。

## R1-G6gメインウィンドウ画像状態所有境界で完了した作業

- `libs/ui/workspace/KisMainWindow.cpp`にあった描画状態の取得と操作を、次の開始箇所と
  具体所有へ接続した。
  - 画像設定変更通知から`libs/ui/dialogs/KisDlgPreferencesNotifications.cpp`。
  - ルートノード設定更新とノード選択アクション生成から
    `libs/ui/nodes/KisNodeManagerImageState.cpp`。
  - 画像の存在と名前、アニメーション長・範囲・フレーム率、投影更新待機から
    `libs/ui/document/KisDocumentImageState.cpp`。
  - 保存済みアニメーション書出し設定読込から
    `libs/impex/animation/KisAnimationRenderingOptions.cpp`。
- 設定通知の順序と全ビュー走査条件、ノード選択アクションの走査順、動画取込の現在長診断、
  範囲拡張条件、フレーム率設定、投影完了待機、描画再実行の設定キーを維持した。
  clangd監査で直接利用0件を確認した`KisMainWindow.cpp`の`krita_utils.h`も除去した。
- アプリケーション調整から描画への確認済み逆方向includeは42件から36件へ縮小した。
  未確定射影0件、製品ターゲット循環0件を維持する。
- 変更した製品翻訳単位と契約試験はmacOSの製品コンパイル条件でコード生成に成功した。
  clangd include-cleaner監査は未使用includeと直接include不足が0件であることを確認した。
  更新した実装オブジェクトを既存ライブラリーへ個別リンクした
  `KisDocumentReplaceTest::testImageStateDelegation`は成功した。
- `KisMainWindow.cpp`は3354行、`kis_node_manager.cpp`は1763行、
  `kis_dlg_preferences.cc`は3327行、`KisDlgAnimationRenderer.cpp`は1089行で、
  各審査済みソース行数上限以内にある。新しい具体所有実装3件は各標準最大値内にある。
- `verify-quick`は方針試験104件、生成台帳、依存方向、製品ターゲット循環、ソース行数、
  文書と図の検査に成功した。

## R1-G6g作業ビュー画像状態所有境界で完了した作業

- `libs/ui/workspace/KisView.cpp`にあった画像状態の取得、信号接続、画像編集を、次の開始箇所と
  具体所有へ接続した。
  - 画像信号接続、表示準備、浮動小数点色深度判定から
    `libs/ui/canvas/KisCanvasImageState.cpp`と`libs/ui/canvas/kis_canvas2.h`。
  - 色ドロップルーティングから`libs/ui/canvas/KisCanvasColorDrop.cpp`の
    塗りつぶしストローク。
  - 内部ノード、画像、URL、参照画像のドロップ処理から
    `libs/ui/document/KisImageManagerDrop.cpp`と`libs/ui/document/kis_image_manager.{h,cc}`。
  - 現在レイヤー、マスク、選択範囲、ノード除去後の選択先取得から
    `libs/ui/nodes/KisNodeManagerImageState.cpp`。
  - 画像メモリー統計取得と更新通知接続から`libs/ui/document/KisDocumentImageState.cpp`。
- ドロップ操作の選択肢、修飾キー、塗りつぶしジョブと取り消し命令の順序、画像とノードの
  共有寿命、ノード通知の直接接続、表示開始時の画像信号接続順、文書タイトルのメモリー表示を
  維持した。`kis_image_manager.h`の新規操作は通常public面を使い、既存slotのメタオブジェクト面を
  維持する。
- アプリケーション調整から描画への確認済み逆方向includeは36件から20件へ縮小した。
  未確定射影0件、製品ターゲット循環0件を維持する。公開ヘッダー、UIクラス責務、構造依存の
  各台帳は新しい実利用経路を記録する。
- 影響する10製品翻訳単位、契約試験、試験用mocはmacOSの製品コンパイル条件でコード生成に
  成功した。clangd include-cleaner監査は未使用includeと直接include不足が0件であることを
  確認した。
  更新した実装オブジェクトを既存ライブラリーへ個別リンクした
  `KisDocumentReplaceTest::testImageStateDelegation`は3件すべて成功した。
- `KisView.cpp`は857行となり、標準ソース行数上限内にある。
  `kis_canvas2.cpp`は1720行、`kis_node_manager.cpp`は1763行を維持し、新しい責務別翻訳単位3件は
  各標準最大値内にある。
- `verify-quick`は方針試験104件、生成台帳、依存方向、製品ターゲット循環、ソース行数、
  文書と図の検査に成功した。

## R1-G6g作業ビュー管理画像状態所有境界で完了した作業

- `libs/ui/workspace/KisViewManager.cpp`にあった描画状態の取得と資源初期化を、次の開始箇所と
  具体所有へ接続した。
  - キャンバス資源変換器、更新仲介、活動資源依存の初期化から
    `libs/ui/canvas/kis_canvas_resource_provider.{h,cpp}`。
  - 画像進捗表示登録、画像取り消し接続取得、読取障壁ロック中の文書複製から
    `libs/ui/document/KisDocument.h`と`libs/ui/document/KisDocumentImageState.cpp`。
  - 活動ノード動画判定と活動レイヤー選択マスク編集可否判定から
    `libs/ui/nodes/kis_node_manager.h`と`libs/ui/nodes/KisNodeManagerImageState.cpp`。
- 資源変換器と依存の登録順、進捗表示の借用寿命、画像信号接続順、複製前の操作完了待機、
  読取障壁ロック、文書保管場所の作成、取り消し接続、活動選択の判定を維持した。
  `KisViewManager.cpp`の`kis_paint_layer.h`直接includeを除去した。
- `KisViewManager.cpp`から描画所有ヘッダー10件への直接includeを除去した。アプリケーション調整から
  描画への確認済み逆方向includeは20件から10件へ縮小し、未確定射影0件、製品ターゲット循環0件を
  維持する。
- 変更した4製品翻訳単位と1契約試験はmacOSの製品コンパイル条件でコード生成に成功した。
  clangd include-cleaner監査は未使用includeと直接include不足が0件であることを確認した。
  `KisDocumentReplaceTest::testImageStateDelegation`と`KisDerivedResourcesTest`は各3件すべて成功した。
- `KisViewManager.cpp`は1643行となり、審査済みソース行数上限を同じ値へ縮小した。
  具体所有へ接続した3実装は各標準最大値内にある。
- `verify-quick`は方針試験104件、生成台帳、依存方向、製品ターゲット循環、ソース行数、
  文書と図の検査に成功した。

## R1-G6g起動資源・共有監視所有境界で完了した作業

- `libs/ui/application/KisApplication.cpp`にあった組込み描画資源登録を、次の所有先へ接続した。
  - ペイントプリセットとブラシローダー登録から
    `libs/ui/canvas/kis_canvas_resource_provider.{h,cpp}`の組込み描画資源登録。
  - PSDレイヤースタイルローダー登録から同じ所有先のレイヤースタイル資源登録。
  - ブラシメタデータ修復登録から同じ所有先のブラシキャッシュ修復登録。
- `libs/ui/application/KisPart.cpp`の共有状態を、次の所有先へ接続した。
  - システム色管理初期化から`libs/ui/canvas/KisDisplayConfig.{h,cpp}`の表示色管理初期化。
  - アプリケーション単位アイドル監視から
    `libs/ui/animation/kis_animation_cache_populator.{h,cpp}`の文書画像追跡とキャッシュ生成通知。
- 組込みローダーと修復処理の登録位置、資源種別、MIME型、優先度、色管理singletonの生成時機、
  アイドル監視の生成・破棄順、文書画像追跡、メモリー統計通知、公開監視ポインターを維持した。
- アプリケーション調整から描画への確認済み逆方向includeは10件から5件へ縮小した。
  未確定射影0件と製品ターゲット循環0件を維持する。公開ヘッダー台帳は描画資源登録と
  アイドル監視の実利用経路を各具体所有へ同期する。
- 変更した5製品翻訳単位と2契約試験はmacOSの製品コンパイル条件でコード生成に成功した。
  clangd include-cleaner監査は未使用includeと直接include不足が0件であることを確認した。
  更新したキャンバス資源所有実装を既存ライブラリーへ個別リンクした
  `TestApplicationPaintingResourceRegistration`は3件すべて成功した。
- `verify-quick`は方針試験104件、生成台帳、依存方向、製品ターゲット循環、ソース行数、
  文書と図の検査に成功した。

## R1-G6g残存設定・セッション・作業空間表示所有境界で完了した作業

- 残存する共有値型と一時ファイル方針を、次の開始ファイルと所有先へ対応させた。
  - `libs/image/KisNodeAdditionFlags.h`から`libs/global/KisNodeAdditionFlags.h`へ、
    ノード追加通知の値フラグを移した。
  - `libs/painting/undo/KisCumulativeUndoData.{h,cpp}`から
    `libs/global/KisCumulativeUndoData.{h,cpp}`へ、取り消し統合の設定値を移した。
  - `libs/image/kis_image_config.{h,cpp}`の一時ファイルとswap配置方針から
    `libs/global/KisTemporaryFileConfiguration.{h,cpp}`へ、書込み可能な配置解決を移した。
    `libs/ui/application/kis_config.cc`と画像設定は同じ具体方針を利用する。
- `libs/ui/workspace/KisSessionResource.cpp`のビュー表示状態は、同ファイルの値地図直列化と
  `libs/ui/workspace/KisView.{h,cpp}`の表示状態取得・復元へ接続した。作業空間表示資源は
  `libs/ui/workspace/kis_workspace_resource.{h,cpp}`から
  `libs/canvas/workspace/kis_workspace_resource.{h,cpp}`へ移した。
- `kritaworkspacepresentation`は作業空間表示資源を`canvas-presentation`責務で所有する。
  `kritaui_EXPORTS`で生成したオブジェクトを`kritaui`へ組み込み、既存の`KRITAUI_EXPORT`公開記号、
  型名、ABIを維持する。`kritaui`から色、画像、ブラシ、描画の各ターゲットへの直接リンクを解消した。
- 取り消し統合の設定キーと既定値、一時ファイル設定キーと既定・代替配置、セッションと作業空間の
  XML形式、Qt信号引数、ノード追加通知を維持する。`KisConfigurationValueTypesTest`は設定値と配置方針、
  `KisWorkspacePersistenceTest`は作業空間とセッションの直列化往復を固定する。
- アプリケーション調整から描画への確認済み逆方向includeと審査済み上限は各0件である。
  未確定射影は0件であり、26中核ターゲットとmacOS 226件、Linux 232件、iOS 218件、
  Android 218件、Windows 235件の製品ターゲットは各構成で循環0件である。
- macOSで`kritaimpexui`、`kritaworkspacepresentation`、`kritaui`の構築に成功した。
  `KisConfigurationValueTypesTest`、`KisWorkspacePersistenceTest`、
  `TestApplicationWorkspaceToolUiPublicHeaders`、`TestKUndo2Stack`はすべて成功した。
  clangdのinclude-cleaner監査により、変更した翻訳単位の未使用includeを除去し、実利用する
  所有ヘッダーを各利用元から直接参照する。
- 固定Nix環境の`./scripts/verify-quick`は105件の運用試験、生成台帳、依存方向、
  製品ターゲット循環、移設元消滅、移設先実在、CMake所有、ソース行数、文書と図の検査に成功した。

## R1-G6g入力UI共有ライブラリー境界で完了した作業

- 入力のQt事象接続、設定表示、診断、プラットフォーム統合を`libs/input/ui`に集約し、
  アプリケーション状態とキャンバス表示を扱う実装を次の所有先へ対応させた。
  - `libs/input/ui/KisCanvasOnlyAction.{h,cpp}`から
    `libs/ui/actions/input/KisCanvasOnlyAction.{h,cpp}`。
  - `libs/input/ui/KisTouchGestureAction.{h,cpp}`から
    `libs/ui/actions/input/KisTouchGestureAction.{h,cpp}`。
  - `libs/input/ui/kis_change_frame_action.{h,cpp}`から
    `libs/ui/actions/input/kis_change_frame_action.{h,cpp}`。
  - `libs/input/ui/kis_gamma_exposure_action.{h,cpp}`から
    `libs/ui/actions/input/kis_gamma_exposure_action.{h,cpp}`。
  - `libs/input/ui/kis_pan_action.{h,cpp}`から`libs/ui/actions/input/kis_pan_action.{h,cpp}`。
  - `libs/input/ui/kis_rotate_canvas_action.{h,cpp}`から
    `libs/ui/actions/input/kis_rotate_canvas_action.{h,cpp}`。
  - `libs/input/ui/kis_select_layer_action.{h,cpp}`から
    `libs/ui/actions/input/kis_select_layer_action.{h,cpp}`。
  - `libs/input/ui/kis_zoom_action.{h,cpp}`から`libs/ui/actions/input/kis_zoom_action.{h,cpp}`。
  - `libs/input/ui/kis_zoom_and_rotate_action.{h,cpp}`から
    `libs/ui/actions/input/kis_zoom_and_rotate_action.{h,cpp}`。
  - `libs/input/ui/KisQtWidgetsTweaker.{h,cpp}`から
    `libs/ui/application/KisQtWidgetsTweaker.{h,cpp}`。
  - `libs/ui/events/KisPopupWidgetInterface.h`から`libs/input/ui/KisPopupWidgetInterface.h`。
  - `libs/ui/canvas/kis_tool_proxy.{h,cpp}`から`libs/input/ui/kis_tool_proxy.{h,cpp}`。
- `libs/input/ui/kis_input_profile_manager.cpp`の入力アクション生成を
  `libs/ui/actions/input/KisApplicationInputActions.{h,cpp}`へ配置し、`KisApplication`が生成結果を
  プロファイル管理器へ渡す。入力UIの設定参照は`libs/input/ui/kis_input_config.{h,cpp}`が
  既存の設定キーと既定値を扱い、入力管理器、プロファイル管理器、設定画面、タブレット診断へ
  同じ値を提供する。
- `KisInputManager`のキャンバス参照は`KoCanvasBase`と`KisToolCanvas`の公開面を利用し、入力UIから
  `KisCanvas2`への具体参照を解消した。`KisPaintingAssistantsDecoration`の活動ツール能力照会は、
  `libs/ui/canvas/kis_painting_assistants_decoration.cpp`から
  `libs/ui/canvas/kis_canvas2.{h,cpp}`の入力接続へ集約し、キャンバス表示から入力解釈への
  逆方向includeを0件に維持する。
- `kritainputui`は独立したライブラリーとして公開記号、版、導入規則を所有する。
  `KRITAINPUTUI_EXPORT`を使う13公開ヘッダーは、入力管理、入力アクション、プロファイル、設定表示、
  タブレット診断、プラットフォーム拡張の公開面を形成する。入力UI内部ヘッダーのパッケージ外参照は
  0件である。
- 公開記号を利用する`kritaui`、入力UI契約試験、アニメーションドッカー、記録ドッカー、
  iOSタッチUI、S Pen設定、Wayland、XCB、Karbonツールは`kritainputui`へ直接リンクする。
  依存方向は`kritaui`から`kritainputui`へ向かい、`kritainputui`から`kritaui`へのリンクは0件である。
- 5構成のCMake台帳はmacOS 678件、Linux 693件、iOS 611件、Android 617件、Windows 647件、
  共通595件、条件付き120件、構成差267件を記録する。`kritainputui`はmacOS、Linux、Android、
  Windowsで共有ライブラリー、iOSで製品へ静的統合するライブラリーである。26中核ターゲットと
  macOS 227件、Linux 233件、iOS 218件、Android 218件、Windows 235件の製品ターゲットは、
  各構成で循環0件である。
- AndroidのQt 5構築では、`libs/widgetutils/xmlgui/KisShortcutsEditor_p.cpp`の`QAction`利用、
  `libs/pigment/compositeops/KoAlphaDarkenParamsWrapper.cpp`と
  `libs/ui/opengl/KisScreenInformationAdapter.cpp`の`QDebug`利用、
  `libs/input/ui/kis_input_manager_p.cpp`の`KoPointerEvent`利用を各定義ヘッダーへ直接接続した。
  iOS構築では、`libs/ui/canvas/kis_painting_assistants_decoration.cpp`の
  キャンバス資源利用を`KoCanvasResourceProvider.h`と`KoCanvasResourcesIds.h`へ直接接続し、
  `libs/ui/opengl/kis_opengl_image_textures.cpp`と`libs/ui/canvas/kis_mirror_axis.cpp`の
  GLES拡張定数を既存の`KisOpenGLIOSCompat.h`へ接続した。
- `scripts/architecture/verify_cmake_graphs.py --remote-host nixos`は同一の
  `c6fdd7504c75447e85200607ce97fff72cd48d3c`を指す清浄作業ツリーでmacOS、iOS、
  Linux、Android、WindowsのCMake台帳と差分行列を生成し、決定的更新と循環検査に成功した。
- 最終ソースのmacOSとLinuxは`kritainputui`、`kritaui`、`krita`を共有ライブラリーと
  実行可能ファイルへリンクした。iOSは`libkritainputui.a`と`libkritaui.a`を
  `LibrePaint.app/LibrePaint`へ静的統合した。Androidは`libkritainputui_arm64-v8a.so`、
  `libkritaui_arm64-v8a.so`、`libkrita_arm64-v8a.so`、Windowsは`libkritainputui.dll`、
  `libkritaui.dll`、`krita.dll`のリンクに成功した。5構成の最終増分`krita`構築も成功した。
- `nix develop .#test --command ./scripts/verify`は最終ソースで高速検査106件、
  macOSの全製品と試験の構築、CTest 353件に成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`は全Nix出力の評価に成功した。
- clangd 21.1.8で変更した9翻訳単位と1公開ヘッダーを照合し、include-cleanerの
  未使用includeと直接include不足は0件である。

## R1-G6hアプリケーション調整境界で完了した作業

- 設定、スナップ方針、Androidファイル交換、アプリケーション調整、作業空間表示を、
  次の開始箇所から具体所有へ移した。
  - `libs/ui/application/kis_config.{h,cc}`から`libs/application/kis_config.{h,cc}`。
  - `libs/ui/canvas/kis_snap_config.{h,cpp}`から`libs/application/kis_snap_config.{h,cpp}`。
  - `libs/ui/application/KisAndroidFileProxy.{h,cpp}`から
    `libs/application/platform-adapters/KisAndroidFileProxy.{h,cpp}`。
  - `libs/ui/application`の残る18ファイルから`libs/application/ui/orchestration`。
  - `libs/ui/workspace`の43ファイルから`libs/application/ui/workspace`。
- `kritaapplication`は設定、スナップ方針、プラットフォームのファイル交換を所有し、
  `kritaapplicationui`はアプリケーション調整、作業空間表示、既存UI実装を所有する。
  24分類クラスは`kritaapplication`の1クラスと`kritaapplicationui`の23クラスへ接続し、
  公開面台帳は各所有者の公開ヘッダー3件と215件を記録する。
- `kritaapplication`と`kritaapplicationui`はmacOS、Linux、Android、Windowsで共有ライブラリー、
  iOSで静的ライブラリーとして構築する。`kritaapplicationui`から`kritaapplication`への
  直接依存を5構成で持ち、逆方向依存は0件である。`kritaui`はソースを持たない
  `INTERFACE`互換集約として`kritaapplicationui`へ転送する。
- 10責務、15公開接続面、27中核所有ターゲットを現在の責務地図へ接続した。
  27ターゲット間の88リンクは同一責務内16件と許可方向72件へ分類され、基準対象、
  許可方向外の責務対、未確定射影、直接include、内部ヘッダー参照は各0件である。
  移行段階は実施履歴を記録し、実装済み責務の依存順は現行の責務層と有向非巡回グラフを
  正本とする。
- 5構成のCMake台帳はmacOS 681件、Linux 696件、iOS 614件、Android 620件、Windows 650件、
  共通598件、条件付き120件、構成差269件を記録する。27中核ターゲットとmacOS 228件、
  Linux 234件、iOS 219件、Android 219件、Windows 236件の全製品構築ターゲットは、
  各構成で循環0件である。
- 固定Nix環境の`./scripts/verify-quick`は106件の運用試験、8更新器の決定性、公開面、
  責務地図、許可依存、再配置計画、ゼロ基準、CMake台帳、文書、リンク、D2生成物の検査に
  成功した。macOS、iOS、Linux、Android、WindowsのCMake台帳と差分行列は各構成の再生成と
  差分検査に成功し、`nix flake check --no-build --all-systems --no-eval-cache`は全Nix出力の
  評価に成功した。
- `nix develop .#test --command ./scripts/verify`は最終ソースで高速検査106件、macOSの
  全製品と試験の構築、CTest 354件に成功した。

## R1-G7移行台帳の退役で完了した作業

- 逆方向依存と構造依存の移行上限が0件へ到達したため、依存違反基準と構造依存基準を
  退役した。`check_package_dependencies.py`と`check_structural_dependencies.py`が現在の
  ソース、責務地図、許可依存方針、公開面、5構成グラフからゼロ状態を直接検査する。
- UI配置の開始パスと宛先パスを記録した4台帳を退役した。公開面検査は`libs/ui`直下の
  2ファイル、分類済み公開ヘッダー、責務別UIディレクトリーの翻訳単位とUI資産のCMake登録を
  現在のソースツリーから直接確認する。
- 文書境界評価は文書責務の実装完了後に退役し、現在の公開クラス所有はUIクラス責務台帳、
  安定した設計は文書パッケージ境界計画が所有する。
- 宣言、利用元、CMake所有を持たない`libs/ui/resources/kis_md5_generator.cpp`を削除し、
  UI配置に残っていた審査済み例外を完了した。
- パッケージ再配置計画は1互換経路、責務配置、移行段階、完了条件を保持し、完了済みの
  ゼロ基準への入力参照を除いた。
- `libs/ui/tool`の公開ヘッダーは`kritaapplicationui`所有のUI接続実装である。既存の公開大域
  C++識別子は確立済みのAPI・ABI名として維持し、新しいAPIを
  責務名前空間へ置く。再配置計画の一時互換経路は`kritaui` CMake集約の1件となった。
- `nix develop .#test --command ./scripts/verify-quick`は95件の運用試験、6更新器の決定性、
  公開面、責務地図、許可依存、直接依存、構造依存、再配置計画、統治、文書、リンク、
  D2生成物の検査に成功した。
- `nix develop .#test --command ./scripts/verify`は全製品と試験の構築、CTest 354件に
  成功した。

## R1-G7移行層の削除とR1完了同期で完了した作業

- `c5dce85b`から`c7f94355`までに、基盤、入出力、描画、UI、ツール、プラットフォームの
  製品と試験を責務別の実所有ターゲットへ直接接続した。`c26f2091`で`kritaui` CMake
  ターゲットを削除し、5構成のターゲット、依存、構築成果物を各0件にした。
- 汎用のグラデーション編集部品8ファイルを
  `libs/ui/widgets/gradient/{KisGradientColorEditor,KisGradientWidgetsUtils,KisStopGradientEditor,KisStopGradientSlider}.{cpp,h}`から
  `libs/widgets/`へ移した。塗りと線の設定部品6ファイルを
  `libs/ui/widgets/{KoFillConfigWidget,KoStrokeConfigWidget}.{cpp,h,ui}`から`libs/widgets/`へ、
  `libs/ui/forms/wdgstopgradienteditor.ui`を`libs/widgets/wdgstopgradienteditor.ui`へ移した。
- 10責務、27中核所有ターゲット、15所有ターゲットの公開境界を現在の責務地図、許可依存方針、
  公開面台帳へ固定した。macOS 681件、Linux 696件、iOS 614件、Android 620件、Windows
  650件のCMake台帳は旧ターゲットとその依存を含まず、中核ターゲットと全製品ターゲットの
  循環は各0件である。5構成の製品構築は各実所有ライブラリーと製品のリンクに成功した。
- 完了した再配置計画台帳、その専用検査器と単体試験を退役した。高速検査は責務地図、許可依存、
  公開面、直接依存、構造依存を現在のソースと5構成グラフから直接検査する。
- `nix develop .#test --command ./scripts/verify-quick`は88件の運用試験、6更新器の決定性、
  公開面、責務地図、許可依存、直接依存、構造依存、統治、文書、リンク、D2生成物の検査に
  成功した。文書検査はMarkdown、43リンク、3件のD2と生成済みSVGの一致を確認した。
  `nix develop .#test --command ./scripts/verify`は全製品と試験の構築、CTest 354件に成功し、
  `nix flake check --no-build --all-systems --no-eval-cache`は全Nix出力の評価に成功した。

## R1完了後の外部検査整理で完了した作業

- R1の移行状態を複製していた
  `docs/architecture/{allowed-package-dependencies,package-responsibilities}.json`、
  `docs/architecture/cmake-target-matrix.json`、
  `docs/architecture/cmake-targets-{macos,linux,ios,android,windows}.json`を、
  `docs/architecture/package-boundaries.json`と
  `scripts/architecture/check_package_boundaries.py`へ置き換えた。生成台帳の保守を廃止し、
  方針は10責務、27中核所有ターゲット、許可依存だけに縮小した。実依存と全製品循環は
  各プラットフォームのCMake構成直後にFile API応答から検査する。
- `docs/architecture/public-surface-inventory.json`、
  `docs/architecture/ui-class-responsibilities.json`、
  `docs/architecture/ui-tool-class-responsibilities.json`を、現在の製品ソースとCMake定義を読む
  `scripts/architecture/check_public_contracts.py`へ置き換えた。公開ヘッダーと172件の
  プラグイン登録について、生成した全件複製を持たずに同じ公開・登録契約を検査する。
- `docs/architecture/source-size-baseline.json`と行数上限検査を削除した。R1-G6hを削除条件と
  していた例外は完了済みであり、履歴上限は製品挙動、現在の構造、再現可能性を保護しない。
  `scripts/check_governance.py`はUTF-8、制御文字、双方向書式文字の検査だけを保持する。
- `scripts/architecture/{regenerate_cmake_graph,regenerate_cmake_graph_matrix,verify_cmake_graphs}.py`、
  `scripts/architecture/{update_allowed_package_dependencies,update_package_responsibility_map}.py`、
  `scripts/architecture/{update_plugin_inventory,update_public_header_inventory}.py`、
  `scripts/architecture/{update_ui_class_responsibilities,update_ui_tool_class_responsibilities}.py`、
  `scripts/architecture/{check_allowed_package_dependencies,check_package_dependencies}.py`、
  `scripts/architecture/{check_package_responsibility_map,check_public_surface_inventory}.py`、
  `scripts/architecture/check_structural_dependencies.py`を退役した。
  `scripts/architecture/extract_cmake_graph.py`はFile API応答の直接検査で再利用し、固定応答の
  抽出契約を保持した。
- 退役した検査器専用の8試験を削除し、境界方針、実グラフ、公開ヘッダー、プラグイン登録、
  テキスト診断を9件の小さいスクリプト試験へ置き換えた。文書検査内でも実行していた
  `scripts/docs/*.sh`の重複したShellCheck呼出しを`verify-quick`から除いた。
- `scripts/build-incremental`、iOS、Android、Windowsの増分構築入口へFile APIの問い合わせと
  実グラフ検査を接続した。`scripts/verify`は同じネイティブ構成入口を利用する。
- `nix develop .#test --command ./scripts/verify-quick`は、スクリプト試験33件、境界方針10責務、
  外部利用ヘッダー532件、プラグイン登録172件、テキスト、ShellCheck、Markdown、32リンク、
  D2生成物の検査に成功した。
- `nix develop .#test --command ./scripts/verify`は、macOSの実グラフ681ターゲット、全製品と
  試験の構築、CTest 354件に成功した。`./scripts/build-incremental ios configure`はiOSの
  実グラフ614ターゲットと既存の配布契約監査に成功した。
- `nix flake check --no-build --all-systems --no-eval-cache`は全Nix出力の評価に成功した。
  `nix build --no-link .#checks.aarch64-darwin.governance`は新規ファイルをGit入力へ含めた状態で
  軽量Nix検査の隔離構築に成功した。
- Linux、Android、Windowsの実構成はx86_64 Linuxホストへ同じ変更を配置した後に実行する。
  3入口のShellCheck、接続試験、Nix出力評価は成功している。

## R2-G1自由描画ストロークの最小契約で完了した作業

- 入力受信、ツール呼出し、ストローク実行、ブラシ画素生成、タイル更新と投影、キャンバス表示の
  6段階について、所有者、主要分岐、観測する状態、不変条件、既存検査を
  `docs/architecture/README.md`へ固定した。
- `libs/ui/tests/freehand_stroke_test.{cpp,h}`を起点として、通常のCTestで実行する
  `libs/ui/tests/FreehandStrokeContractTest.cpp`へ置き換えた。sRGB 8ビット、500×500画素、
  単一ペイントレイヤー、`autobrush_300px.kpp`、2入力点、作業スレッド1本を固定し、終了、
  取消し、アンドゥ、リドゥ、画像更新完了を同じ検査対象にした。
- `libs/ui/tests/data/freehand`の48基準画像から、同一画素だった終了投影1枚を
  `libs/ui/tests/data/freehand-contract/autobrush-finished-projection.png`へ移した。RGB完全一致、
  アルファ値±3を比較規則とし、独立実行5回が同じ結果になることと20回の連続実行を確認した。
- 無効化された旧検査だけが使用した残り47画像と、ハッチング、色混合、テクスチャー、LOD用の
  7プリセットを削除した。旧補助処理が生成していた現在無効な間接描画名、内部・外部投影の
  重複、取消し結果の画像保存を保守対象から外した。
- `clang-format --dry-run --Werror libs/ui/tests/FreehandStrokeContractTest.cpp`は整形差分0件で
  成功した。対象CTestの20回反復と、同検査、`kis_strokes_queue_test`、
  `kis_update_scheduler_test`、`kis_projection_test`、`kis_prescaled_projection_contract_test`の
  関連5件は成功した。
- `nix develop .#test --command ./scripts/verify-quick`はスクリプト試験33件、境界方針、
  公開契約、統治、Markdown 34リンク、D2生成物の検査に成功した。
  `nix develop .#test --command ./scripts/verify`はmacOSの実グラフ681ターゲット、全製品と
  試験の構築、CTest 355件に成功した。
- `nix develop .#test --command ./scripts/build-incremental ios configure`はiOSの資産、ライセンス、
  互換識別子、配布契約、CMake構成、実グラフ614ターゲットの境界検査に成功した。
- Linuxでの画素比較と、Qt生入力からツールまで、投影から実画面までの段階間契約は未検証である。
  R2-G3以降で共通入力データと対象プラットフォーム対応を定義して検証する。

## R2-G2対象試験の構築範囲最適化で完了した作業

- `scripts/run-test`は毎回CMakeプリセットを直接構成する経路から、構成指紋を保つ
  `build-incremental native build <target>`へ対象を渡す経路になった。`scripts/verify`も明示的な
  再構成と別の全体構築命令を同じ増分構築入口へ統合した。スクリプト契約は単一試験と全体検証の
  両方で重複する構成・構築命令を発行しないことを固定する。
- `FreehandStrokeContractTest`は`kritaapplicationui`と`KisViewManager`を使用する共通補助処理から
  独立し、`kritapainting`、`kritalibbrush`、`kritatestsdk`へ直接リンクする。試験内で固定プリセット、
  前景色、背景色を画像・描画資源へ設定し、既存の画素、取消し、アンドゥ、リドゥ契約を維持する。
- 代表4試験の直接所有先は、自由描画が`kritapainting`と`kritalibbrush`、入力照合が`kritainput`、
  ツール呼出しが`kritatools`、投影転送が`kritacanvas`である。後者3件は動的プラグイン、QML、外部
  試験データを使用しない。自由描画は`autobrush_300px.kpp`、維持基準画像、実行時にPaintOpを
  登録する`kritadefaultpaintops`を使用する。
- `kritadefaultpaintops`を`FreehandStrokeContractTest`のCMake依存として明示した。モジュールを
  退避した状態の対象構築は同モジュールのリンク1工程だけを再実行し、続くCTestに成功した。
  アプリケーション実行形式、全プラグイン集合、`all`は対象から到達しない。
- 空のmacOS構築木に対するNinja全コマンド数は`all`が5,489、自由描画が1,816、入力照合が1,037、
  ツール呼出しが1,066、投影転送が1,023である。4対象の和集合は1,829工程であり、自由描画が必要な
  `kritadefaultpaintops`と`kritalibpaintop`の現行閉包が支配する。この閉包をさらに縮小するには
  PaintOp実行処理と設定UIの製品分割が必要なため、依存数だけを減らす変更は行わない。
- 永続構築木の`.ninja_deps`破損により、変更のない対象が1,011工程を繰り返す状態を診断した。
  依存記録の退避と再圧縮後は、最初の対象構築が不足記録を再生成し、次の同一対象構築が
  `ninja: no work to do.`、対象CTest込み2.77秒で成功した。これはCMake依存範囲や`ccache`の
  無効化ではなく、Ninja依存記録の局所的な破損である。
- `nix develop .#test --command ./scripts/verify-quick`は35件の運用試験、責務・公開契約、文書、
  リンク、D2再生成を含む高速検査に成功した。代表4試験と自由描画契約の20回反復もmacOSで
  成功した。
- `nix develop .#test --command ./scripts/verify`は、依存記録修復後に未採取だった2,626工程を構築し、
  681ターゲットのパッケージ境界検査と登録済みCTest 355件すべてに成功した。全体検証の実時間は
  387.43秒であり、不足する試験成果物の調査には使用していない。

## R2-G3マウス入力からツール呼出しまでで完了した作業

- `TestInputShortcutMatcher`はQtのマウス列を入力アクションの開始、入力、終了へ変換する順序を固定し、
  `TestToolCoreContract`は既に生成された`KoPointerEvent`から描画位置、時刻、表示設定を含む
  `KisPaintInformation`への変換を固定する。`KisInputManagerTest`は入力プロファイルと照合器を扱う。
- 両契約間の`libs/input/ui/kis_tool_proxy.cpp`は、ウィジェット座標を文書座標へ変換し、Qtのマウス、
  タブレット、タッチ事象を`KoPointerEvent`として現在ツールへ渡す。このうち固定値が少ない
  マウスの押下、移動、解放を最初の段階間契約とする。
- `libs/input/ui/tests/KisToolProxyContractTest.cpp`を`kritainputui`と`kritatestsdk`だけへ直接リンクする
  独立対象として登録した。固定した具体的キャンバス、座標変換器、画面コントローラー、記録用ツールを
  試験内に構成し、製品実装と公開接続面は変更していない。
- ウィジェット位置`(100, 120)`、`(140, 180)`、`(160, 210)`が、固定変換器の中央補正と
  100 pixels/pointを経て文書位置`(0.5, 0.7)`、`(0.9, 1.3)`、`(1.1, 1.6)`になる現行挙動を固定した。
  同じ入力列でローカル位置、画面位置、左右ボタン状態、Shift修飾、時刻10、20、30、筆圧1、
  傾き0、回転0、マウス入力種別を検査する。
- 押下、移動、解放が主操作の開始、継続、終了へ各1回渡り、操作有効化通知がtrue、falseの順で
  発生する。開始事象を記録用ツールが無視した場合は`forwardEvent()`がfalseを返す。
- 最初の実行可能な契約は、文書位置の期待`(1.2, 1.5)`に対して現行変換値`(0.5, 0.7)`を報告した。
  座標変換器のキャンバス中央補正を含む現行値として分類し、数値を維持基準へ固定した。
- `nix develop .#test --command ./scripts/run-test KisToolProxyContractTest`、
  `TestInputShortcutMatcher`、`KisInputManagerTest`、`TestToolCoreContract`はmacOSで成功した。
  `ctest --preset tdd-macos --tests-regex '^libs-input-ui-KisToolProxyContractTest$' --repeat until-fail:20`
  と`nix develop .#test --command ./scripts/verify-quick`も成功した。
- `nix develop .#test --command ./scripts/verify`は356件中355件に成功した。変更範囲外の既知の
  `KisSafeDocumentLoaderTest`はファイル監視通知が実測1件・0件、期待2件・1件として失敗したが、
  `nix develop .#test --command ./scripts/run-test KisSafeDocumentLoaderTest`の単独再実行は成功した。
  R2-G3対象は統合実行でも成功し、入力、ツール、文書監視、文書読込の製品実装に変更はない。

## R2-G4投影更新キューの順序と圧縮契約で完了した作業

- `FreehandStrokeContractTest`は完了したストロークの投影画素、
  `kis_prescaled_projection_contract_test`はdirty image領域からdirty viewportと表示用画像への変換、
  `kis_update_scheduler_test`は画像側の更新ジョブ実行を固定する。これらの間で`KisCanvas2`が使う
  `KisCanvasUpdatesCompressor`の保留列と起動条件は未検査である。
- 現行圧縮は、新しいdirty領域が包含する、同じ詳細度の圧縮可能な旧更新だけを除去する。
  部分重複、異なる詳細度、`KisMarkerUpdateInfo`は維持し、新しい更新は常に末尾へ置く。
  この順序は、新しい投影内容の後に古いタイルを転送しないための維持契約とする。
- 空のdirty領域、包含、部分重複、詳細度0と1、開始・終了マーカーを固定した具体的な
  `KisUpdateInfo`として投入し、空から非空になる場合だけtrueを返す起動条件、排出順序、排出後の
  空状態を観測する。
- 更新圧縮器は`libs/ui/canvas`内部の非公開クラスであり、製品の公開記号を増やさない。独立試験へ
  `kis_canvas_updates_compressor.cpp`を直接コンパイルし、`kritacanvas`と試験基盤だけへリンクする。
  実画面、アプリケーションUI、タイマー、OpenGL転送は後続の観測境界として分離する。
- `libs/ui/tests/KisCanvasUpdatesCompressorContractTest.cpp`を追加し、非公開の製品実装源
  `libs/ui/canvas/kis_canvas_updates_compressor.cpp`とともに独立対象へコンパイルした。直接リンクは
  `kritacanvas`と`kritatestsdk`だけで、製品実装、公開記号、動的成果物は変更していない。
- 空のdirty領域を最初に投入しても起動要求を返さず、最初の有効更新だけがtrue、後続の保留更新が
  false、全件排出後の最初の更新が再びtrueになることを固定した。排出は2件を投入順に移し、
  続く排出を空にする。
- 詳細度0の`(4, 4, 4, 4)`を後続の`(2, 2, 10, 10)`で置換する一方、部分重複する
  `(10, 4, 4, 4)`、開始マーカー、同じ領域の詳細度1を維持し、置換更新を列の末尾へ置く順序を
  固定した。
- 最初の対象実行は、契約本体が未実装である明示的診断により失敗した。契約実装後の
  `nix develop .#test --command ./scripts/run-test KisCanvasUpdatesCompressorContractTest`、
  `kis_prescaled_projection_contract_test`、`kis_update_scheduler_test`はmacOSで成功した。
  新契約の20回反復と`nix develop .#test --command ./scripts/verify-quick`も成功した。

## R2-G5 QPainter表示用投影の部分転送契約で完了した作業

- `kis_prescaled_projection_contract_test`はdirty image領域から表示用投影画像とdirty viewportを生成し、
  R2-G4は投影更新の圧縮順序を固定する。`KisQPainterCanvas::drawImage()`は、その後にdirty widget領域を
  viewportへ戻し、表示用投影画像の同じ領域をQPainterへ転送するが、この境界は独立契約を持たない。
- `libs/ui/canvas/kis_qpainter_canvas.cpp`の`KisQPainterCanvas::drawImage()`にあった座標変換、平滑化、
  SourceOver合成、部分画像転送を、`libs/ui/canvas/kis_qpainter_canvas_draw_image.h`の内部インライン
  関数へ移した。元の関数は現在の座標変換器、表示用投影画像、更新矩形を同じ処理へ渡す唯一の
  製品利用元であり、公開記号と実装選択機構は追加していない。
- `libs/ui/tests/KisQPainterCanvasDrawImageContractTest.cpp`は、位置別の不透明色を持つ8×8投影画像、
  固定背景色の8×8描画先、恒等変換、更新矩形`(2, 3, 3, 2)`を使用する。矩形内6画素は対応する
  投影画素へ完全一致し、残る58画素は描画前の背景色を維持する。
- 最初の対象構築は`kis_qpainter_canvas_draw_image.h`が存在しない診断で失敗した。抽出後の
  `nix develop .#test --command ./scripts/run-test KisQPainterCanvasDrawImageContractTest`は成功し、
  `./scripts/build-incremental native build kritaapplicationui`は変更した製品呼出元のコンパイルとリンクに
  成功した。
- `kis_prescaled_projection_contract_test`と`kis_coordinates_converter_test`、新契約の20回反復、
  `nix develop .#test --command ./scripts/verify-quick`はmacOSで成功した。

## R2-G6 QPainter表示用投影の水平鏡像分類契約で完了した作業

- `kis_coordinates_converter_test`は水平鏡像後の矩形と座標往復、R2-G5は恒等変換での部分画素転送を
  個別に固定するが、鏡像したviewport変換を通る実際の投映画素順は未検査だった。
- `libs/ui/tests/KisQPainterCanvasDrawImageContractTest.cpp`へ8×8画像と画面、1:1倍率、原点一致、
  画像中心を静止点とする水平鏡像を追加した。viewport端点は`(0, 0)`から`(8, 0)`、`(8, 8)`から
  `(0, 8)`へ移り、座標変換自体は完全な左右反転になる。
- x位置を赤、y位置を緑、両方を青の変化へ符号化した不透明投影を全画面転送した。最初の完全一致は
  描画先`(0, 0)`で期待した投影`(7, 0)`に対し、投影`(6, 0)`相当を得て失敗した。行方向の実測でも
  先頭画素の複製と終端画素の欠落を確認し、QPainter平滑化を伴う変換描画の一画素内側採取を
  修正対象の既知不具合へ分類した。
- 理想の完全一致は代表画素の期待失敗として明示した。全64画素では不透明度を完全一致で検査し、
  理想の左右反転に対する位置符号の差を水平・垂直各1画素、合成値2以内へ制限した。この上限は
  現行差の悪化を検出し、完全一致への修正も受け入れる。
- 変更先は既存の独立試験だけであり、R2-G5の内部描画関数、製品コード、公開面、動的成果物は
  変更していない。最初の対象構築は`KoViewTransformStillPoint`の定義不足で失敗し、必要な所有
  ヘッダーを追加した後に画素診断へ到達した。
- `nix develop .#test --command ./scripts/run-test KisQPainterCanvasDrawImageContractTest`、
  `kis_coordinates_converter_test`、新契約の20回反復、`nix develop .#test --command ./scripts/verify-quick`は
  macOSで成功した。

## R2-G7 2倍拡大時の表示用投影領域契約で完了した作業

- `KisCoordinatesConverter::imageToViewportTransform()`は画像解像度と拡大率を含む一方、
  `viewportToWidgetTransform()`は画面内の移動、回転、鏡像を扱う。R2-G5の最終QPainter転送へ倍率を
  直接追加すると、拡大済み表示用投影を二重に拡大する。
- `libs/canvas/tests/kis_prescaled_projection_contract_test.cpp`の既存fixtureを使い、16×16画像と画面、
  2倍拡大、原点一致、dirty image領域`(2, 3, 2, 2)`を固定する。期待するdirty viewportは
  `(4, 6, 4, 4)`であり、表示用画像自体は画面と同じ16×16を維持する計画だった。
- 最初の対象実装は、直接変換の期待`(4, 6, 4, 4)`に対して実更新領域`(2, 4, 8, 8)`を報告した。
  `KisPrescaledProjection::fillInUpdateInformation()`は補間境界の劣化を避けるため、直接変換後に
  画像側の各辺を1画素広げ、2倍変換後の各辺を2画素広げる。これは維持する安全域として分類した。
- `libs/canvas/tests/kis_prescaled_projection_contract_test.cpp`へ2倍拡大契約を追加した。画像dirty領域の
  直接変換`(4, 6, 4, 4)`、安全域を含む実更新`(2, 4, 8, 8)`、16×16表示用画像、更新領域の緑、
  領域外の直前の赤を固定した。
- 変更先は既存の独立試験だけであり、製品コード、公開面、最終QPainter転送、回転、鏡像、
  非整数倍率、基準画像ファイルは変更していない。
- `nix develop .#test --command ./scripts/run-test kis_prescaled_projection_contract_test`、
  `kis_coordinates_converter_test`、新契約の20回反復、`nix develop .#test --command ./scripts/verify-quick`は
  macOSで成功した。

## R2-G8 QPainter表示用投影の90度回転分類契約で完了した作業

- R2-G6は水平鏡像したQPainter転送で理想値より一画素内側を採取する既知不具合を記録した。
  `kis_coordinates_converter_test`は90度回転時の画素位置合わせを検査するが、表示用投影を
  `viewportToWidgetTransform()`で描く全画素の対応は未検査である。
- R2-G6と同じ8×8位置符号画像、1:1倍率、原点一致、画像中心を静止点とする90度回転を使う。
  viewport四隅と、描画先`(x, y)`に対する投影`(y, 7 - x)`の完全一致を先に実行する。
- 完全一致しない場合は理想値との差を全64画素で実測し、完全一致への修正を受け入れる上限として
  既知不具合を記録する。製品コード、公開面、任意角回転、拡大縮小、OpenGL経路は変更しない。
- `libs/ui/tests/KisQPainterCanvasDrawImageContractTest.cpp`へ画像中心を静止点とする90度回転を追加した。
  viewport四隅は`(0, 0)`から`(8, 0)`、`(8, 0)`から`(8, 8)`、`(8, 8)`から`(0, 8)`、
  `(0, 8)`から`(0, 0)`へ完全一致し、座標変換自体は理想の時計回り回転になる。
- 最初の画素比較は、描画先`(0, 0)`で期待した投影`(0, 7)`に対し、投影`(0, 6)`相当を得て失敗した。
  水平鏡像と同じQPainter平滑化付き変換描画の一画素内側採取であり、修正対象の既知不具合へ分類した。
- 理想の完全一致は代表画素の条件付き期待失敗として明示した。全64画素では不透明度を完全一致で
  検査し、理想の回転に対する位置符号の差を元画像の水平・垂直各1画素、合成値2以内へ制限した。
  現行差の悪化を検出し、完全一致への修正も受け入れる。
- 変更先は既存の独立試験だけであり、製品コード、公開面、任意角回転、拡大縮小、OpenGL経路は
  変更していない。
- `nix develop .#test --command ./scripts/run-test KisQPainterCanvasDrawImageContractTest`、
  `kis_coordinates_converter_test`、新契約の20回反復、`nix develop .#test --command ./scripts/verify-quick`は
  macOSで成功した。

## R2-G9実装前の構築範囲監査で完了した作業

- `nix develop .#test --command ./scripts/build-incremental native plan`を
  `KisQPainterCanvasDrawImageContractTest`対象へ実行し、変更のない対象本体に構築工程がないことを
  確認した。Ninjaのglob再検査と乾式実行上のCMake再生成表示は対象コンパイルではない。
- `libs/ui/tests/CMakeLists.txt`とCMake File API応答で、対象の直接依存が具体的所有者`kritacanvas`と、
  Qt Testおよび試験用定義だけを供給する接続面`kritatestsdk`に限られることを確認した。アプリケーション
  実行形式、アプリケーションUI、プラグイン集合、`all`への直接依存はない。
- 空の構築木に対するNinjaコマンド閉包は1,023工程である。支配する`kritacanvas`は座標変換器、画像、
  QPainter表示用投影転送が属する具体的所有者であり、R2-G9を別対象へ分けても閉包は縮小しない。
  既存の単一試験ソースへ契約を追加するため、変更後の増分構築は自動MOC、同ソース、リンクに局限できる。
- 実装前に増分計画、直接依存、新規・拡張対象の空構築閉包を監査し、過大な範囲を挙動変更より先に
  修正する順序を`AGENTS.md`へ追加した。`docs/architecture/DEVELOPMENT.md`へ監査コマンド、判定条件、
  縮小できない具体的所有者閉包の記録方法を追加した。

## R2-G9 QPainter表示用投影の任意角回転比較契約で完了した作業

- 最初の8×8投影バッファーは回転境界の外側に補間用画素を持たず、透明193、不透明63、半透明0を
  報告した。製品の表示用投影は回転後の画面範囲を持つため、この入力は比較対象として不適切と分類した。
- 16×16画像と画面を17.3度回転すると、画面全体を逆変換したviewportは20.0342×20.0342で、
  表示用投影バッファーは21×21になる。viewport中心`(10.0171, 10.0171)`に最も近い整数位置
  `(6, 6)`から8×8の位置符号領域を置いた。
- xを16、yを16、両者を青へ8と4の間隔で符号化した。不透明領域の変換済み包含矩形は11×11、
  非透明画素領域はその内部の10×10で、実測は透明176、半透明28、完全不透明52画素だった。
  比較契約は透明168〜184、半透明20〜36、完全不透明48〜64を受け入れる。
- 半透明境界の不透明度は15〜194だった。比較契約は最小値1〜32、最大値160〜254を受け入れ、
  透明、平滑化境界、完全不透明な内部を区別する。
- 完全不透明な52画素で、描画先中心をviewportへ逆変換した位置と赤・緑から復元した元画像位置の
  最大差はx・yとも0.0312未満、青の線形符号との差は1.038未満だった。比較契約はx・yを0.05以下、
  青を1.1以下へ制限し、誤った画素採取を検出しながら量子化差を許容する。
- 実装後の対象指定構築はNinjaのglob再検査に続き、自動MOC、変更した
  `libs/ui/tests/KisQPainterCanvasDrawImageContractTest.cpp`、試験実行形式のリンクだけを実行した。
  製品コード、CMake、公開面、基準画像、他の角度、拡大縮小、OpenGL経路は変更していない。
- `nix develop .#test --command ./scripts/run-test KisQPainterCanvasDrawImageContractTest`、
  `kis_coordinates_converter_test`、新契約の20回反復、実装後の対象計画、
  `nix develop .#test --command ./scripts/verify-quick`はmacOSで成功した。

## R2-G10実装前の構築範囲監査と契約選定

- `KisToolProxyContractTest`の変更前増分計画は対象本体に構築工程を持たず、Ninjaのglob再検査と
  乾式実行上のCMake再生成表示だけだった。
- CMake File API応答と`libs/input/ui/tests/CMakeLists.txt`で、直接依存が入力UIの具体的所有者
  `kritainputui`と、Qt Testおよび試験用定義を供給する`kritatestsdk`だけであることを確認した。
  アプリケーション実行形式、アプリケーションUI、プラグイン集合、`all`への直接依存はない。
- 空のmacOS構築木に対するNinjaコマンド閉包は1,172工程である。`KisToolProxy`、入力キャンバス境界、
  `KoPointerEvent`への変換を実製品のまま検査するため`kritainputui`は不可欠であり、試験対象を分けても
  閉包は縮小しない。既存対象へ試験を追加するため、先行する構築構造の変更は不要と判断した。
- `TestInputEventSuppressor`はタブレット事象に伴う合成入力の抑止、`TestToolCoreContract`は既に生成済みの
  ポインター事象から描画情報への変換を固定する。両者の間にあるQtタブレット事象から現在ツールまでの
  筆圧、傾き、接線方向筆圧、回転、Z位置、時刻、入力種別の転送は未検査だった。
- R2-G10は固定したスタイラス装置の`TabletPress`、`TabletMove`、`TabletRelease`を既存の
  `KisToolProxyContractTest`へ追加し、製品コード、公開面、CMake依存、入力プロファイル、入力抑止、
  OS装置層を変更しない範囲に限定する。

## R2-G10タブレット入力からツール呼出しまでの値転送契約で完了した作業

- `libs/input/ui/tests/KisToolProxyContractTest.cpp`の記録値へ接線方向筆圧とZ位置を追加し、既存の
  マウス契約では両値が0になることも明示した。製品コード、公開面、CMake依存は変更していない。
- 位置、筆圧、回転、両傾き、接線方向筆圧、Z位置に対応する能力を持つ固定スタイラス装置を試験内に
  構成し、`TabletPress`、`TabletMove`、`TabletRelease`の3事象を再生した。
- R2-G3と同じウィジェット位置`(100, 120)`、`(140, 180)`、`(160, 210)`が文書位置
  `(0.5, 0.7)`、`(0.9, 1.3)`、`(1.1, 1.6)`になること、ローカル・画面位置、ボタン状態、
  Shift修飾、時刻110、120、130が保持されることを固定した。
- 筆圧0.25、0.5、0.0、x傾き-15、-5、12、y傾き20、10、-8、回転30、45、60、Z位置3、4、5を
  固定した。Qt事象の接線方向筆圧-0.6、0.0、0.8は、`KoPointerEvent`の現行規則で0.2、0.5、0.9へ
  正規化される。
- 3事象はタブレット入力かつ非タッチ入力として現在ツールの主操作開始、継続、終了へ各1回渡り、
  操作有効化通知はtrue、falseの順に発生する。
- 最初の対象限定実行はR2-G10の未実装診断だけで失敗した。実装時の対象構築はNinjaのglob再検査に
  続き、自動MOC、変更した試験ソース、試験実行形式のリンクだけを実行した。実装後の対象計画は
  対象コンパイルを含まない。
- `nix develop .#test --command ./scripts/run-test KisToolProxyContractTest`、
  `TestInputEventSuppressor`、`TestToolCoreContract`、対象契約の20回反復、
  `nix develop .#test --command ./scripts/verify-quick`はmacOSで成功した。

## R2-G11単点タッチ入力からツール呼出しまでの値転送契約で完了した作業

- R2-G10直後の`KisToolProxyContractTest`は、変更のない対象本体に構築工程がなく、直接製品依存が
  `kritainputui`と`kritatestsdk`だけ、空のmacOS構築木に対する閉包が1,172工程のままだった。
- Qt 6の公開`QEventPoint`構築子では任意の筆圧と回転を設定できないため、試験対象だけへ既に構成時に
  取得済みのQtGui非公開ヘッダー検索路を追加した。タッチ筆圧設定を試験用設定領域へ固定するため
  `KF6::ConfigCore`を直接リンクした。再構成後も空構築閉包は1,172工程で、製品依存は増えていない。
- `libs/input/ui/tests/KisToolProxyContractTest.cpp`に固定タッチ画面装置と、単点の`TouchBegin`、
  `TouchUpdate`、`TouchEnd`を追加した。試験開始時にQtの試験用設定領域でタッチ筆圧を有効にし、
  終了時に設定項目の元の有無と値を復元する。
- R2-G3と同じウィジェット位置`(100, 120)`、`(140, 180)`、`(160, 210)`が文書位置
  `(0.5, 0.7)`、`(0.9, 1.3)`、`(1.1, 1.6)`になること、ローカル・画面位置、Shift修飾、
  時刻210、220、230が保持されることを固定した。
- 筆圧0.2、0.6、0.0と回転5、25、45が保持される。3事象は左ボタン押下状態、非タブレットかつ
  タッチ入力として現在ツールの主操作開始、継続、終了へ各1回渡り、傾き、接線方向筆圧、Z位置は0、
  操作有効化通知はtrue、falseの順になる。
- 最初の対象限定実行はR2-G11の未実装診断だけで失敗した。明示的なCMake再構成直後の対象構築では
  生成された版情報の再構築も一度発生したが、契約実装時の対象構築は自動MOC、変更した試験ソース、
  試験実行形式のリンクだけを実行し、その後の隣接対象構築は作業なしだった。
- `nix develop .#test --command ./scripts/run-test KisToolProxyContractTest`、
  `TestInputEventSuppressor`、`TestToolCoreContract`、対象契約の20回反復、実装後の対象計画、
  `nix develop .#test --command ./scripts/verify-quick`はmacOSで成功した。

## R2-G12a実装前の構築範囲監査で完了した作業

- `KisInputManagerTest`と`TestInputEventSuppressor`は変更のない対象本体に構築工程がない。空のmacOS
  構築木に対するNinjaコマンド閉包は前者が1,665工程、後者が1,037工程である。
- `KisInputManagerTest`は`kritainputui`、`kritaapplicationui`、`kritatestsdk`を直接リンクし、
  `libs/input/ui/tests/kis_input_manager_test.cpp`と`sdk/tests/testutil.cpp`を構築する。
- 試験実装は入力プロファイル保存、ショートカット照合、入力操作、増分平均だけを扱い、
  アプリケーションUIの記号と`sdk/tests/testutil.cpp`が提供する画像・ノード試験補助を使用していない。
  `kritaapplicationui`と`testutil.cpp`は責務外の閉包を加える不要な構成と判断した。
- 変更前の`nix develop .#test --command ./scripts/run-test KisInputManagerTest`はmacOSで成功した。

## R2-G12a入力管理器試験の構築範囲縮小で完了した作業

- `libs/input/ui/tests/CMakeLists.txt`の`KisInputManagerTest`登録から、未使用の
  `sdk/tests/testutil.cpp`と`kritaapplicationui`直接依存を除いた。移動先や代替依存はなく、対象ソースは
  `libs/input/ui/tests/kis_input_manager_test.cpp`だけ、直接依存は`kritainputui`と`kritatestsdk`だけである。
- CMake File API応答でアプリケーションUI直接依存と試験補助ソースが消えたことを確認した。
  空のmacOS構築木に対するNinjaコマンド閉包は1,665工程から1,172工程へ493工程、29.6%縮小した。
- 残る閉包は入力管理器、入力プロファイル、抽象入力操作を所有する`kritainputui`が支配する。
  R2-G12bで同じ実製品境界を検査するため、この具体的所有者は分離しない。
- 明示的なCMake再構成直後の対象構築では生成された版情報、自動MOC、試験本体、リンクを再構築した。
  続く対象計画は試験本体の構築工程を持たず、兄弟の`KisToolProxyContractTest`も製品再構築なしで成功した。
- 変更前後の`nix develop .#test --command ./scripts/run-test KisInputManagerTest`、変更後の20回反復、
  `KisToolProxyContractTest`、`nix develop .#test --command ./scripts/verify-quick`はmacOSで成功した。
  製品コード、公開面、製品依存、試験挙動は変更していない。

## R2-G12b Qt合成マウス事象の正規化契約で完了した作業

- R2-G12a後の`KisInputManagerTest`は、直接依存が`kritainputui`と`kritatestsdk`、空のmacOS構築木に
  対する閉包が1,172工程である。当初の移動候補`kis_input_manager_p.h`は入力管理器の3実装単位が使うため、
  正規化専用の内部ヘッダーを分け、変更時の利用元を1製品実装と試験だけに限定した。
- `normalizedButton()`と`normalizedSuppressionEvent()`を
  `libs/input/ui/kis_input_manager_p.cpp`の無名名前空間から
  新規`libs/input/ui/kis_input_event_normalizer_p.h`の`KisInputManagerDetail`内部名前空間へ移した。
  関数本体と入力管理器の呼出し順は変えず、製品と試験が同じ正規化処理を使う。
- `libs/input/ui/tests/kis_input_manager_test.cpp`へ実Qtマウス事象の契約を追加した。移動、押下、解放、
  二重押下が対応する抑止事象となり、左、無、右・中央ボタンが`Left`、`None`、`Other`へ一致する。
- `MouseEventNotSynthesized`は非合成、システム、Qt、アプリケーション由来の3事象源は合成となる。
  マウス遮断中は4事象種別すべて、合成抑止を支援する方針では合成事象だけが`MouseEvent`理由で
  抑止され、その他のQt事象は`Other`として維持される。
- 最初の対象限定構築は、契約が参照した`KisInputManagerDetail::normalizedSuppressionEvent`の未定義を
  診断した。専用内部ヘッダーだけを変更した後の対象構築は`kis_input_manager_p.cpp`、`kritainputui`、
  変更した試験とリンクだけに局限され、`kis_input_manager.cpp`と`kis_input_manager_canvas.cpp`は
  再構築しなかった。その後の対象計画は試験本体の構築工程を持たない。
- macOSの製品設定は合成元だけによる抑止を支援しないが、Qt事象から合成フラグへの正規化自体は共通で
  ある。OSが生成する実際の事象順序と支援有無はプラットフォーム実機の後続段階へ残した。
- `nix develop .#test --command ./scripts/run-test KisInputManagerTest`、`TestInputEventSuppressor`、
  新契約の20回反復、実装後の対象計画、`nix develop .#test --command ./scripts/verify-quick`はmacOSで
  成功した。既存のQt非推奨警告はR6の言語・Qt移行範囲に残し、新規警告はない。

## R2-G13a PaintOp実行対象の構築範囲分離で完了した作業

- 変更前の`FreehandStrokeContractTest`は、直接依存が`kritapainting`、`kritalibbrush`、
  `kritatestsdk`、順序依存が`kritadefaultpaintops`で、空のmacOS構築木に対するNinjaコマンド閉包が
  1,816工程だった。閉包は設定画面を含む`kritalibpaintop`と、画素ブラシ・複製ブラシ・各設定画面を
  含む`kritadefaultpaintops_static`に支配されていた。
- `plugins/paintops/libpaintop/CMakeLists.txt`で、PaintOp実行に必要な45実装ファイルの構築所有を
  `kritalibpaintop`から`kritapaintopruntime`へ分けた。既存共有ライブラリーは同じオブジェクトを
  再集約するため、公開名と公開処理を維持する。
- `plugins/paintops/defaultpaintops/CMakeLists.txt`で、`brush/kis_brushop.cpp`、
  `brush/KisBrushOpResources.cpp`、`brush/KisBrushOpSettings.cpp`、`brush/KisDabRenderingQueue.cpp`、
  `brush/KisDabRenderingQueueCache.cpp`、`brush/KisDabRenderingJob.cpp`、
  `brush/KisDabRenderingExecutor.cpp`の構築所有を`kritadefaultpaintops_static`から`kritapixelbrush`へ
  分けた。既存静的ライブラリーと製品モジュールはこの対象を再集約し、設定画面、複製ブラシ、
  PaintOp識別子と登録経路を維持する。
- 画面を所有せずPaintOp設定値と資源の読書き接続を担う
  `libs/tools/ui/KisPaintopPropertiesBase.{h,cpp}`を
  `libs/image/brushengine/KisPaintopPropertiesBase.{h,cpp}`へ移した。移動先の`kritaimage`が記号を
  公開し、旧ファイルと転送ヘッダーは残していない。分離で露出した製品実装と既存試験の間接見出し
  依存は、それぞれが使用する型の直接見出しへ置き換えた。
- `libs/ui/tests/FreehandStrokeContractTest.cpp`は、実製品の`KisBrushOp`と`KisBrushOpSettings`を生成する
  具体的ファクトリーを試験内で登録する。アプリケーション生成前に空のプラグイン探索先を固定し、
  対象から`kritadefaultpaintops`への順序依存と動的モジュール読込みを除いた。最初の実行は探索先を
  試験初期化後に変更していたため既登録PaintOpを診断し、初期化順を修正した後に既存画素、取消し、
  アンドゥ、リドゥ契約が成功した。
- 変更後の直接依存は`kritapixelbrush`、`kritapainting`、`kritalibbrush`、`kritatestsdk`である。
  `kritapixelbrush`は`kritapaintopruntime`と`kritapainting`、`kritapaintopruntime`は
  `kritalibbrush`と`kritapainting`だけへ直接依存する。空構築閉包は1,816工程から1,107工程へ
  709工程、39.0%縮小した。変更なし計画はglob再検査とCMake再評価だけを示し、対象コンパイルを
  含まない。
- `nix develop .#test --command ./scripts/run-test FreehandStrokeContractTest`、
  `KisDabRenderingQueueTest`、`KisCurveOptionDataTest`、`KisCurveOptionModelTest`、対象指定の
  `kritalibpaintop`、`kritadefaultpaintops`、`kritatoolsui`構築、
  `nix develop .#test --command ./scripts/verify-quick`はmacOSで成功した。
  `FreehandStrokeContractTest.cpp`の書式検査も成功した。既存ソースにはQt非推奨警告と既存の
  全体書式差分が残るが、この変更による新規警告はない。Linuxと全ネイティブ検証は実行していない。

## R2-G13b 既定自由描画の決定的画素契約で完了した作業

- 実装前の`FreehandStrokeContractTest`は変更なし計画に対象コンパイルがなく、直接依存が
  `kritapixelbrush`、`kritapainting`、`kritalibbrush`、`kritatestsdk`、空のmacOS構築木に対する
  Ninjaコマンド閉包がR2-G13aと同じ1,107工程だった。設定UI、動的PaintOpモジュール、
  アプリケーション実行形式は到達しないため、追加の構造分離は行っていない。
- 固定プリセットのPNGメタデータを確認し、`paintbrush`、直径300、比率1、水平・垂直フェード0.25、
  2突起、間隔0.1、角度0の円形自動ブラシであることを試験へ固定した。不透明度と寸法は筆圧センサーを
  使用し、回転、散布、テクスチャは無効である。Fuzzyセンサー、散布、ランダムテクスチャを使わないため、
  この契約ではストローク乱数源が画素結果へ寄与しない。乱数注入用の製品APIは追加していない。
- `KisPaintInformation`は位置`(200, 200)`と`(300, 300)`、筆圧1、x・y傾き0、回転0、接線方向筆圧0、
  遠近1、時刻0、速度0、キャンバス回転0、非ミラー、傾き方向補正0、詳細度0を明示して生成する。
  描画資源は前景黒、背景白、不透明度1、非ミラー、無選択を明示し、既存のsRGB 8ビット、500×500画素、
  単一ペイントレイヤー、作業スレッド1本と合わせて試験する。
- 最初の対象実行では、維持値を未設定にした正確な描画領域の比較だけが失敗し、実結果
  `QRect(50, 50, 385, 385)`を診断した。この値をレイヤーと投影の維持契約へ追加した。
  プリセットと入力値の新しい契約、既存の基準画像、更新通知、取消し、アンドゥ、リドゥ、待機状態は
  同じ実行で成功した。
- 対象実装後の構築は自動MOC、変更した`libs/ui/tests/FreehandStrokeContractTest.cpp`、試験実行形式の
  リンクだけだった。続く計画はglob再検査とCMake再評価だけを示し、対象コンパイルを含まない。
  直接依存と空構築閉包はR2-G13aの上限を維持した。
- `nix develop .#test --command ./scripts/run-test FreehandStrokeContractTest`、正しいCTestプリセットによる
  20回反復、`kis_strokes_queue_test`、`kis_update_scheduler_test`、`kis_projection_test`、
  `kis_prescaled_projection_contract_test`はmacOSで成功した。`FreehandStrokeContractTest.cpp`の書式検査も
  成功した。CTestプリセットを使わない最初の反復命令はアプリケーション資源環境を欠いて起動前に停止し、
  製品処理と契約は実行していない。Linuxと全ネイティブ検証は実行していない。

## R2-G14 既定画素ブラシの筆圧応答契約で完了した作業

- 実装前の`FreehandStrokeContractTest`は変更なし計画に対象コンパイルがなく、直接依存が
  `kritapixelbrush`、`kritapainting`、`kritalibbrush`、`kritatestsdk`、空のmacOS構築木に対する
  Ninjaコマンド閉包が1,107工程だった。筆圧値の追加は同じ製品経路だけを使うため、先行する構造変更は
  行っていない。
- `libs/ui/tests/FreehandStrokeContractTest.cpp`の固定入力生成とストローク実行へ筆圧を値として渡し、
  既定値1.0でR2-G13bの画素、領域、取消し、アンドゥ、リドゥ契約を維持した。新契約は同じプリセット、
  キャンバス、レイヤー、色、2入力点、その他入力値、無選択、非ミラー、作業スレッド1本を使い、
  両端の筆圧だけを0.5へ変える。
- 最初の対象実行は新契約の採取診断だけで失敗し、レイヤーと投影の正確な描画領域
  `QRect(126, 126, 234, 234)`と、RGBA8888へ正規化した全画素のSHA-256
  `ffdae59742d86fcfcc3764eeb7d2e82c126cd9cb08fb7c7c97a94e8b46cd5bb9`を記録した。
  維持値の追加後はレイヤーと投影が完全一致し、筆圧1.0の385×385画素領域より小さいことを固定した。
  ハッシュ不一致時は実画像を試験出力ディレクトリーへ保存する。
- 対象実装後の構築は自動MOC、変更した試験ソース、試験実行形式のリンクだけだった。続く計画はglob
  再検査とCMake再評価だけを示し、対象コンパイルを含まない。直接依存と空構築閉包はR2-G13aの上限を
  維持した。
- `nix develop .#test --command ./scripts/run-test FreehandStrokeContractTest`、CTestプリセットによる
  20回反復、`kis_strokes_queue_test`、`kis_update_scheduler_test`、`kis_projection_test`、
  `kis_prescaled_projection_contract_test`はmacOSで成功した。Linuxと全ネイティブ検証は実行していない。

## R2-G15 既定画素ブラシの筆圧勾配契約で完了した作業

- 実装前の`FreehandStrokeContractTest`は変更なし計画に対象コンパイルがなく、直接依存が
  `kritapixelbrush`、`kritapainting`、`kritalibbrush`、`kritatestsdk`、空のmacOS構築木に対する
  Ninjaコマンド閉包が1,107工程だった。開始点と終了点の筆圧値を分けても同じ製品経路を使うため、
  先行する構造変更は行っていない。
- `libs/ui/tests/FreehandStrokeContractTest.cpp`のストローク実行へ開始筆圧と終了筆圧を個別に渡せるように
  した。固定筆圧0.5と筆圧勾配を同じデータ駆動契約へまとめ、画像正規化、ハッシュ比較、差異画像保存、
  領域比較の重複を増やさずに既存契約を維持した。
- 最初の対象実行は開始筆圧0.25、終了筆圧1.0の採取行だけで失敗し、レイヤーと投影の正確な描画領域
  `QRect(154, 154, 229, 229)`と、RGBA8888へ正規化した全画素のSHA-256
  `e9740f2b00ef8670a37aade2c4f96cec8197dfc96eb3e18adcc20f938b5f87c0`を記録した。
  維持値の追加後は勾配行、固定筆圧0.5行、筆圧1.0の基準画像契約が同じ実行で成功した。
- 対象実装後の構築は自動MOC、変更した試験ソース、試験実行形式のリンクだけだった。続く計画はglob
  再検査とCMake再評価だけを示し、対象コンパイルを含まない。直接依存と空構築閉包はR2-G13aの上限を
  維持した。
- `nix develop .#test --command ./scripts/run-test FreehandStrokeContractTest`、CTestプリセットによる
  20回反復、`kis_strokes_queue_test`、`kis_update_scheduler_test`、`kis_projection_test`、
  `kis_prescaled_projection_contract_test`はmacOSで成功した。Linuxと全ネイティブ検証は実行していない。

## R2-G16 PaintOp乱数経路の決定的契約で完了した作業

- 実装前の`FreehandStrokeContractTest`は変更なし計画に対象コンパイルがなく、直接依存が
  `kritapixelbrush`、`kritapainting`、`kritalibbrush`、`kritatestsdk`、空のmacOS構築木に対する
  Ninjaコマンド閉包が1,107工程だった。設定UI、動的PaintOpモジュール、アプリケーション実行形式へ
  到達しないため、先行する構造変更は行っていない。
- 乱数を使う最小経路として、既定画素ブラシの寸法センサーだけをFuzzy Dabへ切り替えた。
  `libs/image/brushengine/kis_stroke_random_source.{h,cpp}`は描点単位の`KisRandomSource`だけを指定種で
  初期化する構築経路を所有する。`KisPerStrokeRandomSource`と通常の種無指定経路は従来の初期化を
  維持する。
- `libs/painting/strokes/freehand_stroke.{h,cpp}`は指定種をストローク所有の乱数源へ渡す既存型の
  オーバーロードを追加した。最初の対象構築は5引数の構築子が存在しないことだけを診断し、実装後は
  `kis_stroke_random_source.cpp`、`kritaimage`、`freehand_stroke.cpp`、`kritapainting`、変更した試験と
  リンクだけを再構築した。
- `libs/ui/tests/FreehandStrokeContractTest.cpp`は種17の同一入力をアンドゥを挟んで2回実行し、
  レイヤー、投影、正確な描画領域`QRect(142, 142, 271, 271)`、RGBA8888全画素SHA-256
  `34a090d8b904e9950f2bf7868b2c7b1f78c2d5bb3ddb8a531a90f203721c21d3`の一致を固定した。
  種18はSHA-256
  `dc333677c01755e711027da3b64b8c8f719af1a041bfe4a19750e44ffd04e321`となり、異なる種が異なる
  画素結果を生成することを確認した。維持対象は種17の完全結果と種間の差であり、種18の内部系列は
  固定しない。
- 続く変更なし計画はglob再検査とCMake再評価だけを示し、対象コンパイルを含まない。公開ヘッダー変更後の
  旧構築経路確認では、実利用元の`kis_tool_freehand_helper.cpp`、`kis_preset_live_preview_view.cpp`と
  `KisPaintOnTransparencyMaskTest`だけが追加で再構築され、構築範囲の再分割を要する波及はなかった。
- `nix develop .#test --command ./scripts/run-test FreehandStrokeContractTest`、CTestプリセットによる
  20回反復、`KisPerStrokeRandomSourceTest`、`kis_strokes_queue_test`、`kis_update_scheduler_test`、
  `kis_projection_test`、`kis_prescaled_projection_contract_test`、旧構築子を使う
  `KisPaintOnTransparencyMaskTest`の対象構築はmacOSで成功した。変更行だけを整形し、
  `FreehandStrokeContractTest.cpp`の全体書式検査と`nix develop .#test --command ./scripts/verify-quick`も
  成功した。既存の乱数源と自由描画ソースには変更範囲外の書式差分が残る。Linuxと全ネイティブ検証は
  実行していない。

## R2-G17 既定画素ブラシの間隔応答契約で完了した作業

- 実装前の`FreehandStrokeContractTest`は変更なし計画に対象コンパイルがなく、直接依存が
  `kritapixelbrush`、`kritapainting`、`kritalibbrush`、`kritatestsdk`、空のmacOS構築木に対する
  Ninjaコマンド閉包が1,107工程だった。間隔の実行処理は対象が既に使う`KisBrush`と
  `KisBrushBasedPaintOpSettings`が所有し、設定UI、動的PaintOpモジュール、アプリケーション実行形式へ
  到達しないため、先行する構造変更は行っていない。
- `libs/ui/tests/FreehandStrokeContractTest.cpp`は、試験用プリセットの具体的な`KisBrushOpSettings`へ
  間隔0.25を設定する契約を追加した。sRGB 8ビット、500×500画素、単一レイヤー、固定2入力点、
  筆圧1、その他の入力値、色、無選択、非ミラー、作業スレッド1本は既定間隔0.1の契約と同じである。
- 最初の対象実行は新契約の採取診断だけで失敗し、レイヤーと投影の正確な描画領域
  `QRect(50, 50, 353, 353)`と、RGBA8888全画素SHA-256
  `8bdf0e95ea7526b6289bf2393397c7bb005b69da6866891c2cb12bf991d7f210`を記録した。
  維持値の追加後は投影との完全一致と、既定間隔0.1の385×385画素領域より小さいことを固定した。
  ハッシュ不一致時は実画像を試験出力ディレクトリーへ保存する。
- 実装後の増分構築は自動MOC、変更した試験ソース、試験実行形式のリンクだけだった。製品ソースの
  再構築はなく、直接依存とコマンド閉包はR2-G13aの上限を維持した。
- `nix develop .#test --command ./scripts/run-test FreehandStrokeContractTest`、CTestプリセットによる
  20回反復、`kis_auto_brush_factory_test`、`KisBrushModelTest`、`kis_distance_information_test`、
  `KisDabRenderingQueueTest`、`kis_prescaled_projection_contract_test`、変更後の対象計画、
  `FreehandStrokeContractTest.cpp`の書式検査、`nix develop .#test --command ./scripts/verify-quick`は
  macOSで成功した。Linuxと全ネイティブ検証は実行していない。

## R2-G18 既定画素ブラシの速度応答契約で完了した作業

- 実装前の`FreehandStrokeContractTest`は変更なし計画に対象コンパイルがなく、直接依存が
  `kritapixelbrush`、`kritapainting`、`kritalibbrush`、`kritatestsdk`、空のmacOS構築木に対する
  Ninjaコマンド閉包が1,107工程だった。速度センサーは`kritapaintopruntime`の
  `KisDynamicSensorSpeed`が`KisPaintInformation::drawingSpeed()`を読み、既存の画像・PaintOp実行所有だけで
  完結するため、先行する構造変更は行っていない。
- `libs/ui/tests/FreehandStrokeContractTest.cpp`は固定入力生成へ速度を値として渡し、寸法センサーを
  Speedへ切り替えた両入力点へ速度0.5を設定する契約を追加した。sRGB 8ビット、画像、レイヤー、
  プリセット、2入力点、筆圧、間隔、その他の入力値、色、無選択、非ミラー、作業スレッド1本は
  R2-G13bと同じである。
- 最初の対象実行は新契約の採取診断だけで失敗し、レイヤーと投影の正確な描画領域
  `QRect(125, 125, 235, 235)`と、RGBA8888全画素SHA-256
  `3c7c2e19b4b91a27b8d1ddb1068db753012e01f98244eb9e6f688026db4f551a`を記録した。
  維持値の追加後は投影との完全一致と、速度0の既存入力が作る385×385画素領域より小さいことを
  固定した。ハッシュ不一致時は実画像を試験出力ディレクトリーへ保存する。
- 実装後の増分構築は自動MOC、変更した試験ソース、試験実行形式のリンクだけだった。製品ソースの
  再構築はなく、直接依存とコマンド閉包はR2-G13aの上限を維持した。
- `nix develop .#test --command ./scripts/run-test FreehandStrokeContractTest`、CTestプリセットによる
  20回反復、`kis_paint_information_test`、`KisCurveOptionDataTest`、`KisCurveOptionModelTest`、
  `KisDabRenderingQueueTest`、`kis_prescaled_projection_contract_test`、変更後の対象計画、
  `FreehandStrokeContractTest.cpp`の書式検査、`nix develop .#test --command ./scripts/verify-quick`は
  macOSで成功した。Linuxと全ネイティブ検証は実行していない。

## R2-G19a 全public API挙動契約の継続検査基盤で完了した作業

- 公開マクロを持つ製品ヘッダー、異なる製品部品から直接includeされるヘッダー、公開ヘッダー構築契約の
  和集合を公開API採取範囲とし、固定Nix環境の
  Universal Ctags 6.2.1でpublicの型、列挙値、型別名、関数、メソッド、データ、変数を直接採取する。
  基盤確立時の範囲は1,544ヘッダー、29,246 APIであり、製品のCMake構成やコンパイルを実行しない。
- `docs/architecture/public-api-test-contracts.json`が公開面の件数と指紋、移行中の正確な未対応件数、
  CTest対象、試験ソース、具体的な試験関数、観測挙動、分類、API識別子の対応を保持する。
- `scripts/architecture/check_public_api_contracts.py`が公開面の変更、存在しないAPI、試験ソース・
  試験関数の欠落、CMake所有不一致、未対応件数の同期漏れを検査する。`--report`は全未対応APIを
  `build/`以下の作業用JSONへ出力する。
- `TestToolCoreContract`の平滑化設定、ブラシ寸法、ツールファクトリー、選択修飾、矩形・輪郭・折線
  操作、描画情報、速度平滑化の15試験関数を監査し、実際に観測する93 APIを維持契約として登録した。
  ヘッダー構築だけの試験は挙動契約として数えていない。
- 検査用依存は方針ソースから独立した製品構築を発生させず、Nixバイナリキャッシュから
  Universal CtagsとJSON依存を取得した。製品ターゲットとCTest実行形式は構築していない。
- `nix develop .#test --command python3 -m unittest scripts.tests.test_public_api_contracts
  scripts.tests.test_public_contracts`は7件、直接の公開API契約検査は93件の対応と29,153件の
  未対応基準、`nix develop .#test --command ./scripts/verify-quick`は39件の方針試験と全統治検査に
  macOSで成功した。`nix flake check --no-build --all-systems --no-eval-cache`は全構成の評価に成功した。
  `nix build .#checks.aarch64-darwin.governance --no-link`は独立した方針ソース検査に成功した。

## R2-G19b 版情報public API契約で完了した作業

- `libs/version/KritaVersionWrapper.{h,cpp}`の2 APIには専用試験がなく、既存の資源試験は
  `versionString()`を別の生成結果との相対比較に使うだけだった。`KritaVersionWrapperTest`を
  `libs/version/tests/`へ追加し、`libs/version/CMakeLists.txt`から登録した。製品実装は変更していない。
- 実装前の`kritaversion`は変更なしコマンド閉包5工程、入力9件、直接依存Qt Coreだけだった。
  追加した試験の直接CMake依存は`kritaversion`だけで、リンクはQt TestとQt Core、空のコマンド閉包は
  9工程、入力16件であり、アプリケーション、資源、UI対象へ到達しない。
- 最初の対象実行は`versionString(false)`の実値`6.1.0-prealpha`と、Qt 6構成の開発版判定が真で
  あることを診断した。維持契約は固定版文字列ではなく、生成された`KRITA_VERSION_STRING`を返し、
  要求時だけ構成済み`KRITA_GIT_SHA1_STRING`を付加する規則と、Qt 6を開発版とする分類である。
- 2試験関数を両public APIへ対応付け、公開API契約は95件、未対応基準は29,151件になった。
  `nix develop .#test --command ./scripts/run-test KritaVersionWrapperTest`、対象CTestの20回反復、
  変更後の対象計画、直接の公開API契約検査、`verify-quick`はmacOSで成功した。Linuxと全ネイティブ
  検証は実行していない。

## R2-G19b 実行時命令セットpublic API契約で完了した作業

- `libs/multiarch/KisSupportedArchitectures.{h,cpp}`と
  `libs/multiarch/KoMultiArchBuildSupport.{h,cpp}`の公開面を監査した。宣言だけ存在し実装も利用元も
  なかった`KisSupportedArchitectures::bestArch()`は、リンク可能性を要求する最初の対象実行で
  未定義シンボルになることを確認し、公開ヘッダーから削除した。代替APIは追加していない。
- `KisMultiArchPublicApiTest`を`libs/multiarch/tests/`へ追加し、既定のベクトル化設定、基準・最適・
  利用可能命令セットの整合、スカラー生成経路の汎用命令セット、最適化生成経路の実行時選択を固定した。
  ホスト固有名は文字列リテラルで固定せず、xsimdが報告する実行環境との対応を検査する。
- 契約追加前の`kritamultiarch`は、未使用の`kis_debug.h`を介して`kritaglobal`と`kritaversion`まで
  到達し、変更なし構築閉包が61工程、入力119件だった。`libs/multiarch/CMakeLists.txt`から不要な
  `kritaglobal`リンクを外し、`libs/multiarch/KisSupportedArchitectures.cpp`と
  `libs/multiarch/KoMultiArchBuildSupport.cpp`から未使用includeを除いた。製品対象は6工程、入力12件、
  リポジトリ内製品依存0件へ縮小した。
- 追加した試験の直接リポジトリ依存は`kritamultiarch`だけで、変更なし構築閉包は10工程、入力19件で
  ある。7 APIを4試験関数へ対応付け、公開面は1,544ヘッダー、29,245 API、対応済み102 API、
  未対応29,143 APIになった。
- `nix develop .#test --command ./scripts/run-test KisMultiArchPublicApiTest`、対象CTestの20回反復、
  変更後の対象計画、直接の公開API契約検査はmacOSで成功した。Linuxと全ネイティブ検証は実行して
  いない。

## R2-G19b 色管理public API契約で完了した作業

- `libs/color/kis_color_manager.h`と`libs/color/colord/KisColord.h`の17 APIを7試験関数へ
  対応付けた。装置種別の公開数値、直接構築と単一個体、存在しない装置、種別別一覧、DBus装置の
  表示名と複数ICCプロファイル、装置変更信号を観測する。
- `KisColorManagerPublicApiTest`を`libs/color/tests/`へ追加し、macOSと標準Linuxのダミー後段を
  同じ契約で固定した。`KisColordPublicApiTest`は試験プロセス専用のDBusデーモンと
  `FakeColordService`を起動し、実機のcolordサービス、装置、プロファイルに依存せずLinuxの
  `-DHAVE_DBUS=ON`後段を検査する。
- 契約追加前のmacOS `kritacolor`は、使用しない`kritaglobal`と`KF::I18n`を介して
  `kritaversion`まで到達し、変更なし構築閉包が60工程、入力117件だった。
  `libs/color/CMakeLists.txt`から不要なリンクを外し、
  `libs/color/linux/kis_color_manager.cpp`と`libs/color/colord/KisColord.cpp`の不要include・診断依存を
  Qtへ閉じた。macOS製品対象は5工程、入力9件、共通試験は9工程、入力16件になった。標準Linuxは
  製品5工程、入力10件、共通試験9工程、入力18件である。
- 任意colord構成はQt 6で未定義の`qt5_add_dbus_interface`により構成できなかった。
  `libs/color/colord/CMakeLists.txt`をQt 6のDBus生成命令へ更新し、公開ヘッダーが要求するQt DBusと
  生成ヘッダーのinclude経路を公開依存として明示した。`kritacolord`は17工程、入力36件、
  `kritacolor`は22工程、入力44件、私設サービスを含む専用試験は30工程、入力59件で、
  リポジトリ内依存は色管理責務内に閉じている。
- 最初の正のDBus契約は、プロファイル1の要求に先頭プロファイルを返す逆条件を診断した。
  `libs/color/colord/KisColord.cpp`は有効添字の要素を返し、負値と範囲外値を先頭へ安全に戻す。
  標準Linuxは`HAVE_QT_DBUS`を検出しても`HAVE_DBUS`を自動設定せずダミー後段を維持するため、
  自動選択の要否はR2-G19bの未確定設計として追跡する。
- macOSの共通試験、`ssh nixos`上の標準Linux共通試験と任意colord試験は各20回成功した。
  公開API契約は119件、未対応基準は29,126件になった。全ネイティブ検証は実行していない。

## R2-G19b QMLウィジェットpublic API契約で完了した作業

- `qmlmodules/widgets/KisQQuickWidget.{h,cpp}`と`KisQQuickPopupWidget.{h,cpp}`の11 APIを
  5試験関数へ対応付けた。QMLエンジン文脈、sRGB描画形式、ルート暗黙寸法への追従、同梱QMLの
  読み込み、余白設定、親付き個体の破棄を維持契約として観測する。
- 契約追加前の`kritaqmlwidgets`は、便宜的な`kritaapplicationui`リンクとsRGB形式指定だけに使う
  `kritacanvas`を経由し、変更なし構築閉包が1,668工程、入力3,335件だった。
  `qmlmodules/widgets/CMakeLists.txt`は公開ヘッダーが要求するQt Quick依存を公開し、実装専用依存を
  `kritaglobal`、`kritaresources`、KF I18n、Qt QML、Qt Quick Controlsへ限定した。
  `KisQQuickWidget.cpp`はQt自身の同値なsRGB型を使う。製品対象は137工程、入力299件、専用試験は
  141工程、入力306件となり、アプリケーションUIとキャンバスへ到達しない。
- 最初の試験構築はQtに存在しない背景色getterと不足した完全型だけを診断した。実行時にはNixの
  分割Qt配置が標準QMLプラグインを自動探索できないことを確認し、専用CTestだけにQt Quick対象から
  決定したQML配置を設定した。同梱`KisQQuickPopupWidget.qml`を置換せずに読み込む。
- 所有期間契約は、翻訳文脈の親がQMLウィジェットでなく外側の親であることを診断した。
  `KisQQuickWidget.cpp`は翻訳文脈を`this`の子として所有し、`parent=nullptr`を含む公開構築経路でも
  ウィジェット破棄時に解放する。
- `nix develop .#test --command ./scripts/run-test KisQQuickWidgetsPublicApiTest`と、macOS CTest
  プリセットによる20回反復は成功した。公開API契約は130件、未対応基準は29,115件になった。
  Linuxと全ネイティブ検証は実行していない。

## 次の操作

`libs/koplugin`の未対応36 APIについて、既存の`KoPluginLoaderTest`と`KisMimeDatabaseTest`が
実際に観測する挙動、所有対象の変更なし計画、直接依存、空構築閉包を確認する。未観測の取引器、
読み込み器、MIME判定だけへ最小契約を追加する。

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
