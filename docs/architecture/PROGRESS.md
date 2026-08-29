# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-08-29 09:19 JST
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
- `nix develop .#test --command ./scripts/run-test KisQQuickWidgetsPublicApiTest`と対象CTestの
  20回反復はmacOSと`ssh nixos`上のLinuxで成功した。Linuxの初回増分構築は19工程だった。
  公開API契約は130件、未対応基準は29,115件になった。全ネイティブ検証は実行していない。

## R2-G19b 非公開入れ子スコープ除外で完了した作業

- `libs/koplugin/KisMimeDatabase.h`のprivate入れ子型は型自体が公開面から除かれる一方、そのpublic
  メンバー3件だけが公開APIとして誤採取されていた。privateまたはprotectedの型・列挙スコープを
  祖先方向へ照合し、その配下の型、列挙値、型別名、関数、メソッド、データ、変数を除外する。
- 合成Ctags入力の試験は、private型のpublicメンバーとさらに内側のpublic型が最初の実行で残ることを
  診断した。修正後は両者を除外し、同じ外側型に属するpublic入れ子型とそのメンバーを維持する。
- 旧集合との全件比較では非公開入れ子スコープ配下232 APIだけが減り、追加0件、既存130契約の
  除外0件だった。公開ヘッダー1,544件を維持し、公開APIは29,013件、未対応基準は28,883件になった。

## R2-G19b プラグイン探索とMIME判定public API契約で完了した作業

- `libs/koplugin/{KoPluginLoader,KoJsonTrader,KisMimeDatabase}.{h,cpp}`の33 APIを既存2 CTestの
  11試験関数へ対応付けた。プラグイン設定、サービス・MIME条件による候補探索、動的・静的・空の
  候補ラッパー、IDと複数条件による最高版選択、重複除去、拒否一覧、再読込抑止、MIMEの接尾辞・
  内容・説明・優先接尾辞・アイコン判定を私設プラグインと固定入力で観測する。
- 実装前の`kritaplugin`は62工程、入力122件、`KoPluginLoaderTest`は86工程、入力161件、
  `KisMimeDatabaseTest`は66工程、入力129件だった。いずれもR2-G13aの上限以下であり、
  アプリケーションUIへ到達しないため製品所有単位は分割していない。
- `libs/koplugin/KoJsonTrader.h`から、公開宣言に不要だった`kis_pointer_utils.h`のincludeを
  実利用元`libs/koplugin/KoJsonTrader.cpp`へ移した。プラグイン試験と私設モジュールから未使用の
  `kritaglobal`、`kritatestsdk`直接リンクを外し、Qt Testを直接宣言した。空構築閉包の件数は
  製品が元から必要とする`kritaglobal`経路により同じ値を維持する。
- 最初の取引器試験は、公開default構築した`KoJsonTrader::Plugin`の各取得処理がnullローダーを
  参照してSIGSEGVになることを診断した。空候補は個体を持たず、メタデータ、ファイル名、診断を
  空として返すようにし、動的・静的候補の従来経路を維持した。
- macOSと`ssh nixos`上のLinuxで`KoPluginLoaderTest`と`KisMimeDatabaseTest`の対象実行および
  各20回反復が成功した。公開API契約は163件、未対応基準は28,850件になった。
  全ネイティブ検証は実行していない。

## R2-G19b XML直列化public API契約で完了した作業

- `libs/serialization/xml/KoXmlNS.h`と`KoXmlWriter.h`の54 APIを`TestXmlWriter`の9試験関数へ
  対応付けた。32名前空間URIと接頭辞逆引き、文書宣言と装置寿命、全属性・本文入口、予約文字と
  制御文字、基準字下げと抑制、完成済み要素取込、manifest項目展開を固定バイト列で観測する。
- 実装前の`kritaxmlserialization`は6工程・11入力、`TestXmlWriter`は10工程・18入力だった。
  製品はQt Coreだけ、試験は製品対象とQt Testだけへ直接依存し、アプリケーションやUIへ到達しない。
  所有単位とCMake依存は変更していない。
- 最初の対象実行は10試験関数のうち8件が成功し、基準深さ付き閉じタグと完成済み要素後の閉じタグを
  1段深く見積もった2期待値だけが失敗した。現在深さで子を字下げし、親深さで閉じる実出力へ
  期待値を合わせ、製品実装を変更せず対象実行が成功した。
- 公開API契約は217件、未対応基準は28,796件になった。macOSと`ssh nixos`上のLinuxで対象実行と
  20回反復が成功し、Linuxの初回対象構築は5工程だった。macOSの直接公開API契約検査と
  `verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b 文書状態public API契約で完了した作業

- `libs/document/session/`の自動保存、文書識別、変更状態、回復用自動保存、回復状態の5ヘッダーに
  ある51 APIを既存5 CTestの17試験関数へ対応付けた。書出し期間と失敗切替、表示・実体パスと
  MIME状態、保存・自動保存中の変更、回復要求の延期・合流・完了、回復済み遷移を観測する。
- 実装前の`kritadocument`は9工程・17入力、各試験は13工程・24入力だった。試験CMakeから
  `kritatestsdk`の間接依存を外してQt Testを直接宣言し、同じ閉包件数とQt Coreだけの製品依存を
  維持した。製品実装は変更していない。
- 既存試験が49 APIを観測済みだった。未観測の取り消し履歴外変更の明示消去と、回復要求が合流した
  保存パスの明示消去を既存状態遷移へ追加し、5対象の実行がmacOSで成功した。
- 公開API契約は268件、未対応基準は28,745件になった。macOSと`ssh nixos`上のLinuxで5対象実行と
  各20回反復が成功した。Linuxの初回対象構築は識別試験4工程、残る4試験17工程だった。macOSの
  直接公開API契約検査と`verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b 文書ファイルpublic API契約で完了した作業

- `libs/document/files/`の保存先、自動保存ファイル、バックアップの3ヘッダーにある37 APIを
  `kis_document_files_test`の8試験関数へ対応付けた。保存先の存在・書込み可能性、バックアップの
  既定値・単一・世代・失敗・一時配置、自動保存名、回復ファイル列挙・プレビュー・削除を観測する。
- 実装前の`kritadocumentfiles`は71工程・139入力、試験は75工程・146入力だった。試験CMakeから
  `kritatestsdk`の間接依存を外し、書庫生成に使う`kritaresourcestorage`とQt Testを直接宣言して
  同じ閉包件数を維持した。製品実装は変更していない。
- 既存5試験へ、既定回復ディレクトリー入口、既定削除入口、削除の冪等性、私設ZIP内の
  `Thumbnails/thumbnail.png`読出しを追加した。バックアップ設定の既定値、作成失敗、一時配置と
  強制一時配置を3試験関数で補い、対象実行がmacOSで成功した。
- 公開API契約は305件、未対応基準は28,708件になった。macOSと`ssh nixos`上のLinuxで対象実行と
  20回反復が成功し、Linuxの初回対象構築は10工程だった。macOSの直接公開API契約検査と
  `verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b 文書UI契約前の構築範囲分離で完了した作業

- `libs/document/ui/CMakeLists.txt`で一つの`kritadocumentui`ソース集合に属していた実装を、
  ファイルを移動せず5つの内部オブジェクト所有単位へ分けた。起点と分割先は、
  `info/KoDocumentInfoDlg.cpp`と2 UIファイルから`kritadocumentinfoobjects`、
  `io/kis_document_io_presentation.cpp`から`kritadocumentiopresentationobjects`、
  `recovery/KisAutoSaveRecoveryDialog.cpp`から`kritadocumentautosaverecoveryobjects`、
  `recovery/KisRecoverNamedAutosaveDialog.cpp`とUIファイルから`kritadocumentnamedrecoveryobjects`、
  `undo/{kis_document_undo_store,kundo2model,kundo2view}.cpp`から`kritadocumentundoobjects`である。
  公開共有ライブラリー`kritadocumentui`は同じ実装を集約し、公開名、版、公開ヘッダー、
  シンボル所有を維持する。
- `libs/impex/CMakeLists.txt`で`KisImportExportErrorCode.cpp`を
  `kritaimpexerrorobjects`へ、`metadata/KoDocumentInfo.cpp`を
  `kritaimpexmetadataobjects`へ分けた。公開共有ライブラリー`kritaimpex`は両方を集約する。
  文書入出力表示と文書情報表示の試験は、1クラスのために`kritaimpex`全体を構築せず、
  対応する内部実装を直接リンクする。文書メタデータ専用試験も同じ所有単位を直接検査する。
- `libs/document/ui/tests/CMakeLists.txt`は6 CTestを各内部所有単位へ対応付け、
  `kritadocumentui`、`kritadocument`、`kritaimpex`、`kritatestsdk`の一括リンクを除去した。
  変更前は各試験が1,044工程・2,103入力だった。変更後は自動保存回復639工程・1,304入力、
  文書情報637工程・1,302入力、入出力表示65工程・128入力、名前付き回復8工程・17入力、
  取り消し保存と履歴表示が各234工程・497入力である。メタデータ専用試験は136工程・297入力、
  エラー表現とメタデータの内部所有単位はそれぞれ58工程・115入力、132工程・290入力である。
- `KisAutoSaveRecoveryDialog.cpp`から未使用の`kis_debug.h`を除去した。最小構築で表面化した
  `KisImportExportErrorCode.cpp`の`kis_assert.h`依存は所有元`kritaglobal`へ明示した。
  新しい公開API、互換経路、実行時分岐は追加していない。
- macOSと`ssh nixos`上のLinuxで`kritaimpex`と`kritadocumentui`の対象構築、抽出元2 CTestと
  文書UI 6 CTest、8 CTestの20回反復が成功した。Linuxの文書UI試験は自動保存回復667工程・
  1,358入力、文書情報665工程・1,356入力、入出力表示58工程・125入力、名前付き回復8工程・
  23入力、取り消し保存と履歴表示が各227工程・496入力である。macOSの公開シンボル確認と
  `verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b 文書UI public API契約で完了した作業

- `libs/document/ui`の文書情報7 API、入出力表示15 API、自動保存回復5 API、名前付き回復9 API、
  文書履歴保存8 API、履歴表示12 APIを既存6 CTestの20試験関数へ対応付けた。文書値の表示と編集、
  追加ページの承認拒否と適用、読取専用化、一括処理の対話抑止、保存・読込・自動保存通知、
  回復候補の選択、名前付き回復の3決定値、履歴命令・マクロ・スタック・グループ・表示属性を観測する。
- 既存11試験関数が文書情報の構築と承認、保存と自動保存通知、回復候補の一括破棄、プレビュー、
  履歴保存と履歴表示の主要経路を観測済みだった。既存対象へ文書情報3件、入出力表示3件、
  自動保存回復1件、名前付き回復1件、履歴表示3件の試験関数を追加した。製品実装、公開ヘッダー、
  CMake依存は変更していない。
- `KoDocumentInfoDlg::isDocumentSaved()`は構築時と承認後のどちらも偽で、真へ遷移させる実装と
  利用元が存在しない。宣言の削除または保存状態契約の再設計まで、現在値を既知不具合として分類した。
  残る55 APIは維持契約である。
- 公開API契約は361件、未対応基準は28,652件になった。macOSと`ssh nixos`上のLinuxで6対象実行と
  20回反復が成功し、Linuxの初回再構築は変更した5試験だけの16工程だった。両環境の直接公開API
  契約検査、macOSの5試験ソースclang-format検査と`verify-quick`も成功した。全ネイティブ検証は
  実行していない。

## R2-G19b 入出力エラー契約前の試験分離で完了した作業

- `libs/impex/tests/TestImportExportBoundary.cpp`を起点として、エラー結果分類の1試験関数を
  `libs/impex/tests/kis_import_export_error_code_test.cpp`へ移した。元の境界試験はファイル事前条件と
  MIME方向選択を維持し、新しい`kis_import_export_error_code_test`は分離済みの
  `kritaimpexerrorobjects`とQt Testだけを直接リンクする。
- エラー契約の変更なし空構築閉包は、3責務を含む元の`TestImportExportBoundary`の1,018工程・
  2,057入力から62工程・122入力へ縮小した。製品実装、公開API、`kritaimpex`の公開共有ライブラリー、
  元の2契約の挙動は変更していない。
- macOSと`ssh nixos`上のLinuxで新旧2対象の構築、実行、20回反復が成功した。専用対象の
  空構築閉包はmacOSで62工程・122入力、Linuxで55工程・118入力である。触れた2試験ソースの
  macOS clang-format検査と`verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b 入出力エラーと文書メタデータ public API契約で完了した作業

- `libs/impex/KisImportExportErrorCode.h`の43 APIを
  `libs/impex/tests/kis_import_export_error_code_test.cpp`の5試験関数へ対応付けた。既定値と結果分類、
  全16単純結果の診断文、外部由来の失敗理由、Qtファイルエラーの説明と等値性、基底型経由の
  派生診断と破棄、診断出力を観測する。
- `libs/impex/metadata/KoDocumentInfo.h`の14 APIを
  `libs/impex/tests/kis_document_metadata_test.cpp`の6試験関数へ対応付けた。文書項目と生成器、
  変更状態に従う作成者上書き、XML読書きと連絡先、独立した値複製、編集履歴の再初期化を観測する。
- 書込失敗の`operator<<`は利用者向け説明を正しく出力する一方、Qtエラー値に書込側ではなく
  読込側の既定値0を表示する。診断実装を修正するまで現在値を既知不具合として分類し、残る56 APIは
  維持契約とした。製品実装、公開ヘッダー、CMake依存は変更していない。
- 公開API契約は418件、未対応基準は28,595件になった。macOSと`ssh nixos`上のLinuxで2対象の実行、
  20回反復、直接公開API契約検査が成功した。Linuxの初回再構築は各対象4工程だった。macOSの
  2試験ソースclang-format検査と`verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b ファイル検査と形式探索契約前の構築範囲分離で完了した作業

- `libs/impex/KisImportExportAdditionalChecks.cpp`を`kritaimpex`の一括ソースから
  `kritaimpexadditionalchecksobjects`へ移し、Qt Coreだけを直接依存とした。公開ヘッダーから未使用の
  エラー表現includeを除き、ファイル属性検査の変更が入出力全体を構築しない所有単位にした。
- `libs/impex/KisImportExportFilterRegistry.cpp`の`createFilter()`実装を
  `libs/impex/KisImportExportFilterFactory.cpp`へ移した。元ファイルはMIME集約とプラグイン探索を
  `kritaimpexfilterregistryobjects`として所有し、実フィルター生成はフィルター本体と同じ
  `kritaimpex`の一括ソースに維持する。
- 公開`kritaimpex`は新しい2 object対象を集約し、既存の共有ライブラリー名、6公開メソッドの
  シンボル、依存方向を維持する。`libs/impex/tests/TestImportExportBoundary.cpp`は未使用の画像側2ヘッダーを
  除き、新しい2 object対象とQt Testだけへ直接リンクする。外部利用される非exportヘッダー2件は
  `libs/impex/tests/TestImportExportPublicHeaders.cpp`の構築契約へ移し、挙動試験との責務を分けた。
- 変更なし空構築閉包は1,018工程・2,057入力から72工程・140入力へ縮小した。画像と保存へ依存する
  公開ヘッダー構築対象は1,002工程・2,026入力として別に隔離した。macOSで
  `TestImportExportBoundary`の20回反復、`TestImportExportPublicHeaders`と公開`kritaimpex`の対象構築、
  公開6メソッドのシンボル確認、触れたC++のclang-format検査、Python構文検査、`verify-quick`が成功した。
  `ssh nixos`上のLinuxでも境界試験の構築と20回反復、公開ヘッダー対象、公開`kritaimpex`の構築が
  成功し、境界試験の空構築閉包は65工程・137入力だった。全ネイティブ検証は実行していない。

## R2-G19b ファイル検査と形式探索 public API契約で完了した作業

- `libs/impex/KisImportExportAdditionalChecks.h`の4 APIを、既存
  `libs/impex/tests/TestImportExportBoundary.cpp`のファイル事前条件試験へ対応付けた。欠落パスと
  作成済みファイルについて、存在、読込可能、書込可能の判定を観測する。
- `libs/impex/KisImportExportFilterRegistry.h`の7 APIを、同じ境界試験のMIME方向選択と空探索、
  `libs/impex/tests/kis_import_export_filter_factory_test.cpp`の未登録形式生成へ対応付けた。複数宣言の
  重複排除と整列、入力・出力方向の分離、空の固定プラグインディレクトリーにおける対応形式と
  生成フィルターの不在を観測する。
- 既存2試験関数が9 APIを観測済みであり、空探索と未登録形式生成の2試験関数だけを追加した。
  製品実装、公開ヘッダー、依存方向は変更していない。通常の境界試験は72工程・140入力、実際の
  フィルター基底とプラグイン生成へ依存する専用試験は1,023工程・2,066入力である。
- 公開API契約は429件、未対応基準は28,584件になった。macOSと`ssh nixos`上のLinuxで2対象の
  構築、実行、20回反復、直接公開API契約検査が成功した。Linuxの初回再構築は境界対象9工程、
  factory対象5工程だった。macOSの2試験ソースclang-format検査と`verify-quick`も成功した。
  全ネイティブ検証は実行していない。

## R2-G19b 書き出し検査契約前の構築範囲分離で完了した作業

- `libs/impex/KisExportCheckBase.cpp`を`kritaimpex`の一括ソースから
  `kritaimpexexportcheckbaseobjects`へ、`libs/impex/KisExportCheckRegistry.cpp`を
  `kritaimpexexportcheckregistryobjects`へ、`libs/impex/KisPreExportChecker.cpp`を
  `kritaimpexpreexportcheckerobjects`へ分けた。公開共有ライブラリー`kritaimpex`は3対象を集約し、
  既存のライブラリー名と書き出し検査シンボルの所有を維持する。
- `libs/impex/KisExportCheckBase.h`は画像共有ポインター型を前方宣言し、検査接続面だけを扱う基底実装が
  画像ライブラリー全体へ依存する経路を除いた。`libs/impex/ImageSizeCheck.h`は登録処理のヘッダーではなく
  使用する検査基底を直接includeする。公開API、実行時分岐、検査順序は変更していない。
- macOSの変更なし構築閉包は、検査基底が3工程・7入力、組込み検査登録と書き出し前検査が各1,001工程・
  2,026入力である。画像状態を読まない16 APIの反復は軽量な基底所有単位へ隔離し、画像と色空間を使う
  登録・走査の依存は各所有単位に維持する。
- macOSと`ssh nixos`上のLinuxで3 object対象と公開`kritaimpex`の対象構築、基底・登録・
  書き出し前検査の公開シンボル確認が成功した。Linuxの構築閉包は検査基底が3工程・7入力、
  組込み検査登録と書き出し前検査が各1,045工程・2,103入力である。macOSの触れたヘッダーの
  clang-format検査と`verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b 書き出し検査 public API契約で完了した作業

- `libs/impex/KisExportCheckBase.h`の16 APIを
  `libs/impex/tests/kis_export_check_base_test.cpp`の4試験関数へ対応付けた。識別子、3対応水準、
  既定と形式固有の警告、層単位属性、null画像値による具象検査への仮想呼出し、基底所有からの
  検査とファクトリーの仮想破棄を観測する。
- `libs/impex/ImageSizeCheck.h`の12 API、`libs/impex/KisExportCheckRegistry.h`の3 API、
  `libs/impex/KisPreExportChecker.h`の5 APIを`libs/impex/tests/kis_export_checks_test.cpp`の4試験関数へ
  対応付けた。画像寸法の包括積境界、標準値と明示値による生成、組込み登録と単一登録器、未宣言・
  部分対応・非対応・全対応の警告、エラー、成功分類を観測する。
- 基底試験はnull共有ポインターの参照操作だけを試験内で閉じ、画像ライブラリーへリンクしない。
  macOSの変更なし構築閉包は基底試験が7工程・14入力、実画像と全登録器を使う試験が1,011工程・
  2,045入力である。製品実装、公開API、検査順序は変更していない。
- 公開API契約は465件、未対応基準は28,548件になった。macOSと`ssh nixos`上のLinuxで2対象の構築、
  実行、各20回反復、直接公開API契約検査が成功した。Linuxの変更なし構築閉包は基底試験が7工程・
  16入力、実画像と全登録器を使う試験が1,055工程・2,123入力である。macOSの触れた2試験ソースの
  clang-format検査と`verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b フレーム時間検査契約前のヘッダー依存縮小で完了した作業

- `libs/impex/IntegralFrameDuration.h`は組込み検査登録器を経由せず、実際に継承する
  `libs/impex/KisExportCheckBase.h`を直接includeする構成へ変更した。未使用の`KoID.h`も除き、
  フレーム時間検査の利用元が登録器のテンプレートとQtオブジェクト依存を解析する経路を除いた。
- フレーム時間の判定自体は画像とアニメーション状態を読むため、既存の
  `kis_export_checks_test`へ追加する方針を維持する。macOSの変更なし構築閉包は変更前後とも
  1,011工程・2,045入力である。macOSと`ssh nixos`上のLinuxで対象構築と既存試験の直接実行、
  macOSでヘッダーのclang-format検査が成功した。公開APIと実行時挙動は変更しておらず、
  全ネイティブ検証は実行していない。

## R2-G19b フレーム時間検査 public API契約で完了した作業

- `libs/impex/IntegralFrameDuration.h`の9 APIを既存
  `libs/impex/tests/kis_export_checks_test.cpp`の2試験関数へ対応付けた。静止画像、25 fpsで40 msの
  整数フレーム時間、24 fpsで小数ミリ秒になるフレーム時間を実画像で作り、検査の必要性、対応水準、
  既定警告、安定識別子、形式固有警告を持つファクトリー生成を観測する。
- 画像と組込み登録器を既に使う試験対象へ追加したため、macOSの変更なし構築閉包は追加前後とも
  1,011工程・2,045入力である。製品実装、公開API、実行時分岐は変更していない。
- 公開API契約は474件、未対応基準は28,539件になった。macOSと`ssh nixos`上のLinuxで対象の構築、
  実行、20回反復、直接公開API契約検査が成功した。macOSの試験ソースのclang-format検査と
  `verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b 保存領域ライター契約前の構築範囲縮小で完了した作業

- `libs/impex/tests/CMakeLists.txt`の`TestImportExportPublicHeaders`から`kritaimage`のリンクを除いた。
  `libs/image`の抽象ライターヘッダーと構築木の生成済み公開マクロへ検索経路を明示し、実シンボルを
  使わない自己完結ヘッダー検査が画像共有ライブラリー全体を構築する経路を除いた。
- 変更なし構築閉包は、macOSで1,002工程・2,027入力から13工程・27入力へ、Linuxで13工程・
  31入力へ縮小した。macOSと`ssh nixos`上のLinuxで対象の構築と既存CTestが成功した。
  製品実装、公開API、実行時挙動は変更しておらず、全ネイティブ検証は実行していない。

## R2-G19b 保存領域ライター public API契約で完了した作業

- `libs/impex/kis_store_paintdevice_writer.h`の6 APIを
  `libs/impex/tests/kis_store_paintdevice_writer_test.cpp`の2試験関数へ対応付けた。メモリー上のZIP保存領域へ
  バイト列とポインター長の2形式を連続して書き、読戻し内容、格納先保持、未開始時の失敗、画像側の
  抽象ライター所有からの破棄を観測する。
- 専用対象は`kritaresourcestorage`とQt Testだけをリンクし、画像側は抽象ライターヘッダーと生成済み
  公開マクロの検索経路だけを使う。macOSの変更なし構築閉包は13工程・27入力である。製品実装、
  公開API、実行時分岐は変更していない。
- 公開API契約は480件、未対応基準は28,533件になった。macOSと`ssh nixos`上のLinuxで対象の構築、
  実行、20回反復、直接公開API契約検査が成功した。Linuxの変更なし構築閉包は13工程・31入力である。
  macOSの試験ソースのclang-format検査と`verify-quick`も成功した。全ネイティブ検証は実行していない。

## R2-G19b 入出力フィルター契約前の実装所有分離で完了した作業

- `libs/impex/KisImportExportFilter.cpp`に同居していた状態、進捗、設定、書き出し能力、保存結果検証を、
  公開クラスと共有ライブラリーを維持した内部所有単位へ分けた。状態と固定タグは同ファイルに残し、
  進捗と破棄時完了通知を`libs/impex/KisImportExportFilterProgress.cpp`、設定の既定値と保存値読込みを
  `libs/impex/KisImportExportFilterConfiguration.cpp`、能力検査の生成を
  `libs/impex/KisImportExportFilterCapabilities.cpp`、通常ファイルとZIPの保存結果検証を
  `libs/impex/KisImportExportFilterVerification.cpp`へ移した。非公開状態は同じ起点から
  `libs/impex/KisImportExportFilter_p.h`へ移した。
- 5実装単位を`kritaimpexfilterstateobjects`、`kritaimpexfilterprogressobjects`、
  `kritaimpexfilterconfigurationobjects`、`kritaimpexfiltercapabilityobjects`、
  `kritaimpexfilterverificationobjects`として個別構築し、従来どおり`kritaimpex`へ集約した。
  公開ヘッダーのコンパイル要件は内部の`kritaimpexfilterheaders`に集約し、メタオブジェクト生成は
  公開クラス名と一致する状態単位だけが担当する。
- macOSの変更なし構築閉包は状態が3工程・7入力、保存結果検証が2工程・5入力、進捗が134工程・
  296入力、設定と能力判定が各666工程・1,357入力になった。分割前の`kritaimpex`全体は
  1,025工程・2,072入力であり、状態と保存結果検証の契約追加は画像処理全体を構築しない。
- macOSと`ssh nixos`上のLinuxで5内部対象と`kritaimpex`の構築、既存
  `kis_import_export_filter_factory_test`が成功した。Linuxの変更なし構築閉包は状態が3工程・7入力、
  保存結果検証が2工程・5入力、進捗が127工程・294入力、設定と能力判定が各702工程・1,418入力である。
  macOSでは触れたC++のclang-format検査、直接公開API契約検査、`verify-quick`も成功した。公開API、
  ABI、設定キー、診断文、検査順序、実行時分岐は変更していない。全ネイティブ検証は実行していない。

## R2-G19b 入出力フィルター公開挙動の構築範囲縮小で完了した作業

- `libs/impex/KisImportExportFilterConfiguration.cpp`を起点として、既定設定と設定画面生成を同ファイルに
  残し、画像設定から保存済みXMLを読む処理を`libs/impex/KisImportExportFilterSavedConfiguration.cpp`へ
  移した。個別対象は`kritaimpexfilterconfigurationobjects`と
  `kritaimpexfiltersavedconfigurationobjects`である。
- `libs/impex/KisImportExportFilterCapabilities.cpp`を起点として、一般の能力一覧生成と所有を同ファイルに
  残し、全色モデルを走査して検査を生成する処理を`libs/impex/KisImportExportFilterColorModels.cpp`へ
  移した。個別対象は`kritaimpexfiltercapabilityobjects`と`kritaimpexfiltercolormodelobjects`である。
- `libs/impex/KisImportExportFilterVerification.cpp`を起点として、通常ファイルの存在、大きさ、先頭バイト
  検証を同ファイルに残し、ZIP構成要素の検証を`libs/impex/KisImportExportFilterZipVerification.cpp`へ
  移した。個別対象は`kritaimpexfilterverificationobjects`と
  `kritaimpexfilterzipverificationobjects`である。
- macOSの変更なし構築閉包は、既定設定、一般能力、通常ファイル検証が各1工程・3入力になった。
  保存済み設定と全色モデル能力は各666工程・1,357入力、ZIP検証は2工程・5入力である。軽量な
  public API契約は画像処理と保存領域を構築せず、実際にそれらを使う経路だけが対応所有者へ依存する。
- macOSで6内部対象と`kritaimpex`の構築、既存`kis_import_export_filter_factory_test`、触れたC++の
  clang-format検査が成功した。公開API、ABI、設定値、能力検査、診断文、実行時分岐は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 入出力フィルター public API契約で完了した作業

- `libs/impex/KisImportExportFilter.h`の23 APIを2つの専用CTestの8試験関数へ対応付けた。固定された
  能力タグ、入出力装置とガイドの既定対応、表示用・実体ファイル名、バッチ状態、MIME型、利用者通知、
  変換仮想入口、進捗更新と破棄時完了、空の既定設定と設定画面、能力検査の再生成と所有、通常ファイルの
  保存結果検証、形式別の保存済み設定を観測する。
- `libs/impex/tests/kis_import_export_filter_test.cpp`は画像ライブラリーをリンクせず、状態、進捗、既定設定、
  一般能力、通常ファイル検証の内部所有単位へ直接接続する。不存在、10バイト未満、先頭1,000バイトの
  全ゼロを拒否し、同範囲の非ゼロ値を受理する。変更なし構築閉包はmacOSで231工程・499入力である。
- `libs/impex/tests/kis_import_export_filter_saved_configuration_test.cpp`は試験用設定配置へ形式別XMLを保存し、
  既定値へ保存値を重ねる経路と未保存形式の既定値維持を実画像設定実装で検査する。変更なし構築閉包は
  macOSで1,016工程・2,055入力であり、画像設定を使うこの1 APIだけを重い対象へ分離した。
- 公開API契約は503件、未対応基準は28,510件になった。macOSと`ssh nixos`上のLinuxで2対象の構築、
  実行、各20回反復、直接公開API契約検査が成功した。Linuxの変更なし構築閉包は軽量対象が224工程・
  498入力、保存済み設定対象が1,060工程・2,133入力である。macOSの触れた試験ソースのclang-format
  検査と`verify-quick`も成功した。製品実装、公開API、実行時分岐は変更していない。全ネイティブ検証は
  実行していない。

## R2-G19b 利用者確認契約前の実装所有分離で完了した作業

- `libs/impex/ui/KisImportUserFeedbackInterface.cpp`と
  `libs/impex/ui/KisSynchronousImportUserFeedback.cpp`は、巨大な`kritaimpexui`ソース集合から
  `kritaimpexuserfeedbackobjects`へ所有対象を移した。ファイル配置、公開ヘッダー、公開識別子、
  `kritaapplicationui`のABIを維持し、`kritaimpexui`は新対象のオブジェクトを従来どおり集約する。
- 新対象はQt Widgetsだけを公開依存とし、生成済み`kritaui_export.h`の検索経路と既存の公開マクロを
  維持する。自動メタオブジェクト生成を使わない2ソースの変更なし構築閉包はmacOSで2工程・5入力である。
  従来の`kritaimpexui`は1,146工程・2,304入力、既存UI境界試験は1,698工程・3,395入力であり、
  利用者確認契約は文書、画像、描画、アプリケーションUI全体を構築せずに追加できる。
- macOSで新対象と集約後の`kritaimpexui`の限定構築が成功した。公開API、ABI、列挙値、バッチ抑止、
  コールバック実行、親表示部品の所有期間は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 利用者確認 public API契約で完了した作業

- `libs/impex/ui/KisImportUserFeedbackInterface.h`の9 APIと
  `libs/impex/ui/KisSynchronousImportUserFeedback.h`の3 APIを、
  `libs/impex/tests/kis_import_user_feedback_test.cpp`の3試験関数へ対応付けた。非バッチ時の
  親表示部品付き質問、肯定と取消しの結果分類、バッチ時、親なし、親破棄後の質問抑止、
  コールバック実行回数、抽象接続面からの破棄を観測する。
- 専用CTestは`kritaimpexuserfeedbackobjects`とQt Test、Qt Widgetsだけへ接続する。macOSの
  変更なし構築閉包は6工程・13入力で、新対象の実装所有閉包2工程・5入力を維持する。
- 公開API契約は515件、未対応基準は28,498件になった。macOSと`ssh nixos`上の
  Linuxで対象の構築、実行、20回反復、直接公開API契約検査が成功した。Linuxの
  CTestはこの表示部品試験に画面なし表示基盤を登録し、変更なし構築閉包は6工程・19入力である。
  製品実装、公開API、ABI、実行時分岐は変更していない。全ネイティブ検証は実行していない。

## R2-G19b アニメーション出力設定契約前の実装所有分離で完了した作業

- `libs/impex/animation/KisAnimationRenderingOptions.cpp`を起点として、初期値、パス解決、
  出力モード判定を同ファイルに残し、`loadLastUsed()`と`toProperties()`、
  `fromProperties()`を`libs/impex/animation/KisAnimationRenderingOptionsPersistence.cpp`へ移した。
  公開ヘッダー、クラス、メソッド、設定キー、実行順序は維持する。
- 前者を`kritaimpexanimationoptionsstateobjects`、後者を
  `kritaimpexanimationoptionspersistenceobjects`として個別構築し、従来どおり`kritaimpexui`へ
  集約した。設定値の状態対象は共通値とQt Core、保存対象だけが画像設定に依存する。
- macOSの変更なし構築閉包は状態対象が14工程・26入力、保存対象が666工程・
  1,357入力である。分離前の1実装オブジェクトは1,122工程・2,255入力へ到達していた。
  macOSで2対象と集約後の`kritaimpexui`の限定構築、触れたC++のclang-format検査が成功した。
  `ssh nixos`上のLinuxで2対象の構築が成功し、変更なし構築閉包は状態対象7工程・14入力、
  保存対象702工程・1,418入力である。製品の観測可能な挙動は変更していない。
  全ネイティブ検証は実行していない。

## R2-G19b アニメーション出力設定 public API契約で完了した作業

- `libs/impex/animation/KisAnimationRenderingOptions.h`の37 APIを、2つの専用CTestの
  6試験関数へ対応付けた。全公開値の既定値、フレームのみ・映像のみ・両方の出力モード、
  文書パスを基準にした映像ファイルとフレーム保存先解決を観測する。
- `libs/impex/tests/kis_animation_rendering_options_persistence_test.cpp`は、公開値とフレーム
  形式設定の保存・復元、不足キーの既定値、`ANIMATION_EXPORT`領域から最後に使用した
  設定の読込みを実画像設定で固定する。実行時の`scaleFilter`は保存値に含まれない。
- 映像のみモードの`resolveAbsoluteFramesDirectory(documentPath)`が明示引数を使わず、
  `lastDocumentPath`を使う現行挙動は既知不具合に分類した。大規模再編前の契約として、
  修正する場合には利用経路と期待パスの再確認を必要とする。
- 変更なし構築閉包はmacOSで軽量試験60工程・123入力、保存試験1,004工程・
  2,031入力である。保存試験だけが画像実装を構築し、他の34 APIは入出力UI全体を構築しない。
- 公開API契約は552件、未対応基準は28,461件になった。macOSと`ssh nixos`上の
  Linuxで2対象の構築、実行、各20回反復、直接公開API契約検査が成功した。macOSの
  触れたC++のclang-format検査も成功した。Linuxの変更なし構築閉包は軽量試験53工程・
  120入力、保存試験1,048工程・2,109入力である。全ネイティブ検証は実行していない。

## R2-G19b 遠隔ファイル取得契約前の実装所有分離で完了した作業

- `libs/impex/ui/KisRemoteFileFetcher.cpp`の実装所有を、`kritaimpexui`の一括ソース集合から
  `kritaimpexremotefilefetcherobjects`へ移した。ファイル配置、公開ヘッダー、クラス、メソッド、
  メタオブジェクト、`kritaapplicationui`のABIを維持し、`kritaimpexui`は新対象のオブジェクトを
  従来どおり集約する。
- 新対象はQt Network、Qt Widgets、翻訳、`kritaglobal`の共通メッセージ表示だけへ
  直接接続する。macOSの変更なし構築閉包は58工程・115入力で、分離前の単一実装オブジェクトが
  継承していた1,124工程・2,259入力から縮小した。
- macOSで新対象と集約後の`kritaimpexui`の限定構築が成功した。公開API、ABI、
  確認表示、応答待機、バイト列書込み、診断の実行時挙動は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b 遠隔ファイル取得 public API契約で完了した作業

- `libs/impex/ui/KisRemoteFileFetcher.h`の4 APIを
  `libs/impex/tests/kis_remote_file_fetcher_test.cpp`の3試験関数へ対応付けた。QObjectの
  親子所有による破棄、確認不要のデータURL取得、未開放出力装置の書込み用開放、
  全バイトの書込み、完了後の装置閉鎖を観測する。外部ネットワークは使用しない。
- 静的`fetchFile(remote)`が成功応答の本文を読まず空のバイト列を返す現行挙動は、
  既知不具合に分類した。大規模再編前の契約として、修正は静的経路の利用元とエラー応答の
  期待値を確認した独立変更で扱う。
- 変更なし構築閉包はmacOSで62工程・122入力である。公開API契約は556件、
  未対応基準は28,457件になった。macOSで対象の構築、実行、20回反復、触れたC++の
  clang-format検査が成功した。製品実装と外部サービスは変更していない。Linux、直接公開API契約検査、
  全ネイティブ検証は実行していない。

## R2-G19b 入出力補助契約前の実装所有分離で完了した作業

- `libs/impex/ui/KisImportExportUtils.cpp`の実装所有を、`kritaimpexui`の一括ソース集合から
  `kritaimpexutilsobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、名前空間、列挙値、構造体、関数、`kritaapplicationui`のABIを維持し、
  `kritaimpexui`は新対象のオブジェクトを従来どおり集約する。
- 新対象は入出力エラー、利用者確認、画像、色空間、Qt Widgetsの具体的所有者へ直接接続する。
  macOSの変更なし構築閉包は667工程・1,359入力であり、分離前に継承していた1,127工程・
  2,265入力から縮小した。残る閉包の大半は、公開関数が直接操作する`kritaimage`の998工程である。
- macOSで新対象と集約後の`kritaimpexui`の限定構築が成功した。製品実装と観測可能な挙動は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 入出力補助 public API契約で完了した作業

- `libs/impex/ui/KisImportExportUtils.h`の19 APIを2つの専用CTestの4試験関数へ対応付けた。
  保存方針と非同期保存開始結果の全公開列挙値、フラグ合成、書き出しジョブの既定値、
  保存先・形式・フラグの保持、保存先に基づく有効性判定を固定する。
- `libs/impex/tests/kis_import_export_utils_test.cpp`は、編集可能な出力プロファイルを持つ実画像で
  色空間補助関数が成功し、色空間と画像ロックを維持し、利用者確認と変換ダイアログを呼ばない
  無変換経路を観測する。
- 列挙値と書き出しジョブだけを扱う軽量試験の変更なし構築閉包はmacOSで4工程・14入力である。
  実画像試験は1,008工程・2,039入力であり、画像を直接操作する1関数だけが画像所有者を構築する。
- 公開API契約は575件、未対応基準は28,438件になった。macOSで2対象の構築、実行、各20回反復、
  直接公開API契約検査、触れたC++のclang-format検査が成功した。製品実装、公開API、ABI、実行時分岐は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b アニメーション寸法契約前の実装所有分離で完了した作業

- `libs/impex/animation/KisAnimationRender.cpp`を起点として、文書、表示、フレーム保存、映像符号化を
  調整する`render()`を同ファイルに残し、映像形式ごとの偶数寸法要否と縦横寸法の偶奇判定を
  `libs/impex/animation/KisAnimationRenderDimensions.cpp`へ移した。公開ヘッダー、名前空間、関数、
  呼出し順序は維持する。
- 寸法判定を`kritaimpexanimationrenderdimensionsobjects`として個別構築し、従来どおり
  `kritaimpexui`へ集約した。新対象は共通入出力ヘッダーとQt Coreだけへ接続する。
- macOSの変更なし構築閉包は1工程・3入力であり、分離前の描画実装オブジェクトが継承していた
  1,128工程・2,267入力から縮小した。macOSで新対象と集約後の`kritaimpexui`の限定構築が成功した。
  製品の観測可能な挙動は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b アニメーション寸法 public API契約で完了した作業

- `libs/impex/animation/KisAnimationRender.h`の純粋な寸法判定2 APIを、
  `libs/impex/tests/kis_animation_render_dimensions_test.cpp`の2データ駆動試験へ対応付けた。
  ゼロ、正負の偶数、片方または両方が奇数の寸法判定を観測する。
- MP4とMatroskaについて、フレームのみ、映像のみ、フレームと映像の両方という出力モードから
  偶数寸法の要否を固定し、他の映像形式では要求しないことを観測する。
- 変更なし構築閉包はmacOSで5工程・17入力である。公開API契約は577件、未対応基準は
  28,436件になった。macOSで対象の構築、実行、20回反復、直接公開API契約検査、触れたC++の
  clang-format検査が成功した。文書と表示を伴う`render()`の契約は未対応である。製品実装、公開API、
  ABI、判定結果は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 高速角度計算契約前の実装所有分離で完了した作業

- `libs/image/kis_fast_math.cpp`の実装所有を、`kritaimage`の一括ソース集合から
  `kritaimagefastmathobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、名前空間、関数、`kritaimage`のABIを維持し、`kritaimage`は新対象の
  オブジェクトを従来どおり集約する。
- 新対象はQt Coreだけへ直接接続する。macOSの変更なし構築閉包は1工程・3入力であり、既存の
  `kis_fast_math_test`が`kritaimage`全体から継承する1,002工程・2,027入力から分離した。
- macOSで新対象と集約後の`kritaimage`の限定構築が成功した。製品実装と角度計算結果は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 高速角度計算 public API契約で完了した作業

- `libs/image/kis_fast_math.h`の1 APIを、既存
  `libs/image/tests/kis_fast_math_test.cpp`の精度試験へ対応付けた。原点、4本の座標軸、全象限の
  代表値と、第1象限の100万座標で標準`atan2()`との絶対誤差を0.0001未満に固定する。
- 既存試験を画像用共通試験集合から独立させ、`kritaimagefastmathobjects`とQt Testだけへ直接接続した。
  変更なし構築閉包はmacOSで1,002工程・2,027入力から5工程・11入力へ縮小した。
- 公開API契約は578件、未対応基準は28,435件になった。macOSで対象の構築、実行、20回反復、
  直接公開API契約検査、触れたC++のclang-format検査が成功した。製品実装、公開API、ABI、近似精度は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b ダイアログ状態 public API契約で完了した作業

- `libs/widgetutils/KisDialogStateSaver.h`の保存関数だけが公開API棚卸しに現れ、復元関数が欠落していた。
  `QMap<QString, QVariant>()`という既定引数を同値の空リスト既定値へ正規化し、Universal Ctagsが
  テンプレート引数内のコンマを宣言区切りと誤認する採取漏れを解消した。ABIと既定値は変わらない。
- 公開API総数を29,013件から29,014件へ補正し、保存と復元の2 APIを既存
  `libs/widgetutils/tests/KisDialogStateSaverTest.cpp`へ対応付けた。名前付き表示部品の文字列、整数、
  実数、スライダー、チェック状態の保存・復元と、明示した既定値による保存値の上書きを観測する。
- 実装は同じ所有者の独自スピンボックス群を型別に扱うため、試験は`kritawidgetutils`へ直接接続する。
  変更なし構築閉包はmacOSで219工程・469入力である。アプリケーション、文書、画像の所有者は
  構築しないため追加の製品分割は行っていない。
- 公開API契約は580件、未対応基準は28,434件になった。macOSで対象の構築、実行、20回反復、
  直接公開API契約検査、触れたC++のclang-format検査が成功した。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b ファイル名補助契約前の実装所有分離で完了した作業

- `libs/global/KisFileUtils.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalfileutilsobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、名前空間、関数、`kritaglobal`のABIを維持し、`kritaglobal`は新対象の
  オブジェクトを従来どおり集約する。
- 重複名解析は既存正規表現の第3捕捉から接尾辞を直接取得する。先頭文字の除去だけに使っていた
  `KisPortingUtils.h`を外し、画面と表示部品への不要なコンパイル依存を除いた。既存の接尾辞、複数ドット、
  区切り文字を扱う結果は維持する。
- 新対象はQt Coreだけへ直接接続し、変更なし構築閉包はmacOSで1工程・3入力である。分離前の
  `KisGlobalTest`は59工程・116入力だった。macOSで新対象、集約後の`kritaglobal`の限定構築、既存
  `KisGlobalTest`が成功した。Linuxと全ネイティブ検証は実行していない。

## R2-G19b ファイル名補助 public API契約で完了した作業

- `libs/global/KisFileUtils.h`の2 APIを、`libs/global/tests/KisGlobalTest.cpp`へ対応付けた。
  重複名生成は競合なし、複数競合、接尾辞なし、既存番号、複数ドット、指定区切り文字を固定し、
  パス解決は絶対パス、基準ディレクトリー、基準ファイルからの相対パスを固定する。
- 既存試験を`kritaglobal`と試験支援ライブラリーへ接続する共通試験集合から独立させ、
  `kritaglobalfileutilsobjects`とQt Testだけへ直接接続した。変更なし構築閉包はmacOSで
  59工程・116入力から5工程・11入力へ縮小した。
- 公開API契約は582件、未対応基準は28,432件になった。macOSで対象の構築、実行、20回反復が
  成功した。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 数式解析契約前の実装所有分離で完了した作業

- `libs/widgetutils/kis_num_parser.cpp`の実装所有を、`kritawidgetutils`の一括ソース集合から
  `kritawidgetutilsnumericparserobjects`へ移した。起点と移動先のファイルは同じで、CMake上の
  所有対象だけを変更した。公開ヘッダー、名前空間、関数、`kritawidgetutils`のABIを維持し、
  `kritawidgetutils`は新対象のオブジェクトを従来どおり集約する。
- 新対象はQt Coreだけへ直接接続し、変更なし構築閉包はmacOSで1工程・3入力である。既存の
  `kis_simple_math_parser_test`は1,002工程・2,027入力だった。
- macOSで新対象、集約後の`kritawidgetutils`の限定構築、既存の数式解析試験が成功した。
  製品実装、公開API、ABI、式評価結果は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 数式解析 public API契約で完了した作業

- `libs/widgetutils/kis_num_parser.h`の2 APIを、既存
  `libs/widgetutils/tests/kis_simple_math_parser_test.cpp`へ対応付けた。浮動小数点式は四則演算、累乗、
  括弧、三角関数、指数・対数を固定し、整数式は四則演算と丸めを固定する。両関数について成功と
  失敗の通知も観測する。
- 既存試験を`kritawidgetutils`、`kritaimage`、試験支援ライブラリーへ接続する共通試験集合から独立させ、
  `kritawidgetutilsnumericparserobjects`とQt Testだけへ直接接続した。変更なし構築閉包はmacOSで
  1,002工程・2,027入力から5工程・11入力へ縮小した。
- 公開API契約は584件、未対応基準は28,430件になった。macOSで対象の構築、実行、20回反復が
  成功した。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 変更状態復元 public API契約で完了した作業

- `libs/resources/KisDirtyStateSaver.h`のクラス、構築、破棄の3 APIを、新規
  `libs/resources/tests/KisDirtyStateSaverTest.cpp`へ対応付けた。構築時に保存した清浄状態と変更済み状態を、
  保護範囲内で反転した後も破棄時に元へ復元することを固定する。
- 汎用テンプレートが使用していなかった`KoResource.h`と生成エクスポートヘッダーのincludeを外した。
  利用者が渡す型だけが`isDirty()`と`setDirty()`を提供する責務になり、公開テンプレートから資源実装への
  不要なコンパイル依存を除いた。製品利用元は必要な具象型を自身のヘッダーから取得する。
- 新試験はQt Testだけへ接続し、変更なし構築閉包はmacOSで4工程・8入力である。同じCMake集合の
  既存資源試験は629工程・1,286入力であり、その集合へは追加していない。
- 公開API契約は587件、未対応基準は28,427件になった。macOSで対象の構築、実行、20回反復が
  成功した。公開API、ABI、状態復元結果は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 完了済みfuture public API契約で完了した作業

- `libs/global/KisFutureUtils.h`の3 APIを、新規`libs/global/tests/KisFutureUtilsTest.cpp`へ対応付けた。
  値とvoidの完了済みfuture、入力futureから値を受け取る遅延後続処理を固定する。
- 公開ヘッダー自身が使用する`future`、型特性、転送機能の標準ヘッダーを直接includeした。無引数の
  ヘッダー内関数を`inline`にし、複数翻訳単位から安全に利用できるようにした。値版のpromiseは
  戻り値と同じ減衰型を所有し、左辺値を渡した場合も参照promiseとの型不一致なく構築できる。
- 新試験はQt Testだけへ接続し、対象ヘッダーの親ディレクトリーだけを明示する。初回構築は暗黙include
  経路を持たないため対象ヘッダーを発見できず失敗し、明示後の変更なし構築閉包はmacOSで4工程・8入力に
  なった。
- 公開API契約は590件、未対応基準は28,424件になった。macOSで対象の構築、実行、20回反復が
  成功した。既存の右辺値利用結果と公開識別子は維持し、左辺値利用を成立させた。Linuxと全ネイティブ
  検証は実行していない。

## R2-G19b 適合ロック public API契約で完了した作業

- `libs/global/KisAdaptedLock.h`のクラスと6構築・移動API、計7 APIを、既存
  `libs/global/tests/KisAdaptedLockTest.cpp`へ対応付けた。通常、試行、遅延、既存ロック引受の各構築と、
  移動構築・移動代入について、所有状態、取得・解放回数、移動元の非所有化を固定する。
- 既存試験を`kritaglobal`と試験支援ライブラリーへ接続する共通試験集合から独立させ、Qt Testと
  対象ヘッダーの親ディレクトリーだけへ直接接続した。変更なし構築閉包はmacOSで59工程・117入力から
  4工程・8入力へ縮小した。
- 公開API契約は597件、未対応基準は28,417件になった。macOSで対象の構築、実行、20回反復が
  成功した。製品実装、公開API、ロック動作は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 遅延値キャッシュ public API契約で完了した作業

- `libs/global/KisValueCache.h`の構造体、値型、構築、参照、有効性、消去、変換の7 APIを、既存
  `libs/global/tests/KisValueCacheTest.cpp`へ対応付けた。初回参照での生成、値の再利用、消去後の再生成、
  const個体からの参照変換を固定する。
- 変換演算子の初回実体化は、const個体から非constの`value()`を呼ぶ宣言不整合でコンパイルに失敗した。
  キャッシュ値を内部可変状態とし、`value()`を論理constへ揃えた。非constの初期化方針は初回生成時だけ
  内部状態として呼び出す。`declval`の宣言元である標準ヘッダーも公開ヘッダーから直接includeした。
- 既存試験を`kritaglobal`と試験支援ライブラリーへ接続する共通試験集合から独立させ、Qt Testと
  対象ヘッダーの親ディレクトリーだけへ直接接続した。変更なし構築閉包はmacOSで59工程・117入力から
  4工程・8入力へ縮小した。
- 公開API総数29,014件を維持し、`value()`のconst修飾を反映して公開面指紋を更新した。契約は604件、
  未対応基準は28,410件になった。macOSで対象の構築、実行、20回反復が成功した。ヘッダー内テンプレートの
  ためABIは変わらず、既存の非const利用結果を維持してconst変換を成立させた。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 通知接続寿命 public API契約で完了した作業

- `libs/global/kis_signal_auto_connection.h`の接続所有、共有所有別名、複数接続格納、追加、重複防止、
  一括解除、空状態の9 APIを、既存`libs/global/tests/KisSignalAutoConnectionTest.cpp`へ対応付けた。
  直接所有の破棄後に通知が停止する寿命契約、格納先の空・所有・解除状態、同一接続の重複防止を追加した。
- 既存試験を`kritaglobal`と試験支援ライブラリーへ接続する共通試験集合から独立させ、Qt Testと
  対象ヘッダーの親ディレクトリーだけへ直接接続した。変更なし構築閉包はmacOSで59工程・117入力から
  4工程・8入力へ縮小した。
- 公開API契約は613件、未対応基準は28,401件になった。macOSで対象の構築、実行、20回反復が
  成功した。製品実装、公開API、接続配送と解除結果は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 遅延格納 public API契約で完了した作業

- `libs/global/KisLazyStorage.h`のクラス、即時値タグ、構築・移動・コピー禁止、破棄、間接参照の
  12 APIを、既存`libs/global/tests/KisLazyStorageTest.cpp`へ対応付けた。初回参照まで生成しないこと、
  一度だけの生成と破棄、即時値、矢印・間接参照、移動構築・代入、コピー不可の型特性を固定する。
- 移動構築はC++17で既定初期化された原子ポインターを移動代入前に読み出す可能性があったため、
  メンバー宣言でnull初期化した。公開ヘッダーは使用する原子操作、排他、タプル、転送の標準ヘッダーを
  直接includeし、未使用の標準ヘッダーと`KisMpl.h`へのコンパイル依存を外した。
- 既存試験を`kritaglobal`と試験支援ライブラリーへ接続する共通試験集合から独立させ、Qt Testと
  対象ヘッダーの親ディレクトリーだけへ直接接続した。変更なし構築閉包はmacOSで59工程・117入力から
  4工程・8入力へ縮小した。
- 公開API契約は625件、未対応基準は28,389件になった。macOSで対象の構築、実行、20回反復が
  成功した。公開APIと既存の遅延・即時値結果を維持し、移動時の未初期化読み出しを除いた。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b 累積取り消し設定契約前の実装所有分離で完了した作業

- `libs/global/KisCumulativeUndoData.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalcumulativeundodataobjects`へ移した。起点と移動先のファイルは同じで、CMake上の
  所有対象だけを変更した。公開ヘッダー、構造体、関数、`kritaglobal`のAPIとABIを維持し、
  `kritaglobal`は新対象のオブジェクトを従来どおり集約する。
- 新対象はQt CoreとKConfig Coreだけへ直接接続する。変更なし構築閉包はmacOSで1工程・3入力であり、
  既存の`KisConfigurationValueTypesTest`が`kritaglobal`と試験支援ライブラリーから継承する
  59工程・117入力から分離した。
- macOSで新対象の限定構築が成功した。製品実装、公開API、ABI、設定の読書き結果は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 累積取り消し設定 public API契約で完了した作業

- `libs/global/KisCumulativeUndoData.h`の構造体、既定個体、4公開項目、等価比較、設定の読書き、
  診断出力の10 APIを、新規`libs/global/tests/KisCumulativeUndoDataTest.cpp`の3試験へ対応付けた。
  既定値と等価性、4設定キーの往復、型名・項目名・現在値を含む診断文字列を観測する。
- 累積取り消し設定の試験を`libs/global/tests/KisConfigurationValueTypesTest.cpp`から
  `libs/global/tests/KisCumulativeUndoDataTest.cpp`へ移し、既存対象には一時ファイル配置の試験だけを
  残した。CMake所有は`kritaglobal`と試験支援ライブラリーへ接続する共通試験集合から、
  `kritaglobalcumulativeundodataobjects`、Qt Test、KConfig Coreへ直接接続する専用対象へ移した。
- 変更なし構築閉包はmacOSで59工程・117入力から5工程・12入力へ縮小した。初回試験は`QDebug`の
  破棄前に出力文字列を読む試験側の寿命誤りを検出し、出力器を破棄してから既存書式を観測するように
  修正した。対象実行と20回反復が成功し、公開API契約は635件、未対応基準は28,379件になった。
  製品実装、公開API、ABI、設定値と診断結果は変更していない。Linuxと全ネイティブ検証は実行していない。
- 変更した既存試験の限定実行は、macOSで`/var/folders`配下のswap設定を意図的に正規化する製品方針と、
  同じ配下に一時設定先を作る試験条件の矛盾を検出した。保持する設定先の試験だけをホーム配下の
  自動削除一時ディレクトリーへ置き、製品方針を変えずに対象実行と20回反復が成功した。

## R2-G19b 一時ファイル配置契約前の実装所有分離で完了した作業

- `libs/global/KisTemporaryFileConfiguration.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobaltemporaryfileconfigurationobjects`へ移した。起点と移動先のファイルは同じで、CMake上の
  所有対象だけを変更した。公開ヘッダー、名前空間、関数、`kritaglobal`のAPIとABIを維持し、
  `kritaglobal`は新対象のオブジェクトを従来どおり集約する。
- 新対象はQt CoreとKConfig Coreへ直接接続し、macOSだけはサンドボックス判定の実所有者である
  `kritamacosutils`へ接続する。`KisUsageLogger.cpp`も同じ所有者を利用するため、`kritaglobal`自身の
  macOS接続は維持する。
- 変更なし構築閉包はmacOSで2工程・5入力であり、既存の`KisConfigurationValueTypesTest`が
  `kritaglobal`と試験支援ライブラリーから継承する59工程・117入力から縮小した。macOSで新対象の
  限定構築が成功した。製品実装、公開API、ABI、配置選択結果は変更していない。Linuxと全ネイティブ
  検証は実行していない。

## R2-G19b 一時ファイル配置 public API契約で完了した作業

- `libs/global/KisTemporaryFileConfiguration.h`の3 APIを、
  `libs/global/tests/KisTemporaryFileConfigurationTest.cpp`の3試験へ対応付けた。設定済みの書込み可能な
  配置、既定配置、書込み不能時の代替配置、macOSでの一時swap設定の除去と他構成での保持を観測する。
- `libs/global/tests/KisConfigurationValueTypesTest.cpp`の残存試験を
  `libs/global/tests/KisTemporaryFileConfigurationTest.cpp`へ移し、旧ファイルと旧CTest対象を除去した。
  CMake所有は`kritaglobal`と試験支援ライブラリーへ接続する共通試験集合から、
  `kritaglobaltemporaryfileconfigurationobjects`、Qt Test、KConfig Coreへ直接接続する専用対象へ移した。
- 変更なし構築閉包はmacOSで59工程・117入力から12工程・24入力へ縮小した。macOSで対象実行と
  20回反復が成功し、旧CTest対象が消滅して新対象だけが登録されることを確認した。公開API契約は
  638件、未対応基準は28,376件になった。製品実装、公開API、ABI、配置選択結果は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b ノード追加通知フラグ public API契約で完了した作業

- `libs/global/KisNodeAdditionFlags.h`の列挙型、空値、通知抑止値の3 APIを、新規
  `libs/global/tests/KisNodeAdditionFlagsTest.cpp`へ対応付けた。数値値、既定の空フラグ、通知抑止値の
  結合と判定を観測する。既存`libs/painting/tests/TestPublicImageHeaders.cpp`はヘッダーのコンパイル
  可能性だけを扱うため、挙動契約には数えない。
- 製品実装を持たないヘッダー内宣言であり、Qt Testと対象ヘッダーの親ディレクトリーだけへ接続する。
  変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が成功し、公開API契約は
  641件、未対応基準は28,373件になった。製品実装、公開API、フラグ値は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b 通知遮断範囲 public API契約で完了した作業

- `libs/global/kis_signals_blocker.h`のクラス、単一・複数個体の構築、破棄の4 APIを、新規
  `libs/global/tests/KisSignalsBlockerTest.cpp`の2試験へ対応付けた。単一個体の遮断済み・未遮断状態と、
  6個体が持つ異なる事前状態について、保護範囲内の一括遮断と破棄時の個別復元を観測する。
- 製品実装を持たないヘッダー内宣言であり、Qt Testと対象ヘッダーの親ディレクトリーだけへ接続する。
  変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が成功し、公開API契約は
  645件、未対応基準は28,369件になった。製品実装、公開API、通知状態は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b Qt版差吸収補助 public API契約で完了した作業

- `libs/global/KisPortingUtils.h`のUTF-8設定、文字列の先頭・末尾除去、表示部品の画面番号取得の4 APIを、
  新規`libs/global/tests/KisPortingUtilsTest.cpp`の3試験へ対応付けた。日本語のUTF-8バイト列、通常文字列と
  空文字列の除去結果、表示部品を指定しない場合の主画面番号を観測する。
- 製品実装を持たないヘッダー内宣言であり、Qt Test、Qt Widgets、対象ヘッダーの親ディレクトリーだけへ
  接続する。変更なし構築閉包はmacOSで4工程・8入力である。対象実行と表示環境を含む20回反復が成功し、
  公開API契約は649件、未対応基準は28,365件になった。製品実装、公開API、変換結果は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 描画器状態保存契約前の実装所有分離で完了した作業

- `libs/global/KisQPainterStateSaver.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalqpainterstatesaverobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、クラス、構築・破棄、`kritaglobal`のAPIとABIを維持し、`kritaglobal`は新対象の
  オブジェクトを従来どおり集約する。
- 実装の未解決記号は`QPainter::save()`と`restore()`だけであり、新対象はQt Guiだけへ直接接続する。
  変更なし構築閉包はmacOSで1工程・3入力である。macOSで新対象の限定構築が成功した。製品実装、
  公開API、ABI、描画状態の保存・復元順序は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 描画器状態保存 public API契約で完了した作業

- `libs/global/KisQPainterStateSaver.h`のクラス、構築、破棄の3 APIを、新規
  `libs/global/tests/KisQPainterStateSaverTest.cpp`へ対応付けた。`QImage`上の描画器について、保護範囲内で
  変更した変換、透明度、合成方式、ペン、ブラシ、クリップが破棄時に元の状態へ戻ることを観測する。
- 新試験は`kritaglobalqpainterstatesaverobjects`、Qt Test、Qt Guiだけへ直接接続する。変更なし構築閉包は
  macOSで5工程・11入力である。対象実行と20回反復が成功し、公開API契約は652件、未対応基準は
  28,362件になった。製品実装、公開API、ABI、描画状態は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 半精度数値分類 public API契約で完了した作業

- `libs/global/KisHalfTraits.h`の有限性、無限性、NaN、正規化、符号を判定する5 APIを、新規
  `libs/global/tests/KisHalfTraitsTest.cpp`へ対応付けた。半精度の通常値、零、負数、無限大、NaNを用いて、
  各分類の真偽を観測する。
- 製品実装を持たないOpenEXR有効時のヘッダー内宣言であり、OpenEXRが見つかった構成だけQt Testと
  OpenEXRへ接続する専用CTest対象を生成する。変更なし構築閉包はmacOSで4工程・13入力である。
  対象実行と20回反復が成功し、公開API契約は657件、未対応基準は28,357件になった。製品実装、
  公開API、分類結果は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b const型変換とoptional診断 public API契約で完了した作業

- `libs/global/KisCppQuirks.h`の条件付きconst付与・複製を表す2構造体と4型別名、optional診断出力の
  計7 APIを、新規`libs/global/tests/KisCppQuirksTest.cpp`の2試験へ対応付けた。constの付与と除去を
  構造体経由・短縮型名の両方で検査し、値あり・空optionalの診断文字列を観測する。
- 初回構築は、Qt 6.7以降が提供するoptional用`QDebug`演算子とLibrePaintの同じ演算子が通常の
  出力式で曖昧になる公開ヘッダー欠陥を検出した。LibrePaintの補完をQt 6.7未満だけに限定し、
  Qt 6.7以降はQtの標準演算子へ委ねた。古いQtでは既存の値・空状態表現を維持する。
- 新試験はQt Testと対象ヘッダーの親ディレクトリーだけへ直接接続し、変更なし構築閉包はmacOSで
  4工程・8入力である。修正後の対象実行と20回反復が成功し、公開API契約は664件、未対応基準は
  28,350件になった。型変換結果とQt 6.7未満の公開補完を維持し、Qt 6.7以降の曖昧な呼出しを解消した。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b GUIスレッド破棄契約前の実装所有分離で完了した作業

- `libs/global/KisDeleteLaterWrapper.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobaldeletelaterwrapperobjects`へ移した。起点と移動先のファイルは同じで、CMake上の
  所有対象だけを変更した。公開ヘッダー、クラス、構築・破棄、`kritaglobal`のAPIとABIを維持し、
  `kritaglobal`は新対象のオブジェクトを従来どおり集約する。
- 実装が利用するアプリケーション個体とスレッド所属の所有者であるQt Widgetsだけへ新対象を
  直接接続した。変更なし構築閉包はmacOSで1工程・3入力である。macOSで新対象の限定構築が成功した。
  製品実装、公開API、ABI、GUIスレッドへの移動結果は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b GUIスレッド破棄 public API契約で完了した作業

- `libs/global/KisDeleteLaterWrapper.h`のクラス、GUIスレッド移動、値・ポインター構築、生成補助、破棄の
  6 APIを、新規`libs/global/tests/KisDeleteLaterWrapperTest.cpp`の3試験へ対応付けた。ワーカーで生成した
  通常個体とラッパーのGUIスレッド所属、およびポインターラッパーの遅延破棄による所有値の破棄を観測する。
- 新試験は`kritaglobaldeletelaterwrapperobjects`、Qt Test、Qt Widgetsだけへ直接接続する。変更なし
  構築閉包はmacOSで5工程・11入力である。初回実行は試験後処理のスレッド停止待ちを`Q_ASSERT`式内に
  置いたためリリース構成で待機自体が除去される試験欠陥を検出し、待機を無条件実行するよう修正した。
- 修正後の対象実行と20回反復が成功し、公開API契約は670件、未対応基準は28,344件になった。
  製品実装、公開API、ABI、スレッド所属と所有値の破棄結果は変更していない。Linuxと全ネイティブ検証は
  実行していない。`ssh nixos`の到達性確認は鍵応答待ちのまま10秒で終了した。

## R2-G19b バックアップファイル契約前の実装所有分離で完了した作業

- `libs/global/KisBackup.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalbackupobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、クラス、関数、`kritaglobal`のAPIとABIを維持し、`kritaglobal`は新対象の
  オブジェクトを従来どおり集約する。
- 新対象はファイルとディレクトリー操作の所有者であるQt Coreだけへ直接接続する。変更なし構築閉包は
  macOSで1工程・3入力であり、従来の`kritaglobal`集約範囲55工程・110入力から分離した。macOSで
  新対象の限定構築が成功した。製品実装、公開API、ABI、バックアップ結果は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b バックアップファイル public API契約で完了した作業

- `libs/global/KisBackup.h`のクラスと既定・単純・番号付きバックアップの4 APIを、新規
  `libs/global/tests/KisBackupTest.cpp`の3試験へ対応付けた。既定接尾辞にある既存内容の置換、任意の
  保存先と接尾辞への複製、番号付き履歴の新旧順回転と上限外除去を一時ディレクトリー上で観測する。
- 新試験は`kritaglobalbackupobjects`、Qt Test、Qt Coreだけへ直接接続する。変更なし構築閉包は
  macOSで5工程・11入力である。対象実行と20回反復が成功し、公開API契約は674件、未対応基準は
  28,340件になった。製品実装、公開API、ABI、バックアップ内容と命名結果は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b カーソル上書き契約前の実装所有分離で完了した作業

- `libs/global/KisCursorOverrideLock.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalcursoroverridelockobjects`へ移した。起点と移動先のファイルは同じで、CMake上の
  所有対象だけを変更した。公開ヘッダー、クラス、関数、`kritaglobal`のAPIとABIを維持し、
  `kritaglobal`は新対象のオブジェクトを従来どおり集約する。
- 新対象はカーソルとGUIアプリケーションの所有者であるQt Guiだけへ直接接続する。変更なし構築閉包は
  macOSで1工程・3入力である。macOSで新対象の限定構築が成功した。製品実装、公開API、ABI、
  カーソル上書き結果は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b カーソル上書き public API契約で完了した作業

- `libs/global/KisCursorOverrideLock.h`のアダプタークラス、構築、破棄、施錠、解錠の5 APIを、新規
  `libs/global/tests/KisCursorOverrideLockTest.cpp`の2試験へ対応付けた。未施錠での構築・破棄の
  無副作用と、指定カーソルの積み重ねおよび直前カーソルの復元をGUIアプリケーション上で観測する。
- 新試験は`kritaglobalcursoroverridelockobjects`、Qt Test、Qt Guiだけへ直接接続する。変更なし構築閉包は
  macOSで5工程・11入力である。初回構築は試験の丸括弧初期化を関数宣言として解釈して失敗し、
  波括弧初期化で個体構築を明示した。
- 修正後の対象実行と20回反復が成功し、公開API契約は679件、未対応基準は28,335件になった。
  製品実装、公開API、ABI、カーソルの積み重ね結果は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 外れ値除外移動平均契約前の実装所有分離で完了した作業

- `libs/global/KisFilteredRollingMean.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalfilteredrollingmeanobjects`へ移した。起点と移動先のファイルは同じで、CMake上の
  所有対象だけを変更した。公開ヘッダー、クラス、関数、`kritaglobal`のAPIとABIを維持し、
  `kritaglobal`は新対象のオブジェクトを従来どおり集約する。
- 新対象は数値型の所有者であるQt Coreと、循環バッファを提供するヘッダー専用Boostだけへ直接接続する。
  異常時の安全表明診断は従来どおり`kritaglobal`の集約内で解決する。変更なし構築閉包はmacOSで
  1工程・3入力である。macOSで新対象の限定構築が成功した。製品実装、公開API、ABI、平均値は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 外れ値除外移動平均 public API契約で完了した作業

- `libs/global/KisFilteredRollingMean.h`のクラス、構築、値追加、平均取得、空判定の5 APIを、新規
  `libs/global/tests/KisFilteredRollingMeanTest.cpp`の2試験へ対応付けた。空から非空への遷移、通常平均、
  上下の外れ値除外、および窓を越えた最古値の除去を固定値列で観測する。
- 正常計算をGUI診断実装へ接続せず検査するため、試験内の安全表明代替は呼出し回数だけを記録し、
  各計算後に0回であることを検査する。新試験は`kritaglobalfilteredrollingmeanobjects`、Qt Test、Qt Core、
  ヘッダー専用Boostだけへ直接接続し、変更なし構築閉包はmacOSで5工程・11入力である。
- 対象実行と20回反復が成功し、公開API契約は684件、未対応基準は28,330件になった。製品実装、
  公開API、ABI、平均値と窓更新結果は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 共有所有基底契約前の実装所有分離で完了した作業

- `libs/global/kis_shared.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalsharedobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、クラス、関数、`kritaglobal`のAPIとABIを維持し、`kritaglobal`は新対象の
  オブジェクトを従来どおり集約する。
- 新対象は原子的参照数の所有者であるQt Coreだけへ直接接続する。変更なし構築閉包はmacOSで
  1工程・3入力であり、既存の画像共有ポインター試験が持つ1,002工程・2,027入力から基底寿命契約を
  分離した。macOSで新対象の限定構築が成功した。製品実装、公開API、ABI、参照数と弱参照標識は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 共有所有基底 public API契約で完了した作業

- `libs/global/kis_shared.h`のクラス、参照、解除、参照数、弱参照標識の5 APIを、新規
  `libs/global/tests/KisSharedTest.cpp`の2試験へ対応付けた。試験用派生型を用いて参照数の
  0→1→2→1→0遷移、各操作の戻り値、弱参照標識の遅延生成と再利用を観測する。
- 新試験は`kritaglobalsharedobjects`、Qt Test、Qt Coreだけへ直接接続する。変更なし構築閉包は
  macOSで5工程・11入力であり、既存画像共有ポインター試験の1,002工程・2,027入力から縮小した。
  対象実行と20回反復が成功し、公開API契約は689件、未対応基準は28,325件になった。
  製品実装、公開API、ABI、参照数と弱参照標識は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 移動合計契約前の実装所有分離で完了した作業

- `libs/global/KisRollingSumAccumulatorWrapper.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalrollingsumobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、クラス、関数、`kritaglobal`のAPIとABIを維持し、`kritaglobal`は新対象の
  オブジェクトを従来どおり集約する。
- 新対象は数値型と所有ポインターのQt Core、および集計器を提供するヘッダー専用Boostだけへ直接接続する。
  変更なし構築閉包はmacOSで1工程・3入力である。macOSで新対象の限定構築が成功した。製品実装、
  公開API、ABI、移動合計と件数は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 移動合計 public API契約で完了した作業

- `libs/global/KisRollingSumAccumulatorWrapper.h`のクラス、構築、破棄、値追加、合計、件数、再設定の
  7 APIを、新規`libs/global/tests/KisRollingSumAccumulatorWrapperTest.cpp`の2試験へ対応付けた。
  初期状態、窓を越えた最古値の除去、現在件数と合計、再設定後の空状態と新しい窓幅を固定値列で観測する。
- 新試験は`kritaglobalrollingsumobjects`、Qt Test、Qt Core、ヘッダー専用Boostだけへ直接接続する。
  変更なし構築閉包はmacOSで5工程・11入力である。対象実行と20回反復が成功し、公開API契約は
  696件、未対応基準は28,318件になった。製品実装、公開API、ABI、移動合計と件数は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 移動平均契約前の実装所有分離で完了した作業

- `libs/global/KisRollingMeanAccumulatorWrapper.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalrollingmeanobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、クラス、関数、`kritaglobal`のAPIとABIを維持し、`kritaglobal`は新対象の
  オブジェクトを従来どおり集約する。
- 新対象は数値型と所有ポインターのQt Core、および集計器を提供するヘッダー専用Boostだけへ直接接続する。
  変更なし構築閉包はmacOSで1工程・3入力である。macOSで新対象の限定構築が成功した。製品実装、
  公開API、ABI、移動平均と件数は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 移動平均 public API契約で完了した作業

- `libs/global/KisRollingMeanAccumulatorWrapper.h`のクラス、構築、破棄、値追加、通常平均、安全平均、
  件数、再設定の8 APIを、新規`libs/global/tests/KisRollingMeanAccumulatorWrapperTest.cpp`の2試験へ
  対応付けた。空窓のNaNと0の差、窓を越えた最古値の除去、現在件数と平均、再設定後の空状態と
  新しい窓幅を固定値列で観測する。
- 新試験は`kritaglobalrollingmeanobjects`、Qt Test、Qt Core、ヘッダー専用Boostだけへ直接接続する。
  変更なし構築閉包はmacOSで5工程・11入力である。対象実行と20回反復が成功し、公開API契約は
  704件、未対応基準は28,310件になった。製品実装、公開API、ABI、移動平均と件数は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b ピン留め共有所有 public API契約で完了した作業

- `libs/global/kis_pinned_shared_ptr.h`のクラス、既定構築、生ポインター・強参照・弱参照からの構築、
  否定演算、制限付き真偽変換、デバッグ出力の8 APIを、新規
  `libs/global/tests/KisPinnedSharedPtrTest.cpp`の3試験へ対応付けた。空状態の真偽、同一所有個体と
  参照数、最後の強参照後の破棄、保持アドレスの診断表現を観測し、生ポインターへの暗黙変換が
  成立しないことをコンパイル時に固定する。
- 対象はヘッダーだけで完結し、既に分離済みの`kritaglobalsharedobjects`を再利用するため、製品実装の
  所有分離は不要だった。新試験は同対象、Qt Test、Qt Coreだけへ直接接続し、変更なし構築閉包は
  macOSで5工程・11入力である。従来の画像部品内試験は1,002工程・2,027入力だった。対象実行と
  20回反復が成功し、公開API契約は712件、未対応基準は28,302件になった。製品実装、公開API、
  ABI、共有所有の参照規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 即時値ラッパー public API契約で完了した作業

- `libs/global/KisLazyValueWrapper.h`の構造体、値型、公開値、既定・生成関数・ムーブ構築、破棄、
  const参照変換、コピー・ムーブ代入の11 APIを、新規`libs/global/tests/KisLazyValueWrapperTest.cpp`の
  3試験へ対応付けた。既定値、コピー禁止、生成関数の一度実行、格納値への同一参照、単独所有値の
  ムーブ、ラッパー破棄時の格納値破棄を観測する。
- 対象は標準ライブラリだけで完結するヘッダーであり、製品実装の所有分離は不要だった。新試験は
  Qt Testだけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が
  成功し、公開API契約は723件、未対応基準は28,291件になった。製品実装、公開API、ABI、値の
  構築・移動・破棄規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 読み書きロック方針 public API契約で完了した作業

- `libs/global/KisReadWriteLockPolicy.h`の通常・昇格・無ロック方針、各ロッカー型、無操作ロッカーの
  構築を構成する13 APIを、新規`libs/global/tests/KisReadWriteLockPolicyTest.cpp`の3試験へ対応付けた。
  通常ロッカーが反対モードの取得を遮断すること、昇格方針の読み側と無ロック方針がロックを取得
  しないこと、および各方針が公開する具体型と非コピー性を観測する。
- 対象はヘッダーだけで完結するため、製品実装の所有分離は不要だった。新試験はQt Testと
  ヘッダー専用Boostだけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。対象実行と
  20回反復が成功し、公開API契約は736件、未対応基準は28,278件になった。製品実装、公開API、
  ABI、ロック取得規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b ロックなしスタック public API契約で完了した作業

- `libs/global/kis_lockless_stack.h`のクラス、構築、破棄、追加、取得、消去、結合、概算件数、空判定の
  9 APIを、新規`libs/global/tests/KisLocklessStackContractTest.cpp`の4契約試験へ対応付けた。新規空状態、
  空取得時の出力保持、後入れ先出し順、全消去、元スタックを空にする鎖結合、未取得値の破棄時解放を
  観測する。追加の並行試験は、4生成スレッドと4取得スレッド間で2,000値の欠落と重複がないことを
  観測する。
- 従来の`libs/image/tiles3/tests/kis_lockless_stack_test.cpp`は`kritaimage`全体に接続され、変更なし構築
  閉包が1,002工程・2,027入力だった。対象はヘッダーだけで完結するため製品実装は変更せず、新試験を
  Qt Testと標準スレッドだけへ直接接続して4工程・8入力に縮小した。対象実行と20回反復が成功し、
  公開API契約は745件、未対応基準は28,269件になった。製品実装、公開API、ABI、スタック順序と
  並行アクセス規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 状態レンズ変換 public API契約で完了した作業

- `libs/global/KisLager.h`の任意reader畳み込みと、倍率3種、静的型変換、基底型2種、`QVariant`、
  論理反転を扱う9 APIを、新規`libs/global/tests/KisLagerContractTest.cpp`の4試験へ対応付けた。
  存在するreaderだけの畳み込みと全欠落、各レンズの読み出しと書き戻し、整数丸め、および基底部分の
  更新後も派生型固有状態を保持することを固定値で観測する。
- 対象と`lager`はヘッダーだけで完結するため、製品実装の所有分離は不要だった。新試験はQt Testと
  `lager`だけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が
  成功し、公開API契約は754件、未対応基準は28,260件になった。製品実装、公開API、ABI、状態変換
  規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 矩形サンプラー契約前の実装所有分離で完了した作業

- `libs/global/KisSampleRectIterator.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalsamplerectiteratorobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象
  だけを変更した。公開ヘッダー、クラス、関数、`kritaglobal`のAPIとABIを維持し、`kritaglobal`は
  新対象のオブジェクトを従来どおり集約する。
- 新対象は矩形と共有データのQt Core、座標変換型のQt Gui、ヘッダー専用Boostだけへ直接接続する。
  従来の`kritaglobal`は変更なし構築閉包が55工程・110入力、新対象は1工程・3入力である。macOSで
  新対象の限定構築が成功した。製品実装、公開API、ABI、矩形サンプル列は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b 矩形サンプラー public API契約で完了した作業

- `libs/global/KisSampleRectIterator.h`のクラス、既定・矩形・コピー・ムーブ構築、コピー・ムーブ代入、
  破棄、標本番号の9 APIを、新規`libs/global/tests/KisSampleRectIteratorTest.cpp`の4試験へ対応付けた。
  空矩形の既定点、四隅・辺中点・中心の9基準点、最初のHalton点、標本番号、およびコピー後の独立
  進行とムーブ後の位置継続を固定矩形で観測する。正常経路の安全検査呼び出しは0回である。
- 新試験は`kritaglobalsamplerectiteratorobjects`とQt Testだけへ直接接続し、変更なし構築閉包は
  macOSで5工程・11入力である。対象実行と20回反復が成功し、公開API契約は763件、未対応基準は
  28,251件になった。製品実装、公開API、ABI、矩形サンプル列は変更していない。Linuxと全ネイティブ
  検証は実行していない。

## R2-G19b 矩形セル管理契約前の実装所有分離で完了した作業

- `libs/global/KisRectsGrid.cpp`の実装所有を、`kritaglobal`の一括ソース集合から
  `kritaglobalrectsgridobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、クラス、関数、`kritaglobal`のAPIとABIを維持し、`kritaglobal`は新対象の
  オブジェクトを従来どおり集約する。
- 新対象は矩形と配列のQt Core、座標変換型のQt Gui、ヘッダー専用Boostだけへ直接接続する。
  従来の`kritaglobal`は変更なし構築閉包が55工程・110入力、新対象は1工程・3入力である。macOSで
  新対象の限定構築が成功した。製品実装、公開API、ABI、矩形セル管理は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b 矩形セル管理 public API契約で完了した作業

- `libs/global/KisRectsGrid.h`のクラス、構築、整列、追加2種、削除2種、包含判定、境界矩形の9 APIを、
  新規`libs/global/tests/KisRectsGridContractTest.cpp`の5試験へ対応付けた。2の累乗幅への整列、不正幅の
  64への復帰と診断、未読込セルだけの追加、完全包含セルだけの削除、変更セルの格子順を固定矩形で
  観測する。正常な整列済み経路の検査違反は0回である。
- `boundingRect()`が全セル削除後も最後に確保した写像領域を返す挙動は、公開説明の読込済みセル境界と
  一致しないため`known_defect`として固定した。修正時はこの分類と期待値を同じ変更で更新する。
- 新試験は`kritaglobalrectsgridobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで
  5工程・11入力である。対象実行と20回反復が成功し、公開API契約は772件、未対応基準は28,242件に
  なった。製品実装、公開API、ABI、矩形セル管理の現行挙動は変更していない。Linuxと全ネイティブ
  検証は実行していない。

## R2-G19b Qt文字列列互換 public API契約で完了した作業

- `libs/global/KisQStringListFwd.h`がQt 6向けに公開する`QStringList`、`QByteArrayList`、`QVector`の
  3型別名を、新規`libs/global/tests/KisQStringListFwdTest.cpp`の1試験へ対応付けた。対応する`QList`
  実体との型一致に加え、文字列結合、バイト列の先頭追加、整数列の末尾追加が要素順を保つことを
  観測する。
- 対象はQt Coreの前方宣言だけで完結するため、製品実装の所有分離は不要だった。新試験はQt Test
  だけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が成功し、
  公開API契約は775件、未対応基準は28,239件になった。製品実装、公開API、ABI、Qt 6互換型は
  変更していない。`ssh nixos`は15秒間応答せず中断したため、Linuxと全ネイティブ検証は実行して
  いない。

## R2-G19b 変換パイプライン public API契約で完了した作業

- `libs/global/KisZug.h`が公開する空状態、関数化、タプル・引数展開、型変換、乗算、6比較、丸めの
  13 APIを、新規`libs/global/tests/KisZugContractTest.cpp`の4試験へ対応付けた。入力列の型変換と
  乗算、整数の厳密な比較境界、`qreal`のQt近似等価境界、Qt丸め、タプルと複数入力の各要素変換を
  観測する。
- 対象はヘッダー専用で、製品実装の所有分離は不要だった。新試験はQt Testと`zug`だけへ直接接続し、
  変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が成功し、公開API契約は
  788件、未対応基準は28,226件になった。製品実装、公開API、ABI、変換結果は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 汎用登録簿と一覧モデル public API契約で完了した作業

- `libs/global/KoGenericRegistry.h`と`libs/global/KoGenericRegistryModel.h`の構築・破棄、追加、別名、
  検索、削除、列挙、重複記録、行数、値と役割の25 APIを、新規
  `libs/global/tests/KoGenericRegistryContractTest.cpp`の6試験へ対応付けた。null拒否と診断、識別子・
  別名による検索、削除、重複置換、反復、一覧モデルの表示・編集役割と無効値を観測する。
- 登録簿が所有すると説明する生ポインターを破棄しない挙動と、一覧モデルが有効な親にもルート行数を
  返す挙動は、それぞれ所有契約とQt一覧モデル規約に反するため`known_defect`として固定した。修正時は
  分類と期待値を同じ変更で更新する。
- 両対象はヘッダー専用で、製品実装の所有分離は不要だった。試験内の診断記録関数とQt Testだけへ
  直接接続し、構成時生成の公開ヘッダーを直接参照する。変更なし構築閉包はmacOSで4工程・8入力で
  ある。対象実行と20回反復が成功し、公開API契約は813件、未対応基準は28,201件になった。製品実装、
  公開API、ABI、登録簿と一覧モデルの現行挙動は変更していない。Linuxと全ネイティブ検証は実行して
  いない。

## R2-G19b 描画操作設定読込 public API契約で完了した作業

- `plugins/paintops/libpaintop/KisPaintOpOptionUtils.h`の設定読込1 APIを、新規
  `plugins/paintops/libpaintop/tests/KisPaintOpOptionUtilsContractTest.cpp`の1試験へ対応付けた。新しい
  データ値を作り、指定設定を`read()`へ一度渡し、その読込結果を値として返すことを観測する。
- 対象はヘッダー専用で、製品実装の所有分離は不要だった。新試験はQt Testだけへ直接接続し、変更なし
  構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が成功し、公開API契約は814件、未対応
  基準は28,200件になった。製品実装、公開API、ABI、設定読込結果は変更していない。Linuxと全
  ネイティブ検証は実行していない。

## R2-G19b 図形線共有所有型 public API契約で完了した作業

- `libs/flake/KoFlakeTypes.h`の図形線と線モデルの共有ポインター型別名2 APIを、新規
  `libs/flake/tests/KoFlakeTypesContractTest.cpp`の1試験へ対応付けた。対象型との型一致、複製後の
  同一実体参照、複製破棄後の所有継続、最後の解放時の一度だけの実体破棄を観測する。
- 対象は前方宣言と型別名だけで完結するため、製品実装の所有分離は不要だった。新試験はQt Testだけへ
  直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が成功し、公開API
  契約は816件、未対応基準は28,198件になった。製品実装、公開API、ABI、共有所有規則は変更して
  いない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b アニメーション枠キャッシュ所有型 public API契約で完了した作業

- `libs/ui/animation/kis_animation_frame_cache_fwd.h`の強参照と弱参照の型別名2 APIを、新規
  `libs/ui/tests/KisAnimationFrameCacheFwdContractTest.cpp`の1試験へ対応付けた。対象型との型一致、
  強参照の共有、弱参照からの昇格、最後の強参照解放時の一度だけの実体破棄と弱参照無効化を観測する。
- 対象は前方宣言と型別名だけで完結するため、製品実装の所有分離は不要だった。新試験はQt Testと
  公開参照計数ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。対象実行と
  20回反復が成功し、公開API契約は818件、未対応基準は28,196件になった。製品実装、公開API、ABI、
  キャッシュ寿命規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 名称付き実行ジョブ public API契約で完了した作業

- `libs/image/kis_runnable.h`と`libs/image/kis_runnable_with_debug_name.h`のクラス、仮想破棄、実行、
  診断名の5 APIを、新規`libs/image/tests/KisRunnableContractTest.cpp`の1試験へ対応付けた。派生型の
  診断名、基底ポインター経由の一度の実行、同じ基底ポインター経由の一度の派生実体破棄を観測する。
- 両対象は抽象インターフェースだけで完結するため、製品実装の所有分離は不要だった。新試験はQt Test
  と構成時生成の画像公開ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。
  対象実行と20回反復が成功し、公開API契約は823件、未対応基準は28,191件になった。製品実装、
  公開API、ABI、実行と破棄の規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b ストロークジョブ契約前の実装所有分離で完了した作業

- `libs/image/kis_stroke_job_strategy.cpp`と`libs/image/KisRunnableStrokeJobDataBase.cpp`の実装所有を、
  `kritaimage`の一括ソース集合から`kritaimagestrokejobobjects`へ移した。起点と移動先のファイルは同じで、
  CMake上の所有対象だけを変更した。公開ヘッダー、クラス、関数、`kritaimage`のAPIとABIを維持し、
  `kritaimage`は新対象のオブジェクトを従来どおり集約する。
- 新対象はQt Coreだけへ直接接続する。従来の`kritaimage`は変更なし構築閉包が998工程・2,020入力、
  新対象は2工程・5入力である。macOSで新対象の限定構築が成功した。製品実装、公開API、ABI、
  ストロークジョブ挙動は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は
  実行していない。

## R2-G19b ストロークジョブ public API契約で完了した作業

- `libs/image/kis_stroke_job_strategy.h`と`libs/image/KisRunnableStrokeJobDataBase.h`のクラス、全逐次性・
  排他性、構築・破棄、分類、取消可能性、詳細度上書き、複製、戦略実行・診断名の28 APIを、新規
  `libs/image/tests/KisStrokeJobContractTest.cpp`の6試験へ対応付けた。既定値、全列挙値の分類、状態変更と
  派生複製、基底ポインターからの破棄、戦略の実行対象、実行可能ジョブの指定値を観測する。
- 新試験は`kritaimagestrokejobobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで6工程・
  13入力である。対象実行と20回反復が成功し、公開API契約は851件、未対応基準は28,163件になった。
  製品実装、公開API、ABI、ストロークジョブ挙動は変更していない。Linuxと全ネイティブ検証は実行して
  いない。

## R2-G19b 実行可能ストロークジョブ契約前の実装所有分離で完了した作業

- `libs/image/KisRunnableStrokeJobData.cpp`の実装所有を、`kritaimage`の一括ソース集合から
  `kritaimagestrokejobobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。未使用だった`libs/global/kis_assert.h`へのincludeを同じ変更で除去した。公開ヘッダー、
  クラス、関数、`kritaimage`のAPIとABIを維持し、`kritaimage`は新対象のオブジェクトを従来どおり
  集約する。
- 拡張後の対象はQt Coreだけへ直接接続し、変更なし構築閉包は3工程・7入力である。macOSで対象の
  限定構築が成功した。製品挙動、公開API、ABI、実行と自動破棄の規則は変更していない。Linux、
  製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 実行可能ストロークジョブ public API契約で完了した作業

- `libs/image/KisRunnableStrokeJobData.h`のクラス、2構築、破棄、実行の5 APIを、
  `libs/image/tests/KisStrokeJobContractTest.cpp`へ追加した3試験へ対応付けた。関数の実行と空関数の
  安全な無視、`QRunnable`の実行、自動破棄が有効な場合のジョブ所有、自動破棄が無効な場合の呼出側
  所有を観測する。
- 試験は`kritaimagestrokejobobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで7工程・
  15入力である。対象実行と20回反復が成功し、公開API契約は856件、未対応基準は28,158件になった。
  製品実装、公開API、ABI、実行と所有の規則は変更していない。Linuxと全ネイティブ検証は実行して
  いない。

## R2-G19b 実行可能ジョブ一覧契約前の実装所有分離で完了した作業

- `libs/image/KisRunnableStrokeJobsInterface.cpp`の実装所有を、`kritaimage`の一括ソース集合から
  `kritaimagestrokejobobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、クラス、関数、`kritaimage`のAPIとABIを維持し、`kritaimage`は新対象の
  オブジェクトを従来どおり集約する。
- 拡張後の対象はQt Coreと`libs/global/kis_pointer_utils.h`のヘッダー専用変換だけへ直接接続し、
  変更なし構築閉包は4工程・9入力である。macOSで対象の限定構築が成功した。製品挙動、公開API、
  ABI、単体・一覧追加の委譲規則は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ
  検証は実行していない。

## R2-G19b 実行可能ジョブ一覧 public API契約で完了した作業

- `libs/image/KisRunnableStrokeJobsInterface.h`のクラス、破棄、単体追加、基底型一覧追加、派生型一覧追加の
  5 APIを、`libs/image/tests/KisStrokeJobContractTest.cpp`へ追加した3試験へ対応付けた。単体から1要素
  一覧への委譲、派生型一覧から基底型一覧への要素数・ポインター順を保つ変換、基底ポインターからの
  派生実体破棄を観測する。
- 試験は`kritaimagestrokejobobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで8工程・
  17入力である。対象実行と20回反復が成功し、公開API契約は861件、未対応基準は28,153件になった。
  製品実装、公開API、ABI、委譲・変換・破棄の規則は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 代替実行可能ジョブ処理契約前の実装所有分離で完了した作業

- `libs/image/KisFakeRunnableStrokeJobsExecutor.cpp`の実装所有を、`kritaimage`の一括ソース集合から
  `kritaimagestrokejobobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、クラス、関数、`kritaimage`のAPIとABIを維持し、`kritaimage`は新対象の
  オブジェクトを従来どおり集約する。
- 拡張後の対象はQt Core、`libs/global/kis_pointer_utils.h`、`libs/global/kis_assert.h`と生成済み公開
  宣言だけへ直接接続し、`kritaglobal`ライブラリーへは接続しない。変更なし構築閉包は5工程・11入力で
  ある。macOSで対象の限定構築が成功した。製品挙動、公開API、ABI、ジョブ実行・破棄・検査の規則は
  変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 代替実行可能ジョブ処理 public API契約で完了した作業

- `libs/image/KisFakeRunnableStrokeJobsExecutor.h`のクラス、フラグ型と2値、2構築、一覧追加の7 APIを、
  `libs/image/tests/KisStrokeJobContractTest.cpp`へ追加した4試験へ対応付けた。通常ジョブの入力順実行と
  所有実体破棄、障壁許可時の実行、既定時の障壁・排他ジョブ診断を観測する。外部診断関数は試験内の
  記録実装へ置き換え、診断条件と、診断が復帰した後の実行・破棄を決定的に検査する。
- 試験は`kritaimagestrokejobobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで9工程・
  19入力である。対象実行と20回反復が成功し、公開API契約は868件、未対応基準は28,146件になった。
  製品実装、公開API、ABI、ジョブ実行・破棄・診断の規則は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 二次元ベクトル契約前の試験分離で完了した作業

- `libs/image/tests/kis_vec_test.cpp`を、`kritaimage`と試験SDKへ接続する一括試験集合から、同じ
  `libs/image/tests/kis_vec_test.cpp`を所有する単独CTest対象へ移した。`libs/image/tests/kis_vec_test.h`
  と同実装の試験入口をQt Testへ直接置き換え、既存のゼロベクトル生成契約とCTest名を維持した。
- 単独対象はQt Gui、Qt Test、Eigen、`libs/image/kis_vec.h`だけへ直接接続する。変更なし構築閉包は
  1,002工程・2,027入力から4工程・8入力へ縮小した。macOSで対象実行と20回反復が成功した。製品実装、
  公開API、ABI、二次元ベクトル挙動は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 二次元ベクトル public API契約で完了した作業

- `libs/image/kis_vec.h`の型別名、整数点変換、浮動小数点変換、式から点への変換の4 APIを、
  `libs/image/tests/kis_vec_test.cpp`の3試験へ対応付けた。ゼロベクトル生成、符号と小数値を保つ点から
  ベクトルへの変換、評価前のEigen式から計算済み`QPointF`への変換を観測する。
- 試験はQt Gui、Qt Test、Eigen、対象ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・
  8入力である。対象実行と20回反復が成功し、公開API契約は872件、未対応基準は28,142件になった。
  製品実装、公開API、ABI、座標変換規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 回り込み軸 public API契約で完了した作業

- `libs/image/KisWraparoundAxis.h`の列挙型と両方向・水平方向・垂直方向の4 APIを、新規
  `libs/image/tests/KisWraparoundAxisContractTest.cpp`の1試験へ対応付けた。永続設定で識別に使う
  数値が両方向0、水平方向1、垂直方向2であることを観測する。
- 新試験はQt Testと対象ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。
  対象実行と20回反復が成功し、公開API契約は876件、未対応基準は28,138件になった。製品実装、
  公開API、ABI、列挙値は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b Linux同期検証で完了した作業

- macOSの`develop`を`ad635bae`までGit履歴として`ssh nixos`上の
  `/home/masato/Documents/librepaint`へfast-forwardし、同期後の作業木がクリーンであることを確認した。
- Linux実機の永続Ninja木で`KisStrokeJobContractTest`、`kis_vec_test`、
  `KisWraparoundAxisContractTest`、`KisTimingInformationContractTest`、
  `KisPaintopSettingsIdsContractTest`、`KisGreenCoordinatesContractTest`、
  `KisSpeedSmootherContractTest`、`KisRectangleInteractionContractTest`、`KoIDContractTest`、
  `KoColorModelStandardIdsContractTest`、`KisSelectionTagsContractTest`、`KisCommandIdsContractTest`、
  `KoCanvasResourceIdsContractTest`、`KisResourceTypesContractTest`、
  `KoColorProfileConstantsContractTest`、`KisImageSignalsContractTest`、`KisDabShapeContractTest`、
  `KoInsetsContractTest`、`KisSpacingInformationContractTest`、`KisHistoryListTest`、
  `KritaContainerUtilsContractTest`、`TestKoIntegerMaths`、`KoColorSpaceConstantsContractTest`、
  `kritapigmentcmykmathsobjects`、`KoCmykColorSpaceMathsContractTest`、`kritapigmentmathsobjects`、
  `KoCmykColorSpaceTraitsContractTest`、`KoColorSpacePreserveLightnessUtilsContractTest`、
  `kritaimagenodecommandobjects`、`kis_node_commands_test`、
  `kritapigmentlabmathsobjects`、
  `KoLabColorSpaceMathsContractTest`、`KoGrayColorSpaceTraitsContractTest`、
  `KoBgrColorSpaceTraitsContractTest`だけを限定構築した。
  各CTestの単発実行と20回反復が成功し、直近の契約と構築分離がmacOSとLinuxで一致した。変更なし構築
  閉包はLinuxでケージ座標試験が5工程・16入力、
  速度平滑化試験が6工程・18入力、矩形操作試験が5工程・16入力、識別子値試験が5工程・14入力、標準色
  識別子試験が6工程・21入力、選択方式・操作値試験が4工程・10入力、取り消しコマンドID試験が4工程・
  10入力、キャンバス資源ID試験が4工程・10入力、資源種別試験が6工程・16入力である。資源種別の製品
  実装対象は2工程・5入力、色プロファイル標準コード試験は4工程・10入力、画像信号の製品実装対象は
  1工程・3入力、画像信号試験は5工程・13入力、描画点形状試験は4工程・10入力、図形余白の製品実装対象は
  1工程・3入力、図形余白試験は5工程・13入力、描画間隔試験は4工程・13入力、整列履歴試験は4工程・
  10入力、コンテナー補助試験、整数演算試験、不透明度端点試験は各4工程・10入力、CMYK尺度定数とLab
  尺度定数の製品実装対象は各1工程・3入力、各試験は5工程・21入力、基礎色数値特性の製品実装対象は
  1工程・3入力、CMYK画素特性試験は6工程・23入力、灰色ブラシ明度保持試験は5工程・21入力、ノード
  コマンドの製品実装対象は1工程・3入力、ノードコマンド試験は223工程・489入力、灰色・BGR画素特性
  試験は各4工程・18入力である。全ネイティブ検証は実行していない。

## R2-G19b 描画タイミング情報 public API契約で完了した作業

- `libs/image/kis_timing_information.h`のクラス、長時間値、2構築、時間間隔有効判定、間隔取得の6 APIを、
  新規`libs/image/tests/KisTimingInformationContractTest.cpp`の2試験へ対応付けた。既定時の時間間隔配置
  無効と有限長時間値、指定時の有効化とミリ秒値保持を観測する。
- 新試験が対象ヘッダーを先頭でincludeした初回構築は`qreal`未宣言で失敗した。宣言元`QtGlobal`を
  `libs/image/kis_timing_information.h`自身へ追加し、利用側のinclude順依存を除去した。新試験はQt Testと
  対象ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が
  成功し、公開API契約は882件、未対応基準は28,132件になった。公開API、ABI、タイミング値は変更して
  いない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b マスキング描画設定識別子契約前の実装所有分離で完了した作業

- `libs/image/brushengine/KisPaintopSettingsIds.cpp`の実装所有を、`kritaimage`の一括ソース集合から
  `kritaimagepaintopsettingsidsobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象
  だけを変更した。公開ヘッダー、変数、`kritaimage`のAPIとABIを維持し、`kritaimage`は新対象の
  オブジェクトを従来どおり集約する。
- 新対象はソース側と生成側の画像includeディレクトリーだけを持ち、リンク依存を持たない。従来の
  `kritaimage`は変更なし構築閉包が998工程・2,020入力、新対象は1工程・3入力である。macOSで新対象の
  限定構築が成功した。製品挙動、公開API、ABI、設定識別子値は変更していない。Linux、製品ライブラリー
  の再リンク、全ネイティブ検証は実行していない。

## R2-G19b マスキング描画設定識別子 public API契約で完了した作業

- `libs/image/brushengine/KisPaintopSettingsIds.h`の描画操作ID、使用可否、合成方法、親サイズ使用、
  親サイズ係数、埋込プリセット接頭辞の6変数を、新規
  `libs/image/tests/KisPaintopSettingsIdsContractTest.cpp`の1試験へ対応付けた。既存プリセット互換性に
  必要な綴り、大文字小文字、区切り、接頭辞末尾の`/`を観測する。
- 新試験は`kritaimagepaintopsettingsidsobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで
  5工程・11入力である。対象実行と20回反復が成功し、公開API契約は888件、未対応基準は28,126件に
  なった。製品実装、公開API、ABI、設定識別子値は変更していない。Linuxと全ネイティブ検証は実行して
  いない。

## R2-G19b ケージ座標変形契約前の実装所有分離で完了した作業

- `libs/image/kis_green_coordinates_math.cpp`の実装所有を、`kritaimage`の一括ソース集合から
  `kritaimagegreencoordinatesobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象
  だけを変更した。公開ヘッダー、クラス、関数、`kritaimage`のAPIとABIを維持し、`kritaimage`は
  新対象のオブジェクトを従来どおり集約する。
- 新対象はQt Core/Gui、Boostヘッダー、画像と大域数式ヘッダーへ直接接続する。従来の`kritaimage`は
  変更なし構築閉包が998工程・2,020入力、新対象は1工程・3入力である。macOSで新対象の限定構築が
  成功した。製品挙動、公開API、ABI、ケージ座標計算は変更していない。Linux、製品ライブラリーの
  再リンク、全ネイティブ検証は実行していない。

## R2-G19b ケージ座標変形 public API契約で完了した作業

- `libs/image/kis_green_coordinates_math.h`のクラス、構築、破棄、座標事前計算、変形先法線生成、
  座標取得の6 APIを、新規`libs/image/tests/KisGreenCoordinatesContractTest.cpp`の2試験へ対応付けた。
  正方形ケージ内の2点について、恒等変形では元座標を保ち、ケージ全体の平行移動では同じ移動量を
  許容誤差`1e-9`以内で反映することを観測する。
- 新試験は`kritaimagegreencoordinatesobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで
  5工程・11入力である。対象実行と20回反復が成功し、公開API契約は894件、未対応基準は28,120件に
  なった。製品実装、公開API、ABI、ケージ座標計算は変更していない。Linuxと全ネイティブ検証は実行
  していない。

## R2-G19b 速度平滑化契約前の実装所有分離で完了した作業

- `libs/tools/kis_speed_smoother.cpp`の実装所有を、`kritatools`の一括ソース集合から
  `kritatoolsspeedsmootherobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、クラス、関数、`kritatools`のAPIとABIを維持し、`kritatools`は新対象の
  オブジェクトを従来どおり集約する。
- 実装が距離計算だけに使っていた`libs/global/kis_algebra_2d.h`への広い参照を、同じ`kisDistance()`を
  所有する`libs/global/kis_global.h`への参照へ狭めた。新対象はQt Core/Gui、Boostヘッダー、toolsと
  globalの生成・ソースヘッダーだけへ直接接続する。既存`TestToolCoreContract`は変更なし構築閉包が
  1,067工程・2,149入力、新対象は1工程・3入力である。macOSで新対象の限定構築が成功した。製品挙動、
  公開API、ABI、速度計算は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は
  実行していない。

## R2-G19b 速度平滑化 public API契約で完了した作業

- `libs/tools/kis_speed_smoother.h`のクラス、構築、破棄、設定、消去、次速度計算、最終速度取得の
  7 APIを、新規`libs/tools/tests/KisSpeedSmootherContractTest.cpp`の2試験へ対応付けた。固定した座標と
  イベント時刻の系列から算出する速度、同一点入力での最終値保持、同時刻移動の有限値、消去後のゼロ値と
  新系列開始、正常系列で内部診断が発生しないことを観測する。
- 新試験は`kritatoolsspeedsmootherobjects`、`kritaglobalfilteredrollingmeanobjects`、Qt Testだけへ
  直接接続し、変更なし構築閉包はmacOSで6工程・13入力である。初回リンクは移動平均実装が参照する
  回復可能診断関数の未解決で失敗した。試験内の記録実装へ接続して正常系列で診断がないことも検査し、
  対象実行と20回反復が成功した。公開API契約は896件、未対応基準は28,118件になった。製品実装、
  公開API、ABI、速度計算は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 矩形操作契約前の実装所有分離で完了した作業

- `libs/tools/kis_rectangle_interaction.cpp`の実装所有を、`kritatools`の一括ソース集合から
  `kritatoolsrectangleinteractionobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象
  だけを変更した。公開ヘッダー、クラス、関数、`kritatools`のAPIとABIを維持し、`kritatools`は
  新対象のオブジェクトを従来どおり集約する。
- 新対象はtoolsのソース・生成ヘッダーとQt Core/Guiだけへ直接接続する。既存
  `TestToolCoreContract`は変更なし構築閉包が1,067工程・2,149入力、新対象は1工程・3入力である。
  macOSで新対象の限定構築が成功した。製品挙動、公開API、ABI、矩形計算は変更していない。Linux、
  製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 矩形操作 public API契約で完了した作業

- `libs/tools/kis_rectangle_interaction.h`のクラス、制約、修飾キー、開始、更新、2種類の矩形取得、始点、
  中心、終点、回転角、移動状態の12 APIを、新規
  `libs/tools/tests/KisRectangleInteractionContractTest.cpp`の4試験へ対応付けた。縦横比・固定寸法、
  Shift/Alt/Controlによる正方形化・移動・中心基準操作、幾何的回転角、現在の終点、任意の始点と終点を
  丸めて正規化する矩形を観測する。
- 新試験は`kritatoolsrectangleinteractionobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで
  5工程・11入力である。対象実行と20回反復が成功し、公開API契約は898件、未対応基準は28,116件に
  なった。製品実装、公開API、ABI、矩形計算は変更していない。Linuxと全ネイティブ検証は実行して
  いない。

## R2-G19b 識別子値契約前の実装所有分離で完了した作業

- `libs/global/KoID.cpp`の実装所有を、`kritaglobal`の一括ソース集合から`kritaglobalidobjects`へ移した。
  起点と移動先のファイルは同じで、CMake上の所有対象だけを変更した。公開ヘッダー、クラス、演算子、
  `kritaglobal`のAPIとABIを維持し、`kritaglobal`は新対象のオブジェクトを従来どおり集約する。
- 新対象はglobalのソース・生成ヘッダー、Qt Core、KF I18n、Boostヘッダーだけへ直接接続する。従来の
  `kritaglobal`は変更なし構築閉包が55工程・110入力、新対象は1工程・3入力である。macOSで新対象の
  限定構築が成功した。製品挙動、公開API、ABI、識別子値は変更していない。Linux、製品ライブラリーの
  再リンク、全ネイティブ検証は実行していない。

## R2-G19b 識別子値 public API契約で完了した作業

- `libs/global/KoID.h`のクラス、3構築、代入、識別子・表示名取得、不一致・大小比較、表示名比較、
  デバッグ出力の13 APIを、新規`libs/global/tests/KoIDContractTest.cpp`の4試験へ対応付けた。既定の空値、
  安定識別子と表示名、遅延翻訳名、複製・代入、識別子順序、表示名順序、デバッグ表現を観測する。
- 新試験は`kritaglobalidobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで5工程・12入力で
  ある。対象実行と20回反復が成功し、公開API契約は911件、未対応基準は28,103件になった。製品実装、
  公開API、ABI、識別子値は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 標準色識別子契約前の実装所有分離で完了した作業

- `libs/pigment/KoColorModelStandardIds.cpp`の実装所有を、`kritapigment`の一括ソース集合から
  `kritapigmentstandardidsobjects`へ移した。起点と移動先のファイルは同じで、CMake上の所有対象だけを
  変更した。公開ヘッダー、識別子変数、`kritapigment`のAPIとABIを維持し、`kritapigment`は新対象の
  オブジェクトを従来どおり集約する。
- 新対象はpigmentとglobalのソース・生成ヘッダー、Qt Core、KF I18n、Boostヘッダーだけへ直接接続する。
  従来の`kritapigment`は変更なし構築閉包が300工程・630入力、新対象は1工程・3入力である。macOSで
  新対象の限定構築が成功した。製品挙動、公開API、ABI、標準色識別子値は変更していない。Linux、
  製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 標準色識別子 public API契約で完了した作業

- `libs/pigment/KoColorModelStandardIds.h`の13識別子変数と
  `libs/pigment/KoColorModelStandardIdsUtils.h`の型から色深度への変換・色深度から型処理への分配の
  2 APIを、新規`libs/pigment/tests/KoColorModelStandardIdsContractTest.cpp`の3試験へ対応付けた。8色モデルと
  5色深度の永続文字列・表示名、整数・浮動小数点チャンネル型の対応、不明な色深度の例外を観測する。
- 新試験は`kritapigmentstandardidsobjects`、`kritaglobalidobjects`、Qt Test、OpenEXRだけへ直接接続し、
  変更なし構築閉包はmacOSで6工程・19入力である。対象実行と20回反復が成功し、公開API契約は926件、
  未対応基準は28,088件になった。製品実装、公開API、ABI、標準色識別子値と型対応は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 選択方式・操作値 public API契約で完了した作業

- `libs/image/KisSelectionTags.h`の2列挙型、画素選択・図形保護の2値、置換・追加・減算・交差・対称差・
  既定の6値からなる10 APIを、新規`libs/image/tests/KisSelectionTagsContractTest.cpp`の2試験へ対応付けた。
  入力処理と設定で識別に使う数値を観測する。
- 初回構築は試験対象へ画像ソースのincludeディレクトリーが伝播せず失敗した。試験対象へ対象ヘッダーの
  所有ディレクトリーだけを追加し、Qt Test以外のリンク依存を持たせず、変更なし構築閉包をmacOSで
  4工程・8入力に限定した。対象実行と20回反復が成功し、公開API契約は936件、未対応基準は28,078件に
  なった。製品実装、公開API、ABI、選択値は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 取り消しコマンドID public API契約で完了した作業

- `libs/painting/undo/kis_command_ids.h`の列挙型と36コマンドIDからなる37 APIを、新規
  `libs/painting/undo/tests/KisCommandIdsContractTest.cpp`のデータ駆動試験へ対応付けた。図形移動の9999から
  SVG文字経路情報変更の10034まで、取り消し履歴の統合判定で各操作を区別する値を観測する。
- 既存`TestKUndo2Stack`の製品ライブラリーと試験支援ライブラリーへの依存を引き継がず、新試験はQt Testと
  対象ヘッダーの所有ディレクトリーだけへ直接接続した。変更なし構築閉包はmacOSで4工程・8入力である。
  対象実行と20回反復が成功し、公開API契約は973件、未対応基準は28,041件になった。製品実装、公開API、
  ABI、コマンドID値は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b キャンバス資源ID public API契約で完了した作業

- `libs/resources/KoCanvasResourcesIds.h`の列挙型と57資源IDからなる58 APIを、新規
  `libs/resources/tests/KoCanvasResourceIdsContractTest.cpp`のデータ駆動試験へ対応付けた。共通資源の0から
  12とLibrePaint固有資源の6000から6043を観測し、キャンバス単位の資源管理で各用途を区別する。
- 既存資源試験集合の製品ライブラリー群への依存を引き継がず、新試験はQt Testと対象ヘッダーの所有
  ディレクトリーだけへ直接接続した。変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が
  成功し、公開API契約は1,031件、未対応基準は27,983件になった。製品実装、公開API、ABI、資源ID値は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 資源種別契約の限定構築で完了した作業

- 資源種別の永続キーと表示名だけを固定する試験が、資源DB・格納・検索を含む製品ライブラリー全体の
  構築閉包を引き継がない所有境界を追加した。
- `libs/resources/KisResourceTypes.cpp`と`libs/resources/ResourceDebug.cpp`は同じファイル位置を保ち、
  `kritaresources`の直接ソースから新規`kritaresourcestypesobjects`の所有へ移した。`kritaresources`は新対象の
  オブジェクトを集約して従来の公開シンボルを供給する。
- 従来の`kritaresources`は変更なし構築閉包が129工程・285入力、新対象は2工程・5入力である。macOSで
  新対象の限定構築が成功した。製品挙動、公開API、ABI、資源種別キーと表示名は変更していない。Linux、
  製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 資源種別 public API契約で完了した作業

- `libs/resources/KisResourceTypes.h`の15種別キー、9下位種別キー、15表示名、表示名変換からなる40 APIを、
  新規`libs/resources/tests/KisResourceTypesContractTest.cpp`の3試験へ対応付けた。格納場所とデータベースに
  使う永続文字列、読込器選択に使う永続文字列、各種別から地域化された空でない表示名への対応を観測する。
- 新試験は`kritaresourcestypesobjects`とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで6工程・14入力
  である。対象実行と20回反復が成功し、公開API契約は1,071件、未対応基準は27,943件になった。製品実装、
  公開API、ABI、資源種別キー、下位種別キー、表示名対応は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 色プロファイル標準コード public API契約で完了した作業

- `libs/pigment/KoColorProfileConstants.h`の2列挙型、14色原色コード、22伝達特性コードからなる38 APIを、
  新規`libs/pigment/tests/KoColorProfileConstantsContractTest.cpp`の2試験へ対応付けた。画像形式とプロファイル
  生成で使うITU H.273標準範囲と、Adobe RGB、ProPhoto、ガンマ、Labの拡張範囲の番号を観測する。
- 新試験はQt Testと対象ヘッダーの所有ディレクトリーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・
  8入力である。対象実行と20回反復が成功し、公開API契約は1,109件、未対応基準は27,905件になった。
  製品実装、公開API、ABI、色原色・伝達特性コードは変更していない。Linuxと全ネイティブ検証は実行して
  いない。

## R2-G19b 画像信号契約の限定構築で完了した作業

- 画像信号の値と反転だけを固定する試験が、タイル、描画器、更新処理を含む画像ライブラリー全体の構築閉包を
  引き継がない所有境界を追加した。
- `libs/image/KisImageSignals.cpp`は同じファイル位置を保ち、`kritaimage`の直接ソースから新規
  `kritaimagesignalsobjects`の所有へ移した。`kritaimage`は新対象のオブジェクトを集約して従来の公開
  シンボルを供給する。実装から未使用の`libs/image/kis_node.h`依存を削除し、前方宣言されたノード共有
  ポインターを値として扱う責務に限定した。
- 従来の`kritaimage`は変更なし構築閉包が998工程・2,020入力、新対象は1工程・3入力である。macOSで新対象の
  限定構築が成功した。製品挙動、公開API、ABI、画像信号値と反転処理は変更していない。Linux、製品
  ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 画像信号 public API契約で完了した作業

- `libs/image/KisImageSignals.h`の列挙型と7識別値、寸法変更値、ノード再選択値、信号包み、信号列からなる
  33 APIを、新規`libs/image/tests/KisImageSignalsContractTest.cpp`の4試験へ対応付けた。識別値、変更前後の
  静止点と矩形中心変換、再選択前後のノード状態、信号の値と列順序、各値の反転を観測する。
- 新試験は`kritaimagesignalsobjects`とQt Testだけへ直接接続する。前方宣言されたノードは試験内の不透明な
  識別値と参照計数関数で表現し、ノード実装を構築せず共有ポインターの値と順序を観測する。変更なし構築
  閉包はmacOSで5工程・11入力である。対象実行と20回反復が成功し、公開API契約は1,142件、未対応基準は
  27,872件になった。製品実装、公開API、ABI、画像信号値と反転処理は変更していない。Linuxと全ネイティブ
  検証は実行していない。

## R2-G19b 描画点形状 public API契約で完了した作業

- `libs/brush/kis_dab_shape.h`のクラス、2構築、等価比較、倍率・縦横比・回転の取得からなる9 APIを、
  新規`libs/brush/tests/KisDabShapeContractTest.cpp`の3試験へ対応付けた。既定形状、倍率と縦横比から求める
  縦横倍率、回転、各成分の浮動小数点近似による等価比較を観測する。
- 既存の最小関連試験は変更なし構築閉包が1,028工程・2,077入力である。新試験はQt Testとヘッダーの
  所有ディレクトリーだけへ直接接続し、4工程・8入力へ限定した。macOSで対象実行と20回反復が成功し、
  公開API契約は1,151件、未対応基準は27,863件になった。製品実装、公開API、ABI、形状計算は変更して
  いない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 図形余白契約の限定構築で完了した作業

- 図形余白の値とデバッグ表現だけを固定する試験が、図形、文字、資源、操作道具を含む図形ライブラリー
  全体の構築閉包を引き継がない所有境界を追加した。
- `libs/flake/KoInsets.cpp`は同じファイル位置を保ち、`kritaflake`の直接ソースから新規
  `kritaflakeinsetsobjects`の所有へ移した。`kritaflake`は新対象のオブジェクトを集約して従来の公開シンボルを
  供給する。
- 従来の`kritaflake`は変更なし構築閉包が521工程・1,074入力、新対象は1工程・3入力である。新対象は
  Qt Coreと図形ライブラリーのソース・生成ヘッダーだけへ直接接続し、macOSの限定構築が成功した。
  製品挙動、公開API、ABI、余白値とデバッグ表現は変更していない。Linux、製品ライブラリーの再リンク、
  全ネイティブ検証は実行していない。

## R2-G19b 図形余白 public API契約で完了した作業

- `libs/flake/KoInsets.h`の構造体、2構築、消去、上下左右の値、デバッグ出力からなる9 APIを、新規
  `libs/flake/tests/KoInsetsContractTest.cpp`の3試験へ対応付けた。既定値、明示した各辺の値、消去後の値、
  診断構成と製品構成のデバッグ表現を観測する。
- 初回実行は製品構成のデバッグ表現を空文字列と期待して失敗し、既存実装が値を省略して終端空白を返す
  ことを確認した。その構成別挙動を契約へ固定した。新試験は`kritaflakeinsetsobjects`とQt Testだけへ直接
  接続し、変更なし構築閉包はmacOSで5工程・11入力である。対象実行と20回反復が成功し、公開API契約は
  1,160件、未対応基準は27,854件になった。製品実装、公開API、ABI、余白値とデバッグ表現は変更して
  いない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 描画間隔 public API契約で完了した作業

- `libs/image/kis_spacing_information.h`のクラス、5構築、距離間隔の有効状態、両軸値、等方判定、スカラー
  近似、回転、座標系反転からなる12 APIを、新規`libs/image/tests/KisSpacingInformationContractTest.cpp`の
  3試験へ対応付けた。既定値、等方間隔、異方間隔とベクトル長、有効状態、回転、座標系を観測する。
- ヘッダーを先頭で読む初回構築は`qreal`、`QPointF`、`QVector2D`の宣言元がなく失敗した。
  `libs/image/kis_spacing_information.h`へ実際に使う`QPointF`と`QVector2D`の宣言元を追加し、利用側のinclude順
  依存を除去した。既存関連試験は変更なし構築閉包が1,002工程・2,027入力、新試験はQt GuiとQt Testだけへ
  直接接続して4工程・8入力である。macOSの対象実行と20回反復が成功し、公開API契約は1,172件、未対応
  基準は27,842件になった。公開API、ABI、描画間隔の計算は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 整列履歴契約の限定構築で完了した作業

- 整列履歴の既存試験入口を、製品ライブラリーと汎用試験支援へ接続する一括試験対象から独立したQt Test
  対象へ移した。試験ソースとCTest名は維持し、GUI・資源・同期接続を初期化する入口を事象ループ不要の
  Qt Test入口へ置き換えた。
- 従来の`KisHistoryListTest`は変更なし構築閉包が59工程・117入力、新対象は4工程・8入力である。新対象は
  Qt Testとglobalの所有・生成ヘッダーだけへ直接接続する。有効範囲だけを観測する試験内に範囲違反診断の
  無操作実装を置き、製品診断実装を構築せずテンプレート値を検査する。macOSの既存対象実行と20回反復が
  成功した。製品実装、公開API、ABI、履歴の挙動は変更していない。Linuxと全ネイティブ検証は実行して
  いない。

## R2-G19b 整列履歴 public API契約で完了した作業

- `libs/global/KisSortedHistoryList.h`のクラス、2型別名、構築、追加、位置参照、件数、最大件数、消去、比較
  関数設定、定数反復子からなる12 APIを、`libs/global/tests/KisHistoryListTest.cpp`の既存整列試験と新規
  位置参照・消去試験へ対応付けた。最近使用した値の保持、件数上限、昇順・降順・履歴順への再整列、位置
  参照、反復範囲、消去を観測する。
- 試験はQt Testとglobalの所有・生成ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力で
  ある。対象実行と20回反復が成功し、公開API契約は1,184件、未対応基準は27,830件になった。製品実装、
  公開API、ABI、整列履歴の挙動は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b コンテナー補助 public API契約で完了した作業

- `libs/global/krita_container_utils.h`の順不同比較、一意化、絞り込み、コンテナーと末尾追加能力を検出する
  特性からなる15 APIを、新規`libs/global/tests/KritaContainerUtilsContractTest.cpp`の3試験へ対応付けた。
  順不同比較の件数・包含規則、一意化後の整列値、絞り込み後の値と順序、標準ベクターと整数の特性判定を
  観測する。
- 順不同比較は同じ件数の左辺各要素が右辺に存在することだけを調べるため、`{1, 1}`と`{1, 2}`を等しいと
  判定する。この重複数を区別しない現行挙動を既知不具合として固定した。利用側が必要とする集合または
  多重集合の意味を特定し、置換後の挙動を別契約として承認した時点で既知不具合契約を解除する。
- 既存の専用試験はない。新試験はQt Testとglobal所有ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで
  4工程・8入力である。対象実行と20回反復が成功し、公開API契約は1,199件、未対応基準は27,815件になった。
  製品実装、公開API、ABI、コンテナー補助の挙動は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 整数演算契約の限定構築で完了した作業

- `libs/pigment/tests/TestKoIntegerMaths.cpp`と`libs/pigment/tests/TestKoIntegerMaths.h`は同じファイル位置と
  CTest名を保ち、`libs/pigment/tests/CMakeLists.txt`の色管理ライブラリー・共通試験支援へ接続する一括試験
  対象から、Qt Testと`libs/pigment/KoIntegerMaths.h`だけへ接続する独立対象`TestKoIntegerMaths`の所有へ移した。
  試験入口は共通試験支援の包含をQt Testの直接包含へ置き換えた。
- 従来の`TestKoIntegerMaths`は変更なし構築閉包が304工程・637入力、新対象は4工程・8入力である。macOSで
  既存対象の限定構築、単発実行、20回反復が成功した。製品実装、公開API、ABI、整数演算の挙動は変更して
  いない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 整数演算 public API契約で完了した作業

- `libs/pigment/KoIntegerMaths.h`の公開整数型、範囲制限、8ビット尺度の拡大積・積・除算・三項積・混合、
  16ビット符号なし尺度の積・除算・混合、符号付き尺度の積・混合、8ビットと16ビットの相互変換からなる
  14 APIを、`libs/pigment/tests/TestKoIntegerMaths.cpp`の既存3試験と新規3試験へ対応付けた。上下限、単位値、
  中間値、ゼロ、負値における整数の丸め、切り捨て、ビット拡張を観測する。
- 試験はQt Testとpigment所有ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力である。
  対象実行と20回反復が成功し、公開API契約は1,213件、未対応基準は27,801件になった。製品実装、公開API、
  ABI、整数演算の挙動は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 不透明度端点 public API契約で完了した作業

- `libs/pigment/KoColorSpaceConstants.h`の8ビットと浮動小数点による透明・不透明端点4 APIを、新規
  `libs/pigment/tests/KoColorSpaceConstantsContractTest.cpp`の2試験へ対応付けた。8ビット尺度の0と255、
  浮動小数点尺度の0.0と1.0を観測する。
- 既存の専用試験はない。新試験はQt Testとpigment所有ヘッダーだけへ直接接続し、変更なし構築閉包は
  macOSで4工程・8入力である。対象実行と20回反復が成功し、公開API契約は1,217件、未対応基準は
  27,797件になった。製品実装、公開API、ABI、不透明度端点は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b CMYK尺度定数契約の限定構築で完了した作業

- `libs/pigment/KoCmykColorSpaceMaths.cpp`は同じファイル位置を保ち、
  `libs/pigment/CMakeLists.txt`の`kritapigment`直接ソースから新規`kritapigmentcmykmathsobjects`の所有へ
  移した。`kritapigment`は新対象のオブジェクトを集約して従来の公開シンボルを供給する。
- 従来の実装所有者`kritapigment`は変更なし構築閉包が300工程・630入力、新対象は1工程・3入力である。
  新対象はQt Core、Qt Gui、OpenEXR、pigmentとglobalの所有・生成ヘッダーだけへ接続し、macOSの限定構築が
  成功した。製品挙動、公開API、ABI、CMYK尺度定数は変更していない。Linux、製品ライブラリーの再リンク、
  全ネイティブ検証は実行していない。

## R2-G19b CMYK尺度定数 public API契約で完了した作業

- `libs/pigment/KoCmykColorSpaceMaths.h`の数値特性とCMYK用ゼロ・半分・単位値からなる4 APIを、新規
  `libs/pigment/tests/KoCmykColorSpaceMathsContractTest.cpp`の2試験へ対応付けた。整数型が基礎色空間の
  数値範囲を継承することと、単精度・倍精度型が0、50、100の百分率尺度を持つことを観測する。
- 初回構築は公開ヘッダーが必要とする`kis_global.h`の検索経路が試験へ伝播せず失敗した。globalのソース・
  生成ヘッダー経路を`kritapigmentcmykmathsobjects`の公開使用条件へ移し、製品ライブラリーへの接続を追加せず
  解消した。試験は専用実装対象とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで5工程・16入力で
  ある。対象実行と20回反復が成功し、公開API契約は1,221件、未対応基準は27,793件になった。製品実装、
  公開API、ABI、CMYK尺度定数は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b Lab尺度定数契約の限定構築で完了した作業

- `libs/pigment/KoLabColorSpaceMaths.cpp`は同じファイル位置を保ち、
  `libs/pigment/CMakeLists.txt`の`kritapigment`直接ソースから新規`kritapigmentlabmathsobjects`の所有へ移した。
  `kritapigment`は新対象のオブジェクトを集約して従来の公開シンボルを供給する。
- 従来の実装所有者`kritapigment`は変更なし構築閉包が300工程・630入力、新対象は1工程・3入力である。
  新対象はQt Core、Qt Gui、OpenEXR、pigmentとglobalの所有・生成ヘッダーだけへ接続し、macOSの限定構築が
  成功した。製品挙動、公開API、ABI、Lab尺度定数は変更していない。Linux、製品ライブラリーの再リンク、
  全ネイティブ検証は実行していない。

## R2-G19b Lab尺度定数 public API契約で完了した作業

- `libs/pigment/KoLabColorSpaceMaths.h`の数値特性とL・a/b各軸のゼロ・半分・単位値からなる7 APIを、新規
  `libs/pigment/tests/KoLabColorSpaceMathsContractTest.cpp`の2試験へ対応付けた。整数表現のL範囲とa/b符号化
  中心値、単精度・倍精度表現のL=0/50/100とa/b=-128/0/127を観測する。
- 初回リンクはヘッダー内で初期化された整数`static const`をQt比較が参照実体として要求し、別実体がない
  ため失敗した。各宣言型の値へ変換して定数式として観測し、製品定義を追加せず解消した。試験は専用実装
  対象とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで5工程・16入力である。対象実行と20回反復が
  成功し、公開API契約は1,228件、未対応基準は27,786件になった。製品実装、公開API、ABI、Lab尺度定数は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 灰色画素特性 public API契約で完了した作業

- `libs/pigment/KoGrayColorSpaceTraits.h`の基礎特性、数値型・親特性の型別名、画素構造と灰色・不透明度値、
  灰色位置、取得・設定、6数値型の名前付き特性からなる15 APIを、新規
  `libs/pigment/tests/KoGrayColorSpaceTraitsContractTest.cpp`の3試験へ対応付けた。灰色と不透明度の配置、
  灰色だけを更新する操作、8・16・32ビット整数と16・32・64ビット浮動小数点への対応を観測する。
- 既存の専用試験はない。新試験はQt Gui、Qt Test、OpenEXR、pigmentとglobalの所有・生成ヘッダーだけへ
  直接接続し、変更なし構築閉包はmacOSで4工程・13入力である。対象実行と20回反復が成功し、公開API契約は
  1,243件、未対応基準は27,771件になった。製品実装、公開API、ABI、灰色画素配置と操作は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b BGR画素特性 public API契約で完了した作業

- `libs/pigment/KoBgrColorSpaceTraits.h`の基礎特性、数値型・親特性の型別名、画素構造と青・緑・赤・
  不透明度値、各色位置、取得・設定、6数値型の名前付き特性からなる23 APIを、新規
  `libs/pigment/tests/KoBgrColorSpaceTraitsContractTest.cpp`の3試験へ対応付けた。BGRと不透明度の配置、
  各色だけを更新する操作、8・16・32ビット整数と16・32・64ビット浮動小数点への対応を観測する。
- 既存の反転・畳み込み試験は一部の名前付き特性を間接利用する。新試験はQt Gui、Qt Test、OpenEXR、
  pigmentとglobalの所有・生成ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・13入力で
  ある。対象実行と20回反復が成功し、公開API契約は1,266件、未対応基準は27,748件になった。製品実装、
  公開API、ABI、BGR画素配置と操作は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 基礎色数値特性の限定構築で完了した作業

- `libs/pigment/KoColorSpaceMaths.cpp`は同じファイル位置を保ち、`libs/pigment/CMakeLists.txt`の
  `kritapigment`直接ソースから新規`kritapigmentmathsobjects`の所有へ移した。`kritapigment`は新対象の
  オブジェクトを集約し、従来の公開シンボルを供給する。
- CMYK画素特性試験の初回リンクは、浮動小数点の基礎数値特性実体が不足して失敗した。製品ライブラリー
  全体への接続を避けるため、実体を持つ1ソースだけをQt Core、Qt Gui、OpenEXR、pigmentとglobalの
  所有・生成ヘッダーへ接続した。従来の実装所有者`kritapigment`は変更なし構築閉包が300工程・630入力を
  保ち、新対象は1工程・3入力でmacOSの限定構築に成功した。製品挙動、公開API、ABI、色数値特性は変更
  していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b CMYK画素特性 public API契約で完了した作業

- `libs/pigment/KoCmykColorSpaceTraits.h`の基礎特性、画素構造とCMYK・不透明度の配置、各色の取得・
  設定、整数2型と浮動小数点3型の名前付き特性、浮動小数点型の文字列・配列正規化からなる35 APIを、
  新規`libs/pigment/tests/KoCmykColorSpaceTraitsContractTest.cpp`の6試験へ対応付けた。CMYKと不透明度の
  配置、各色だけを更新する操作、百分率CMYKと単位範囲不透明度の双方向変換を観測する。
- 試験は基礎色数値特性とCMYK尺度定数の専用実装対象、Qt Testだけへ直接接続し、変更なし構築閉包は
  macOSで6工程・18入力である。対象実行と20回反復が成功し、公開API契約は1,301件、未対応基準は
  27,713件になった。製品実装、公開API、ABI、CMYK画素配置と正規化は変更していない。Linuxと全
  ネイティブ検証は実行していない。

## R2-G19b 灰色ブラシ明度保持 public API契約で完了した作業

- `libs/pigment/KoColorSpacePreserveLightnessUtils.h`の画素型・数値型別名と、灰色ブラシから色相を保つ
  塗りつぶし、既存画素の明度変調からなる6 APIを、新規
  `libs/pigment/tests/KoColorSpacePreserveLightnessUtilsContractTest.cpp`の2試験へ対応付けた。黒・白の固定
  マスクに対する出力色、塗りつぶし時のマスク・描画色不透明度の小さい方、変調時の既存不透明度保持を
  観測する。
- 試験は基礎色数値特性の専用実装対象とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで5工程・
  16入力である。対象実行と20回反復が成功し、公開API契約は1,307件、未対応基準は27,707件になった。
  製品実装、公開API、ABI、明度保持計算は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b ノードコマンド契約の限定構築で完了した作業

- `libs/image/commands/kis_node_command.cpp`は同じファイル位置を保ち、`libs/image/CMakeLists.txt`の
  `kritaimage`直接ソースから新規`kritaimagenodecommandobjects`の所有へ移した。`kritaimage`は新対象の
  オブジェクトを集約し、従来の公開シンボルを供給する。
- `libs/image/tests/kis_node_commands_test.cpp`は同じファイル位置を保ち、
  `libs/image/tests/CMakeLists.txt`の`kritaimage`・`kritatestsdk`一括試験群から、ノードコマンド実装対象、
  取り消しコマンド実装、Qt Testだけへ接続する独立試験へ移した。変更なし構築閉包は1,002工程・2,027
  入力から230工程・490入力へ縮小し、新しい製品実装対象は1工程・3入力、`kritaimage`は998工程・2,020
  入力である。macOSの限定構築に成功し、製品挙動、公開API、ABI、コマンドシンボルは変更していない。
  Linux、製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b ノードコマンド基底 public API契約で完了した作業

- `libs/image/commands/kis_node_command.h`のコマンド基底クラスとコンストラクタからなる2 APIを、既存
  `libs/image/tests/kis_node_commands_test.cpp`の空試験を置き換えた1試験へ対応付けた。表示名が取り消し
  コマンド基底へ渡され、nullノードがそのまま保持されることを観測する。
- 試験はQt Testを直接利用し、nullノードでは呼び出されない参照計数シンボルだけを試験内の連結継ぎ目で
  供給する。変更なし構築閉包はmacOSで230工程・490入力である。対象実行と20回反復が成功し、公開API
  契約は1,309件、未対応基準は27,705件になった。製品実装、公開API、ABI、コマンドの所有関係は変更
  していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 共有ポインター変換 public API契約で完了した作業

- `libs/global/kis_pointer_utils.h`の生ポインター所有権移譲、一覧の共有・弱参照変換、暗黙要素変換、
  生ポインター取得、共有ポインター特性からなる15 APIを、新規
  `libs/global/tests/KisPointerUtilsContractTest.cpp`の5試験へ対応付けた。順序と個体の保持、弱参照復元の
  全件成功規則と部分復元、Qt・Krita・固定共有ポインターの型特性と動的型変換を観測する。
- 新試験はQt Testとglobal所有ヘッダーだけへ直接接続し、変更なし構築閉包はmacOSで4工程・8入力で
  ある。対象実行と20回反復が成功し、公開API契約は1,324件、未対応基準は27,690件になった。製品実装、
  公開API、ABI、共有ポインターの所有規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b ベジェパッチ区間 public API契約で完了した作業

- `libs/global/KisBezierPatchParamSpaceUtils.h`の区間値、長さ・中点・空判定、内判定、相対区間による
  絞り込み、前向き距離、矩形変換、診断出力、一次元パラメーター探索からなる15 APIを、新規
  `libs/global/tests/KisBezierPatchParamSpaceUtilsContractTest.cpp`の7試験へ対応付けた。区間の幾何値、
  接触・重複時の未定義距離、矩形との往復、要求区間を挟む外側・内側探索結果を観測する。
- `Range::contains()`は通常の内点を偽とし、両端の双方と近似一致する内点だけを真とする現行挙動を
  既知不具合として固定した。利用側が開区間・閉区間・近似端点の要件を決定し、置換挙動を別契約として
  承認した時点でこの既知不具合契約を解除する。
- 新試験はQt Test、Qt Gui、Boostとglobal所有ヘッダーだけへ直接接続し、予期しない安全断言を失敗へ
  変換する試験内の連結継ぎ目を持つ。変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回
  反復が成功し、公開API契約は1,339件、未対応基準は27,675件になった。製品実装、公開API、ABI、区間
  計算は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 詳細度座標変換 public API契約で完了した作業

- `libs/global/kis_lod_transform_base.h`の詳細度・尺度変換、描画装置の詳細度取得、変換行列、整数・
  浮動小数点矩形と一般点の写像、逆写像、画素ブロック境界整列、整数矩形とスカラーの拡大縮小からなる
  20 APIを、新規`libs/global/tests/KisLodTransformBaseContractTest.cpp`の6試験へ対応付けた。尺度の上下限と
  切り下げ、正負座標、整列済み矩形の往復、模擬描画装置を使うテンプレート経路を観測する。
- 公開ヘッダー先頭包含による初回構築は、`KIS_ASSERT_RECOVER_NOOP`の宣言を先行包含へ依存して失敗した。
  `libs/global/kis_lod_transform_base.h`から所有元`libs/global/kis_assert.h`を直接包含し、利用側の包含順に
  依存せずコンパイルできるようにした。
- 新試験はQt Test、Qt Guiとglobal所有ヘッダーだけへ直接接続し、予期しない回復可能断言を失敗へ変換する
  試験内の連結継ぎ目を持つ。変更なし構築閉包はmacOSで4工程・8入力である。対象実行と20回反復が成功し、
  公開API契約は1,359件、未対応基準は27,655件になった。公開API、ABI、詳細度座標計算は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b メタ処理契約の限定構築で完了した作業

- `libs/global/tests/KisMplTest.cpp`は同じファイル位置を保ち、`libs/global/tests/CMakeLists.txt`の
  `KisSignalCompressorTest`・`KisForestTest`・`KisRectsGridTest`との一括試験対象から、共有参照実装対象と
  Qt Testだけへ接続する独立対象`KisMplTest`の所有へ移した。試験入口は共通試験支援からQt Testの直接
  入口へ置き換え、自動moc生成を一経路にした。
- 変更なし構築閉包はmacOSで59工程・117入力から5工程・11入力へ縮小した。既存13試験の対象実行と20回
  反復が成功した。製品実装、公開API、ABI、メタ処理の挙動は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b メタ処理 public API契約で完了した作業

- `libs/global/KisMpl.h`の型列生成、先頭型選択、タプル写像・展開、optional畳み込み、訪問関数合成、
  メンバー値の単項・二項比較、メンバー値の算術処理、終了時処理からなる92 APIを、
  `libs/global/tests/KisMplTest.cpp`の既存13試験と新規4試験へ対応付けた。データメンバーとconst・非const・
  `noexcept`関数の全オーバーロード、個体同士と値との比較、共有参照個体、破棄時の一度だけの実行を
  観測する。
- 既存の以下比較試験でconst関数経路を重複していた箇所を非const `noexcept`関数へ置き換え、5種類の
  取得経路をすべて実行するようにした。限定構築閉包はmacOSで5工程・11入力を保ち、対象実行と20回
  反復が成功した。公開API契約は1,451件、未対応基準は27,563件になった。製品実装、公開API、ABI、
  メタ処理の規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 森構造契約の限定構築で完了した作業

- `libs/global/tests/KisForestTest.cpp`は同じファイル位置を保ち、`libs/global/tests/CMakeLists.txt`の
  `KisSignalCompressorTest`・`KisRectsGridTest`との一括試験対象から、Qt Testだけへ接続する独立対象
  `KisForestTest`の所有へ移した。試験入口は共通試験支援からQt Testの直接入口へ置き換え、ヘッダー実装が
  要求する通常断言と安全断言を、予期しない呼出しを致命的失敗にする試験内の連結継ぎ目で供給した。
- 変更なし構築閉包はmacOSで59工程・117入力から4工程・8入力へ縮小した。既存27試験の対象実行と20回
  反復が成功した。製品実装、公開API、ABI、森構造の挙動は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 森構造 public API契約で完了した作業

- `libs/global/KisForest.h`のノード格納、全反復子型、走査状態、可変・const変換、森コンテナー、自由関数の
  走査入口、挿入・消去・部分木移動、複製・代入・交換、深さ・要素数からなる133 APIを、既存27試験と
  `libs/global/tests/KisForestTest.cpp`の新規4試験へ対応付けた。親子・兄弟リンク、全反復方向と境界、型別名、
  const性、診断表示、所有値の破棄、複数根を持つ三段の森を観測する。
- 初回構築は合成反復子と深さ優先反復子の可変からconstへの公開変換が、実在しない内部メンバーを参照して
  コンパイルに失敗した。`libs/global/KisForest.h`で公開済みの`node()`、`state()`と保持する基底反復子の
  状態からconst反復子を構築し、公開変換を利用可能にした。公開宣言とABIは変更していない。
- 限定構築閉包はmacOSで4工程・8入力を保ち、31試験の対象実行と20回反復が成功した。公開API契約は
  1,584件、未対応基準は27,430件になり、`libs/global/KisForest.h`の未対応は0件になった。Linuxと全
  ネイティブ検証は実行していない。

## R2-G19b 乱数源契約の限定構築で完了した作業

- `libs/global/kis_random_source.cpp`は同じファイル位置を保ち、`libs/global/CMakeLists.txt`の
  `kritaglobal`直接ソースから新規`kritaglobalrandomsourceobjects`の所有へ移した。`kritaglobal`は新対象の
  オブジェクトを集約し、従来の公開シンボルを供給する。
- 新対象はQt CoreとBoostだけへ直接接続し、変更なし構築閉包はmacOSで1工程・3入力である。製品所有者
  `kritaglobal`の構築閉包は55工程・110入力であり、乱数源の契約試験は製品ライブラリー全体を構築せずに
  実装1ソースへ接続できる。限定構築に成功し、製品挙動、公開API、ABI、乱数列は変更していない。Linux、
  製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 乱数源 public API契約で完了した作業

- `libs/global/kis_random_source.h`の名前付き共有・弱参照型、既定・固定種・複製構築、代入、破棄、生の
  整数、指定閉区間、単位範囲、正規分布の生成からなる12 APIを、新規
  `libs/global/tests/KisRandomSourceContractTest.cpp`の3試験へ対応付けた。同じ固定種の全生成経路、各値域、
  現在位置からの列の分岐、自己代入、強参照解放後の弱参照失効を観測する。
- 新試験は乱数源と共有参照の専用実装対象、Qt Testだけへ直接接続し、変更なし構築閉包はmacOSで6工程・
  13入力である。対象実行と20回反復が成功し、公開API契約は1,596件、未対応基準は27,418件になった。
  製品実装、公開API、ABI、乱数生成規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 遅延追跡契約の限定構築で完了した作業

- `libs/global/kis_latency_tracker.cpp`は同じファイル位置を保ち、`libs/global/CMakeLists.txt`の
  `kritaglobal`直接ソースから新規`kritagloballatencytrackerobjects`の所有へ移した。`kritaglobal`は新対象の
  オブジェクトを集約し、従来の公開シンボルを供給する。
- 新対象はQt CoreとBoostだけへ直接接続し、変更なし構築閉包はmacOSで1工程・3入力である。製品所有者
  `kritaglobal`は55工程・110入力を保ち、遅延追跡の契約試験は製品ライブラリー全体を構築せずに実装
  1ソースへ接続できる。限定構築に成功し、製品挙動、公開API、ABI、集計規則は変更していない。Linux、
  製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 遅延追跡 public API契約で完了した作業

- `libs/global/kis_latency_tracker.h`の移動最大値、汎用スカラー追跡、イベント遅延追跡の構築・追加・破棄
  からなる11 APIを、新規`libs/global/tests/KisLatencyTrackerContractTest.cpp`の3試験へ対応付けた。空の
  最大値診断、窓境界、平均・分散・最大値の出力周期、基底ポインターからの仮想破棄、現在時刻とイベント
  時刻の差を観測する。
- `KisRollingMax`は指定窓幅より1件多い標本を保持し、次の追加時に最古値を除く現行挙動を既知不具合として
  固定した。`KisScalarTracker`の平均・分散と最大値が同じ標本集合を使う置換契約を承認し、全利用側への
  影響を確認した時点でこの既知不具合契約を解除する。
- 新試験は遅延追跡と共有参照の専用実装対象、Qt Testだけへ直接接続し、変更なし構築閉包はmacOSで
  6工程・13入力である。対象実行と20回反復が成功し、公開API契約は1,607件、未対応基準は27,407件に
  なった。製品実装、公開API、ABI、集計規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 信号圧縮契約の限定構築で完了した作業

- `libs/global/kis_signal_compressor.cpp`と`libs/global/kis_signal_compressor_with_param.cpp`は同じファイル
  位置を保ち、`libs/global/CMakeLists.txt`の`kritaglobal`直接ソースから新規
  `kritaglobalsignalcompressorobjects`の所有へ移した。`kritaglobal`は新対象のオブジェクトを集約し、
  従来の公開シンボルとQtメタオブジェクトを供給する。
- 新対象はQt CoreとBoostだけへ直接接続し、自動メタオブジェクト生成を含む変更なし構築閉包はmacOSで
  4工程・9入力である。製品所有者`kritaglobal`は57工程・114入力であり、短い信号圧縮契約は製品
  ライブラリー全体と既存の長時間試験を構築せずに実装2ソースへ接続できる。限定構築に成功し、製品挙動、
  公開API、ABI、タイマー規則は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は
  実行していない。

## R2-G19b 信号圧縮 public API契約で完了した作業

- `libs/global/kis_signal_compressor.h`の方式・低速処理方式、3構築経路、遅延・方式設定、即時・延期・待機
  配送、待機判定、保留照会、開始・停止からなる20 APIと、
  `libs/global/kis_signal_compressor_with_param.h`の関数・信号変換、引数付き圧縮からなる14 APIを、新規
  `libs/global/tests/KisSignalCompressorContractTest.cpp`の4試験へ対応付けた。
- 新試験は初回即時と次回保留、延期、初回即時後延期、初回待機、待機判定による早期配送、関数とQt信号の
  双方向変換、引数付き延期中の最新値、停止・破棄後の配送抑止を観測する。絶対時間の統計は既存の
  `KisSignalCompressorTest`が担当し、新試験は100ミリ秒以内の順序契約に限定する。
- 新試験は信号圧縮の専用実装対象とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで8工程・16入力で
  ある。対象実行と20回反復が成功し、公開API契約は1,641件、未対応基準は27,373件になった。製品実装、
  公開API、ABI、タイマー規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b スレッド安全信号圧縮契約の限定構築で完了した作業

- `libs/global/kis_thread_safe_signal_compressor.cpp`は同じファイル位置を保ち、
  `libs/global/CMakeLists.txt`の`kritaglobal`直接ソースから新規
  `kritaglobalthreadsafesignalcompressorobjects`の所有へ移した。`kritaglobal`は新対象のオブジェクトを
  集約し、従来の公開シンボルとQtメタオブジェクトを供給する。
- 新対象はQt Widgetsだけへ直接接続し、自動メタオブジェクト生成を含む変更なし構築閉包はmacOSで
  3工程・7入力である。製品所有者`kritaglobal`は59工程・118入力であり、スレッド安全配送の契約試験は
  通常版の専用実装対象とこの追加1ソースだけへ接続できる。限定構築に成功し、製品挙動、公開API、ABI、
  スレッド移送規則は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は実行して
  いない。

## R2-G19b スレッド安全信号圧縮 public API契約で完了した作業

- `libs/global/kis_thread_safe_signal_compressor.h`の構築、遅延・開始・停止、保留照会、3内部制御信号から
  なる9 APIを、新規`libs/global/tests/KisThreadSafeSignalCompressorContractTest.cpp`の2試験へ対応付けた。
- 新試験はアプリケーションスレッドへの所属、通常スレッドでの即時配送と保留解除、各制御信号の送出、
  作業スレッドからの開始要求がQtの事象処理を介してアプリケーションスレッドで配送されることを観測する。
- 新試験は通常版とスレッド安全版の専用実装対象、Qt Test、Qt Widgetsへ直接接続し、変更なし構築閉包は
  macOSで11工程・22入力である。対象実行と20回反復が成功し、公開API契約は1,650件、未対応基準は
  27,364件になった。製品実装、公開API、ABI、スレッド移送規則は変更していない。Linuxと全ネイティブ
  検証は実行していない。

## R2-G19b 非循環信号接続契約の限定構築で完了した作業

- `libs/global/kis_acyclic_signal_connector.cpp`は同じファイル位置を保ち、
  `libs/global/CMakeLists.txt`の`kritaglobal`直接ソースから新規
  `kritaglobalacyclicsignalconnectorobjects`の所有へ移した。`kritaglobal`は新対象のオブジェクトを集約し、
  従来の公開シンボルとQtメタオブジェクトを供給する。
- 新対象はQt Coreだけへ直接接続し、自動メタオブジェクト生成を含む変更なし構築閉包はmacOSで
  3工程・7入力である。製品所有者`kritaglobal`は61工程・122入力であり、接続方向、値型、再帰抑止、
  協調ロックの契約試験を製品ライブラリー全体から分離できる。限定構築に成功し、製品挙動、公開API、
  ABI、接続規則は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 非循環信号接続 public API契約で完了した作業

- `libs/global/kis_acyclic_signal_connector.h`の7値型に対する前方向・逆方向接続、範囲付きロック型、
  構築・破棄、手動ロック、状態照会、協調接続器からなる22 APIを、新規
  `libs/global/tests/KisAcyclicSignalConnectorContractTest.cpp`の4試験へ対応付けた。
- 新試験は各方向の同値配送、重複接続の抑止、入れ子ロックと範囲付きロックによる配送抑止、協調接続器間の
  ロック共有と親所有寿命を観測する。色値は接続器が内容を参照しない不透明な参照として検査し、色管理
  ライブラリーを試験依存へ追加していない。
- 新試験は非循環信号接続の専用実装対象とQt Test、Qt Coreだけへ直接接続し、変更なし構築閉包はmacOSで
  7工程・14入力である。対象実行と20回反復が成功し、公開API契約は1,672件、未対応基準は27,342件に
  なった。製品実装、公開API、ABI、接続・ロック・寿命規則は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b Qt公開シグナル採取の補正で完了した作業

- `scripts/architecture/check_public_api_contracts.py`は`Q_SIGNALS`をC++のpublicアクセス指定へ正規化し、
  信号節の先頭宣言とprivateスロット節に続く信号宣言を公開APIとして採取する。
- `scripts/tests/test_public_api_contracts.py`はprivateスロット後の2公開シグナルが欠落する初期診断を再現し、
  型、通常publicメソッド、2シグナルだけが公開面へ入ることを固定した。全5基盤試験が成功した。
- 全1,544公開ヘッダーの再採取で公開シグナル387件を追加し、公開面を29,401 API、指紋を
  `7f973237db2d71edce1325ce4470c0b2c870c8cb44ee91a8e1635adf62084517`へ更新した。直近の信号圧縮と
  非循環信号接続試験が既に観測する16シグナルを対応付け、公開API契約は1,688件、未対応基準は
  27,713件になった。製品実装、公開API、ABI、構築対象は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 信号値対応付け契約の限定構築で完了した作業

- `libs/global/KisSignalMapper.cpp`は同じファイル位置を保ち、`libs/global/CMakeLists.txt`の
  `kritaglobal`直接ソースから新規`kritaglobalsignalmapperobjects`の所有へ移した。`kritaglobal`は
  新対象のオブジェクトを集約し、従来の公開シンボルとQtメタオブジェクトを供給する。
- 新対象はQt Coreだけへ直接接続し、自動メタオブジェクト生成を含む変更なし構築閉包はmacOSで
  3工程・7入力である。製品所有者`kritaglobal`は63工程・126入力であり、送信元と4値型の対応付け、
  配送、除去、寿命の契約試験を製品ライブラリー全体から分離できる。限定構築に成功し、製品挙動、
  公開API、ABI、信号対応付け規則は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ
  検証は実行していない。

## R2-G19b 信号値対応付け public API契約で完了した作業

- `libs/global/KisSignalMapper.h`の4値型に対する設定・逆引き・公開シグナル、明示・送信元指定配送、
  対応除去、構築・破棄からなる18 APIを、新規`libs/global/tests/KisSignalMapperContractTest.cpp`の
  4試験へ対応付けた。
- 新試験は同一送信元に対応する整数、文字列、ウィジェット識別子、オブジェクトの同値配送と逆引き、
  Qt送信元からの引数なし配送、明示除去と送信元破棄による対応消去、QObject親所有の寿命を観測する。
- 新試験は信号値対応付けの専用実装対象とQt Test、Qt Coreだけへ直接接続し、変更なし構築閉包はmacOSで
  7工程・14入力である。対象実行と20回反復が成功し、公開API契約は1,706件、未対応基準は27,695件に
  なった。製品実装、公開API、ABI、信号対応付け・寿命規則は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 明示オーバーライド採取の補正で完了した作業

- `scripts/architecture/check_public_api_contracts.py`は`override`をC++の指定子として除外し、
  明示オーバーライドされた公開メソッドとデストラクターを通常の公開宣言として採取する。
- `scripts/tests/test_public_api_contracts.py`は公開基底デストラクターを上書きする公開デストラクターが
  欠落する初期診断を再現し、上書き側も公開面へ入ることを固定した。
- 全1,544公開ヘッダーの再採取で明示オーバーライド572件を回収し、`override`を引数名に使う1宣言を
  型だけの同等な識別子へ正規化した。公開面を29,972 API、指紋を
  `ee8d114ea870ea9c3dc0ebbd656b80ba23158cfe08a87f13471d78a532a9cbb8`へ更新し、公開API契約は
  1,706件、未対応基準は28,266件になった。製品実装、公開API、ABI、構築対象は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 非再帰同期接続契約の限定構築で完了した作業

- `libs/global/KisSynchronizedConnection.cpp`は同じファイル位置を保ち、`libs/global/CMakeLists.txt`の
  `kritaglobal`直接ソースから新規`kritaglobalsynchronizedconnectionobjects`の所有へ移した。
  `kritaglobal`は新対象のオブジェクトを集約し、従来の公開シンボルを供給する。
- 新対象はQt CoreとBoostだけへ直接接続し、変更なし構築閉包はmacOSで1工程・3入力である。製品所有者
  `kritaglobal`は63工程・126入力であり、同期事象、同一スレッド即時配送、待ち行列、信号・コールバック
  接続の契約試験を製品ライブラリー全体から分離できる。限定構築に成功し、製品挙動、公開API、ABI、
  配送規則は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 非再帰同期接続 public API契約で完了した作業

- `libs/global/KisSynchronizedConnection.h`の同期事象と宛先寿命、基底の事象型・試験方式・配送障壁、
  テンプレート接続の引数組・コールバック、2構築経路、開始・保留照会、入力・出力・一括接続からなる
  22 APIを、新規`libs/global/tests/KisSynchronizedConnectionContractTest.cpp`の5試験へ対応付けた。
- 新試験は事象コピーと宛先弱参照、同一スレッド即時配送、通常事象待ち行列の入力順配送、Qt入力信号と
  非QObject出力関数の個別・一括接続、試験方式の設定・照会、全配送障壁の呼出しを観測する。
- 新試験は非再帰同期接続の専用実装対象とQt Test、Qt Coreだけへ直接接続し、変更なし構築閉包はmacOSで
  5工程・11入力である。対象実行と20回反復が成功し、公開API契約は1,728件、未対応基準は28,244件に
  なった。製品実装、公開API、ABI、事象・配送・接続規則は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b Qtフラグ型別名採取の補正で完了した作業

- `scripts/architecture/check_public_api_contracts.py`は`Q_DECLARE_FLAGS`を`QFlags`型別名へ展開し、
  マクロ名を偽の関数またはメソッドとして数えず、生成される公開型を採取する。
- `scripts/tests/test_public_api_contracts.py`は2列挙値から宣言したQtフラグ型が欠落する初期診断を再現し、
  型別名として採取され、マクロ名の関数が残らないことを固定した。
- 全1,544公開ヘッダーの再採取で実宣言64件を回収し、偽のマクロ関数24件を除去した。公開面を
  30,012 API、指紋を`421efe1139c87d97ec40853421fe5161f8bd7f2ed8203904b0fc5def7c3cde44`へ
  更新し、既存のノード追加フラグ試験へ型別名1件を対応付けた。公開API契約は1,729件、未対応基準は
  28,283件になった。製品実装、公開API、ABI、構築対象は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b Qt登録マクロ後続宣言の補正で完了した作業

- `scripts/architecture/check_public_api_contracts.py`は`Q_DECLARE_METATYPE`と
  `Q_DECLARE_OPERATORS_FOR_FLAGS`を登録マクロとして除外し、直後の公開宣言を独立して採取する。
- `scripts/tests/test_public_api_contracts.py`は2登録マクロの直後にある最初の公開関数が欠落する初期診断を
  再現し、連続する2公開関数がともに公開面へ入ることを固定した。
- 全1,544公開ヘッダーの再採取で後続の実宣言9件を回収し、偽の登録マクロ関数11件を除去した。公開面を
  30,010 API、指紋を`e735c0c494ac85e517915a573efbf355fc0e63579c7132e89f49becc5730bf8d`へ
  更新し、公開API契約は1,729件、未対応基準は28,281件になった。製品実装、公開API、ABI、構築対象は
  変更していない。容量確保のため追跡外の`build/tdd-macos/plugins`生成物だけを削除し、`libs/global`の
  増分構築木を保持した。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 矩形領域契約の限定構築で完了した作業

- `libs/global/KisRegion.cpp`は同じファイル位置を保ち、`libs/global/CMakeLists.txt`の`kritaglobal`
  直接ソースから新規`kritaglobalregionobjects`の所有へ移した。`kritaglobal`は新対象のオブジェクトを
  集約し、従来の公開シンボルを供給する。
- 新対象はQt Core、Qt Gui、Boostだけへ直接接続し、変更なし構築閉包はmacOSで1工程・3入力である。
  製品所有者`kritaglobal`は63工程・126入力であり、疎矩形結合、重複近似、構築・比較・交差、Qt領域変換、
  平行移動の契約試験を製品ライブラリー全体から分離できる。限定構築に成功し、製品挙動、公開API、ABI、
  矩形領域規則は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 矩形領域 public API契約で完了した作業

- `libs/global/KisRegion.h`のクラス、6構築経路、コピー代入、比較、疎矩形結合、重複近似、重複除去、
  形状照会、矩形との積、Qt領域との相互変換、直接・複製平行移動からなる22 APIを、新規
  `libs/global/tests/KisRegionContractTest.cpp`の5試験へ対応付けた。
- 新試験は空・単一・列挙・疎矩形からの構築、縦横タイル結合、重複しない近似結果による入力領域の
  完全被覆、Qt領域の往復と矩形積、元領域を保持する複製移動と直接移動の一致を観測する。初回構築で
  Qt領域の未提供API使用を診断し、初回実行で公開保証より強い内部矩形分割数の仮定を診断した後、
  領域包含と非重複という公開保証へ試験を限定した。製品実装は変更していない。
- 新試験は矩形領域の専用実装対象とQt Test、Qt Guiだけへ直接接続し、変更なし構築閉包はmacOSで
  5工程・11入力である。対象実行と20回反復が成功し、公開API契約は1,751件、未対応基準は28,259件に
  なった。製品挙動、公開API、ABI、矩形領域規則は変更していない。Linux実機は接続応答がなく、
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 関数信号変換器の既存観測対応で完了した作業

- `libs/global/tests/KisSignalCompressorContractTest.cpp`の既存
  `functionAndSignalProxiesForwardCalls`は`FunctionToSignalProxy::start()`から
  `FunctionToSignalProxy::timeout()`が一度送出されることを`QSignalSpy`で既に観測していたため、
  Qt信号採取後に未対応となっていた同信号APIを既存契約へ対応付けた。
- 専用試験対象の変更なし構築閉包はmacOSで8工程・16入力であり、対象実行に成功した。公開API契約は
  1,752件、未対応基準は28,258件になった。試験、製品実装、公開API、ABI、信号変換挙動、構築対象は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 安定識別子の既存等価比較対応で完了した作業

- `libs/global/tests/KoIDContractTest.cpp`の既存
  `comparisonOperatorsUseIdentifierAndDisplayName`は同一識別子の表示名が異なっても
  `operator==`が真になることを既に観測していたため、未対応となっていた同APIを既存契約へ
  対応付けた。
- 専用試験対象の変更なし構築閉包はmacOSで5工程・12入力であり、対象実行に成功した。公開API契約は
  1,753件、未対応基準は28,257件になった。試験、製品実装、公開API、ABI、識別子比較挙動、構築対象は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b メモリー参照追跡契約の限定構築で完了した作業

- `libs/global/kis_memory_leak_tracker.cpp`は同じファイル位置を保ち、`libs/global/CMakeLists.txt`の
  `kritaglobal`直接ソースから新規`kritaglobalmemoryleaktrackerobjects`の所有へ移した。
  `kritaglobal`は新対象のオブジェクトを集約し、従来の公開シンボルを供給する。
- 新対象はQt Coreだけへ直接接続し、変更なし構築閉包はmacOSで1工程・3入力である。製品所有者
  `kritaglobal`は63工程・126入力であり、単一個体、参照・解除、参照情報出力、非Linux・リリース構成の
  無操作規則を製品ライブラリー全体から分離して試験できる。限定構築に成功し、製品挙動、公開API、ABI、
  参照追跡規則は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b メモリー参照追跡 public API契約で完了した作業

- `libs/global/kis_memory_leak_tracker.h`のクラス、構築・破棄、共有個体照会、生ポインターと型付き
  ポインターの参照登録・解除、個別・全体の参照情報出力からなる10 APIを、新規
  `libs/global/tests/KisMemoryLeakTrackerContractTest.cpp`の2試験へ対応付けた。
- 新試験は共有個体の非null性と照会間の同一性を観測する。参照追跡経路は、追跡構成でQt診断を生成し、
  `NDEBUG`または非Linuxで追跡を無効にする構成では全経路が無出力の無操作になることを観測する。
- 新試験はメモリー参照追跡の専用実装対象とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで
  5工程・11入力である。対象実行に成功し、公開API契約は1,763件、未対応基準は28,247件になった。
  製品実装、公開API、ABI、参照追跡規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 描画補助契約の限定構築で完了した作業

- `libs/global/kis_painting_tweaks.cpp`は同じファイル位置を保ち、`libs/global/CMakeLists.txt`の
  `kritaglobal`直接ソースから新規`kritaglobalpaintingtweaksobjects`の所有へ移した。
  `kritaglobal`は新対象のオブジェクトを集約し、従来の公開シンボルを供給する。
- 新対象はQt CoreとQt Guiだけへ直接接続し、変更なし構築閉包はmacOSで1工程・3入力である。
  製品所有者`kritaglobal`は63工程・126入力であり、安全な切抜き領域、点線ペン、ペン・ブラシ状態寿命、
  色演算、矩形頂点変換を製品ライブラリー全体から分離して試験できる。限定構築に成功し、製品挙動、
  公開API、ABI、描画補助規則は変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は
  実行していない。

## R2-G19b 描画補助 public API契約で完了した作業

- `libs/global/kis_painting_tweaks.h`の切抜き領域、点線ペン初期化、ペン・ブラシ状態保存器、色演算、
  矩形の描画頂点・テクスチャ座標変換からなる18 APIを、新規
  `libs/global/tests/KisPaintingTweaksContractTest.cpp`の6試験へ対応付けた。
- 新試験は複合切抜き領域と外接矩形、指定した点線・実線・色、4構築経路の状態復元とnull無操作、
  RGB混合・色差・色分離、2三角形の座標順を観測する。初回実行は粗い輝度計算が黒を0.0ではなく
  0.1へ量子化する既存挙動を診断した。この1 APIを既知不具合として他の維持契約から分け、sRGB表を
  使う黒・白と表を使わない赤の現在値を固定した。製品実装は変更していない。
- 新試験は描画補助の専用実装対象とQt Test、Qt Guiだけへ直接接続し、変更なし構築閉包はmacOSで
  5工程・11入力である。対象実行に成功し、公開API契約は1,781件、未対応基準は28,229件になった。
  製品挙動、公開API、ABI、描画補助規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 操作ハンドル様式契約の限定構築で完了した作業

- `libs/global/KisHandleStyle.cpp`は同じファイル位置を保ち、`libs/global/CMakeLists.txt`の
  `kritaglobal`直接ソースから新規`kritaglobalhandlestyleobjects`の所有へ移した。
  `kritaglobal`は新対象のオブジェクトを集約し、従来の公開シンボルを供給する。
- 新対象は描画補助の専用実装対象、Qt Core、Qt Guiだけへ直接接続し、変更なし構築閉包はmacOSで
  1工程・3入力である。製品所有者`kritaglobal`は63工程・126入力であり、ハンドル配色、反復描画様式、
  継承・選択・グラデーション・強調様式を製品ライブラリー全体から分離して試験できる。限定構築に成功し、
  製品挙動、公開API、ABI、操作ハンドル様式は変更していない。Linux、製品ライブラリーの再リンク、
  全ネイティブ検証は実行していない。

## R2-G19b 操作ハンドル様式 public API契約で完了した作業

- `libs/global/KisHandleStyle.h`の既定配色、反復描画値、公開反復列、継承・主選択・副選択・
  グラデーション・強調・選択済み様式からなる27 APIを、新規
  `libs/global/tests/KisHandleStyleContractTest.cpp`の5試験へ対応付けた。
- 新試験は8配色役割のRGBA値、有効・無効反復値とペン・ブラシ、継承様式の共有寿命、破線系様式の
  輪郭・塗り、グラデーション矢印と主選択の同一性、強調・選択系の配色を観測する。初回構築は試験側の
  `QBrush`初期化が関数宣言として解釈される構文曖昧性を診断し、波括弧初期化で値構築を明示した。
  製品実装は変更していない。
- 新試験は操作ハンドル様式と描画補助の専用実装対象、Qt Test、Qt Guiだけへ直接接続し、変更なし
  構築閉包はmacOSで6工程・13入力である。対象実行に成功し、公開API契約は1,808件、未対応基準は
  28,202件になった。製品挙動、公開API、ABI、操作ハンドル様式は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b 診断カテゴリ契約の限定構築で完了した作業

- `libs/global/kis_debug.cpp`は同じファイル位置を保ち、`libs/global/CMakeLists.txt`の`kritaglobal`
  直接ソースから新規`kritaglobaldebugobjects`の所有へ移した。`kritaglobal`は新対象のオブジェクトを
  集約し、従来の公開シンボルを供給する。
- 新対象はQt Coreだけへ直接接続し、変更なし構築閉包はmacOSで1工程・3入力である。製品所有者
  `kritaglobal`は63工程・126入力であり、21診断カテゴリ、関数名整形、構成依存バックトレースを
  製品ライブラリー全体から分離して試験できる。限定構築に成功し、製品挙動、公開API、ABI、診断規則は
  変更していない。Linux、製品ライブラリーの再リンク、全ネイティブ検証は実行していない。

## R2-G19b 診断カテゴリ public API契約で完了した作業

- `libs/global/kis_debug.h`の21診断カテゴリ、GCC互換関数名整形、構成依存バックトレースからなる
  23 APIを、新規`libs/global/tests/KisDebugContractTest.cpp`の3試験へ対応付けた。
- 新試験は各診断カテゴリの安定名称、戻り値型と引数列を除く名前空間付き・自由関数名の整形、
  バックトレース対応構成の角括弧・改行形式と非対応構成の空文字列を観測する。
- 新試験は診断カテゴリの専用実装対象とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで
  5工程・11入力である。対象実行に成功し、公開API契約は1,831件、未対応基準は28,179件になった。
  製品実装、公開API、ABI、診断規則は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 構成変更通知契約の限定構築で完了した作業

- `libs/global/kis_config_notifier.cpp`と`libs/global/kis_config_notifier.h`は同じファイル位置を保ち、
  `libs/global/CMakeLists.txt`の`kritaglobal`直接ソースから新規
  `kritaglobalconfignotifierobjects`の所有へ移した。`kritaglobal`は新対象のオブジェクトを集約し、
  従来の公開シンボルを供給する。
- 新対象は診断カテゴリと信号圧縮器の専用実装対象、Qt Coreだけへ直接接続し、変更なし構築閉包は
  macOSで8工程・18入力である。製品所有者`kritaglobal`は65工程・130入力であり、構成変更通知と
  連続通知の圧縮規則を製品ライブラリー全体から分離して試験できる。限定構築に成功し、製品挙動、
  公開API、ABI、構成変更通知規則は変更していない。Linux、製品ライブラリーの再リンク、
  全ネイティブ検証は実行していない。

## R2-G19b 構成変更通知 public API契約で完了した作業

- `libs/global/kis_config_notifier.h`の全プラットフォーム共通のクラス、構築・破棄、共有個体照会、
  8通知、8信号からなる20 APIを、新規`libs/global/tests/KisConfigNotifierContractTest.cpp`の
  4試験へ対応付けた。Android固有の3通知と3信号はmacOSで宣言・実装されないため、Android実機の
  後続契約に残した。
- 新試験は共有個体の同一性と局所個体の独立性、5種類の引数なし通知の同期送出、配色テーマ名と
  長押し有効値の転送、フレーム欠落方式通知の初回即時送出と後続圧縮を観測する。初回対象実行に成功し、
  20回反復にも成功した。
- 新試験は構成変更通知、信号圧縮器、診断カテゴリの専用実装対象とQt Test、Qt Coreだけへ直接接続し、
  変更なし構築閉包はmacOSで12工程・25入力である。公開API契約は1,851件、未対応基準は28,159件に
  なった。製品実装、公開API、ABI、構成変更通知規則は変更していない。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 代替ジョブ実行フラグ型別名の挙動契約で完了した作業

- `libs/image/KisFakeRunnableStrokeJobsExecutor.h`の`Flags`型別名を、既存
  `libs/image/tests/KisStrokeJobContractTest.cpp`の`fakeExecutorFlagValuesDescribeBarrierOptIn`へ
  対応付けた。試験は既定フラグが障壁許可を持たず、型別名に同フラグを設定すると照会できることを
  明示して、列挙値だけでなく`QFlags`としての公開契約を観測する。
- 既存試験はストロークジョブ専用実装対象とQt Testだけへ直接接続し、変更なし構築閉包はmacOSで
  9工程・19入力である。対象実行と20回反復に成功し、公開API契約は1,852件、未対応基準は
  28,158件になった。製品実装、公開API、ABI、ジョブ実行規則、構築対象は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b 保存方針フラグ型別名の既存観測対応で完了した作業

- `libs/impex/tests/kis_import_export_utils_values_test.cpp`の既存
  `publicEnumValuesRemainStable`は`KritaUtils::SaveFlags`で3保存方針を合成し、各ビットを個別に
  照会できることを既に観測していたため、未対応となっていた同型別名APIを既存契約へ対応付けた。
- 専用試験対象の変更なし構築閉包はmacOSで4工程・14入力である。対象実行と20回反復に成功し、
  公開API契約は1,853件、未対応基準は28,157件になった。試験、製品実装、公開API、ABI、保存方針、
  構築対象は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 画像公開型 public API契約で完了した作業

- `libs/image/kis_types.h`の未対応だった侵入型・固定型・制限型・Qt型の共有・弱参照、一覧・
  ベクター・反復子・進捗監視・投影更新印からなる101型別名と、サムネイル境界方式の列挙と
  2列挙値からなる104 APIを、新規`libs/image/tests/KisImageTypesContractTest.cpp`の4試験へ
  対応付けた。
- 新試験は各型別名を`std::is_same_v`で宣言された所有対象、参照方式、要素型、格納方式へ厳密に
  照合し、ストローク識別子がストローク弱参照と同じ型であること、粗い境界と正確な境界の安定値を
  観測する。ヘッダーの構築可否だけでなく、呼出し側が依存する型同一性を契約にした。
- 新試験は製品ライブラリーへ接続せず、画像・全体ヘッダーとQt Core、Qt Testだけを使用する。
  変更なし構築閉包はmacOSで4工程・8入力であり、初回対象実行と20回反復に成功した。公開API契約は
  1,957件、未対応基準は28,053件になった。製品実装、公開API、ABI、所有方式は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 図形基礎列挙 public API契約で完了した作業

- `libs/flake/KoFlake.h`の塗り変種、塗り方式、マーカー位置、選択一覧、選択ハンドル、形状探索、
  前景・背景様式、アンカー位置、キャンバス資源からなる9列挙と40列挙値を、既存
  `libs/flake/tests/KoFlakeTypesContractTest.cpp`の5試験へ対応付けた。
- 試験は各列挙の宣言順に対応する整数値、選択ハンドルの時計回り順、アンカーの3掛ける3格子順、
  ホット位置資源の固定識別値を観測する。既存専用試験は製品ライブラリーへ接続せず、図形ヘッダーと
  Qt Testだけを使用する。
- 変更なし構築閉包はmacOSで4工程・8入力であり、対象実行と20回反復に成功した。公開API契約は
  2,006件、未対応基準は28,004件になった。製品実装、公開API、ABI、図形基礎値、構築対象は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 関数局所宣言の公開面除外で完了した作業

- `scripts/architecture/check_public_api_contracts.py`はCtagsの`scopeKind`が関数またはメソッドを
  示す宣言と、その局所型配下の宣言を公開APIから除外する。新規単体試験は関数内の型別名、局所構造体、
  局所構造体の公開データが採取されず、外側の公開関数だけが残ることを固定する。初回単体試験は
  4件すべてを公開APIとして返す既存診断を再現し、実装後は公開関数1件だけになった。
- 全公開面から関数局所の44型別名を除いた。38件は未対応台帳から除去し、既に関数挙動へ誤対応していた
  森反復処理2件と灰色ブラシ色処理4件は対応表から除去した。関数本体の挙動契約は維持している。
  公開面は29,966 API、対応済みは2,000件、未対応基準は27,966件になった。製品実装、公開API、ABI、
  既存試験、構築対象は変更していない。

## R2-G19b 全体基礎値 public API契約で完了した作業

- `libs/global/kis_global.h`の整数・選択・筆圧定数、輪郭・現行カーソル・旧カーソル列挙、角度・
  冪・幾何・矩形切出し関数からなる69 APIを、新規
  `libs/global/tests/KisGlobalValuesContractTest.cpp`の5試験へ対応付けた。
- 新試験は安定した設定値、角度正規化と単位変換、距離・射影・吸着、整数・浮動小数点矩形の分割と
  残り領域を観測する。初回実行は`nextPowerOfTwo(16)`が16ではなく厳密に次の2冪32を返す既存規則を
  診断したため、同規則を期待値として固定した。製品実装は変更していない。
- 新試験は製品ライブラリーへ接続せず、全体ヘッダーとQt Core、Qt Testだけを使用する。変更なし
  構築閉包はmacOSで4工程・8入力であり、修正後の対象実行と20回反復に成功した。公開API契約は
  2,069件、未対応基準は27,897件になった。公開API、ABI、基礎値は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b PSD資源識別子 public API契約で完了した作業

- `libs/psd/psd_resource_section.h`と
  `plugins/impex/tiff/kis_tiff_psd_resource_record.h`で重複して公開されるPSD資源識別子を監査し、
  両ヘッダーの列挙型と90列挙値からなる182 APIを、新規
  `plugins/impex/tiff/tests/PsdResourceIdContractTest.cpp`へ対応付けた。
- 新試験は90識別子それぞれの固定数値とPSD本体・TIFF記録間の一致を同じ期待値表から観測する。
  初回対象構築はTIFF公開ヘッダーが未使用の`psd.h`を要求する診断で停止したため、同includeを除去して
  公開ヘッダーをQt型と自身の出力指定だけで利用できるようにした。公開APIとABIは変更していない。
- 新試験は製品ライブラリーへ接続せず、両公開ヘッダー、Qt Test、TIFFの公開構築情報だけを使用する。
  変更なし構築閉包はmacOSで4工程・9入力であり、対象実行と20回反復に成功した。公開API契約は2,251件、
  未対応基準は27,715件になった。クラス本体、入出力、公開状態の各9 APIは後続の実動作契約へ残した。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b SVG文字基礎列挙 public API契約で完了した作業

- `libs/flake/text/KoSvgText.h`の文字方向、双方向制御、基線、空白・改行、整列、装飾、文字経路、
  CSS長さ単位、字体・数字・東アジア字形からなる34列挙と148列挙値、3フラグ型別名、2公開
  キーワード表の187 APIを、新規`libs/flake/tests/KoSvgTextEnumContractTest.cpp`の5試験へ対応付けた。
- 新試験は各方式の宣言順と独立ビット値、フラグの型同一性と合成・照会、字体伸縮9語と絶対字体寸法
  7語のCSS対応順を観測する。既存の総合SVG文字試験は使用せず、製品ライブラリーへ接続しない
  ヘッダー専用対象とした。
- 初回対象構築は直接・推移的なDOMヘッダー要求を診断した。`KoSvgText.h`自身で未使用だった
  `QDomDocument` includeを除去し、実際にDOM型を使う`KoShape.h`由来のQt Xmlだけを対象へ明示した。
  変更なし構築閉包はmacOSで4工程・8入力であり、対象実行と20回反復に成功した。公開API契約は
  2,438件、未対応基準は27,528件になった。公開API、ABI、列挙値、CSSキーワードは変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b SVG文字プロパティ識別子 public API契約で完了した作業

- `libs/flake/text/KoSvgTextProperties.h`の文字方向、字体、装飾、空白、段落、KRA固有情報を格納する
  1列挙と62識別子の63 APIを、既存`libs/flake/tests/KoSvgTextEnumContractTest.cpp`の
  `propertyIdentifiersRemainContiguous`へ対応付けた。
- 試験は`WritingModeId`から反復上限`LastPropertyId`まで全識別子を列挙し、0起点で欠番のない宣言順を
  観測する。直前のヘッダー専用対象を再利用し、製品ライブラリーや総合SVG文字試験への依存を増やして
  いない。変更なし構築閉包はmacOSで4工程・8入力のままであり、対象実行と20回反復に成功した。
  公開API契約は2,501件、未対応基準は27,465件になった。公開API、ABI、識別子は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b Exifタグ識別子 public API契約で完了した作業

- `libs/painting/metadata/kis_meta_data_tags.h`のExif Image 242件、Photo 81件、GPSInfo 32件、
  Iop 5件からなる360公開タグ定数を、新規
  `libs/painting/metadata/tests/KisMetaDataTagsContractTest.cpp`の4試験へ対応付けた。
- 宣言をCtagsで機械採取し、完全修飾名と16進期待値を各名前空間の観測へ一対一で接続した。試験は
  外部Exif/TIFF形式との対応に使う16ビット識別値を全件で比較し、自動生成元由来の大量定数に手作業の
  抜けや転記順依存を持ち込まない構成とした。
- 新試験は製品メタデータライブラリーへ接続せず、定数ヘッダーとQt Testだけを使用する。変更なし
  構築閉包はmacOSで4工程・8入力であり、対象実行と20回反復に成功した。公開API契約は2,861件、
  未対応基準は27,105件になった。公開API、ABI、タグ値、メタデータ実装は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b PSD形式型の所有分離とpublic API契約で完了した作業

- `libs/psdutils/psd.h`先頭に同居していた寸法・チャンネル上限、固定小数型、バイト順、圧縮、色、
  色採取、レイヤー効果の85 APIを、新規`libs/psdutils/psd_types.h`へ移した。`psd.h`は新所有先を
  includeして既存利用元への公開を維持する。
- 軽量利用元のincludeを次のように新所有先へ向け、色・資源・描画効果クラスの宣言を不要にした。
  - `libs/psdutils/compression.h`から`libs/psdutils/psd_types.h`
  - `libs/psd/psd_header.h`から`libs/psdutils/psd_types.h`
  - `plugins/impex/psd/psd_colormode_block.h`から`libs/psdutils/psd_types.h`
  - `plugins/impex/psd/psd_image_data.h`から`libs/psdutils/psd_types.h`
  - `plugins/impex/tiff/kis_tiff_psd_layer_record.h`から`libs/psdutils/psd_types.h`
  - `plugins/impex/tiff/kis_tiff_converter.cc`から`libs/psdutils/psd_types.h`
- 新規`libs/psdutils/tests/PsdFormatValuesContractTest.cpp`は形式上限・格納方式29 API、色採取17 API、
  レイヤー効果39 APIの型と固定値を観測する。製品ライブラリーへ接続しない変更なし構築閉包はmacOSで
  4工程・8入力であり、対象実行と20回反復に成功した。
- 宣言所有の移動により公開ヘッダーは1,545件になり、API総数29,966件と識別子集合は維持した。
  対応済みは2,946件、未対応基準は27,020件になった。製品API、ABI、形式値は変更していない。
  変更した製品翻訳単位はオブジェクト指定でも`kritapsdutils`側533工程、PSD/TIFF側1,710工程超の
  順序依存を持つため構築しておらず、後続の製品対象分割に残す。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 合成方式識別子の所有分離とpublic API契約で完了した作業

- `libs/pigment/KoCompositeOpRegistry.h`に登録処理と同居していた148の合成方式文字列を、
  新規`libs/pigment/KoCompositeOpIds.h`へ移した。旧ヘッダーは新所有先をincludeし、既存利用元への
  公開と文字列値を維持する。
- 値だけを使う公開利用元を次のように軽量所有先へ向け、登録クラス、`KoID`、翻訳基盤への推移依存を
  除いた。
  - `libs/painting/KisStrokeCompatibilityInfo.h`から`libs/pigment/KoCompositeOpIds.h`
  - `plugins/paintops/libpaintop/KisMaskingBrushOptionProperties.h`から`libs/pigment/KoCompositeOpIds.h`
  - `plugins/tools/basictools/kis_tool_fill.h`から`libs/pigment/KoCompositeOpIds.h`
- 最初の対象構築は旧ヘッダーから`KoID.h`を経て`klocalizedstring.h`を要求する診断で停止した。
  分離後の新規`libs/pigment/tests/KoCompositeOpIdsContractTest.cpp`は製品ライブラリーへ接続せず、
  148識別子と共有文字列を一対一で観測する。変更なし構築閉包はmacOSで4工程・8入力であり、
  対象実行と20回反復に成功した。
- 公開ヘッダーは1,546件、API総数は29,966件となり、宣言識別子集合を維持した。対応済みは3,094件、
  未対応基準は26,872件になった。製品API、ABI、合成方式値、登録処理は変更していない。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b Android終了情報識別子 public API契約で完了した作業

- `libs/global/KisAndroidExitInfo.h`の終了理由1列挙・17列挙値と、プロセス重要度1列挙・12列挙値の
  31 APIを、既存`libs/global/tests/KisGlobalValuesContractTest.cpp`の
  `androidExitCodesMatchPlatformContract`へ対応付けた。
- 試験はAndroid `ApplicationExitInfo`の理由0から16と、`RunningAppProcessInfo`の重要度100から
  1,000を全件で比較する。既存のヘッダー専用対象を再利用し、製品ライブラリーとAndroid実装を
  接続していない。変更なし構築閉包はmacOSで4工程・8入力のままであり、対象実行と20回反復に
  成功した。
- 対応済みは3,125件、未対応基準は26,841件になった。終了情報の取得・妥当性・診断文字列を扱う
  残り8 APIはAndroid実機で観測する後続契約へ残した。製品実装、公開API、ABI、OS対応値は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 色選択器構成 public API契約で完了した作業

- `libs/widgets/KisColorSelectorConfiguration.h`の2列挙・28列挙値、4公開状態値、構築、文字列化、
  復元、等価性からなる全41 APIを、新規
  `libs/widgets/tests/KisColorSelectorConfigurationContractTest.cpp`の3試験へ対応付けた。
- 試験は形状と色成分の保存順、既定状態と明示状態、文字列往復、不完全・非数値・負値・上限超過の
  入力で状態を維持する誤り動作を観測する。最初の誤り契約は、文字列構築子が不完全な入力で4状態値を
  初期化しないことを診断した。文字列構築子を既定構築へ委譲し、変換成否と上下限を状態変更前に
  検証するようにした。
- 新試験は製品ライブラリーへ接続せず、対象ヘッダーとQt Testだけを使用する。変更なし構築閉包は
  macOSで4工程・8入力であり、修正後の対象実行と20回反復に成功した。対応済みは3,166件、
  未対応基準は26,800件になった。公開API、ABI、保存形式、有効入力の動作は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 長さ単位実装の限定構築とpublic API契約で完了した作業

- `libs/global/KoUnit.cpp`はファイル位置を維持し、65工程・130入力の`kritaglobal`直接ソースから、
  新規`kritaglobalunitobjects`の1工程・3入力へ実装所有を分けた。`kritaglobal`は同じオブジェクトを
  集約し、製品リンク、公開API、ABIを維持する。
- CMakeから外れていた試験を次のように所有元へ移し、宣言と実装を局所検証できるようにした。
  - `libs/flake/tests/TestKoUnit.cpp`から`libs/global/tests/KoUnitContractTest.cpp`
  - `libs/flake/tests/TestKoUnit.h`から`libs/global/tests/KoUnitContractTest.cpp`内の試験クラス
- 新試験は14換算係数、7丸め係数、2列挙・12列挙値・フラグ型、構築と等価性、記号、UI一覧、
  利用者値換算、解析、画素変換、診断出力からなる全61 APIを9試験へ対応付けた。最初の実行は
  ラジアン入力の生成精度が期待許容差を下回る試験入力を診断し、17桁の入力へ固定した。製品実装は
  変更していない。
- 専用試験は単位実装対象、Qt Test、Qt Core、Qt Gui、翻訳基盤だけへ接続し、変更なし構築閉包は
  macOSで5工程・12入力である。対象実行と20回反復に成功し、`kritaglobal`全体は構築していない。
  対応済みは3,227件、未対応基準は26,739件になった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 図形アンカー識別子 public API契約で完了した作業

- `libs/flake/KoShapeAnchor.h`の水平位置・基準、垂直位置・基準、アンカー方式からなる5列挙と
  39列挙値の44 APIを、新規`libs/flake/tests/KoShapeAnchorEnumContractTest.cpp`の3試験へ
  対応付けた。
- 試験はODF配置規則に対応する水平・垂直の宣言順と、文字内、文字、段落、ページのアンカー順を
  全件で観測する。新試験は製品図形ライブラリーへ接続せず、対象ヘッダー、Qt Test、Qt Xmlだけを
  使用する。変更なし構築閉包はmacOSで4工程・8入力であり、対象実行と20回反復に成功した。
- 対応済みは3,271件、未対応基準は26,695件になった。配置戦略、文字位置、アンカー状態と所有期間を
  扱う残り33 APIは図形実装の後続契約へ残した。製品実装、公開API、ABI、配置識別値は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 標準操作識別子 public API契約で完了した作業

- `libs/widgetutils/config/kstandardaction.h`の標準操作1列挙と67列挙値の68 APIを、新規
  `libs/widgetutils/tests/KStandardActionEnumContractTest.cpp`の`standardActionIdsRemainStable`へ
  対応付けた。
- 試験はファイル、編集、表示、移動、しおり、ツール、設定、ヘルプ、その他の操作を宣言順に並べ、
  `ActionNone`の0から`SwitchApplicationLanguage`の66まで全識別子が連続することを観測する。
  製品ウィジェットライブラリーへ接続せず、対象ヘッダー、Qt Test、外部KConfig GUIだけを使用する。
- 変更なし構築閉包はmacOSで4工程・10入力であり、対象実行と20回反復に成功した。対応済みは
  3,339件、未対応基準は26,627件になった。操作生成、名前、ショートカットを扱う残り74 APIは
  ウィジェット実装の後続契約へ残した。製品実装、公開API、ABI、操作識別値は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 配色役割識別子 public API契約で完了した作業

- `libs/widgetutils/config/kcolorscheme.h`の配色集合、背景役割、前景役割、装飾役割、陰影役割から
  なる5列挙と28列挙値の33 APIを、新規
  `libs/widgetutils/tests/KColorSchemeEnumContractTest.cpp`の3試験へ対応付けた。
- 試験は配色集合の表示用途と、背景・前景、装飾・陰影の描画規則が参照する0始まりの宣言順を
  全件で観測する。製品ウィジェットライブラリーへ接続せず、対象ヘッダー、Qt Test、Qt Gui、
  外部KConfig Coreだけを使用する。
- 変更なし構築閉包はmacOSで4工程・9入力であり、対象実行と20回反復に成功した。対応済みは
  3,372件、未対応基準は26,594件になった。配色の構築、ブラシ取得、陰影計算、状態別ブラシを
  扱う残り19 APIは実装を接続する後続契約へ残した。製品実装、公開API、ABI、配色識別値は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 色度計算 public API契約と構築所有分離で完了した作業

- 色度計算だけを検証しても`kritapigment`全体の310工程・650入力へ接続する構造だったため、
  `libs/pigment/KoColorimetryUtils.cpp`の構築所有を`kritapigment_SRCS`から新規
  `kritapigmentcolorimetryobjects`へ移した。ファイル位置と製品APIは維持し、生成オブジェクトを
  `kritapigment`へ1回だけ集約する。
- 新規`libs/pigment/tests/KoColorimetryUtilsContractTest.cpp`の7試験が、xy・xyY・XYZ値、
  行列構築、3入力形式の色度構築、10標準色度、色域検証、白色点と色順応、測色・LMS変換、
  診断出力からなる`libs/pigment/KoColorimetryUtils.h`の全61 APIを観測する。
- 専用実装対象はQt Core、Qt Gui、Boostだけへ接続し、変更なし構築閉包は1工程・3入力、試験は
  macOSで5工程・11入力である。対象実行と20回反復に成功し、`kritapigment`全体は構築していない。
  対応済みは3,433件、未対応基準は26,533件になった。製品挙動、公開API、ABI、ファイル位置は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 表面色管理 public API契約と条件付き構築分離で完了した作業

- macOS既定では表面色管理機能が無効であり、既存の条件付き製品ディレクトリー内では契約試験を
  構成できなかった。`libs/surfacecolormanagementapi/surfacecolormanagement/KisSurfaceColorimetry.cpp`の
  構築所有を`kritasurfacecolormanagementapi_LIB_SRCS`から新規
  `libs/surfacecolormanagementapi/colorimetry/CMakeLists.txt`の
  `kritasurfacecolorimetryobjects`へ移した。ファイル位置と生成exportヘッダーの従来出力位置を維持し、
  機能有効時は生成オブジェクトを`kritasurfacecolormanagementapi`へ1回だけ集約する。
- 新規`libs/surfacecolormanagementapi/tests/KisSurfaceColorimetryContractTest.cpp`の6試験が、
  原色・伝達関数・描画意図の識別値、表示・マスタリング輝度、HDR判定、任意メタデータ、等価性、
  診断・表面報告からなる`KisSurfaceColorimetry.h`の全63 APIを観測する。報告は空白と引用符を
  正規化し、原色、伝達関数、CLL、FALLの意味内容を固定する。
- 専用実装対象はQt Core、Qt Gui、Boostだけへ接続し、通常構築から除外した。変更なし構築閉包は
  1工程・3入力、色度計算対象と組み合わせた試験はmacOSで6工程・13入力であり、対象実行と20回
  反復に成功した。機能有効の別構築木は製品閉包319工程・666入力を計画し、表面色度実装が1回だけ
  集約されることを確認して削除した。製品全体は構築していない。
- 対応済みは3,496件、未対応基準は26,470件になった。製品挙動、公開API、ABI、ファイル位置は
  変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b ブラシテクスチャ値 public API契約で完了した作業

- `plugins/paintops/libpaintop/KisTextureOptionData.h`の機能フラグ、16合成方式、19公開メンバー、
  値等価性、構造体からなる43 APIを、新規
  `plugins/paintops/libpaintop/tests/KisTextureOptionDataValueContractTest.cpp`の4試験へ対応付けた。
- 試験は機能ビットの合成、合成方式の宣言順、無効な乗算方式と等倍・中立補正・全濃度範囲から
  なる既定値、埋込み資源と全18描画設定の各差分を検出する等価性を観測する。
- 製品描画実行対象へ接続せず、対象ヘッダー、Qt Test、Qt Gui、外部KDE翻訳ヘッダーだけを使い、
  変更なし構築閉包をmacOSで4工程・9入力に保った。対象実行と20回反復に成功し、対応済みは
  3,539件、未対応基準は26,427件になった。製品実装、公開API、ABI、既定値は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b ブラシテクスチャLOD制約契約と構築所有分離で完了した作業

- `KisTextureOptionData::lodLimitations()`の実装を
  `plugins/paintops/libpaintop/KisTextureOptionData.cpp`から新規
  `plugins/paintops/libpaintop/KisTextureOptionLodLimitations.cpp`へ移した。新規
  `kritapaintoptexturelodobjects`が実装を所有し、`kritapaintopruntime`と`kritalibpaintop`へ
  生成オブジェクトを1回だけ集約する。公開ヘッダーとメソッド宣言は維持した。
- 新規`plugins/paintops/libpaintop/tests/KisTextureOptionLodContractTest.cpp`は、無効な設定が
  制約を返さず、有効な設定が`texture-pattern`識別子の低品質プレビュー制約を1件返すことを
  観測する。
- 1,103工程・2,226入力の描画実行対象へ接続せず、専用実装は1工程・3入力、試験はmacOSで
  6工程・14入力に収めた。対象実行と20回反復に成功し、対応済みは3,540件、未対応基準は
  26,426件になった。`KisTextureOptionData.h`の未対応は設定読込・書込の2 APIである。
  製品全体は構築していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b アプリケーション設定識別子 public API契約で完了した作業

- `libs/application/kis_config.h`のタッチ描画、色採取プレビュー、キャンバス表面とビット深度、
  起動セッション、メニューアイコン、OCIO管理、既定背景、選択操作バー、ルート表面、レイヤー情報、
  描画補助線からなる14列挙・51列挙値の65 APIを、新規
  `libs/application/tests/KisConfigEnumContractTest.cpp`の5試験へ対応付けた。
- 試験は設定保存と表示処理が共有する0始まりの宣言順を全件で観測する。500超の設定APIを持つ
  製品アプリケーション実装へ接続せず、対象ヘッダー、Qt Test、Qt Gui、外部KConfig、KDE翻訳、
  Boost、OpenEXRのヘッダー提供対象だけを使用する。
- 変更なし構築閉包はmacOSで4工程・15入力であり、対象実行と20回反復に成功した。対応済みは
  3,605件、未対応基準は26,361件になった。製品実装、公開API、ABI、設定識別値は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b SVG文字カーソル識別子 public API契約で完了した作業

- `plugins/tools/svgtexttool/SvgTextCursor.h`の移動方式、組版操作ハンドルからなる2列挙・
  31列挙値の33 APIを、新規
  `plugins/tools/svgtexttool/tests/SvgTextCursorEnumContractTest.cpp`の2試験へ対応付けた。
- 試験は文字・単語・行・段落移動と、位置・行寸法・基準線ハンドルが共有する0始まりの
  宣言順を全件で観測する。1,755工程・3,507入力の既存SVG文字カーソル試験へ接続せず、
  宣言を解釈するためのヘッダー提供対象だけを使用する。
- 変更なし構築閉包はmacOSで4工程・14入力であり、対象実行と20回反復に成功した。対応済みは
  3,638件、未対応基準は26,328件になった。製品実装、公開API、ABI、識別値は変更していない。
  同ヘッダーの未対応はクラス2件・メソッド61件である。Linuxと全ネイティブ検証は実行していない。

## R2-G19b ツール操作対応付け public API契約と構築所有分離で完了した作業

- `KisTool::actionToAlternateAction()`の実装を`libs/tools/kis_tool.cc`から新規
  `libs/tools/kis_tool_action_mapping.cpp`へ移した。新規`kritatoolactionmappingobjects`が実装を
  所有し、`kritatools`と専用試験へ同じ生成オブジェクトを1回ずつ集約する。公開ヘッダー、
  メソッド宣言、ABIは維持した。
- `libs/tools/kis_tool.h`の独自設定フラグ、主・代替操作、ノード描画能力、常時有効化識別子、
  操作変換からなる4列挙・31列挙値・1メソッド・1変数の37 APIを、新規
  `libs/tools/tests/KisToolActionContractTest.cpp`の5試験へ対応付けた。
- `Q_ENUMS`を空の登録マクロとして展開する採取規則と回帰試験を追加し、直後にある静的な
  操作変換メソッド1件を公開面へ復元した。全公開ヘッダーで新たに採取されるAPIは同メソッドだけで、
  公開面は1,546ヘッダー・29,967 APIになった。
- 1,077工程・2,169入力の既存ツール契約へ接続せず、専用実装は1工程・3入力、専用試験は
  macOSで5工程・17入力に収めた。対象実行と20回反復に成功した。製品`kritatools`は
  1,074工程・2,164入力のグラフで専用オブジェクトを1回だけ集約することを確認し、構築していない。
  対応済みは3,675件、未対応基準は26,292件になった。同ヘッダーの未対応はクラス1件・
  メソッド31件である。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 標準ダイアログ識別子 public API契約で完了した作業

- `libs/widgets/KoDialog.h`の標準ボタン、ボタンメニュー表示方式、題名構成からなる3列挙・
  22列挙値・3フラグ型別名の28 APIを、新規
  `libs/widgets/tests/KoDialogEnumContractTest.cpp`の3試験へ対応付けた。
- 試験は16ボタンのビット値と組合せ、即時・遅延表示方式、アプリケーション名・変更状態の
  題名ビット、HIG準拠値の別名関係を観測する。639工程・1,306入力の既存ウィジェット試験や
  635工程・1,299入力の製品ウィジェット実装へ接続せず、QtとKDEの宣言提供対象だけを使用する。
- 変更なし構築閉包はmacOSで4工程・10入力であり、対象実行と20回反復に成功した。対応済みは
  3,703件、未対応基準は26,264件になった。同ヘッダーの未対応はクラス1件・メソッド66件である。
  製品実装、公開API、ABI、識別値は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 資源モデル識別子 public API契約で完了した作業

- `libs/resources/KisResourceModel.h`の資源表列、資源表示フィルター、保管場所表示フィルターからなる
  3列挙・24列挙値の27 APIを、新規
  `libs/resources/tests/KisResourceModelEnumContractTest.cpp`の3試験へ対応付けた。
- 試験は18列の0始まりの宣言順と、無効・有効・全件からなる資源および保管場所フィルターの
  宣言順を観測する。639工程・1,306入力の既存資源モデル試験や139工程・305入力の製品資源実装へ
  接続せず、Qt、KDE翻訳、Boostの宣言提供対象だけを使用する。
- 変更なし構築閉包はmacOSで4工程・9入力であり、対象実行と20回反復に成功した。対応済みは
  3,730件、未対応基準は26,237件になった。同ヘッダーの未対応はクラス4件・メソッド72件である。
  製品実装、公開API、ABI、識別値は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 操作有効化識別子 public API契約で完了した作業

- `libs/application/ui/orchestration/kis_action.h`の画像、ノード、選択、クリップボード状態と
  編集・OpenGL条件からなる2列挙・26列挙値・2フラグ型別名の30 APIを、新規
  `libs/application/tests/KisActionEnumContractTest.cpp`の2試験へ対応付けた。
- 試験は無条件値、20個の独立した状態ビット、4個の独立した追加条件ビットとフラグ型の組合せを
  観測する。1,713工程・3,425入力の既存操作管理試験や1,709工程・3,418入力の製品UI実装へ接続せず、
  Qtと全体診断の宣言提供対象だけを使用する。
- 変更なし構築閉包はmacOSで4工程・8入力であり、対象実行と20回反復に成功した。対応済みは
  3,760件、未対応基準は26,207件になった。同ヘッダーの未対応はクラス1件・メソッド15件である。
  製品実装、公開API、ABI、識別値は変更していない。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 図形変更・描画順識別子 public API契約で完了した作業

- `libs/flake/KoShape.h`の変更通知、描画順、子図形Z順からなる3列挙・26列挙値の29 APIを、
  新規`libs/flake/tests/KoShapeEnumContractTest.cpp`の3試験へ対応付けた。
- 試験は幾何・階層・外観・内容の20変更通知、塗り・輪郭・マーカーの描画順、通常の親子Z順と
  既定値の別名、子通過方式を観測する。535工程・1,101入力の既存図形描画試験や
  531工程・1,094入力の製品図形実装へ接続せず、Qtの宣言提供対象だけを使用する。
- 変更なし構築閉包はmacOSで4工程・8入力であり、対象実行と20回反復に成功した。対応済みは
  3,789件、未対応基準は26,178件になった。同ヘッダーの未対応はクラス1件・構造体1件・
  データ2件・メソッド114件である。製品実装、公開API、ABI、識別値は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 環境設定ページ・タブ識別子 public API契約で完了した作業

- `libs/ui/dialogs/kis_dlg_preferences.h`の色空間選択、キャンバス表面方式の型別名、環境設定ページ、
  一般・色・表示・性能タブからなる6列挙・30列挙値・2型別名の38 APIを、新規
  `libs/ui/tests/KisDlgPreferencesEnumContractTest.cpp`の7試験へ対応付けた。
- 試験は色空間方式、9ページ、19タブの0始まりの宣言順と、キャンバス表面方式が
  `KisConfig`所有型の別名であることを観測する。1,761工程・3,519入力の既存公開ヘッダー試験や
  1,709工程・3,418入力の製品UI実装へ接続せず、宣言の直接提供対象だけを使用する。
- 変更なし構築閉包はmacOSで4工程・17入力であり、対象実行と20回反復に成功した。対応済みは
  3,827件、未対応基準は26,140件になった。同ヘッダーの未対応はクラス17件・構造体2件・
  データ27件・メソッド80件である。製品実装、公開API、ABI、識別値は変更していない。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 入力ショートカット値 public API契約と構築所有分離で完了した作業

- `libs/input/kis_shortcut_configuration.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から
  新規`kritainputshortcutconfigurationobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/input/kis_shortcut_configuration.h`のクラス、ショートカット種別、ホイール方向、
  macOSジェスチャー、生成・複製・代入・比較、全値の設定・取得、プロファイル直列化、
  無操作判定からなる1クラス・3列挙・18列挙値・22メソッドの44 APIを、新規
  `libs/input/tests/KisShortcutConfigurationContractTest.cpp`の7対応試験へ全件対応付けた。
  不正なプロファイル表現の拒否も同じ対象の追加試験で固定した。
- 1,048工程・2,113入力の既存入力プロファイル試験と1,044工程・2,106入力の製品入力実装へ
  専用試験を接続せず、専用実装は1工程・3入力、専用試験はmacOSで5工程・11入力に収めた。
  対象実行と20回反復に成功した。対応済みは3,871件、未対応基準は26,096件になり、
  同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 入力プロファイル所有権 public API契約と構築所有分離で完了した作業

- `libs/input/kis_input_profile.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から新規
  `kritainputprofileobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを1回だけ集約する
  構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/input/kis_input_profile.h`の生成・破棄、名前と変更通知、ショートカットの所有、全件一覧、
  操作ID別索引、索引からの除去からなる1クラス・9メソッドの10 APIを、新規
  `libs/input/tests/KisInputProfileContractTest.cpp`の5試験へ全件対応付けた。試験は同じ操作への
  複数入力、除去後に呼出し側へ残る所有権、プロファイル破棄時の登録済み入力の解放を観測する。
- 1,048工程・2,113入力の既存入力プロファイル試験へ接続せず、専用実装は3工程・7入力、
  専用試験はmacOSで8工程・17入力に収めた。対象実行と20回反復に成功した。対応済みは
  3,881件、未対応基準は26,086件になり、同ヘッダーのpublic APIは全件対応済みになった。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 基本ショートカット・入力操作 public API契約と構築所有分離で完了した作業

- `libs/input/kis_abstract_shortcut.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から
  新規`kritainputabstractshortcutobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/input/kis_abstract_shortcut.h`の借用操作、操作内索引、優先度、操作置換、入力グループと
  操作状態による利用可能性からなる1クラス・7メソッドの8 APIと、
  `libs/input/KisInputAction.h`の入力ライフサイクルと照合特性からなる1クラス・11メソッドの
  12 APIを、新規`libs/input/tests/KisAbstractShortcutContractTest.cpp`の7対応試験へ全件対応付けた。
  保護されたキー集合比較の順序非依存性も同じ対象の追加試験で固定した。
- 1,050工程・2,117入力の既存入力照合試験へ接続せず、専用実装は1工程・3入力、専用試験は
  macOSで5工程・11入力に収めた。対象実行と20回反復に成功した。対応済みは3,901件、
  未対応基準は26,066件になり、両ヘッダーのpublic APIは全件対応済みになった。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 入力グループマスク public API契約と構築所有分離で完了した作業

- `libs/canvas/KisInputActionGroup.cpp`は同じ配置のまま、`kritacanvas`の直接ソース所有から
  新規`kritacanvasinputactiongroupobjects`の所有へ移し、製品`kritacanvas`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/canvas/KisInputActionGroup.h`の独立した3入力グループビット、全グループ値、マスク型、
  共有マスク接続面、期間限定マスクの入れ子復元と多態的破棄からなる2型別名・1クラス・1構造体・
  1列挙・5列挙値・5メソッドの15 APIを、新規
  `libs/canvas/tests/KisInputActionGroupContractTest.cpp`の3試験へ全件対応付けた。
- 1,050工程・2,117入力の既存入力照合試験と1,030工程・2,080入力の製品キャンバス実装へ
  専用試験を接続せず、専用実装は1工程・3入力、専用試験はmacOSで5工程・11入力に収めた。
  対象実行と20回反復に成功した。対応済みは3,916件、未対応基準は26,051件になり、
  同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 入力継続時間判定 public API契約と構築所有分離で完了した作業

- `libs/input/KisTimedSignalThreshold.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から
  新規`kritainputtimedsignalthresholdobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/input/KisTimedSignalThreshold.h`のQObject寿命、開始、停止、有効状態、閾値更新、強制完了、
  完了信号からなる1クラス・8メソッドの9 APIを、新規
  `libs/input/tests/KisTimedSignalThresholdContractTest.cpp`の6対応試験へ全件対応付けた。
  取消猶予を超えた開始列が現在期間を破棄する挙動も同じ対象の追加試験で固定した。
- 製品実装は1回だけ集約し、専用実装は3工程・7入力、専用試験はmacOSで7工程・14入力に
  収めた。対象実行と20回反復に成功した。対応済みは3,925件、未対応基準は26,042件になり、
  同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 入力事象抑制 public API契約と構築所有分離で完了した作業

- `libs/input/KisInputEventSuppressor.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から
  新規`kritainputeventsuppressorobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/input/KisInputEventSuppressor.h`の正規化済み事象値、抑制理由、マウス・タッチ遮断、
  一回限りの遅延押下、副ボタン対策、合成入力方針からなる1クラス・1構造体・3列挙・
  15列挙値・3データ・10メソッドの33 APIを、既存
  `libs/input/tests/TestInputEventSuppressor.cpp`の5試験へ全件対応付けた。
- 1,052工程・2,121入力だった既存試験を製品入力実装から分離し、専用実装は1工程・3入力、
  同じ試験はmacOSで5工程・11入力に縮小した。対象実行と20回反復に成功した。対応済みは
  3,958件、未対応基準は26,009件になり、同ヘッダーのpublic APIは全件対応済みになった。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b タッチジェスチャー種別 public API契約で完了した作業

- `libs/input/KisTouchGestureType.h`は実装を持たない公開列挙であり、製品ライブラリへ接続せず、
  `libs/input/tests/KisTouchGestureTypeContractTest.cpp`をQt Core/Testだけへ接続した。
- 未対応、タップ、ドラッグ、長押しの1列挙・4列挙値からなる5 APIを、公開された連続識別値を
  固定する1試験へ全件対応付けた。
- 最初の局所構築でヘッダー検索経路の欠落を確認し、試験対象の`libs/input`だけを私有検索経路へ
  追加した。専用試験はmacOSで4工程・8入力に収め、対象実行と20回反復に成功した。対応済みは
  3,963件、未対応基準は26,004件になり、同ヘッダーのpublic APIは全件対応済みになった。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b ネイティブジェスチャー照合 public API契約と構築所有分離で完了した作業

- `libs/input/kis_native_gesture_shortcut.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から
  新規`kritainputnativegestureshortcutobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/input/kis_native_gesture_shortcut.h`の構築、基本型を通した寿命、優先度、Qtの
  ネイティブジェスチャー種別照合からなる1クラス・4メソッドの5 APIを、新規
  `libs/input/tests/KisNativeGestureShortcutContractTest.cpp`の2試験へ全件対応付けた。
- 1,052工程・2,121入力の既存入力照合試験へ接続せず、専用実装は1工程・3入力、専用試験は
  macOSで6工程・13入力に収めた。対象実行と20回反復に成功した。対応済みは3,968件、
  未対応基準は25,999件になり、同ヘッダーのpublic APIは全件対応済みになった。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b ストローク開始状態 public API契約と構築所有分離で完了した作業

- `libs/input/kis_stroke_shortcut.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から
  新規`kritainputstrokeshortcutobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/input/kis_stroke_shortcut.h`の構築、基本型を通した寿命、修飾キー・ボタン構成、準備・
  開始状態、優先度、合成解放事象からなる1クラス・7メソッドの8 APIを、新規
  `libs/input/tests/KisStrokeShortcutContractTest.cpp`の4対応試験へ全件対応付けた。
  空のボタン構成が既存構成を置換しない挙動も同じ対象の追加試験で固定した。
- 1,052工程・2,121入力の既存入力照合試験へ接続せず、専用実装は1工程・3入力、専用試験は
  macOSで6工程・13入力に収めた。対象実行と20回反復に成功した。対応済みは3,976件、
  未対応基準は25,991件になり、同ヘッダーのpublic APIは全件対応済みになった。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b タッチジェスチャー照合 public API契約と構築所有分離で完了した作業

- `libs/input/kis_touch_shortcut.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から
  新規`kritainputtouchshortcutobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/input/kis_touch_shortcut.h`の構築、基本型を通した寿命、優先度、ジェスチャー種別、
  タッチ点数範囲、タッチ描画中の無効化からなる1クラス・12メソッドの13 APIを、新規
  `libs/input/tests/KisTouchShortcutContractTest.cpp`の5試験へ全件対応付けた。
- 1,052工程・2,121入力の既存入力照合試験へ接続せず、専用実装は1工程・3入力、専用試験は
  macOSで6工程・13入力に収めた。対象実行と20回反復に成功した。対応済みは3,989件、
  未対応基準は25,978件になり、同ヘッダーのpublic APIは全件対応済みになった。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 単一キー・ホイール照合 public API契約と構築所有分離で完了した作業

- `libs/input/kis_single_action_shortcut.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から
  新規`kritainputsingleactionshortcutobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- 修飾キーだけの不正なキー列は従来どおり競合なしを返し、同じ分岐で行っていた画面・ログへの
  安全表明診断を削除した。これにより単一操作短縮実装から`kritaglobal`への診断依存を除いた。
- `libs/input/kis_single_action_shortcut.h`の構築、基本型を通した寿命、優先度、キー・ホイール構成、
  5ホイール識別値、照合、キー列競合からなる1クラス・1列挙・5列挙値・8メソッドの15 APIを、
  新規`libs/input/tests/KisSingleActionShortcutContractTest.cpp`の4試験へ全件対応付けた。
- 1,052工程・2,121入力の既存入力照合試験と65工程・130入力の全体診断実装へ接続せず、
  専用実装は1工程・3入力、専用試験はmacOSで6工程・13入力に収めた。対象実行と20回反復に
  成功した。対応済みは4,004件、未対応基準は25,963件になり、同ヘッダーのpublic APIは
  全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 入力照合状態 public API契約と構築所有分離で完了した作業

- `libs/input/kis_shortcut_matcher.cpp`は同じ配置のまま、`kritainput`の直接ソース所有から
  新規`kritainputshortcutmatcherobjects`の所有へ移し、製品`kritainput`が生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/input/kis_shortcut_matcher.h`の4種の短縮操作登録、キー・ボタン・ホイール、ストローク、
  タッチ、ネイティブジェスチャー、状態同期、焦点・ツール遷移、抑制、操作群マスク、登録解除から
  なる1定数・1クラス・39メソッドの41 APIを、拡張した
  `libs/input/tests/TestInputShortcutMatcher.cpp`の9対応試験へ全件対応付けた。
- 操作群マスクは準備表示ではなく実行開始を許可する段階で評価する既存境界を試験で固定した。
  追加試験の最初の実行でこの境界との差を検出し、実行開始の拒否・許可を直接観測する形へ直した。
- 1,052工程・2,121入力だった既存試験を製品入力・キャンバス実装から分離し、専用実装は
  23工程・45入力、同じ試験はmacOSで76工程・151入力に縮小した。対象実行と20回反復に成功した。
  対応済みは4,045件、未対応基準は25,922件になり、同ヘッダーのpublic APIは全件対応済みに
  なった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 拡張修飾キー・プラグイン境界 public API契約と構築所有分離で完了した作業

- `libs/input/ui/KisExtendedModifiersMapperPluginInterface.cpp`は同じ配置のまま、`kritainputui`の
  直接ソース所有から新規`kritainputuiextendedmodifiersinterfaceobjects`の所有へ移し、製品
  `kritainputui`が生成オブジェクトを1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは
  維持した。
- `libs/input/ui/KisExtendedModifiersMapperPluginInterface.h`の型別名、基本型を通したQObject寿命、
  修飾キー列の取得からなる1型別名・1クラス・2メソッドの4 APIを、新規
  `libs/input/ui/tests/KisExtendedModifiersMapperPluginInterfaceContractTest.cpp`の2試験へ全件
  対応付けた。
- 1,183工程・2,375入力の製品入力UIへ接続せず、専用実装は3工程・7入力、専用試験はmacOSで
  7工程・14入力に収めた。対象実行と20回反復に成功した。対応済みは4,049件、未対応基準は
  25,918件になり、同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b ポップアップ表示境界 public API契約で完了した作業

- `libs/input/ui/KisPopupWidgetInterface.h`の指定位置表示、表示状態、破棄、完了通知、Qtの
  インターフェース変換、基本型を通した寿命からなる1クラス・5メソッドの6 APIを、新規
  `libs/input/ui/tests/KisPopupWidgetInterfaceContractTest.cpp`の3試験へ全件対応付けた。製品実装と
  公開面は変更していない。
- 1,767工程・3,531入力の既存公開ヘッダー集約試験へ挙動確認を接続せず、Qt CoreとQt Testだけを
  使う専用試験はmacOSで4工程・8入力に収めた。対象実行と20回反復に成功した。対応済みは
  4,055件、未対応基準は25,912件になり、同ヘッダーのpublic APIは全件対応済みになった。
  Linuxと全ネイティブ検証は実行していない。

## R2-G19b 拡張修飾キー写像 public API契約と構築所有分離で完了した作業

- `libs/input/ui/kis_extended_modifiers_mapper.cpp`とmacOS用
  `libs/input/ui/kis_extended_modifiers_mapper_osx.mm`は同じ配置のまま、`kritainputui`の直接ソース
  所有から新規`kritainputuiextendedmodifiersmapperobjects`の所有へ移し、製品`kritainputui`が
  生成オブジェクトを各1回だけ集約する構造にした。公開ヘッダー、製品ABI、プラットフォーム別の
  修飾キー取得経路は維持した。
- macOS用`libs/input/ui/kis_extended_modifiers_mapper_osx.h`から未使用の入力照合ヘッダー依存を
  除き、2実装の重複除去は全体診断ライブラリーのコンテナー補助ではなく標準ライブラリーで同じ
  整列・重複除去を行う形にした。
- `libs/input/ui/kis_extended_modifiers_mapper.h`の構築、破棄、型別名、Qt修飾キー変換、Metaキー補正、
  アプリケーション状態取得、プラグイン委譲、macOS局所監視からなる1型別名・1クラス・8メソッドの
  10 APIを、新規`libs/input/ui/tests/KisExtendedModifiersMapperContractTest.cpp`の5試験へ全件
  対応付けた。
- 1,185工程・2,379入力の製品入力UIへ接続せず、専用実装は4工程・9入力、専用試験はmacOSで
  11工程・22入力に収めた。対象実行と20回反復に成功した。対応済みは4,065件、未対応基準は
  25,902件になり、同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 画像挙動境界 public API契約と構築所有分離で完了した作業

- 次の開始ファイルは同じ配置のまま、`kritaimage`の直接ソース所有から新規
  `kritaimagebehaviorinterfaceobjects`の所有へ移し、製品`kritaimage`が各生成オブジェクトを1回だけ
  集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
  - `libs/image/KisCroppedOriginalLayerInterface.cpp`
  - `libs/image/KisDecoratedNodeInterface.cpp`
  - `libs/image/KisDelayedUpdateNodeInterface.cpp`
  - `libs/image/KisInterstrokeDataFactory.cpp`
  - `libs/image/KisTransactionWrapperFactory.cpp`
  - `libs/image/KisTransformMaskTestingInterface.cpp`
- 対応する6公開ヘッダーの隠領域更新、装飾表示、遅延更新、ストローク間データの適合・生成、
  トランザクション前後命令、5種の変形マスク通知、多態的寿命からなる6クラス・21メソッドの
  27 APIを、新規`libs/image/tests/KisBehaviorInterfacesContractTest.cpp`の6試験へ全件対応付けた。
- 最初の試験構築は空の描画装置共有ポインターを呼出側で破棄するための完全型不足を検出した。
  試験へ`kis_paint_device.h`と必要な色変換ヘッダー探索位置を追加し、製品ライブラリーへのリンクは
  増やしていない。
- 1,008工程・2,040入力の製品画像ライブラリーへ接続せず、専用実装は6工程・13入力、専用試験は
  macOSで10工程・21入力に収めた。対象実行と20回反復に成功した。対応済みは4,092件、未対応基準は
  25,875件になり、6ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b Flake挙動境界 public API契約と構築所有分離で完了した作業

- `libs/flake/KoCanvasSupervisor.cpp`、`libs/flake/KoLoadingShapeUpdater.cpp`、
  `libs/flake/KoShapeUserData.cpp`、`libs/flake/KoToolSelection.cpp`は同じ配置のまま、`kritaflake`の
  直接ソース所有から新規`kritaflakebehaviorinterfaceobjects`の所有へ移し、製品`kritaflake`が
  各生成オブジェクトを1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- 対応する4公開ヘッダーと`libs/flake/KoShapeBulkActionInterface.h`の大量操作開始・終了、形状付加
  データ複製、既定ツール選択、キャンバス監視一覧、読込済み形状通知、QObjectと多態的寿命からなる
  1構造体・4クラス・15メソッドの20 APIを、新規
  `libs/flake/tests/KoBehaviorInterfacesContractTest.cpp`の5試験へ全件対応付けた。
- 531工程・1,094入力の製品Flakeへ接続せず、専用実装は6工程・13入力、専用試験はmacOSで
  10工程・20入力に収めた。対象実行と20回反復に成功した。対応済みは4,112件、未対応基準は
  25,855件になり、5ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b Flake共有データ寿命 public API契約と構築所有分離で完了した作業

- `libs/flake/KoSharedLoadingData.cpp`と`libs/flake/KoSharedSavingData.cpp`は同じ配置のまま、
  `kritaflake`の直接ソース所有から既存`kritaflakebehaviorinterfaceobjects`の所有へ移し、製品
  `kritaflake`が各生成オブジェクトを1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは
  維持した。
- 対応する2公開ヘッダーの構築と基本型を通した多態的寿命からなる2クラス・4メソッドの6 APIを、
  拡張した`libs/flake/tests/KoBehaviorInterfacesContractTest.cpp`の2対応試験へ全件対応付けた。
- 531工程・1,094入力の製品Flakeへ接続せず、専用実装は8工程・17入力、専用試験はmacOSで
  12工程・24入力に収めた。対象実行と20回反復に成功した。対応済みは4,118件、未対応基準は
  25,849件になり、2ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b Flake勾配補助 public API契約と構築所有分離で完了した作業

- `libs/flake/KoGradientHelper.cpp`は同じ配置のまま、`kritaflake`の直接ソース所有から新規
  `kritaflakegradienthelperobjects`の所有へ移し、製品`kritaflake`が生成オブジェクトを1回だけ
  集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/flake/KoGradientHelper.h`の既定勾配生成、型変換、停止点色補間からなる3関数を、新規
  `libs/flake/tests/KoGradientHelperContractTest.cpp`の3試験へ全件対応付けた。
- 停止点と完全一致する位置では一致色を候補から除外し、先頭位置で末尾色、末尾位置で先頭色を返す
  現行挙動を既知不具合として分類した。空・単一・範囲外・中間補間の通常境界も同じ試験で固定した。
- 531工程・1,094入力の製品Flakeへ接続せず、専用実装は1工程・3入力、専用試験はmacOSで
  5工程・11入力に収めた。対象実行と20回反復に成功した。対応済みは4,121件、未対応基準は
  25,846件になり、同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 曲線制御戦略工場 public API契約と構築所有分離で完了した作業

- `plugins/paintops/libpaintop/KisCurveOptionInputControlsStrategyInterface.cpp`と
  `plugins/paintops/libpaintop/KisCurveOptionRangeControlsStrategyInterface.cpp`は同じ配置のまま、
  `kritalibpaintop`の直接ソース所有から新規`kritapaintopcurvecontrolinterfaceobjects`の所有へ
  移し、製品`kritalibpaintop`が各生成オブジェクトを1回だけ集約する構造にした。公開ヘッダー、
  実装、製品ABIは維持した。
- 対応する2公開ヘッダーの制御部品工場型、基本境界、多態的寿命からなる2型別名・2クラス・
  2メソッドの6 APIを、新規
  `plugins/paintops/libpaintop/tests/KisCurveControlStrategyInterfacesContractTest.cpp`の2試験へ
  全件対応付けた。工場へ渡す模型と部品の順序・同一性、および基本型からの破棄を固定した。
- 1,852工程・3,702入力の製品描画処理ライブラリーへ接続せず、専用実装は2工程・5入力、専用試験は
  macOSで6工程・13入力に収めた。対象実行と20回反復に成功した。対応済みは4,127件、未対応基準は
  25,840件になり、2ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b キャンバス資源参照 public API契約と構築所有分離で完了した作業

- `libs/resources/KoCanvasResourcesInterface.cpp`は同じ配置のまま、`kritaresources`の直接ソース
  所有から新規`kritacanvasresourcesinterfaceobjects`の所有へ移し、製品`kritaresources`が生成
  オブジェクトを1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/resources/KoCanvasResourcesInterface.h`の基本境界、キー指定の資源参照、多態的寿命からなる
  1クラス・2メソッドの3 APIを、新規
  `libs/resources/tests/KoCanvasResourcesInterfaceContractTest.cpp`の1試験へ全件対応付けた。指定キーの
  配送、戻り値の保持、基本型からの破棄を固定した。
- 製品へ接続しない最初の試験構築が、基本型デストラクターと型情報の未接続をリンク診断した後、
  139工程・305入力の製品資源ライブラリーへ接続せず、専用実装は1工程・3入力、専用試験はmacOSで
  5工程・11入力に収めた。対象実行と20回反復に成功した。対応済みは4,130件、未対応基準は
  25,837件になり、同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 共有資源キャッシュ public API契約と構築所有分離で完了した作業

- `libs/resources/KoResourceCacheInterface.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有
  から新規`kritaresourcecacheinterfaceobjects`の所有へ移し、製品`kritaresources`が生成
  オブジェクトを1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- `libs/resources/KoResourceCacheInterface.h`の基本境界、取得・格納、関連資源識別子、設定・取得、
  多態的寿命からなる1型別名・1クラス・5メソッドの7 APIを、新規
  `libs/resources/tests/KoResourceCacheInterfaceContractTest.cpp`の2対応試験へ全件対応付けた。共有
  ポインター型のQtメタ型登録も同じ専用試験の独立試験で固定した。
- 製品へ接続しない最初の試験構築が、関連資源識別子の設定・取得、基本型デストラクター、型情報の
  未接続をリンク診断した後、139工程・305入力の製品資源ライブラリーへ接続せず、専用実装は
  1工程・3入力、専用試験はmacOSで5工程・11入力に収めた。対象実行と20回反復に成功した。
  対応済みは4,137件、未対応基準は25,830件になり、同ヘッダーのpublic APIは全件対応済みに
  なった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 埋込み資源値 public API契約と構築所有分離で完了した作業

- `libs/resources/KoEmbeddedResource.cpp`、`libs/resources/KoMD5Generator.cpp`、
  `libs/resources/KoResourceSignature.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有から
  新規`kritaembeddedresourcevalueobjects`の所有へ移し、製品`kritaresources`が各生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。
- 対応する3公開ヘッダーの構築、公開署名項目、デバッグ書式、バイト列・装置・ファイルのMD5生成、
  埋込みデータ・署名・妥当性からなる3クラス・1関数・4メンバー・11メソッドの19 APIを、新規
  `libs/resources/tests/KoEmbeddedResourceValueContractTest.cpp`の6試験へ全件対応付けた。署名の等価
  判定が種別を比較しないことと、デバッグ書式が末尾空白を持つことも現行挙動として固定した。
- 製品へ接続しない最初の試験構築が3実装群の未接続をリンク診断した後、139工程・305入力の製品
  資源ライブラリーへ接続せず、専用実装は3工程・7入力、専用試験はmacOSで7工程・15入力に
  収めた。対象実行と20回反復に成功した。対応済みは4,156件、未対応基準は25,811件になり、
  3ヘッダーの採取済みpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b friend関数採取と重複型別名契約で完了した作業

- `scripts/architecture/check_public_api_contracts.py`は、Universal Ctagsが省略するセミコロン終端の
  friend関数宣言を、原文のコメントと文字列を保護した一時コピーで空本文付きに変換し、既存の
  名前空間関数一覧へ統合するようにした。本文付きfriend関数と通常宣言は既存採取結果を維持する。
- `scripts/tests/test_public_api_contracts.py`へ、公開・非公開節のfriend関数、複数行宣言、名前空間、
  friend class除外、コメント・文字列・既存本文の保持を追加し、全12試験に成功した。公開面には
  19関数が追加され、1,546公開ヘッダー、29,986 APIになった。
- 既存`libs/resources/tests/KoEmbeddedResourceValueContractTest.cpp`の署名等価試験を新規採取した
  `KoResourceSignature::operator==`へ対応付けた。重複宣言を一つの識別子へ統合して別ヘッダーを
  代表元にする共有ポインター型別名は、既存資源キャッシュのメタ型試験と、拡張した
  `libs/resources/tests/KoCanvasResourcesInterfaceContractTest.cpp`の所有寿命試験へ対応付けた。
- キャンバス資源試験は製品資源ライブラリーへ接続せず、5工程・11入力のまま対象実行と20回反復に
  成功した。対応済みは4,159件、未対応基準は25,827件になった。追加採取した残り18 friend関数は
  通常の未対応作業列へ入り、Linuxと全ネイティブ検証は実行していない。

## R2-G19b 資源読込結果 public API契約と構築所有分離で完了した作業

- `libs/resources/KoResource.cpp`と`libs/resources/KoResourceLoadResult.cpp`は同じ配置のまま、
  `kritaresources`の直接ソース所有から新規`kritaresourceloadvalueobjects`の所有へ移し、製品
  `kritaresources`が各生成オブジェクトを1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは
  維持した。この対象は後続の資源基底型契約でも再利用する。
- `libs/resources/KoResourceLoadResult.h`の3結果種別、基本・派生共有ポインター、埋込み値、失敗署名、
  コピー・代入・破棄、型付き取得、署名取得、デバッグ表示からなる1クラス・1列挙・3列挙値・
  1関数・11メソッドの17 APIを、新規`libs/resources/tests/KoResourceLoadResultContractTest.cpp`の
  5試験へ全件対応付けた。
- 製品へ接続しない最初の試験構築が資源基底と読込結果の未接続をリンク診断した後、専用対象へ
  `KF::I18n`と生成済みglobalヘッダーの直接依存を明示した。安全アサートの画面表示・利用記録実装は
  引き込まず、有効値で呼ばれた場合に停止する試験診断へ置換し、ログ分類だけ既存
  `kritaglobaldebugobjects`から取得した。
- 139工程・305入力の製品資源ライブラリーへ接続せず、専用資源基底・読込結果は2工程・5入力、
  専用試験はmacOSで12工程・26入力に収めた。対象実行と20回反復に成功した。対応済みは4,176件、
  未対応基準は25,810件になり、同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ
  検証は実行していない。

## R2-G19b 資源基底型 public API契約と未定義試験宣言除去で完了した作業

- `libs/resources/KoResource.h`から、製品定義と製品利用がなく、実装と利用が
  `libs/resources/tests/ResourceTestHelper.h`内だけに閉じる
  `ResourceTestHelper::overrideResourceVersion`宣言を除いた。試験補助は試験所有の定義を維持し、
  製品の公開面から未定義関数1件を除去した。
- 残る`KoResource.h`の構築、コピー代入禁止、状態、ファイル入出力、MD5・署名、仮想既定、関連資源、
  等価性・ハッシュ・デバッグ表示、複製・寿命からなる1クラス・3関数・49メソッドの53 APIと、
  重複宣言を別ヘッダーへ代表配置する2共有ポインター型別名を、新規
  `libs/resources/tests/KoResourceContractTest.cpp`の9対応試験へ全件対応付けた。存在しない・空ファイル
  の拒否も独立試験で固定した。派生`saveToDevice()`が失敗を返しても`save()`が成功を返す現行挙動は、
  既知不具合として固定した。
- 既存`kritaresourceloadvalueobjects`を再利用し、139工程・305入力の製品資源ライブラリーへ接続せず、
  専用試験をmacOSで12工程・26入力に収めた。対象実行と20回反復に成功した。公開面は
  1,546ヘッダー、29,985 API、対応済み4,231件、未対応25,754件になり、同ヘッダーのpublic APIは
  全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 資源供給境界 public API契約と構築所有分離で完了した作業

- `libs/resources/KisResourcesInterface.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有から
  新規`kritaresourcesinterfaceobjects`の所有へ移し、製品`kritaresources`がその生成オブジェクトを
  1回だけ集約する構造にした。公開ヘッダー、実装、製品ABIは維持した。分離によって露出した
  `KF::I18n`への直接ヘッダー依存を専用対象へ明示した。
- `libs/resources/KisResourcesInterface.h`の境界と基底・型付き供給元、種類別の遅延生成とキャッシュ、
  並行初回取得、所有寿命、MD5候補順位、保存名・表示名による旧形式検索、厳密一致、結果付き検索、
  型付き動的変換からなる3クラス・17メソッドの20 APIを、新規
  `libs/resources/tests/KisResourcesInterfaceContractTest.cpp`の6試験へ全件対応付けた。
- 実装を接続しない最初の試験構築は対象境界の未解決参照をリンク診断した。139工程・305入力の製品
  資源ライブラリーへ接続せず、専用実装は1工程・3入力、専用試験はmacOSで13工程・28入力に
  収めた。対象実行と並行キャッシュを含む20回反復に成功した。公開面は1,546ヘッダー、29,985 API、
  対応済み4,251件、未対応25,734件になり、同ヘッダーのpublic APIは全件対応済みになった。Linuxと
  全ネイティブ検証は実行していない。

## R2-G19b 資源供給private状態の公開面縮小と寿命契約で完了した作業

- `libs/resources/KisResourcesInterface_p.h`から、最低Qt 5.15の対応範囲では到達不能なQt 5.13以前用
  `std::hash<QString>`特殊化を除いた。内部キャッシュ、読書きロック、検索関数はprivate節へ移し、
  `KisResourcesInterface`だけをfriendとして公開採取対象から除いた。実利用は
  `libs/resources/KisResourcesInterface.cpp`と`libs/resources/KisLocalStrokeResources.cpp`内に閉じ、
  外部向け`libs/resources/KisResourcesInterface.h`と製品ABI、資源供給挙動は維持した。
- 公開面に残るprivate状態クラスと仮想デストラクターの2 APIは、拡張した
  `libs/resources/tests/KisResourcesInterfaceContractTest.cpp`で、注入した派生private状態が境界の
  所有終了時に多態的に破棄される挙動へ対応付けた。
- 既存の専用試験13工程・28入力を維持し、macOSの対象実行と20回反復に成功した。公開面は
  1,546ヘッダー、29,981 API、対応済み4,253件、未対応25,728件になり、同ヘッダーのpublic APIは
  全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 局所ストローク資源 public API契約と構築所有分離で完了した作業

- `libs/resources/KisLocalStrokeResources.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有から
  新規`kritalocalstrokeresourcesobjects`の所有へ移し、製品`kritaresources`がその生成オブジェクトを
  1回だけ集約する構造にした。外部向けヘッダー、実装、製品ABIは維持した。
- `libs/resources/KisLocalStrokeResources.h`の空・初期一覧構築、現在一覧、追加、全一致削除、複製から
  なる1クラス・6メソッドの7 APIを、新規
  `libs/resources/tests/KisLocalStrokeResourcesContractTest.cpp`の4試験へ全件対応付けた。初期値と追加時の
  空資源拒否、順序と重複、共有資源を持つ独立容器への複製、種類別検索、供給元生成後の一覧変更反映も
  固定した。
- 実装を接続しない最初の試験構築は局所格納7 APIの未解決参照をリンク診断した。139工程・305入力の
  製品資源ライブラリーへ接続せず、専用実装は1工程・3入力、専用試験はmacOSで14工程・30入力に
  収めた。対象実行と20回反復に成功した。公開面は1,546ヘッダー、29,981 API、対応済み4,260件、
  未対応25,721件になり、同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は
  実行していない。

## R2-G19b 大域資源供給 public API契約とモデル依存分離で完了した作業

- `libs/resources/KisGlobalResourcesInterface.cpp`に同居していた単一個体管理とモデル供給元を分けた。
  単一個体管理と`createSourceImpl()`は同じファイルに残して、`kritaresources`の直接ソース所有から
  新規`kritaglobalresourcesinstanceobjects`へ移した。`GlobalResourcesSource`と資源モデル取得は新規
  `libs/resources/KisGlobalResourcesInterfaceSource.cpp`へ抽出し、新規
  `kritaglobalresourcesmodelsourceobjects`の所有とした。製品`kritaresources`は両生成オブジェクトを
  1回ずつ集約する。
- `createSourceImpl()`からモデル供給元生成だけを内部関数境界にし、製品では
  `KisResourceModelProvider`実装、局所試験では空供給元を接続した。これにより製品の仮想メソッド実装を
  試験でも共通に保ち、データベース初期化を単一個体契約から分離した。
- `libs/resources/KisGlobalResourcesInterface.h`の大域境界と単一個体取得の2 APIを、新規
  `libs/resources/tests/KisGlobalResourcesInterfaceContractTest.cpp`の1試験へ全件対応付けた。並行初回
  取得を含む共有ポインター同一性、具象型、種類別供給元の一度だけの生成と再利用を固定した。
- 641工程・1,310入力の既存大域利用試験へ接続せず、専用試験は14工程・30入力、単一個体とモデル
  供給元の各実装は1工程・3入力に収めた。製品閉包は翻訳単位の分割により139工程・305入力から
  140工程・307入力になった。macOSで両実装をコンパイルし、対象実行と20回反復に成功した。製品
  `kritaresources`のリンク、Linux、全ネイティブ検証は実行していない。公開面は1,546ヘッダー、
  29,981 API、対応済み4,262件、未対応25,719件になり、同ヘッダーのpublic APIは全件対応済みになった。

## R2-G19b 資源ログ分類 public API契約で完了した作業

- `libs/resources/ResourceDebug.h`の資源ログ分類関数1 APIを、既存
  `libs/resources/tests/KisResourceTypesContractTest.cpp`へ追加した1試験に対応付けた。同じ静的個体、
  `krita.lib.resource`分類名、デバッグ無効・情報以上有効の既定重要度を固定した。
- 既存`kritaresourcestypesobjects`を再利用し、製品資源ライブラリーへ接続せず、清浄時の専用試験を
  6工程・14入力、所有対象を2工程・5入力に維持した。macOSの対象実行と20回反復に成功した。公開面は
  1,546ヘッダー、29,981 API、対応済み4,263件、未対応25,718件になり、同ヘッダーのpublic APIは
  全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 格納ログ分類 public API契約と構築所有分離で完了した作業

- `libs/resources/storage/StoreDebug.cpp`は同じ配置のまま、`kritaresourcestorage`の直接ソース所有から
  新規`kritaresourcestoragelogobjects`の所有へ移し、製品`kritaresourcestorage`が生成オブジェクトを
  1回だけ集約する構造にした。外部向けヘッダー、実装、製品ABIは維持した。
- `libs/resources/storage/StoreDebug.h`の格納ログ分類関数1 APIを、新規
  `libs/resources/storage/tests/StoreDebugContractTest.cpp`の1試験へ対応付けた。同じ静的個体、
  `krita.lib.store`分類名、デバッグ無効・情報以上有効の既定重要度を固定した。
- 清浄時の既存格納契約13工程・27入力に対し、新規試験を5工程・11入力、所有対象を1工程・3入力に
  収めた。製品格納ライブラリーは9工程を維持し、対象入力は20件から21件になった。macOSで製品
  `kritaresourcestorage`をリンクし、既存アーカイブ契約、対象実行、20回反復に成功した。公開面は
  1,546ヘッダー、29,981 API、対応済み4,264件、未対応25,717件になり、同ヘッダーのpublic APIは
  全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 接頭辞付き資源キャッシュ public API契約と構築所有分離で完了した作業

- `libs/resources/KoResourceCachePrefixedStorageWrapper.cpp`は同じ配置のまま、`kritaresources`の直接
  ソース所有から新規`kritaresourcecacheprefixedwrapperobjects`の所有へ移し、製品
  `kritaresources`が生成オブジェクトを1回だけ集約する構造にした。外部向けヘッダー、実装、
  製品ABIは維持した。
- `libs/resources/KoResourceCachePrefixedStorageWrapper.h`の境界、構築、取得、格納からなる1クラス・
  3メソッドの4 APIを、新規
  `libs/resources/tests/KoResourceCachePrefixedStorageWrapperContractTest.cpp`の2試験へ全件対応付けた。
  接頭辞の連結、空接頭辞、取得・格納の配送と値、共有基底キャッシュの所有寿命を固定した。
- 清浄時の既存資源キャッシュ境界契約5工程・11入力に対し、所有対象を1工程・3入力、新規試験を
  6工程・13入力に収めた。製品資源ライブラリーは140工程・307入力を維持した。macOSの対象実行と
  20回反復に成功した。公開面は1,546ヘッダー、29,981 API、対応済み4,268件、未対応25,713件に
  なり、同ヘッダーのpublic APIは全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 値格納資源キャッシュ public API契約と構築所有分離で完了した作業

- `libs/resources/KoResourceCacheStorage.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有から
  新規`kritaresourcecachestorageobjects`の所有へ移し、製品`kritaresources`が生成オブジェクトを
  1回だけ集約する構造にした。外部向けヘッダー、実装、製品ABIは維持した。
- `libs/resources/KoResourceCacheStorage.h`の境界、構築・破棄、取得、格納からなる1クラス・4メソッドの
  5 APIを、新規`libs/resources/tests/KoResourceCacheStorageContractTest.cpp`の3試験へ全件対応付けた。
  空状態と未知キー、異なるキーの型付き値、同一キー再格納時の安全表明と置換、多態的所有寿命を
  固定した。
- 実装を接続しない最初の試験構築は5工程・11入力に収まり、対象5 APIと型情報の未解決参照を
  リンク診断した。分離後は所有対象を1工程・3入力、新規試験を6工程・13入力に収め、製品資源
  ライブラリーは140工程・307入力を維持した。macOSの対象実行と20回反復に成功した。公開面は
  1,546ヘッダー、29,981 API、対応済み4,273件、未対応25,708件になり、同ヘッダーのpublic APIは
  全件対応済みになった。製品`kritaresources`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 局所キャンバス資源 public API契約と構築所有分離で完了した作業

- `libs/resources/KoLocalStrokeCanvasResources.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有
  から新規`kritalocalstrokecanvasresourcesobjects`の所有へ移し、製品`kritaresources`が生成
  オブジェクトを1回だけ集約する構造にした。外部向けヘッダー、実装、製品ABIは維持した。
- `libs/resources/KoLocalStrokeCanvasResources.h`の共有ポインター型、境界、構築・複製・代入・破棄、
  取得・格納からなる1型別名・1クラス・6メソッドの8 APIを、新規
  `libs/resources/tests/KoLocalStrokeCanvasResourcesContractTest.cpp`の4試験へ全件対応付けた。空状態、
  型付き値の格納・置換、コピーと代入後の容器独立性、代入戻り値と自己代入、共有所有寿命を固定した。
- 既存の描画境界試験は1,037工程・2,095入力であるため接続せず、5工程・11入力の基底境界契約を
  比較対象にした。実装を接続しない最初の試験構築は同じ5工程・11入力で全メソッドと型情報の
  未解決参照をリンク診断した。分離後は所有対象を1工程・3入力、新規試験を6工程・13入力に収め、
  製品資源ライブラリーは140工程・307入力を維持した。macOSの対象実行と20回反復に成功した。
  公開面は1,546ヘッダー、29,981 API、対応済み4,281件、未対応25,700件になり、同ヘッダーの
  public APIは全件対応済みになった。製品`kritaresources`のリンク、Linux、全ネイティブ検証は
  実行していない。

## R2-G19b 資源メタデータ取得 public API契約と構築所有分離で完了した作業

- `libs/resources/KisResourceMetaDataModel.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有から
  新規`kritaresourcemetadatamodelobjects`の所有へ移し、製品`kritaresources`が生成オブジェクトを
  1回だけ集約する構造にした。外部向けヘッダー、実装、製品ABIは維持した。
- `libs/resources/KisResourceMetaDataModel.h`の境界、構築・破棄、値取得からなる1クラス・3メソッドの
  4 APIを、新規`libs/resources/tests/KisResourceMetaDataModelContractTest.cpp`の2対応試験へ全件
  対応付けた。メモリーSQLite上で表名・資源ID・キーの絞り込み、Base64化された整数・文字列の
  QVariant復元、欠落・空データの無効値、破棄後の準備済み問い合わせ解放を固定した。
- 642工程・1,312入力の既存資源DB試験へ接続せず、実装を接続しない最初の試験構築は4工程・8入力で
  構築・破棄・取得の未解決参照をリンク診断した。分離後は所有対象を1工程・3入力、新規試験を
  5工程・11入力に収め、製品資源ライブラリーは140工程・307入力を維持した。macOSの対象実行と
  20回反復に成功した。公開面は1,546ヘッダー、29,981 API、対応済み4,285件、未対応25,696件に
  なり、同ヘッダーのpublic APIは全件対応済みになった。製品`kritaresources`のリンク、Linux、
  全ネイティブ検証は実行していない。

## R2-G19b データベース取引ロック public API契約と構築所有分離で完了した作業

- `libs/resources/KisDatabaseTransactionLock.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有
  から新規`kritadatabasetransactionlockobjects`の所有へ移し、製品`kritaresources`が生成
  オブジェクトを1回だけ集約する構造にした。外部向けヘッダー、実装、製品ABIは維持した。
- `libs/resources/KisDatabaseTransactionLock.h`のアダプター境界・構築・施錠・解錠・確定、包装型・
  基底型別名・明示取消しからなる1構造体・1クラス・1型別名・5メソッドの8 APIを、新規
  `libs/resources/tests/KisDatabaseTransactionLockContractTest.cpp`の3対応試験へ全件対応付けた。メモリー
  SQLite上でスコープ終了時の自動取消し、明示確定、明示取消しと所有解除、取引終了後の無操作を
  固定した。
- 4工程・8入力の既存`KisAdaptedLockTest`を比較対象とし、642工程・1,312入力の資源DB試験へは
  接続しなかった。実装を接続しない最初の試験構築は4工程・8入力でアダプター4メソッドの未解決
  参照をリンク診断した。分離後は所有対象を1工程・3入力、新規試験を5工程・11入力に収め、製品
  資源ライブラリーは140工程・307入力を維持した。macOSの対象実行と20回反復に成功した。公開面は
  1,546ヘッダー、29,981 API、対応済み4,293件、未対応25,688件になり、同ヘッダーのpublic APIは
  全件対応済みになった。製品`kritaresources`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 一時資源格納ロック public API契約と外部境界分離で完了した作業

- `libs/resources/KisTemporaryResourceStorageLock.cpp`に同居していた格納名選択・RAIIと、実ロケーター
  操作を分けた。格納名選択・RAIIは同じファイルに残して`kritaresources`の直接ソース所有から新規
  `kritatemporaryresourcestoragelockobjects`へ移した。`KisResourceLocator`への存在確認・メモリー格納
  登録・除去は新規`libs/resources/KisTemporaryResourceStorageLocator.{h,cpp}`へ抽出し、新規
  `kritatemporaryresourcestoragelocatorobjects`の所有とした。製品`kritaresources`は両生成オブジェクトを
  1回ずつ集約する。外部向けヘッダーと製品ABIは維持した。
- 宣言だけで製品定義と利用がなかった`KisTemporaryResourceStorageLockAdapter::try_lock()`は、既存名を
  避ける通常施錠を実行して成功を返すように実装し、継承済み非待機構築経路を利用可能にした。
  外部ロケーター操作を内部の具体的な橋渡しへ置いたため、試験は格納名集合を供給しつつ製品と同じ
  格納名選択・RAII実装を直接検査する。
- `libs/resources/KisTemporaryResourceStorageLock.h`のアダプターと包装型、構築、施錠・非待機施錠・
  解錠、現在名、基底型別名からなる2クラス・1型別名・5メソッドの8 APIを、新規
  `libs/resources/tests/KisTemporaryResourceStorageLockContractTest.cpp`の3対応試験へ全件対応付けた。
  基本名と連番による衝突回避、登録・除去、現在名、非待機成功、スコープ終了時の自動除去を固定した。
- 642工程・1,312入力のロケーター試験へ接続せず、4工程・8入力の既存汎用ロック試験を比較対象に
  した。実装を接続しない最初の試験構築は4工程・8入力で、未定義`try_lock()`を含む5メソッドを
  リンク診断した。分離後はロック本体と実ロケーター橋渡しを各1工程・3入力、新規試験を5工程・
  11入力に収めた。製品資源ライブラリーは外部境界分離により140工程・307入力から141工程・309入力に
  なった。macOSで両実装をコンパイルし、対象実行と20回反復に成功した。公開面は1,546ヘッダー、
  29,981 API、対応済み4,301件、未対応25,680件になり、同ヘッダーのpublic APIは全件対応済みに
  なった。製品`kritaresources`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源一覧表示方式 public API契約で完了した作業

- `libs/resources/ui/ResourceListViewModes.h`の表示方式と格子・横方向帯・詳細の列挙値からなる
  1列挙型・3列挙値の4 APIを、新規
  `libs/resources/ui/tests/ResourceListViewModesContractTest.cpp`の1試験へ全件対応付けた。各値の
  識別性と宣言順に対応する値0、1、2を固定した。ヘッダーだけの既存挙動を対象とするため、製品
  実装と製品CMake所有に変更はない。
- 252工程・534入力の既存UI契約へ接続せず、4工程・8入力の既存最小Qt契約を比較対象にした。
  新規試験も4工程・8入力に収まり、最初の構築と実行で既存挙動を確認した。macOSの対象実行と
  20回反復に成功した。公開面は1,546ヘッダー、29,981 API、対応済み4,305件、未対応25,676件に
  なり、同ヘッダーのpublic APIは全件対応済みになった。Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源UI記述子 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisResourceUiDescriptor.cpp`は同じ配置のまま、`kritaresourceui`の直接ソース所有
  から新規`kritaresourceuidescriptorobjects`の所有へ移し、製品`kritaresourceui`が生成
  オブジェクトを1回だけ集約する構造にした。外部向けヘッダー、実装、製品ABIは維持した。
- `libs/resources/ui/KisResourceUiDescriptor.h`の境界、構築、資源種別取得、プレビュー可否取得からなる
  1クラス・3メソッドの4 APIを、新規
  `libs/resources/ui/tests/KisResourceUiDescriptorContractTest.cpp`の1試験へ全件対応付けた。構築時の
  資源種別、省略時のプレビュー無効、明示時のプレビュー有効を固定した。
- 252工程・534入力の既存UI契約へ接続せず、4工程・8入力の最小Qt契約を比較対象にした。分離後は
  所有対象を1工程・3入力、新規試験を5工程・11入力に収め、製品UIライブラリーは248工程・527入力を
  維持した。macOSの対象実行と20回反復に成功した。公開面は1,546ヘッダー、29,981 API、対応済み
  4,309件、未対応25,672件になり、同ヘッダーのpublic APIは全件対応済みになった。製品
  `kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b タグ表示 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisTagLabel.cpp`は同じ配置のまま、`kritaresourceui`の直接ソース所有から新規
  `kritataglabelobjects`の所有へ移し、製品`kritaresourceui`が生成オブジェクトを1回だけ集約する
  構造にした。自動メタオブジェクト生成は新しい所有対象が担当する。外部向けヘッダー、実装、
  製品ABIは維持した。分離コンパイルで検出した未使用の色取得2件は、描画結果に寄与しない死んだ
  計算として`libs/resources/ui/KisTagLabel.cpp`から除いた。
- `libs/resources/ui/KisTagLabel.h`の境界、構築、破棄、文字列取得からなる1クラス・3メソッドの4 APIを、
  新規`libs/resources/ui/tests/KisTagLabelContractTest.cpp`の1試験へ全件対応付けた。構築時文字列、
  QWidget親子関係、親の終了に伴う子の自動破棄を固定した。
- 252工程・534入力の既存UI契約へ接続せず、Qt Widgetsと自動メタオブジェクト生成を含む所有対象を
  3工程・7入力、新規試験を7工程・14入力に収めた。製品UIライブラリーは独立した自動
  メタオブジェクト生成に必要な2工程・4入力が加わり、248工程・527入力から250工程・531入力に
  なった。macOSの対象実行と20回反復、死んだ計算の除去後の再実行に成功した。公開面は1,546
  ヘッダー、29,981 API、対応済み4,313件、未対応25,668件になり、同ヘッダーのpublic APIは全件
  対応済みになった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源検索条件 public API契約と試験構築範囲分離で完了した作業

- `libs/resources/KisResourceSearchBoxFilter.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有
  から新規`kritaresourcesearchboxfilterobjects`の所有へ移し、製品`kritaresources`が生成
  オブジェクトを1回だけ集約する構造にした。使われていなかった`kis_debug.h`のincludeを除き、
  直接依存をQt Coreだけにした。外部向けヘッダー、検索実装、製品ABIは維持した。
- 既存`libs/resources/tests/TestResourceSearchBoxFilter.cpp`を全資源試験の共通リンク集合から独立させ、
  `simpletest.h`のアプリケーション・試験資源初期化をQtのGUIなし試験入口へ置き換えた。名前の部分一致・
  完全一致、複数条件、タグの包含・除外、大文字小文字非依存、空条件を検査する既存試験内容は維持した。
- `libs/resources/KisResourceSearchBoxFilter.h`の境界、構築・破棄、条件更新、空状態、資源照合からなる
  1クラス・5メソッドの6 APIを、既存`TestResourceSearchBoxFilter::testResourceSearch`へ全件対応付けた。
  645工程・1,318入力だった試験を5工程・11入力、所有対象を1工程・3入力に縮め、製品資源
  ライブラリーは141工程・309入力を維持した。macOSの対象実行と20回反復に成功した。公開面は
  1,546ヘッダー、29,981 API、対応済み4,319件、未対応25,662件になり、同ヘッダーのpublic APIは
  全件対応済みになった。製品`kritaresources`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源サムネイルキャッシュ public API契約と外部境界分離で完了した作業

- `libs/resources/KisResourceThumbnailCache.cpp`は同じ配置のまま、`kritaresources`の直接ソース所有から
  新規`kritaresourcethumbnailcacheobjects`の所有へ移し、製品`kritaresources`が生成オブジェクトを
  1回だけ集約する構造にした。外部向けヘッダー、キャッシュ実装、製品ABIは維持した。
- `libs/resources/KisResourceThumbnailCache.cpp`から`KisResourceLocator`の非公開格納場所正規化を直接
  呼ぶ経路は、新規内部宣言`libs/resources/KisResourceThumbnailStorageLocation.h`へ移した。製品定義は
  既存`libs/resources/KisResourceLocator.cpp`へ置き、`libs/resources/KisResourceLocator.h`の友達指定を
  キャッシュ本体から内部橋渡しへ移した。格納場所の所有責務と製品翻訳単位数を維持し、試験では
  正規化結果だけを置換できる構造にした。
- `libs/resources/KisResourceThumbnailCache.h`の境界、構築・破棄、共有個体取得、画像取得からなる
  1クラス・4メソッドの5 APIを、新規
  `libs/resources/tests/KisResourceThumbnailCacheContractTest.cpp`の2試験へ全件対応付けた。製品の問い合わせ
  写像と同じ友達経路で原画像を投入し、格納場所正規化、原寸取得、縦横比を保つ縮小、返却画像の
  キャッシュ破棄後寿命、共有個体の同一性を固定した。
- 642工程・1,312入力の既存ロケーター試験へ接続せず、キャッシュ所有対象を1工程・3入力、新規試験を
  5工程・12入力に収め、製品資源ライブラリーは141工程・309入力を維持した。macOSでロケーターの
  単一生成物をコンパイルし、対象実行と20回反復に成功した。公開面は1,546ヘッダー、29,981 API、
  対応済み4,324件、未対応25,657件になり、同ヘッダーのpublic APIは全件対応済みになった。製品
  `kritaresources`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b アイコン説明表示 public API契約と構築所有分離で完了した作業

- `libs/widgetutils/KoCheckerBoardPainter.cpp`は同じ配置のまま、`kritawidgetutils`の直接ソース所有から
  新規`kritacheckerboardpainterobjects`へ移した。`libs/widgetutils/KoItemToolTip.cpp`も同じ配置のまま
  新規`kritaitemtooltipobjects`へ移し、製品`kritawidgetutils`が両生成オブジェクトを1回ずつ集約する
  構造にした。チェッカー描画はQt Gui、基底説明枠はQt Widgetsだけを直接依存とする。
- `libs/resources/ui/KisIconToolTip.cpp`は同じ配置のまま、`kritaresourceui`の直接ソース所有から新規
  `kritaicontooltipobjects`へ移し、製品`kritaresourceui`が生成オブジェクトを1回だけ集約する構造に
  した。外部向けヘッダー、説明文書生成、両製品ABIは維持した。
- `libs/resources/ui/KisIconToolTip.h`の境界、構築・破棄、固定寸法設定、チェッカー背景設定からなる
  1クラス・4メソッドの5 APIを、新規`libs/resources/ui/tests/KisIconToolTipContractTest.cpp`の3試験へ
  全件対応付けた。サムネイルの縦横比を保つ固定論理寸法、透明画像に対するチェッカー背景の有効・
  無効切替、生成文書の親子所有と破棄を固定した。
- チェッカー描画は1工程・3入力、基底説明枠は3工程・7入力、アイコン説明生成は1工程・3入力、
  新規試験は分離済みキャッシュを含め10工程・22入力に収めた。基底説明枠の独立した自動
  メタオブジェクト生成により、製品`kritawidgetutils`は226工程・485入力から228工程・489入力、
  その下流の`kritaresourceui`は250工程・531入力から252工程・535入力になった。macOSの対象実行と
  20回反復に成功した。公開面は1,546ヘッダー、29,981 API、対応済み4,329件、未対応25,652件に
  なり、同ヘッダーのpublic APIは全件対応済みになった。両製品ライブラリーのリンク、Linux、
  全ネイティブ検証は実行していない。

## R2-G19b 資源サムネイル描画 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisResourceThumbnailPainter.cpp`は同じ配置のまま、`kritaresourceui`の直接ソース
  所有から新規`kritaresourcethumbnailpainterobjects`の所有へ移し、製品`kritaresourceui`が生成
  オブジェクトを1回だけ集約する構造にした。外部向けヘッダー、描画実装、製品ABIは維持した。
- `libs/resources/ui/KisResourceThumbnailPainter.h`の境界、構築・破棄、準備済み画像生成、直接描画から
  なる1クラス・4メソッドの5 APIを、新規
  `libs/resources/ui/tests/KisResourceThumbnailPainterContractTest.cpp`の3試験へ全件対応付けた。QObject
  親子寿命、指定寸法のパターン画像生成、選択色の外周余白と資源画像の内部描画を固定した。
- 描画器所有対象を自動メタオブジェクト生成込みで3工程・7入力、新規試験を分離済みキャッシュ、
  チェッカー描画、資源種別値込みで11工程・24入力に収めた。最初のリンクは描画分岐が参照する
  3つの`ResourceType`定義不足を診断し、既存`kritaresourcestypesobjects`を契約へ明示接続して解決した。
  製品UIライブラリーは独立した自動メタオブジェクト生成に必要な2工程・4入力が加わり、252工程・
  535入力から254工程・539入力になった。macOSの対象実行と20回反復に成功した。公開面は1,546
  ヘッダー、29,981 API、対応済み4,334件、未対応25,647件になり、同ヘッダーのpublic APIは全件
  対応済みになった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源項目描画 public API契約と索引解決境界分離で完了した作業

- `libs/resources/ui/KisResourceItemDelegate.cpp`は同じ配置のまま、`kritaresourceui`の直接ソース
  所有から新規`kritaresourceitemdelegateobjects`の所有へ移し、製品`kritaresourceui`が生成
  オブジェクトを1回だけ集約する構造にした。使われていなかったデバッグ、サムネイルキャッシュ、
  アイコンのincludeを除き、外部向けヘッダー、描画実装、製品ABIは維持した。
- `libs/resources/ui/KisResourceItemDelegate.cpp`から`KisResourceModelProvider`を直接呼ぶ索引解決経路は、
  新規内部宣言`libs/resources/KisResourceModelIndexResolver.h`へ接続した。製品定義は既存
  `libs/resources/KisResourceModelProvider.cpp`へ置き、資源種別と識別番号から全体索引を得る責務と
  製品翻訳単位数を維持し、試験では決定的な全体索引だけを置換できる構造にした。
- `libs/resources/ui/KisResourceItemDelegate.h`の境界、構築・破棄、描画、文字表示切替、索引変換切替、
  推奨寸法からなる1クラス・6メソッドの7 APIを、新規
  `libs/resources/ui/tests/KisResourceItemDelegateContractTest.cpp`の5試験へ全件対応付けた。QObject親子寿命、
  装飾寸法、文字有無によるサムネイル領域、選択色の外周、局所索引から解決した全体索引の画像を
  固定した。
- 委譲描画所有対象を自動メタオブジェクト生成込みで3工程・7入力、新規試験を分離済みサムネイル
  描画器、キャッシュ、チェッカー描画、資源種別値込みで14工程・30入力に収めた。直接依存監査では
  国際化依存を外した構築が`KisResourceTypes.h`の`klocalizedstring.h`不足を診断したため、同依存を
  必須として維持した。製品UIライブラリーは独立した自動メタオブジェクト生成に必要な2工程・4入力が
  加わり、254工程・539入力から256工程・543入力になった。macOSで資源モデル提供元の単一生成物を
  コンパイルし、対象実行と20回反復に成功した。公開面は1,546ヘッダー、29,981 API、対応済み
  4,341件、未対応25,640件になり、同ヘッダーのpublic APIは全件対応済みになった。製品
  `kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源選択項目寸法同期 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisResourceItemChooserSync.cpp`は同じ配置のまま、`kritaresourceui`の直接ソース
  所有から新規`kritaresourceitemchoosersyncobjects`の所有へ移し、製品`kritaresourceui`が生成
  オブジェクトを1回だけ集約する構造にした。直接依存はQt Coreだけとし、外部向けヘッダー、同期実装、
  製品ABIは維持した。
- `libs/resources/ui/KisResourceItemChooserSync.h`の境界、構築・破棄、共有個体取得、基準長取得・設定・
  変更通知からなる1クラス・6メソッドの7 APIを、新規
  `libs/resources/ui/tests/KisResourceItemChooserSyncContractTest.cpp`の4試験へ全件対応付けた。既定値50、
  25から100への丸め、同じ実効値を再設定した場合を含む毎回の変更通知、共有個体の同一性、直接構築
  個体のQObject破棄を固定した。
- 寸法同期所有対象を自動メタオブジェクト生成込みで3工程・7入力、新規試験を7工程・14入力に収めた。
  製品UIライブラリーは独立した自動メタオブジェクト生成に必要な2工程・4入力が加わり、256工程・
  543入力から258工程・547入力になった。macOSの対象実行と20回反復に成功した。公開面は
  1,546ヘッダー、29,981 API、対応済み4,348件、未対応25,633件になり、同ヘッダーのpublic APIは
  全件対応済みになった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 運動スクロール public API契約と構築所有分離で完了した作業

- `libs/widgetutils/KisKineticScroller.cpp`は同じ配置のまま、`kritawidgetutils`の直接ソース所有から
  新規`kritakineticscrollerobjects`の所有へ移し、製品`kritawidgetutils`が生成オブジェクトを1回だけ
  集約する構造にした。直接依存はQt WidgetsとKF ConfigCoreに限定し、外部向けヘッダー、設定解釈、
  製品ABIは維持した。
- `libs/widgetutils/KisKineticScroller.h`の設定済み操作種別取得、対象領域へのスクローラー構成、状態別
  カーソル設定からなる3 APIを、新規`libs/widgetutils/tests/KisKineticScrollerContractTest.cpp`の4試験へ
  全件対応付けた。設定値0から3と不明値の対応、無効時の空結果、有効な項目表示の縦横画素単位移動、
  押下・ドラッグ・その他状態のカーソルを固定した。設定はQtの試験用標準パスだけに書き、各試験後に
  運動スクロール用キーを消去する。
- 運動スクロール所有対象を自動メタオブジェクト生成込みで3工程・7入力、新規試験を7工程・15入力に
  収めた。製品ウィジェット補助ライブラリーは独立した自動メタオブジェクト生成に必要な2工程・4入力が
  加わり、228工程・489入力から230工程・493入力になった。macOSの対象実行と20回反復に成功した。
  公開面は1,546ヘッダー、29,981 API、対応済み4,351件、未対応25,630件になり、同ヘッダーの
  public APIは全件対応済みになった。製品`kritawidgetutils`のリンク、Linux、全ネイティブ検証は
  実行していない。

## R2-G19b 資源項目表 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisResourceItemView.cpp`は同じ配置のまま、`kritaresourceui`の直接ソース所有から
  新規`kritaresourceitemviewobjects`の所有へ移し、製品`kritaresourceui`が生成オブジェクトを1回だけ
  集約する構造にした。直接コンパイル依存はQt Widgetsとウィジェット補助ヘッダーに限定し、外部向け
  ヘッダー、項目表実装、製品ABIは維持した。
- `libs/resources/ui/KisResourceItemView.h`の境界、表示方式と2列挙値、構築・破棄、4通知、スクロール
  状態スロットからなる11 APIを、新規`libs/resources/ui/tests/KisResourceItemViewContractTest.cpp`の
  8試験へ全件対応付けた。固定列・固定行のスクロール方針、選択と解除、現在項目の再クリック、文脈
  メニューの大域位置、寸法変更、状態別カーソル、QObject破棄を固定した。
- 最初の実行では、要求行高20がmacOS様式の最小値24へ丸められることと、非表示の最上位
  ウィジェットへ対する寸法設定では寸法変更事象が生じないことを診断した。契約は様式最小値を含む
  実効行高を検査し、試験派生型から寸法変更事象を決定的に渡す構造へ修正した。
- 項目表所有対象を自動メタオブジェクト生成込みで3工程・7入力、新規試験を分離済み運動スクロール、
  説明枠、説明生成、チェッカー描画、サムネイルキャッシュ込みで16工程・35入力に収めた。運動
  スクロール分離後の製品UIライブラリーは、独立した自動メタオブジェクト生成に必要な2工程・4入力が
  加わり、260工程・551入力から262工程・555入力になった。macOSの対象実行と20回反復に成功した。
  公開面は1,546ヘッダー、29,981 API、対応済み4,362件、未対応25,619件になり、同ヘッダーの
  public APIは全件対応済みになった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は
  実行していない。

## R2-G19b 資源項目一覧 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisResourceItemListView.cpp`は同じ配置のまま、`kritaresourceui`の直接ソース所有
  から新規`kritaresourceitemlistviewobjects`の所有へ移し、製品`kritaresourceui`が生成オブジェクトを
  1回だけ集約する構造にした。直接コンパイル依存はQt Widgetsとウィジェット補助ヘッダーに限定し、
  外部向けヘッダー、一覧実装、製品ABIは維持した。
- `libs/resources/ui/KisResourceItemListView.h`の境界、構築・破棄、表示方式・項目寸法・厳格選択・
  説明表示の4設定、4通知、スクロール状態スロットからなる13 APIを、新規
  `libs/resources/ui/tests/KisResourceItemListViewContractTest.cpp`の11試験へ全件対応付けた。格子・横帯・
  詳細表示、厳格選択中の現在行削除、説明画像の固定寸法とチェッカー背景、選択・クリック・文脈
  メニュー通知、状態別カーソル、QObject破棄を固定した。
- 内部説明表示への設定委譲は、`libs/resources/ui/KisIconToolTip.h`の非公開友達から生成文書画像を
  観測した。privateの寸法変更処理は`libs/resources/ui/KisResourceItemListView.h`の非公開友達から
  決定的な事象を渡した。どちらも公開APIとABIを変更しない試験アクセスである。
- 最初のコンパイルはprivate寸法処理への派生型アクセスを診断した。最初の実行では、通常の事象配送、
  macOSの単一選択に対するCtrl押下、非公開表示の座標クリックが対象分岐や通知を決定的に生じないことを
  診断し、private処理への友達アクセス、現在行削除、Qtのクリック信号転送という所有責務そのものへ
  観測を絞った。宣言済み`sigSizeChanged()`が寸法処理後も送出されない現行挙動は既知不具合に分類した。
- 一覧所有対象を自動メタオブジェクト生成込みで3工程・7入力、新規試験を分離済み運動スクロール、
  説明枠、説明生成、チェッカー描画、サムネイルキャッシュ込みで16工程・35入力に収めた。製品UI
  ライブラリーは独立した自動メタオブジェクト生成に必要な2工程・4入力が加わり、262工程・555入力
  から264工程・559入力になった。macOSの対象実行と20回反復に成功した。公開面は1,546ヘッダー、
  29,981 API、対応済み4,375件、未対応25,606件になり、同ヘッダーのpublic APIは全件対応済みに
  なった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 項目内蔵資源一覧 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisResourceItemListWidget.cpp`は同じ配置のまま、`kritaresourceui`の直接ソース所有
  から新規`kritaresourceitemlistwidgetobjects`の所有へ移し、製品`kritaresourceui`が生成オブジェクトを
  1回だけ集約する構造にした。直接コンパイル依存はQt Widgetsとウィジェット補助ヘッダーに限定し、
  外部向けヘッダー、項目内蔵一覧実装、製品ABIは維持した。
- `libs/resources/ui/KisResourceItemListWidget.h`の境界、構築・破棄、表示方式・項目寸法・厳格選択・
  説明表示の4設定、4通知、スクロール状態スロットからなる13 APIを、新規
  `libs/resources/ui/tests/KisResourceItemListWidgetContractTest.cpp`の11試験へ全件対応付けた。複数選択の
  既定、格子・横帯・詳細表示、厳格単一選択中の現在項目削除、説明画像の固定寸法とチェッカー背景、
  選択・クリック・文脈メニュー通知、状態別カーソル、QObject破棄を固定した。
- 内部説明表示への設定委譲は、既存の`libs/resources/ui/KisIconToolTip.h`の非公開友達から生成文書画像を
  観測した。privateの寸法変更処理は`libs/resources/ui/KisResourceItemListWidget.h`の非公開友達から
  決定的な事象を渡した。後者は公開APIとABIを変更しない試験アクセスである。宣言済み
  `sigSizeChanged()`が寸法処理後も送出されない現行挙動は既知不具合に分類した。
- 項目内蔵一覧所有対象を自動メタオブジェクト生成込みで3工程・7入力、新規試験を分離済み運動
  スクロール、説明枠、説明生成、チェッカー描画、サムネイルキャッシュ込みで16工程・35入力に収めた。
  製品UIライブラリーは独立した自動メタオブジェクト生成に必要な2工程・4入力が加わり、264工程・
  559入力から266工程・563入力になった。macOSの対象実行と20回反復に成功した。公開面は1,546
  ヘッダー、29,981 API、対応済み4,388件、未対応25,593件になり、同ヘッダーのpublic APIは全件
  対応済みになった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 格納場所描画委譲 public API契約と構築所有分離で完了した作業

- 描画と格納場所モデル更新が一つの翻訳単位にあり、`KisStorageChooserWidget.cpp`のオブジェクトだけを
  指定しても製品依存が239工程・510入力まで展開していた。宣言は
  `libs/resources/ui/KisStorageChooserWidget.h`から新規
  `libs/resources/ui/KisStorageChooserDelegate.h`へ、実装は
  `libs/resources/ui/KisStorageChooserWidget.cpp`から新規
  `libs/resources/ui/KisStorageChooserDelegate.cpp`へ移した。描画実装を新規
  `kritastoragechooserdelegateobjects`が所有し、製品`kritaresourceui`は生成オブジェクトを1回だけ
  集約する。残る`libs/resources/ui/KisStorageChooserWidget.cpp`も製品の直接ソース所有から新規
  `kritastoragechooserwidgetobjects`へ移し、次の契約追加で製品全体を構築せず検証できるようにした。
  元のウィジェットヘッダーは公開include互換を維持するため新ヘッダーを取り込む。
- `libs/resources/ui/KisStorageChooserDelegate.h`の境界、構築・破棄、描画、寸法からなる5 APIを、
  新規`libs/resources/ui/tests/KisStorageChooserDelegateContractTest.cpp`の3試験へ全件対応付けた。
  QObjectの親所有、固定幅と装飾高に基づく寸法、無効索引の無描画、有効索引の画像、押下状態、
  チェック状態を固定した。試験はFusion様式の描画呼出しを記録し、アイコン読込を空の決定的な
  外部境界へ置き換えるため、製品のアイコン資源やモデルデータベースを構築しない。
- 最初の分離対象コンパイルは`KisStorageModel.h`の推移的な翻訳文字列ヘッダー要求を診断し、直接依存へ
  KF I18nだけを追加した。最初の試験コンパイルは`Q_OBJECT`を持たない記録様式への`qobject_cast`を
  診断し、通常のC++実行時型検査へ修正した。分離後の選択ウィジェットコンパイルは、描画側とともに
  除いた`QApplication`が`qApp`に必要であることを診断し、利用元へ明示的に戻した。最初の試験実行と
  20回反復は成功した。
- 描画委譲所有対象と選択ウィジェット所有対象を各3工程・7入力、新規試験を7工程・15入力に収めた。
  製品UIライブラリーは二つの独立実装と自動メタオブジェクト生成を集約し、266工程・563入力から
  271工程・573入力になった。公開面は1,547ヘッダー、29,981 API、対応済み4,393件、未対応25,588件に
  なり、描画委譲のpublic APIは全件対応済みになった。製品`kritaresourceui`のリンク、Linux、
  全ネイティブ検証は実行していない。

## R2-G19b ポップアップボタン public API契約と構築所有分離で完了した作業

- `libs/widgetutils/KisPopupButton.cpp`は同じ配置のまま、`kritawidgetutils`の直接ソース所有から新規
  `kritapopupbuttonobjects`の所有へ移し、製品`kritawidgetutils`が生成オブジェクトを1回だけ集約する
  構造にした。直接コンパイル依存はQt Widgetsとインラインの画面内配置関数を持つglobalヘッダーに
  限定し、外部向けヘッダー、実装、製品ABIは維持した。
- `libs/widgetutils/KisPopupButton.h`の境界、構築・破棄、ポップアップ設定、表示・非表示・状態取得、
  表示通知、幅・位置・分離・矢印設定からなる13 APIを、新規
  `libs/widgetutils/tests/KisPopupButtonContractTest.cpp`の6試験へ全件対応付けた。親所有とオブジェクト名、
  内部表示枠を介したポップアップ寿命、表示状態と通知、表示枠幅、ポップアップとダイアログの切替、
  画面内配置、矢印描画を固定した。
- 最初の実行はポップアップがボタンの直接のQObject子ではなく、最上位ウィジェット配下の内部表示枠の
  子であることを診断した。契約は直接親の推測を除き、ボタン単独の破棄が内部表示枠とポップアップを
  ともに破棄する実際の所有期間を検査する形へ修正した。対象実行と20回反復はmacOSで成功した。
- 製品直接所属時に154工程・336入力まで展開した対象を、自動メタオブジェクト生成込みで3工程・7入力、
  新規試験を7工程・14入力に収めた。製品ウィジェット補助ライブラリーは独立した自動メタオブジェクト
  生成に必要な2工程・4入力が加わり、230工程・493入力から232工程・497入力になった。公開面は
  1,547ヘッダー、29,981 API、対応済み4,406件、未対応25,575件になり、同ヘッダーのpublic APIは
  全件対応済みになった。製品`kritawidgetutils`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 格納場所絞り込み public API契約と構築所有分離で完了した作業

- `libs/resources/KisStorageFilterProxyModel.cpp`は同じ配置のまま、製品`kritaresources`の直接ソース
  所有から新規`kritastoragefilterproxymodelobjects`へ移し、製品は生成オブジェクトを1回だけ集約する。
  格納場所取得の具象モデル依存は元実装から新規`libs/resources/KisStorageFilterProxyModelSource.cpp`へ
  分け、新規内部ヘッダー`libs/resources/KisStorageFilterProxyModelSource_p.h`を介して呼び出す。
  この実装は`kritastoragefilterproxymodelsourceobjects`が所有し、製品が生成オブジェクトを1回だけ
  集約する。外部向けヘッダーと製品ABIは維持した。
- `libs/resources/KisStorageFilterProxyModel.h`の型、絞り込み種別、構築・破棄、条件設定、格納場所
  取得からなる9 APIを、新規`libs/resources/tests/KisStorageFilterProxyModelContractTest.cpp`の5試験へ
  全件対応付けた。列挙値、QObject親所有、最初の評価前に設定するファイル名・格納場所型・有効状態、
  代理索引から元索引への変換と取得委譲を維持契約として固定した。評価後の条件変更が選別済み行を
  再評価しない現状は既知不具合として分離した。
- 最初の分離対象コンパイルは、公開格納場所型が推移的に要求する`QImage`を診断し、利用する二つの
  所有対象へQt Guiを直接依存として追加した。対象試験の最初の実行と20回反復はmacOSで成功した。
- 製品直接所属時に112工程・252入力まで展開した絞り込み実装を、自動メタオブジェクト生成込みで
  3工程・7入力、格納場所取得実装を1工程・3入力、新規試験を7工程・15入力に収めた。製品
  `kritaresources`は141工程・309入力から144工程・315入力になった。公開面は1,547ヘッダー、
  29,981 API、対応済み4,415件、未対応25,566件になり、同ヘッダーのpublic APIは全件対応済みに
  なった。製品`kritaresources`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 格納場所選択ウィジェット public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisStorageChooserWidget.cpp`に同居していた表示構成、製品モデル取得、クリック後の
  有効状態更新と警告判定を分けた。表示構成と自動メタオブジェクト生成は同じファイルと既存
  `kritastoragechooserwidgetobjects`に維持し、クリック後の処理を新規
  `libs/resources/ui/KisStorageChooserWidgetActivation.cpp`と
  `kritastoragechooserwidgetactivationobjects`へ移した。製品の格納場所モデル取得を新規
  `libs/resources/ui/KisStorageChooserWidgetModelSource.cpp`と
  `kritastoragechooserwidgetmodelsourceobjects`へ移した。モデル取得は内部ヘッダーを介して呼び、
  製品`kritaresourceui`は三つの生成オブジェクトを各1回集約する。外部向けヘッダー、クラス配置、
  製品ABIを維持した。
- `libs/resources/ui/KisStorageChooserWidget.h`の型、構築、破棄からなる3 APIを、新規
  `libs/resources/ui/tests/KisStorageChooserWidgetContractTest.cpp`の3試験へ全件対応付けた。親所有、
  64画素画像の単一選択一覧、絞り込みモデルと描画委譲、資源種別に応じた束・ブラシライブラリー・
  様式ライブラリーの選別、クリック通知の配線、所有する表示物の破棄を固定した。試験は製品の
  単一実体モデルとクリック後処理を決定的な内部境界へ置き換え、実データベースを構築しない。
- 最初の試験リンクは、同じ翻訳単位の非公開クリック処理を介して`KisStorageModel::instance()`、
  `KisResourceModel`の構築・破棄・絞り込み設定まで要求する診断になった。責務分割後、表示構成、
  クリック処理、モデル取得の各対象コンパイル、対象試験の単発実行と20回反復はmacOSで成功した。
- 表示構成対象は3工程・7入力を維持し、クリック処理とモデル取得を各1工程・3入力に収めた。必要な
  既存のポップアップ、格納場所絞り込み、描画委譲、資源型を直接集約する新規試験は18工程・38入力に
  収めた。製品`kritaresourceui`は276工程・583入力から278工程・587入力になった。公開面は
  1,547ヘッダー、29,981 API、対応済み4,418件、未対応25,563件になり、同ヘッダーのpublic APIは
  全件対応済みになった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源上書き確認・名前重複 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisResourceUserOperations.cpp`は製品`kritaresourceui`の直接ソース所有から同じ
  配置の`kritaresourceuseroperationsobjects`へ移し、残る取込・名前変更・追加・更新を所有する。
  上書き確認を元ファイルから新規`libs/resources/ui/KisResourceUserOperationsOverwrite.cpp`と
  `kritaresourceuseroperationsoverwriteobjects`へ、名前重複判定を新規
  `libs/resources/ui/KisResourceUserOperationsNameUsage.cpp`と
  `kritaresourceuseroperationsnameusageobjects`へ移した。具象モデルから資源IDを取得する処理は新規
  `libs/resources/ui/KisResourceUserOperationsNameSource.cpp`と
  `kritaresourceuseroperationsnamesourceobjects`が所有し、内部ヘッダーを介して呼ぶ。製品は四つの
  生成オブジェクトを各1回集約する。
- `libs/resources/ui/KisResourceUserOperations.h`のクラス、上書き確認、名前重複判定からなる3 APIを、
  新規`libs/resources/ui/tests/KisResourceUserOperationsContractTest.cpp`の2試験へ対応付けた。上書き
  確認の基底ファイル名、はいと取消し、取消しの既定選択と戻り値、完全一致名と空白を下線へ置換した
  名前の検索順、指定資源IDだけの除外を固定した。実装も利用元もなかった`userAllowsRename()`宣言は
  公開面から除去し、転送宣言や旧名の別名は設けていない。
- 最初の試験リンクは同じオブジェクトに残る取込・追加・更新処理を介して資源モデル、資源DB、保存先
  変換、全体資源インターフェースまで要求する診断になった。分割後の最初のmacOS実行は標準
  ダイアログの題名を英語と仮定した検査を診断し、翻訳文言ではなく標準ボタンと既定選択を検査する
  形へ修正した。対象試験の単発実行と20回反復は成功した。
- 製品直接所属時に246工程・524入力まで展開した元実装を1工程・3入力に縮め、上書き確認、名前重複、
  具象名前検索も各1工程・3入力、新規試験を6工程・14入力に収めた。製品`kritaresourceui`は
  278工程・587入力から281工程・593入力になった。公開面は未定義宣言の除去により1,547ヘッダー、
  29,980 APIとなり、対応済み4,421件、未対応25,559件になった。製品`kritaresourceui`のリンク、
  Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源取込・名前変更 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisResourceUserOperations.cpp`に残っていた取込を新規
  `libs/resources/ui/KisResourceUserOperationsImport.cpp`と
  `kritaresourceuseroperationsimportobjects`へ、名前変更を新規
  `libs/resources/ui/KisResourceUserOperationsRename.cpp`と
  `kritaresourceuseroperationsrenameobjects`へ移した。具象資源モデルと確認・警告を使う取込効果は新規
  `libs/resources/ui/KisResourceUserOperationsImportSource.cpp`と
  `kritaresourceuseroperationsimportsourceobjects`、名前変更効果は新規
  `libs/resources/ui/KisResourceUserOperationsRenameSource.cpp`と
  `kritaresourceuseroperationsrenamesourceobjects`が所有する。各高水準処理は対応する内部ヘッダーを介し、
  製品`kritaresourceui`が四つの生成オブジェクトを各1回集約する。
- `libs/resources/ui/KisResourceUserOperations.h`の取込と名前変更2 APIを、既存
  `libs/resources/ui/tests/KisResourceUserOperationsContractTest.cpp`の2試験へ対応付けた。通常取込の
  成功、既定保存先だけの上書き確認、取消し、承認後の再試行、保存先指定時の確認抑止、最終失敗警告、
  重複名の取消し・承認、一意名の直接変更、変更失敗警告を固定した。資源モデルの結果と警告効果は
  決定的な内部境界へ置き換え、実データベースを構築しない。
- 最初の試験リンクは、同じオブジェクトに残る追加・更新を介して安全検査、資源DB、保存先変換、
  直列化まで要求する診断になった。分割後の各対象コンパイル、対象試験のmacOS単発実行と20回反復は
  成功した。
- 追加・更新を所有する元対象、取込判断、取込効果、名前変更判断、名前変更効果を各1工程・3入力に
  収め、上書き確認と名前重複の既存対象も直接集約する試験を8工程・18入力に収めた。製品
  `kritaresourceui`は281工程・593入力から285工程・601入力になった。公開面は1,547ヘッダー、
  29,980 API、対応済み4,423件、未対応25,557件になり、同ヘッダーの未対応は2 APIになった。製品
  `kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源追加・更新 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisResourceUserOperations.cpp`に残っていた資源追加の判断を新規
  `libs/resources/ui/KisResourceUserOperationsAdd.cpp`と
  `kritaresourceuseroperationsaddobjects`へ、資源DB照合、追加、確認・警告を新規
  `libs/resources/ui/KisResourceUserOperationsAddSource.cpp`と
  `kritaresourceuseroperationsaddsourceobjects`へ移した。資源更新の判断は新規
  `libs/resources/ui/KisResourceUserOperationsUpdate.cpp`と
  `kritaresourceuseroperationsupdateobjects`へ、保存先変換、登録済み資源への直列化、名前照合、更新、
  確認・警告は新規`libs/resources/ui/KisResourceUserOperationsUpdateSource.cpp`と
  `kritaresourceuseroperationsupdatesourceobjects`へ移した。空になった開始ファイルと
  `kritaresourceuseroperationsobjects`は削除し、製品`kritaresourceui`は四つの生成オブジェクトを
  各1回集約する。
- `libs/resources/ui/KisResourceUserOperations.h`の追加と更新2 APIを、既存
  `libs/resources/ui/tests/KisResourceUserOperationsContractTest.cpp`の2試験へ対応付けた。既存
  ファイル名の上書き取消しと更新への委譲、重複名の取消しと追加許可、追加失敗警告、外部資源の
  登録済み個体への移送と移送失敗、変更名の重複取消しと更新許可、更新失敗警告を固定した。判断試験は
  DB、保存先、直列化、モデル、ダイアログを決定的な内部境界へ置き換え、実データベースを構築しない。
- 最初の試験リンクは、追加・更新と具象効果が同じオブジェクトにあったため、`KoResource`、
  `KisResourceModel`、資源DB、資源配置、全体資源インターフェース、直列化の未解決参照を診断した。
  分割後の具象DB照合の単体コンパイルでは、資源DBの私有検索を既存の友達クラス以外から呼ぶ診断が
  出たため、DB検索と外部資源移送を`KisResourceUserOperations`の私有補助に維持した。DB検索失敗時の
  出力IDは`-1`で初期化し、失敗後に未初期化値を参照する未定義状態を除いた。
- 追加判断、追加効果、更新判断、更新効果を各1工程・3入力に収め、上書き確認、名前重複、取込、
  名前変更の既存対象も直接集約する試験を10工程・22入力に収めた。製品`kritaresourceui`は
  285工程・601入力から288工程・607入力になった。各対象コンパイル、対象CTestのmacOS単発実行と
  20回反復、公開API契約検査は成功した。公開面は1,547ヘッダー、29,980 API、対応済み4,425件、
  未対応25,555件になり、同ヘッダーのpublic APIは全件対応済みになった。製品`kritaresourceui`の
  リンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b タグ選択ウィジェット構築・所有 public API契約と構築所有分離で完了した作業

- 製品`kritaresourceui`の直接ソースだった`libs/resources/ui/KisTagChooserWidget.cpp`から、構築と破棄を
  同じ開始ファイルと新規`kritatagchooserwidgetobjects`へ移した。資源種別に対応する表示モデル、
  全タグモデル、タグ操作表示の具象生成を新規
  `libs/resources/ui/KisTagChooserWidgetConstructionSource.cpp`と
  `kritatagchooserwidgetconstructionsourceobjects`へ、選択、設定保存、タグ変更、タグ追加、ダイアログを
  新規`libs/resources/ui/KisTagChooserWidgetOperations.cpp`と
  `kritatagchooserwidgetoperationsobjects`へ移した。内部状態は新規
  `libs/resources/ui/KisTagChooserWidget_p.h`に集約し、製品は三つの生成オブジェクトを各1回集約する。
- `libs/resources/ui/KisTagChooserWidget.h`の型、構築、破棄3 APIを、新規
  `libs/resources/ui/tests/KisTagChooserWidgetContractTest.cpp`の2試験へ対応付けた。指定親への所属、
  資源種別の伝達、一覧の挿入・寸法方針、全タグモデルとタグ操作表示の所有、余白のない配置、破棄時の
  子表示とモデルの解放を固定した。具象タグモデルとタグ操作表示は決定的な内部データ源へ置き換えた。
- 実装も利用元もなかった`selectedTagIsReadOnly()`宣言は公開面から除去し、転送宣言や旧名の別名は
  設けていない。最初の試験リンクは、同じ翻訳単位の残存操作を介してタグモデル、タグ操作ボタン、
  設定保存、タグ資源関連付け、`KoResource`、`KisTag`、安全検査まで要求する診断になった。分割後は
  自動メタオブジェクトが要求する共有ポインターの診断表示だけを試験内の値実装で満たし、製品資源を
  リンクしていない。
- 構築対象を3工程・7入力、具象生成と残存操作を各1工程・3入力に収め、新規試験を7工程・15入力に
  収めた。製品`kritaresourceui`は288工程・607入力から292工程・615入力になった。三つの対象
  コンパイル、対象CTestのmacOS単発実行と20回反復、公開API契約検査は成功した。公開面は
  1,547ヘッダー、29,979 API、対応済み4,428件、未対応25,551件になり、同ヘッダーの未対応は9 APIに
  なった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b タグ選択・通知 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisTagChooserWidgetOperations.cpp`に同居していたURL選択、現在索引、現在タグ、
  選択変更、選択通知、アイコン更新を新規`libs/resources/ui/KisTagChooserWidgetSelection.cpp`と
  `kritatagchooserwidgetselectionobjects`へ移した。タグ操作表示への反映、資源種別別の設定保存、
  アイコン再読込みは新規`libs/resources/ui/KisTagChooserWidgetSelectionSource.cpp`と
  `kritatagchooserwidgetselectionsourceobjects`へ移した。製品`kritaresourceui`は二つの生成オブジェクトを
  各1回集約し、残存操作対象は非公開タグ操作と公開タグ追加だけを所有する。
- `libs/resources/ui/KisTagChooserWidget.h`の選択、通知、アイコン更新6 APIを、既存
  `libs/resources/ui/tests/KisTagChooserWidgetContractTest.cpp`の3試験へ対応付けた。URL完全一致への移動、
  不一致時の選択維持、現在索引、タグ役割値、未選択時のnull、タグ操作表示と資源種別別設定への反映、
  名前順の整列、同じタグの通知、負の索引から先頭への復帰、アイコン再読込みを固定した。
- 最初の試験リンクは、分離前の試験対象に選択実装がなく、`setCurrentItem()`、`currentIndex()`、
  `currentlySelectedTag()`、`updateIcons()`の未解決参照を診断した。選択対象の分離後、具象効果の
  コンパイルはタグ操作表示の私有操作を友達クラス以外から呼ぶ診断になったため、操作表示への反映と
  アイコン再読込みは`KisTagChooserWidget`の私有補助として具象効果対象に維持し、外部公開しなかった。
- 選択判断と具象効果を各1工程・3入力に収め、構築対象と直接集約する試験を8工程・18入力に収めた。
  製品`kritaresourceui`は292工程・615入力から294工程・619入力になった。選択判断、具象効果、残存操作
  の対象コンパイル、対象CTestのmacOS単発実行と20回反復、公開API契約検査は成功した。公開面は
  1,547ヘッダー、29,979 API、対応済み4,434件、未対応25,545件になり、同ヘッダーの未対応は3 APIに
  なった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b タグ追加 public API契約と構築所有分離で完了した作業

- `libs/resources/ui/KisTagChooserWidgetOperations.cpp`に残っていた三つのタグ追加入口と既存名の確認処理を
  新規`libs/resources/ui/KisTagChooserWidgetAdd.cpp`と`kritatagchooserwidgetaddobjects`へ移した。
  タグ値の名前・URL取得、モデル照合、予約名警告、既存タグと資源の復元、新規追加、名前順整列、
  確認ダイアログを新規`libs/resources/ui/KisTagChooserWidgetAddSource.cpp`と
  `kritatagchooserwidgetaddsourceobjects`へ移し、内部境界の宣言を新規
  `libs/resources/ui/KisTagChooserWidgetAddSource_p.h`に置いた。製品`kritaresourceui`は二つの生成
  オブジェクトを各1回集約し、残存操作対象は非公開タグ操作だけを所有する。
- `libs/resources/ui/KisTagChooserWidget.h`のタグ追加3 APIを、既存
  `libs/resources/ui/tests/KisTagChooserWidgetContractTest.cpp`の3試験へ対応付けた。文字列入口のnull資源
  委譲、予約名と空名の拒否、一意な名前とタグ値の追加、任意資源の伝達、URLによる既存タグ照合、
  復元・取消し・置換の分岐、資源種別の伝達を固定した。タグモデル、資源関連付け、ダイアログは
  決定的な内部効果へ置き換えた。
- 最初の赤試験は追加入口が無操作だったため、予約名警告、既存名確認、タグ値の予約名警告がいずれも
  期待1回に対して実績0回と診断した。判断と具象効果の分離後、追加判断、追加効果、残存操作をそれぞれ
  対象指定でコンパイルし、既存の製品処理を具象効果へそのまま維持した。
- 追加判断と具象効果を各1工程・3入力に収め、構築・選択の既存対象と直接集約する試験を9工程・20入力に
  収めた。製品`kritaresourceui`は294工程・619入力から296工程・623入力になった。対象CTestのmacOS
  単発実行と20回反復は成功した。公開面は1,547ヘッダー、29,979 API、対応済み4,437件、未対応25,542件に
  なり、同ヘッダーのpublic APIは全件対応済みになった。製品`kritaresourceui`のリンク、Linux、全
  ネイティブ検証は実行していない。

## R2-G19b タグUI操作 public API契約と構築所有分離で完了した作業

- 製品`kritaresourceui`の直接ソースだった`libs/resources/ui/TagActions.cpp`を同じファイル位置のまま
  新規`kritatagactionsobjects`へ移した。同ファイルにあった自己起動入力欄とアイコンの具象生成を新規
  `libs/resources/ui/TagActionsSource.cpp`と`kritatagactionssourceobjects`へ移し、内部境界の宣言を
  新規`libs/resources/ui/TagActionsSource_p.h`に置いた。製品は二つの生成オブジェクトを各1回集約する。
- `libs/resources/ui/TagActions.h`の既存タグ操作、文字入力操作、利用者入力タグ操作、資源付き新規タグ
  操作、タグ比較器からなる26 APIを、新規`libs/resources/ui/tests/TagActionsContractTest.cpp`の5試験へ
  対応付けた。親所有と破棄、タグ名表示、タグ・資源通知、入力欄とアイコンの構成、表示状態、親を閉じる
  方針、入力通知と消去、資源差替え、nullを含むタグURL比較を固定した。入力欄とアイコン生成は決定的な
  内部データ源へ置き換えた。
- 変更前の`TagActions.cpp`は製品`kritaresourceui`の公開依存であるQt Core、Qt Widgets、資源、
  ウィジェット補助と、非公開依存であるQt SQL、版、全体補助、プラグイン、KDE Frameworks各部を含む
  296工程・623入力の閉包に所属していた。新規試験の最初のコンパイルは公開ヘッダーが直接必要とする
  国際化ヘッダーの不足を診断した。依存を明示した後の最初のリンクは、全公開操作、通知、メタ
  オブジェクトと資源診断表示の未解決参照を示し、製品全体ではなく分離した操作対象だけを接続した。
- タグ操作対象を自動メタオブジェクト生成込みで3工程・7入力、具象生成を1工程・3入力、新規試験を
  7工程・15入力に収めた。製品`kritaresourceui`は296工程・623入力から299工程・629入力になった。
  二つの対象コンパイル、対象CTestのmacOS単発実行と20回反復は成功した。公開面は1,547ヘッダー、
  29,979 API、対応済み4,463件、未対応25,516件になり、同ヘッダーのpublic APIは全件対応済みになった。
  製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源項目選択の型・構築 public API契約と構築所有分離で完了した作業

- 製品`kritaresourceui`の直接ソースだった`libs/resources/ui/KisResourceItemChooser.cpp`から内部状態を
  新規`libs/resources/ui/KisResourceItemChooser_p.h`へ、構築、破棄、記述子取得を新規
  `libs/resources/ui/KisResourceItemChooserConstruction.cpp`と
  `kritaresourceitemchooserconstructionobjects`へ移した。具体的なタグモデル、タグ管理、一覧、描画委譲、
  表示方式・格納場所ボタン、プレビュー、取込・除去ボタン、応答配置の生成を新規
  `libs/resources/ui/KisResourceItemChooserConstructionSource.cpp`と
  `kritaresourceitemchooserconstructionsourceobjects`へ移した。開始ファイルの残存操作は同じファイル位置の
  まま新規`kritaresourceitemchooseroperationsobjects`へ移し、製品は三つの生成オブジェクトを各1回
  集約する。
- `libs/resources/ui/KisResourceItemChooser.h`の型、二つの列挙、六つの列挙値、構築、破棄、記述子取得
  からなる12 APIを、既存`libs/resources/ui/tests/TestResourceUiContract.cpp`の3試験へ対応付けた。
  ボタンと応答配置の数値、指定親への所属、資源種別とプレビュー方針の保持、具象表示構築への単一委譲、
  破棄後の監視参照無効化を固定した。
- 既存`TestResourceUiContract`は製品`kritaresourceui`全体へ直接リンクしていた。局所対象への変更後、
  最初のリンクは資源種別定数と資源診断表示の未解決参照を示したため、それぞれ既存の資源種別対象と
  試験内の値実装へ接続した。試験のメタオブジェクトが参照する未対象のスロットと保護仮想関数は、
  挙動契約として数えず試験内の無操作実装で満たした。
- 構築対象を自動メタオブジェクト生成込みで3工程・7入力、具象構築と残存操作を各1工程・3入力、
  記述子・資源種別と直接集約する試験を10工程・22入力に収めた。製品`kritaresourceui`は299工程・
  629入力から303工程・637入力になった。三つの対象コンパイル、対象CTestのmacOS単発実行と20回反復は
  成功した。公開面は1,547ヘッダー、29,979 API、対応済み4,475件、未対応25,504件になり、同ヘッダーの
  未対応は28 APIになった。製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源項目選択の表示制御 public API契約と効果分離で完了した作業

- `libs/resources/ui/KisResourceItemChooser.cpp`にあった応答配置の有効化判断、表示方式の適用判断、タグ欄、
  行列寸法、描画委譲、操作ボタン、プレビュー方向、公開子表示取得、アイコン更新を、新規
  `libs/resources/ui/KisResourceItemChooserPresentation.cpp`へ移した。そこから呼ぶ具体的な一覧、タグ管理、
  ボタン、分割表示、アイコンの操作を新規`libs/resources/ui/KisResourceItemChooserPresentationSource.cpp`へ
  移し、内部境界を新規`libs/resources/ui/KisResourceItemChooserPresentationSource_p.h`に置いた。開始
  ファイルには資源選択、プレビュー生成、同期、入力、応答配置の具体的な組替えが残り、製品は新しい
  判断対象と具象効果対象を各1回集約する。
- `libs/resources/ui/KisResourceItemChooser.h`の表示制御15 APIを、既存
  `libs/resources/ui/tests/TestResourceUiContract.cpp`の4試験へ対応付けた。応答配置を無効から有効へ移す
  ときだけ再配置する状態遷移、縦系配置だけに表示方式を反映する条件、タグ欄、行列寸法、描画委譲、
  操作ボタン、プレビュー方向の伝達、所有する模型・子表示の取得、全アイコンの更新を固定した。
- 最初の契約構築は、対象とした15公開メソッドだけを未解決記号として診断した。判断を製品全体から
  分離して試験へ直接接続し、具象効果は試験内の決定的な記録処理へ置き換えた。具象効果対象の最初の
  コンパイルで資源型が必要とする国際化ヘッダーの直接依存不足を検出し、対象へ明示した。
- 表示判断と具象効果を各1工程・3入力、記述子、構築、資源種別と直接集約する試験を11工程・24入力に
  収めた。製品`kritaresourceui`は303工程・637入力から305工程・641入力になった。表示判断、具象効果、
  残存操作の対象コンパイルと対象CTestのmacOS単発実行、20回反復は成功した。公開面は1,547ヘッダー、
  29,979 API、対応済み4,490件、未対応25,489件になり、同ヘッダーの未対応は13 APIになった。製品
  `kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源項目選択・通知 public API契約と模型効果分離で完了した作業

- `libs/resources/ui/KisResourceItemChooser.cpp`にあった現在資源取得、資源個体・名前・行による選択、
  タイル・グレースケール指定、模型索引からの活性化、再クリック通知を、新規
  `libs/resources/ui/KisResourceItemChooserSelection.cpp`へ移した。具体的な絞込み模型走査、一覧選択、
  資源名・有効性取得を新規`libs/resources/ui/KisResourceItemChooserSelectionSource.cpp`へ移し、内部境界を
  新規`libs/resources/ui/KisResourceItemChooserSelectionSource_p.h`に置いた。開始ファイルにはプレビュー
  画素生成、除去可否、同期、入力、取込・除去、応答配置の具体操作が残り、製品は新しい選択判断対象と
  模型効果対象を各1回集約する。
- `libs/resources/ui/KisResourceItemChooser.h`の選択・通知8 APIを、既存
  `libs/resources/ui/tests/TestResourceUiContract.cpp`の5試験へ対応付けた。表示中と絞込みで隠れた資源の
  取得差、資源個体・名前・行の索引解決、無効索引での資源記憶、有効資源だけの選択通知、通知中の
  再入抑止、表示中資源の再クリック通知、次のプレビューへ反映するタイル・グレースケール状態を固定した。
- 実装前のリンクは対象8公開メソッドと、選択・クリックを駆動する二つの内部スロットだけを未解決記号
  として診断した。選択判断を製品全体から分離して試験へ直接接続し、模型、一覧、資源値の具象効果は
  試験内の決定的な索引と資源同一性記録へ置き換えた。
- 選択判断、模型効果、残存操作を各1工程・3入力、記述子、構築、表示判断、資源種別と直接集約する試験を
  12工程・26入力に収めた。製品`kritaresourceui`は305工程・641入力から307工程・645入力になった。
  三つの対象コンパイルと対象CTestのmacOS単発実行、20回反復は成功した。公開面は1,547ヘッダー、
  29,979 API、対応済み4,498件、未対応25,481件になり、同ヘッダーの未対応は5 APIになった。製品
  `kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源項目選択の同期・入力 public API契約と効果分離で完了した作業

- `libs/resources/ui/KisResourceItemChooser.cpp`にあった寸法同期の有効化、共有基準長変更、Ctrl+ホイール
  入力、運動スクロール状態処理を、新規`libs/resources/ui/KisResourceItemChooserInput.cpp`へ移した。
  共有同期個体への接続・切断・値取得更新、一覧寸法、カーソルの具体操作を新規
  `libs/resources/ui/KisResourceItemChooserInputSource.cpp`へ移し、内部境界を新規
  `libs/resources/ui/KisResourceItemChooserInputSource_p.h`に置いた。また開始ファイルの三つの応答配置
  経路に重複していた一覧表示方式の適用と通知を、既存
  `libs/resources/ui/KisResourceItemChooserPresentation.cpp`の単一処理へ移した。開始ファイルには取込・除去、
  プレビュー画素生成、除去可否、応答配置の具体的な組替えが残り、製品は新しい入力判断対象と具象効果
  対象を各1回集約する。
- `libs/resources/ui/KisResourceItemChooser.h`の同期・入力・表示方式通知4 APIを、既存
  `libs/resources/ui/tests/TestResourceUiContract.cpp`の4試験へ対応付けた。同期開始時の共有基準長適用、
  重複接続の抑止、解除後の変更無視、Ctrl付きホイール120単位による基準長10増加と入力消費、修飾なし・
  同期解除後の非消費、スクロール状態のカーソル伝達、一覧へ適用した表示方式と通知値の一致を固定した。
- 実装前のリンクは対象3公開操作、表示方式通知を通す新しい内部処理、共有基準長の内部スロットだけを
  未解決記号として診断した。同期と入力の判断を製品全体から分離して試験へ直接接続し、共有同期個体、
  一覧、カーソルの具象効果は試験内の決定的な値記録へ置き換えた。
- 入力判断、具象効果、表示判断、残存操作を各1工程・3入力、記述子、構築、選択判断、資源種別と直接
  集約する試験を13工程・28入力に収めた。製品`kritaresourceui`は307工程・645入力から309工程・649入力に
  なった。四つの対象コンパイルと対象CTestのmacOS単発実行、20回反復は成功した。公開面は1,547
  ヘッダー、29,979 API、対応済み4,502件、未対応25,477件になり、同ヘッダーの未対応は1 APIになった。
  製品`kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 資源項目選択の取込・除去 public API契約と外部効果分離で完了した作業

- `libs/resources/ui/KisResourceItemChooser.cpp`に残っていた取込・除去ボタン分岐を、新規
  `libs/resources/ui/KisResourceItemChooserButton.cpp`へ移した。MIME型取得、ファイル選択、可読性検査、
  資源登録、絞込み模型の索引取得・無効化・並替えを新規
  `libs/resources/ui/KisResourceItemChooserButtonSource.cpp`へ移し、内部境界を新規
  `libs/resources/ui/KisResourceItemChooserButtonSource_p.h`に置いた。開始ファイルにはプレビュー画素生成、
  除去可否、応答配置と表示事象の具体操作が残り、製品は新しいボタン判断対象と外部効果対象を各1回
  集約する。
- `libs/resources/ui/KisResourceItemChooser.h`の最後の1 APIを、既存
  `libs/resources/ui/tests/TestResourceUiContract.cpp`の取込・除去2試験へ対応付けた。資源種別に対応する
  MIME型、不可読ファイルの除外、置換で消えた選択の新資源への復元、模型が追跡した選択のプレビュー
  更新、取込後の並替え、除去資源の無効化、前行の選択・活性化、先頭行での行0維持を固定した。
- 実装前のリンクは`slotButtonClicked(int)`だけを未解決記号として診断した。ボタン判断を製品全体から
  分離して既存の選択判断とともに試験へ直接接続し、ファイル、登録、模型の具象効果は試験内の決定的な
  ファイル列、可読性、資源同一性、模型索引へ置き換えた。具象効果の最初のコンパイルで資源読込登録簿が
  必要とする全体補助ヘッダーの直接include経路を検出し、同対象へ明示した。
- ボタン判断、具象効果、残存操作を各1工程・3入力、記述子、構築、表示・選択・入力判断、資源種別と
  直接集約する試験を14工程・30入力に収めた。製品`kritaresourceui`は309工程・649入力から311工程・
  653入力になった。三つの対象コンパイルと対象CTestのmacOS単発実行、20回反復は成功した。公開面は
  1,547ヘッダー、29,979 API、対応済み4,503件、未対応25,476件になり、
  `libs/resources/ui/KisResourceItemChooser.h`と資源UI責務の公開APIは全件対応済みになった。製品
  `kritaresourceui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ASL値捕捉・購読配送 public API契約と構築所有分離で完了した作業

- `libs/psdutils/asl/kis_asl_object_catcher.cpp`と
  `libs/psdutils/asl/kis_asl_callback_object_catcher.cpp`は、製品`kritapsdutils`の直接ソース所有から、
  同じファイル位置の新規`kritaaslobjectcatcherobjects`へ移した。製品はこの生成オブジェクトを1回
  集約する。基底の未処理値診断と派生の購読配送を一つのASL値捕捉責務として局所構築できる。
- 両ヘッダーの72 APIを、新規
  `libs/psdutils/tests/KisAslObjectCatcherContractTest.cpp`の6試験へ対応付けた。基底捕捉器の配列状態付き
  未処理診断、数値・文字列・真偽値・生データ、色・点・変換・矩形、曲線・模様・グラデーション・
  新規スタイル開始の値保持、列挙型識別子と単位の一致条件、同じパスの再購読による置換、破棄時の
  コールバック所有解放を固定した。
- 局所対象の最初のコンパイルは`KoColor.h`が直接必要とする半精度数値ヘッダーの不足を診断し、
  OpenEXRの既存依存を対象へ明示した。試験の最初のリンクは、製品全体が暗黙に供給していたログ分類、
  色の診断表示、資源の表示用取得関数だけを未解決参照として示したため、試験内の値境界で満たし、
  `kritapigment`、`kritaresources`、`kritaglobal`を試験へ接続していない。
- ASL値捕捉対象を2工程・5入力、新規試験を6工程・19入力に収めた。製品`kritapsdutils`の閉包は
  561工程・1,152入力であり、対象CTestのmacOS単発実行と20回反復、公開API契約検査は成功した。
  公開面は1,547ヘッダー、29,979 API、対応済み4,575件、未対応25,404件になり、両ASL値捕捉
  ヘッダーのpublic APIは全件対応済みになった。製品`kritapsdutils`のリンク、Linux、全ネイティブ
  検証は実行していない。

## R2-G19b アルファ色空間 public API契約と構築所有分離で完了した作業

- `libs/pigment/colorspaces/KoAlphaColorSpace.cpp`は、製品`kritapigment`の直接ソース所有から、同じ
  ファイル位置の新規`kritaalphacolorspaceobjects`へ移した。製品はこの生成オブジェクトを1回集約し、
  アルファ色空間の実体生成を1工程・3入力で局所構築できる。型、別名、チャネル識別子だけを検査する
  `KoAlphaColorSpaceTypeContractTest`は製品へ接続せず6工程・19入力に収めた。
- `libs/pigment/colorspaces/KoSimpleColorSpaceFactory.h`にインラインであったプロファイル適合判定を新規
  `libs/pigment/colorspaces/KoSimpleColorSpaceFactory.cpp`へ移し、内部`KoDummyColorProfile`型への参照を
  製品内に閉じた。アルファ色空間と生成器の明示的テンプレート実体は新規
  `libs/pigment/kritapigment_export_instance.h`のプラットフォーム別規則で共有ライブラリから公開し、
  公開生成器基底`KoSimpleColorSpaceFactory`は利用側から構築できる境界になった。
- `libs/pigment/colorspaces/KoAlphaColorSpace.h`の52 APIと、境界修正で公開面へ加わった
  `libs/pigment/colorspaces/KoSimpleColorSpaceFactory.h`の14 APIを、新規
  `libs/pigment/tests/KoAlphaColorSpaceTypeContractTest.cpp`と
  `libs/pigment/tests/KoAlphaColorSpaceContractTest.cpp`の10試験へ対応付けた。画素特性・具象型の対応、
  深度別識別子、構築情報、複製、QColor・表示画像・LabA16・RGBA16変換、文字列表現、畳込み、未定義
  操作の中立結果、生成器情報、GrayA四深度との双方向変換経路を固定した。
- 製品へ接続した最初の試験リンクは、macOS共有ライブラリがアルファ色空間テンプレートの構築、破棄、
  複製を公開していないことを診断した。生成器の構築は、公開基底のインライン判定が内部プロファイル型の
  型情報を利用側へ漏らしていたことを追加で診断した。書き出し規則と判定実装の移動後は製品経由で
  構築できる。画素差契約の最初の実行は、引数順序を逆にした8ビット差が符号なし減算の縮退により
  64ではなく192になる不具合を検出し、比較後の非負差を尺度変換する実装へ修正した。
- 具象色空間は`KoColorSpace`の仮想関数表、合成演算、変換経路、プロファイルを実所有者から利用するため、
  実体契約は`kritapigment`へ接続する。製品閉包は321工程・672入力から、単純生成器判定の実装単位追加に
  より322工程・674入力、実体契約は326工程・681入力である。この具体所有者を分解せず値変換と生成器の
  実挙動を検査する最小の残存閉包として記録する。
- 公開API採取器はCtagsが`extern template class`を変数として返す誤採取を除外し、実変数の採取を維持する
  回帰試験を追加した。二つの対象構築、対象CTestのmacOS単発実行と20回反復、公開API契約検査は成功した。
  公開面は1,549ヘッダー、29,989 API、対応済み4,641件、未対応25,348件になり、両対象ヘッダーの
  public APIは全件対応済みになった。Linuxと全ネイティブ検証は実行していない。

## R2-G19b 合成方法一覧模型 public API契約と外部効果分離で完了した作業

- `libs/tools/ui/kis_categories_mapper.cpp`と`libs/tools/ui/kis_categorized_list_model.cpp`は、製品
  `kritatoolsui`の直接ソース所有から、同じファイル位置の新規
  `kritatoolsuicategorizedmodelobjects`へ移した。`libs/tools/ui/kis_composite_ops_model.cc`は、同じ
  製品の直接ソース所有から、同じファイル位置の新規`kritatoolsuicompositeopsmodelobjects`へ移した。
  製品は分類模型と合成方法模型の生成オブジェクトを各1回集約する。
- `libs/tools/ui/kis_composite_ops_model.cc`にあった合成方法登録簿の列挙・照合、設定の読書き、警告
  アイコン取得を、新規`libs/tools/ui/kis_composite_ops_model_source.cpp`へ移し、内部境界を新規
  `libs/tools/ui/kis_composite_ops_model_source_p.h`に置いた。具体効果ファイルも新規
  `kritatoolsuicompositeopsmodelsourceobjects`が1回生成し、製品の既存`kritapigment`、
  `kritawidgetutils`、設定依存の下で集約する。模型判断は登録集合、保存値、色空間別の利用可否、警告画像を
  決定的な値として受け取れる。
- `libs/tools/ui/kis_composite_ops_model.h`の18 APIを、新規
  `libs/tools/ui/tests/KisCompositeOpsModelContractTest.cpp`の7試験へ対応付けた。KoID表示変換、型関係、
  共有個体の一度だけの初期化、通常・レイヤースタイル登録集合の選択、分類展開と検査可能性、既定・保存済み
  お気に入りの同期、色空間による有効化と警告装飾、お気に入り優先整列を固定した。
- 最初の契約リンクは、対象とした通常模型の初期化、共有個体、お気に入り、検証、データ操作と仮想関数表
  だけを未解決記号として診断し、外部ライブラリー由来の未解決参照を含まなかった。模型対象を接続した後は、
  試験内の決定的な登録集合、設定値、色空間判定、警告画像だけで全経路を実行できる。
- 変更前の製品`kritatoolsui`閉包は1,124工程・2,268入力、既存設定UI試験は1,128工程・2,275入力
  だった。分類模型は4工程・9入力、模型判断と具体効果は各1工程・3入力、新規試験は10工程・22入力に
  収めた。製品閉包は1,127工程・2,274入力である。製品所属の具象ソースオブジェクトを直接指定しても
  製品の順序依存を引くため、具体効果も局所生成対象として維持する。
- 三つの局所対象コンパイル、対象CTestのmacOS単発実行と20回反復、公開API契約検査、高速検査は
  成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,659件、未対応25,330件になり、同ヘッダーの
  public APIは全件対応済みになった。製品`kritatoolsui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 描画選択肢一覧模型 public API契約と状態効果分離で完了した作業

- `libs/tools/ui/kis_paintop_options_model.cpp`は、製品`kritatoolsui`の直接ソース所有から、同じ
  ファイル位置の新規`kritatoolsuipaintopoptionsmodelobjects`へ移した。製品は模型判断の生成オブジェクトを
  1回集約する。
- `libs/tools/ui/kis_paintop_options_model.cpp`にあった`KisPaintOpOption`の名前・分類・検査可否・検査状態・
  有効状態の取得、検査状態変更、状態通知接続を、新規
  `libs/tools/ui/kis_paintop_options_model_source.cpp`へ移し、内部境界を新規
  `libs/tools/ui/kis_paintop_options_model_source_p.h`に置いた。具体効果は新規
  `kritatoolsuipaintopoptionsmodelsourceobjects`が1回生成し、製品が模型判断とともに集約する。
  `libs/tools/ui/kis_paintop_options_model.h`の`KisOptionInfo::index`は、未初期化値から無効なウィジェット
  索引`-1`へ変更し、既定構築と同一性比較を決定的にした。
- 同ヘッダーの18 APIを、新規`libs/tools/ui/tests/KisPaintOpOptionsModelContractTest.cpp`の5試験へ
  対応付けた。選択肢情報の既定値・複写・表示変換・同一性、五分類の表示名、項目追加時の分類展開と
  検査可否・検査状態・有効状態、模型から選択肢への検査状態伝達、選択肢から模型への状態同期、明示的な
  変更通知を固定した。
- 同一性比較は、異なる選択肢名を持つ値が分類・検査状態の比較を迂回して同一になる条件分岐の欠陥を
  検出した。索引が一致した後にnull性、選択肢名、分類、検査可否、検査状態を順に比較する実装へ修正し、
  表示ラベルと有効状態は一覧内の同一性から独立する契約を維持した。
- 局所対象の最初のコンパイルは、公開選択肢ヘッダーが推移的に必要とするEigenとKI18nを製品が暗黙供給
  していたことを診断したため、両外部依存を対象へ明示した。最初の契約リンクは値同一性、模型構築、
  分類名、追加、データ、検査状態、明示通知と仮想関数表だけを未解決記号として示し、画像・資源製品の
  記号を含まなかった。
- 変更前の製品`kritatoolsui`閉包は1,127工程・2,274入力、既存設定UI試験は1,131工程・2,281入力
  だった。模型判断と具体効果は各1工程・3入力、新規試験は9工程・25入力に収めた。製品閉包は
  1,128工程・2,276入力である。両局所対象コンパイル、対象CTestのmacOS単発実行と20回反復、公開API
  契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,677件、未対応
  25,312件になり、同ヘッダーのpublic APIは全件対応済みになった。製品`kritatoolsui`のリンク、Linux、
  全ネイティブ検証は実行していない。

## R2-G19b 描画方式一覧模型 public API契約と状態取得分離で完了した作業

- `libs/tools/ui/kis_paint_ops_model.cpp`は、製品`kritatoolsui`の直接ソース所有から、同じファイル位置の
  新規`kritatoolsuipaintopsmodelobjects`へ移した。製品は分類模型と描画方式一覧模型の生成オブジェクトを
  各1回集約する。
- `libs/tools/ui/kis_paint_ops_model.cpp`にあった`KisPaintOpFactory`の識別子、名前、分類、アイコン、優先度の
  取得と安定分類の取得を、新規`libs/tools/ui/kis_paint_ops_model_source.cpp`へ移し、内部境界を新規
  `libs/tools/ui/kis_paint_ops_model_source_p.h`に置いた。具体的な状態取得元は新規
  `kritatoolsuipaintopsmodelsourceobjects`が1回生成し、製品が模型本体とともに集約する。
- `libs/tools/ui/kis_paint_ops_model.h`にあった整列模型の構築、一覧充足、比較処理は
  `libs/tools/ui/kis_paint_ops_model.cpp`へ移した。公開ヘッダーは描画方式工場の実装ヘッダーに代えて前方宣言を
  持ち、公開値に必要な`QIcon`だけを明示的に含む。
- 同ヘッダーの20 APIを、新規`libs/tools/ui/tests/KisPaintOpsModelContractTest.cpp`の4試験へ対応付けた。
  描画方式情報の決定的な既定値・完全値・識別子同一性・名前変換、工場状態からの分類済み行構築と全分類展開、
  表示名・アイコン・整列キー、安定分類優先と分類内優先度順を固定した。
- 最初の契約リンクは、通常模型と整列模型の構築、`fill`、`data`だけを未解決記号として診断し、画像・資源・
  プラグイン製品の記号を含まなかった。試験は決定的な工場状態値だけを模型本体へ渡し、具体的な描画方式工場を
  連結せずに全契約を実行する。
- 変更前の製品`kritatoolsui`閉包は1,128工程・2,276入力、既存設定UI試験は1,132工程・2,283入力
  だった。模型判断と具体状態取得は各1工程・3入力、新規試験は9工程・19入力に収めた。製品閉包は
  1,129工程・2,278入力である。両局所対象コンパイル、対象CTestのmacOS単発実行と20回反復、公開API
  契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,697件、未対応25,292件になり、
  同ヘッダーのpublic APIは全件対応済みになった。製品`kritatoolsui`のリンク、Linux、全ネイティブ検証は
  実行していない。

## R2-G19b 選択操作パネル public API契約と外部効果分離で完了した作業

- `libs/ui/selection/kis_selection_actions_panel.cpp`に混在していた表示可否、配置、入力事象、描画順の判断は同ファイルに
  残し、表示管理、選択管理、設定、設定画面、アイコン、色変換、具体ボタンとハンドルへの接続を新規
  `libs/ui/selection/kis_selection_actions_panel_source.cpp`へ移した。両者の内部境界は新規
  `libs/ui/selection/kis_selection_actions_panel_source_p.h`が所有する。
- `libs/ui/CMakeLists.txt`の製品直接ソース`libs/ui/selection/kis_selection_actions_panel.cpp`は、新規
  `kritauiselectionactionspanelobjects`と`kritauiselectionactionspanelsourceobjects`へ移り、製品
  `kritaapplicationui`が両生成物を各1回集約する。無効化操作はパネルを親に持ち、従来の所有者なし生成による
  寿命漏れを解消した。
- `libs/ui/selection/kis_selection_actions_panel.h`は、公開値に必要な設定型とQt基底だけを直接含み、キャンバス装飾、
  入力補助、描画補助、具体ボタンとハンドルの実装ヘッダーを内部実装側へ移した。
- 同ヘッダーの16 APIを、新規`libs/ui/tests/KisSelectionActionsPanelContractTest.cpp`の5試験へ対応付けた。公開型、
  既定構築禁止、構築と8操作登録、表示・有効・選択状態、方向・ハンドル・表示領域交換、描画条件、左ボタンによる
  移動と移動量保存を固定した。`KisSelectionActionsPanelSP`は侵入型参照数基底を持たないパネルを指して実体を保持
  できず、利用元もないため、既知不具合として後続の公開面整理対象に分類した。
- 最初の契約リンクは、パネルのコンストラクタ、デストラクタ、表示、有効化、方向、ハンドル、表示領域交換、描画、
  入力事象、位置更新だけを未解決記号として診断し、文書・画像・操作管理製品の記号を含まなかった。試験は検証用の
  選択状態、設定値、表示領域、操作、描画先、移動量だけをパネル判断へ渡す。
- 変更前の製品`kritaapplicationui`閉包は1,790工程・3,580入力、既存公開ヘッダー試験は1,842工程・3,681入力、
  既存選択装飾試験は1,795工程・3,589入力だった。パネル判断は3工程・7入力、具体効果は1工程・3入力、新規試験は
  7工程・21入力に収めた。製品閉包は1,793工程・3,586入力である。両局所対象コンパイル、対象CTestのmacOS単発
  実行と20回反復、公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,713件、
  未対応25,276件になり、同ヘッダーのpublic APIは全件対応済みになった。製品`kritaapplicationui`のリンク、Linux、
  全ネイティブ検証は実行していない。

## R2-G19b 複数ノード属性 public API契約とノード効果分離で完了した作業

- `libs/ui/nodes/kis_multinode_property.h`にインラインで混在していた、合成方法、名前、色ラベル、
  不透明度、レイヤー属性、色空間、チャンネルフラグの具体ノード取得・変更を、新規
  `libs/ui/nodes/kis_multinode_property_source.cpp`へ移した。公開アダプターの値集約、無視状態、UI接続、
  undo判断は既存ヘッダーと`libs/ui/nodes/kis_multinode_property.cpp`が所有し、保護されたノード接続境界を
  通して同じ具体効果を利用する。
- 製品`kritaapplicationui`が直接所有していた`libs/ui/nodes/kis_multinode_property.cpp`は、同じファイル位置の
  新規`kritauimultinodepropertyobjects`へ移した。新しい具体ノード効果は
  `kritauimultinodepropertysourceobjects`が所有し、製品は両生成オブジェクトを各1回集約する。
  `libs/ui/tests/kis_multinode_property_test.h`の試験宣言は
  `libs/ui/tests/kis_multinode_property_test.cpp`へ統合し、旧試験ヘッダーを除去した。
- `libs/ui/nodes/kis_multinode_property.h`の109 APIを、専用
  `KisMultinodePropertyContractTest`の7試験へ対応付けた。基底と四つの単純属性変換、真偽値レイヤー属性の
  集約と変更、同じ色空間に限定したチャンネル属性、複数値と単一値の初期状態、無視状態による保存値の
  復元と現在値の適用、値変更通知、チェックボックスと入力による有効化、三状態真偽値接続、ノード別旧値と
  共通新値のundo・redoを固定した。同順位項目間の順序は未規定とし、異なる表示優先度の前後関係を検査する。
- 実装接続前のリンクは複数ノード属性の接続器とインターフェースだけを未解決記号として診断した。Qtの
  `KisBaseNode::Property`診断表示が要求した画像製品への参照は、値の等価性を直接検査して除去した。
  試験は決定的なノード状態を保護された接続境界へ渡し、画像、文書、UI製品を連結せずに全契約を実行する。
- 変更前の製品`kritaapplicationui`閉包は1,793工程・3,586入力、既存試験は1,845工程・3,687入力だった。
  属性判断は3工程・7入力、具体ノード効果は1工程・3入力、専用試験は253工程・541入力に収めた。残る
  `kritapaintingundo`直接依存は公開`KUndo2Command`の構築、破棄、仮想undo・redoを実行するために必要である。
  製品閉包は1,796工程・3,592入力になった。
- 両局所対象コンパイル、対象CTestのmacOS単発実行と20回反復、公開API契約検査、高速検査は成功した。公開面は
  1,549ヘッダー、29,989 API、対応済み4,822件、未対応25,167件になり、同ヘッダーのpublic APIは
  全件対応済みになった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード管理の画像状態 public API契約と具体画像効果分離で完了した作業

- 製品`kritaapplicationui`が直接所有していた`libs/ui/nodes/KisNodeManagerImageState.cpp`は、同じ
  ファイル位置の新規`kritauinodemanagerimagestateobjects`へ移した。同ファイルにあった画像グラフ、
  レイヤー、マスク、選択への具体アクセスは、新規
  `libs/ui/nodes/KisNodeManagerImageStateSource.cpp`の
  `kritauinodemanagerimagestatesourceobjects`へ移した。製品は判断と具体アクセスの生成オブジェクトを
  各1回集約する。
- `libs/ui/nodes/kis_node_manager.h`のうち6 APIを、新規
  `libs/ui/tests/KisNodeManagerImageStateContractTest.cpp`の3試験へ対応付けた。削除後の近傍ノード、
  レイヤーとマスクの分類、マスクから親レイヤーへの解決、現在ノードのアニメーション、選択マスクの
  編集可能性、所有レイヤーの選択取得を固定した。画像設定更新とノード有効化操作作成は、具体画像を
  観測する後続契約へ残した。
- 実装接続前のリンクは、対象とした`nearestNodeAfterRemoval`、`layerForNode`、`maskForNode`、
  `activeNodeIsAnimated`、`activeSelectionIsEditable`、`selectionForNode`だけを未解決記号として診断し、
  製品ライブラリー由来の未解決参照を含まなかった。最初の実行は、判断側が派生ノード共有ポインターを
  保持すると具体型の参照管理まで引き込むことを診断した。判断側は基底ノードと真偽値だけで分岐し、
  レイヤー、マスク、選択への変換を具体アクセス側の最終返却へ限定した。
- 変更前の製品`kritaapplicationui`閉包は1,796工程・3,592入力、既存隔離対象
  `KisNodeManagerTest`は1,800工程・3,599入力だった。判断と具体アクセスは各1工程・3入力、新規試験は
  5工程・17入力に収めた。製品閉包は1,797工程・3,594入力である。
- 両局所対象コンパイル、対象CTestのmacOS単発実行と20回反復、公開API契約検査は成功した。公開面は
  1,549ヘッダー、29,989 API、対応済み4,828件、未対応25,161件になった。製品
  `kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード管理の状態分類 public API契約と具体ノードアクセス分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった現在ノードのレイヤー判定、型継承、編集可能性、編集可能な
  描画装置と、仮想ノード・画像全体の選択マスクの非表示判断を、新規
  `libs/ui/nodes/KisNodeManagerNodeState.cpp`へ移した。具体ノード型と状態の取得は新規
  `libs/ui/nodes/KisNodeManagerNodeStateSource.cpp`へ移し、製品`kritaapplicationui`は新規
  `kritauinodemanagernodestateobjects`と`kritauinodemanagernodestatesourceobjects`を各1回集約する。
- `libs/ui/nodes/kis_node_manager.h`のうち5 APIを、新規
  `libs/ui/tests/KisNodeManagerNodeStateContractTest.cpp`の2試験へ対応付けた。現在ノードがない場合の
  各判定、レイヤー、指定型、編集可能性、編集可能描画装置の独立した状態反映、仮想ノードの常時非表示、
  画像全体の選択を隠す場合の最上位選択マスク、階層内選択マスクの表示を固定した。
- 実装接続前のリンクは、対象とした`activeNodeIsLayer`、`activeNodeInherits`、`activeNodeIsEditable`、
  `activeNodeHasEditablePaintDevice`、`isNodeHidden`だけを未解決記号として診断し、製品ライブラリー由来の
  未解決参照を含まなかった。
- 変更前の既存隔離対象`KisNodeManagerTest`は1,801工程・3,601入力だった。判断と具体アクセスは
  各1工程・3入力、新規試験は5工程・17入力に収めた。製品`kritaapplicationui`閉包は
  1,797工程・3,594入力から1,799工程・3,598入力になった。
- 両局所対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,833件、
  未対応25,156件になった。元オブジェクトのNinja出力は429工程の順序依存を持ち、無関係な既存
  `kritaresourceui`リンクの未解決記号で停止したため、コンパイルデータベースの1命令だけで検証した。
  製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード属性変更 public API契約とundo効果分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった名前、不透明度、合成方法の変更不要判定、百分率不透明度の
  尺度変換、現在ノードへの変更適用を、新規`libs/ui/nodes/KisNodeManagerNodeChange.cpp`の
  `kritauinodemanagernodechangeobjects`へ移した。具体ノード値の取得と画像undo命令の発行は、同じ公開クラスの
  既存具体所有として`libs/ui/nodes/kis_node_manager.cpp`に残し、製品`kritaapplicationui`は判断生成物を
  1回集約する。
- `libs/ui/nodes/kis_node_manager.h`のうち5 APIを、新規
  `libs/ui/tests/KisNodeManagerNodeChangeContractTest.cpp`の3試験へ対応付けた。対象なしと同値変更の無操作、
  名前・不透明度・合成方法の差分適用、現在ノードへの不透明度と合成方法の適用を固定した。
- 最初の実行は、50%の不透明度が`int(50 * 2.55 + 0.5)`の二進浮動小数点誤差により128ではなく127になる
  既存挙動を診断した。0%は0、100%以上は255へ変換する上限制限とともに既知不具合として固定し、製品挙動は
  変更していない。
- 実装接続前のリンクは、対象とした`setNodeName`、`setNodeOpacity`、`setNodeCompositeOp`、
  `nodeOpacityChanged`、`nodeCompositeOpChanged`だけを未解決記号として診断し、具体undoや製品ライブラリー
  由来の未解決参照を含まなかった。
- 変更前の既存隔離対象`KisNodeManagerTest`は1,803工程・3,605入力だった。判断は1工程・3入力、
  新規試験は5工程・17入力に収めた。製品`kritaapplicationui`閉包は1,799工程・3,598入力から
  1,800工程・3,600入力になった。
- 判断対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,838件、
  未対応25,151件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード変更可否 public API契約と画面通知分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった複数ノードから最初の編集不可ノードを選ぶ判断、単一ノードの
  委譲、警告文の選択を、新規`libs/ui/nodes/KisNodeManagerModification.cpp`の
  `kritauinodemanagermodificationobjects`へ移した。具体ノード状態の取得と画面通知は、同じ公開クラスの
  既存具体所有として`libs/ui/nodes/kis_node_manager.cpp`に残し、製品`kritaapplicationui`は判断生成物を
  1回集約する。
- `libs/ui/nodes/kis_node_manager.h`のうち2 APIを、新規
  `libs/ui/tests/KisNodeManagerModificationContractTest.cpp`の2試験へ対応付けた。空集合と全編集可能集合、
  最初の編集不可ノードでの停止と名前付き警告、単一ノード向け一般警告、警告抑止を固定した。
- 実装接続前のリンクは`canModifyLayers`と`canModifyLayer`だけを未解決記号として診断し、具体ノードや
  製品ライブラリー由来の未解決参照を含まなかった。
- 変更前の既存隔離対象`KisNodeManagerTest`は1,804工程・3,607入力だった。判断は1工程・3入力、
  新規試験は5工程・17入力に収めた。製品`kritaapplicationui`閉包は1,800工程・3,600入力から
  1,801工程・3,602入力になった。
- 判断対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,840件、
  未対応25,149件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード移動可否 public API契約で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった親ノードの編集可能性と最初の移動不可親を選ぶ判断、
  単一ノードの委譲、警告文の選択を、既存`libs/ui/nodes/KisNodeManagerModification.cpp`へ移した。
  具体的な親ノード、編集可能性、名前、画面通知は既存の保護境界を拡張して利用し、新しいCMake対象や
  製品依存を追加していない。
- `libs/ui/nodes/kis_node_manager.h`のうち2 APIを、既存
  `libs/ui/tests/KisNodeManagerModificationContractTest.cpp`の2試験へ対応付けた。親なしノード、編集可能な親、
  最初の編集不可親での停止と名前付き警告、単一ノードの委譲、警告抑止を固定した。
- 実装接続前のリンクは`canMoveLayers`と`canMoveLayer`だけを未解決記号として診断した。既存判断対象は
  1工程・3入力、拡張後の試験も5工程・17入力を維持し、製品`kritaapplicationui`閉包は
  1,801工程・3,602入力のままである。
- 対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,842件、
  未対応25,147件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 現在描画対象 public API契約と取得優先順位分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった現在レイヤー、描画装置、合成用色空間の取得実装のうち、
  マスク、レイヤー、親レイヤー、画像を選ぶ優先順位判断を、新規
  `libs/ui/nodes/KisNodeManagerActive.cpp`へ移した。レイヤー管理、マスク管理、画像への具体アクセスは
  移動元の保護境界に残し、製品`kritaapplicationui`は新規`kritauinodemanageractiveobjects`の生成物を
  1回集約する。
- `libs/ui/nodes/kis_node_manager.h`のうち3 APIを、新規
  `libs/ui/tests/KisNodeManagerActiveContractTest.cpp`の3試験へ対応付けた。現在レイヤー値の透過返却、
  現在マスクの描画装置をレイヤーの描画装置より優先する規則、マスク描画装置、親レイヤー、画像の順に
  合成用色空間を選ぶ規則を固定した。
- 実装接続前のリンクは`activeLayer`、`activePaintDevice`、`activeColorSpace`だけを未解決記号として診断し、
  具体管理や製品ライブラリー由来の未解決参照を含まなかった。判断対象の直接CMake依存はQt Core、Gui、
  Widgets、Xml、KI18n、Boost、Eigen、OpenEXRに限定し、試験は決定的な共有ポインターと色空間識別値を
  保護境界へ渡して全分岐を実行する。
- 変更前の既存隔離対象`KisNodeManagerTest`は1,805工程・3,609入力だった。判断は1工程・3入力、
  新規試験は5工程・17入力に収めた。製品`kritaapplicationui`閉包は1,801工程・3,602入力から
  1,802工程・3,604入力になった。
- 判断対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,845件、
  未対応25,144件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 現在ノード public API契約で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった現在ビューの有無と現在ノードの取得を、既存の保護された
  具体アクセス境界へ移し、公開`activeNode`の透過返却を既存`libs/ui/nodes/KisNodeManagerActive.cpp`へ
  移した。既存`kritauinodemanageractiveobjects`を拡張し、新しいCMake対象や依存を追加していない。
- `libs/ui/nodes/kis_node_manager.h`のうち1 APIを、既存
  `libs/ui/tests/KisNodeManagerActiveContractTest.cpp`の1試験へ対応付けた。現在ビューがない場合の空値と、
  現在ビューが返す現在ノードの透過返却を固定した。
- 実装接続前のリンクは`activeNode`だけを未解決記号として診断した。判断対象は1工程・3入力、拡張後の
  試験も5工程・17入力、製品`kritaapplicationui`閉包も1,802工程・3,604入力を維持した。
- 判断対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,846件、
  未対応25,143件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード管理状態取得 public API契約と取得所有分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった選択ノード一覧、選択接続器、挿入接続器、表示方式接続器の
  透過返却を、新規`libs/ui/nodes/KisNodeManagerAccessors.cpp`へ移した。`Private`が保持する一覧と
  所有ポインターへの具体アクセスは移動元の保護境界に残し、製品`kritaapplicationui`は新規
  `kritauinodemanageraccessorobjects`の生成物を1回集約する。
- `libs/ui/nodes/kis_node_manager.h`のうち4 APIを、新規
  `libs/ui/tests/KisNodeManagerAccessorsContractTest.cpp`の2試験へ対応付けた。空の選択状態、複数ノードの
  格納順序と共有所有、三つの管理下接続器について空値と同一ポインターの返却を固定した。
- 実装接続前のリンクは`selectedNodes`、`nodeSelectionAdapter`、`nodeInsertionAdapter`、
  `nodeDisplayModeAdapter`だけを未解決記号として診断した。局所対象の直接CMake依存はQt Core、Gui、
  Widgets、Xml、KI18n、Boost、Eigen、OpenEXRに限定した。
- 変更前の既存`KisNodeManagerTest`は1,806工程・3,611入力、直近の専用契約は5工程・17入力だった。
  取得対象は1工程・3入力、新規試験は5工程・17入力に収めた。製品`kritaapplicationui`閉包は
  1,802工程・3,604入力から1,803工程・3,606入力、既存試験は1,807工程・3,613入力になった。
- 取得対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,850件、
  未対応25,139件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 画像ノード効果委譲 public API契約で完了した作業

- 既存`libs/ui/nodes/KisNodeManagerImageState.cpp`が所有する画像ノード設定更新とノード有効化操作作成の
  透過委譲を、既存`libs/ui/tests/KisNodeManagerImageStateContractTest.cpp`の2試験へ対応付けた。
  空画像を含む指定画像の一度だけの委譲と、操作集合、呼出元ノード管理の同一値での引渡しを固定した。
- 最初の実行は、観測値をまだ記録しない試験境界に対して両効果の呼出回数0を診断し、既存3試験は成功した。
  具体効果を所有する`libs/ui/nodes/KisNodeManagerImageStateSource.cpp`や製品挙動は変更せず、既存の保護境界で
  呼出回数と引数を記録して緑化した。
- 判断対象と具体効果対象は各1工程・3入力、拡張後の試験は5工程・17入力、製品
  `kritaapplicationui`閉包は1,803工程・3,606入力を維持した。判断対象、具体効果対象、対象CTestの
  macOS構築、単発実行と20回反復、公開API契約検査、高速検査は成功した。
- 公開面は1,549ヘッダー、29,989 API、対応済み4,852件、未対応25,137件になった。製品
  `kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 選択ノード状態 public API契約と通知判断分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった画像からの再選択要求に対する空値除外と、選択ノード一覧の
  保存・通知順序を、新規`libs/ui/nodes/KisNodeManagerSelectionState.cpp`へ移した。`Private`の選択一覧への
  具体書込みは移動元の保護境界に残し、製品`kritaapplicationui`は新規
  `kritauinodemanagerselectionstateobjects`の生成物を1回集約する。
- `libs/ui/nodes/kis_node_manager.h`のうち3 APIを、新規
  `libs/ui/tests/KisNodeManagerSelectionStateContractTest.cpp`の2試験へ対応付けた。空集合を含む明示設定の
  状態保存、保存後の同一一覧通知、画像再選択要求での空の現在ノードと空の選択一覧の独立した除外を
  固定した。
- 実装接続前のリンクは`slotSetSelectedNodes`と`slotImageRequestNodeReselection`だけを未解決記号として
  診断し、通知シグナルや製品記号を含まなかった。局所対象の直接CMake依存はQt Core、Gui、Widgets、
  Xml、KI18n、Boost、Eigen、OpenEXRに限定した。
- 変更前の既存`KisNodeManagerTest`は1,807工程・3,613入力、直近の専用契約は5工程・17入力だった。
  選択状態対象は1工程・3入力、新規試験は5工程・17入力に収めた。製品`kritaapplicationui`閉包は
  1,803工程・3,606入力から1,804工程・3,608入力、既存試験は1,808工程・3,615入力になった。
- 選択状態対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,855件、
  未対応25,134件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード属性適用 public API契約と拒否判断分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった描画レイヤー、オニオンスキン属性、完全不透明背景の組合せによる
  属性適用可否判断を、新規`libs/ui/nodes/KisNodeManagerProperties.cpp`へ移した。具体ノード型、属性識別、
  画素状態、画面警告、自動undo適用は移動元の保護境界に残し、製品`kritaapplicationui`は新規
  `kritauinodemanagerpropertiesobjects`の生成物を1回集約する。
- `libs/ui/nodes/kis_node_manager.h`の`trySetNodeProperties`を、新規
  `libs/ui/tests/KisNodeManagerPropertiesContractTest.cpp`の1試験へ対応付けた。三条件がすべて成立するときだけ
  警告して拒否する真理値表と、その他の場合に指定ノード、空画像、属性名・状態を自動undo適用へ渡す規則を
  固定した。
- 実装接続前のリンクは`trySetNodeProperties`だけを未解決記号として診断した。最初の緑化リンクは属性一覧の
  診断比較が画像製品の出力演算子を要求したため、件数・名前・状態の個別比較へ狭めた。非nullの偽画像は
  製品側の汎用参照操作に適さないため空画像で委譲を検査し、具体画像の構築依存を追加していない。
- 変更前の既存`KisNodeManagerTest`は1,808工程・3,615入力、直近の専用契約は5工程・17入力だった。
  属性判断対象は1工程・3入力、新規試験は5工程・17入力に収めた。製品`kritaapplicationui`閉包は
  1,804工程・3,608入力から1,805工程・3,610入力、既存試験は1,809工程・3,617入力になった。
- 属性判断対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,856件、
  未対応25,133件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード属性画面 public API契約と画面選択分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`に混在していたノード種別による属性画面の選択と、現在ノードの
  一時置換・復元を、新規`libs/ui/nodes/KisNodeManagerPropertyDialog.cpp`へ移した。具体的な型判定、
  レイヤー・マスク属性画面の表示、現在ノードの取得・設定は移動元の保護境界に残し、製品
  `kritaapplicationui`は新規`kritauinodemanagerpropertydialogobjects`の生成物を1回集約する。
- `libs/ui/nodes/kis_node_manager.h`の2 APIを、新規
  `libs/ui/tests/KisNodeManagerPropertyDialogContractTest.cpp`の2試験へ対応付けた。レイヤーをマスクより
  優先する画面選択、該当種別がない場合の無操作、指定ノードへの一時変更、画面処理後の元ノード復元を
  固定した。レイヤー判断前に選択一覧を取得する冗長な内部経路を除き、観測可能な画面選択は維持した。
- 実装接続前のリンクは`nodeProperties`と`nodePropertiesIgnoreSelection`だけを未解決記号として診断した。
  変更前の既存`KisNodeManagerTest`は1,809工程・3,617入力だった。画面選択対象は1工程・3入力、新規試験は
  5工程・17入力に収めた。製品`kritaapplicationui`閉包は1,805工程・3,610入力から1,806工程・3,612入力、
  既存試験は1,810工程・3,619入力になった。
- 画面選択対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復、
  公開API契約検査、高速検査は成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,858件、
  未対応25,131件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b クローン元変更 public API契約で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあったクローン元変更画面への透過委譲を、既存
  `libs/ui/nodes/KisNodeManagerPropertyDialog.cpp`へ移した。レイヤー管理への具体接続は移動元の保護境界に
  残し、既存`kritauinodemanagerpropertydialogobjects`を拡張して新しいCMake対象や依存を追加していない。
- `libs/ui/nodes/kis_node_manager.h`の`changeCloneSource`を、既存
  `libs/ui/tests/KisNodeManagerPropertyDialogContractTest.cpp`の1試験へ対応付け、レイヤー管理効果への一度だけの
  委譲を固定した。実装接続前のリンクは`changeCloneSource`だけを未解決記号として診断した。
- 属性画面対象は1工程・3入力、拡張後の試験は5工程・17入力、製品`kritaapplicationui`閉包は
  1,806工程・3,612入力、既存`KisNodeManagerTest`は1,810工程・3,619入力を維持した。対象CTestのmacOS
  単発実行と20回反復、元の`kis_node_manager.cpp`単体のコンパイル、公開API契約検査、高速検査は成功した。
  公開面は1,549ヘッダー、29,989 API、対応済み4,859件、未対応25,130件になった。製品
  `kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 高速カラーオーバーレイ属性画面 public API契約で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった高速カラーオーバーレイ用マスクの存在判定と属性画面呼出しを、
  既存`libs/ui/nodes/KisNodeManagerPropertyDialog.cpp`へ移した。具体的な`KisLayer`判定とマスク取得は移動元の
  保護境界に残し、既存`kritauinodemanagerpropertydialogobjects`を拡張して新しいCMake対象や依存を
  追加していない。
- `libs/ui/nodes/kis_node_manager.h`の`colorOverlayMaskProperties`を、既存
  `libs/ui/tests/KisNodeManagerPropertyDialogContractTest.cpp`の1試験へ対応付けた。マスクなしの無操作と、
  マスクがある場合の一時選択、マスク属性画面表示、元ノード復元の順序を固定した。実装接続前のリンクは
  `colorOverlayMaskProperties`だけを未解決記号として診断した。
- 属性画面対象は1工程・3入力、拡張後の試験は5工程・17入力、製品`kritaapplicationui`閉包は
  1,806工程・3,612入力、既存`KisNodeManagerTest`は1,810工程・3,619入力を維持した。対象CTestのmacOS
  単発実行と20回反復は成功した。元実装の単体コンパイルで派生・基底共有ポインター間の条件演算子が曖昧に
  なる診断を検出し、空値を明示的に返す分岐へ修正して成功した。公開API契約検査と高速検査も成功した。
  公開面は1,549ヘッダー、29,989 API、対応済み4,860件、未対応25,129件になった。製品
  `kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード変更後の画面同期 public API契約と画面効果分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`に混在していた現在ノードの有無による更新判断、レイヤー、マスク、表示、
  選択、時間軸固定状態の同期順序を、新規`libs/ui/nodes/KisNodeManagerNodeUpdate.cpp`へ移した。現在ノードと
  固定状態の取得、各管理器と画面への具体効果は移動元の保護境界に残し、製品`kritaapplicationui`は新規
  `kritauinodemanagernodeupdateobjects`の生成物を1回集約する。
- `libs/ui/nodes/kis_node_manager.h`の`nodesUpdated`を、新規
  `libs/ui/tests/KisNodeManagerNodeUpdateContractTest.cpp`の1試験へ対応付けた。現在ノードなしの無操作、
  各更新効果の順序、未固定・固定の両方の時間軸状態同期を固定した。実装接続前のリンクは`nodesUpdated`だけを
  未解決記号として診断した。
- 変更前の既存`KisNodeManagerTest`は1,810工程・3,619入力、直近の専用試験は5工程・17入力だった。製品未接続の
  赤試験は4工程・14入力、判断対象は1工程・3入力、緑化後の試験は5工程・17入力に収めた。製品
  `kritaapplicationui`閉包は1,806工程・3,612入力から1,807工程・3,614入力、既存試験は1,811工程・3,621入力に
  なった。
- 判断対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復は成功した。
  公開API契約検査と高速検査も成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,861件、
  未対応25,128件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 選択ノードの時間軸固定 public API契約で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった選択ノード一覧の時間軸固定状態設定を、既存
  `libs/ui/nodes/KisNodeManagerNodeUpdate.cpp`へ移した。選択一覧の取得と各ノードへの具体設定は移動元の
  保護境界に残し、既存`kritauinodemanagernodeupdateobjects`を拡張して新しいCMake対象や依存を追加していない。
- `libs/ui/nodes/kis_node_manager.h`の`slotPinToTimeline`を、既存
  `libs/ui/tests/KisNodeManagerNodeUpdateContractTest.cpp`の1試験へ対応付けた。空選択の無操作、複数選択の
  一覧順、全ノードへの`true`と`false`の設定を固定した。実装接続前のリンクは`slotPinToTimeline`だけを
  未解決記号として診断した。
- ノード更新対象は1工程・3入力、拡張後の試験は5工程・17入力、製品`kritaapplicationui`閉包は
  1,807工程・3,614入力、既存`KisNodeManagerTest`は1,811工程・3,621入力を維持した。対象CTestのmacOS
  単発実行と20回反復、元の`kis_node_manager.cpp`単体のコンパイル、公開API契約検査、高速検査は成功した。
  公開面は1,549ヘッダー、29,989 API、対応済み4,862件、未対応25,127件になった。
  製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 兄弟ノード移動 public API契約と方向委譲分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった次・前の兄弟ノード有効化要求を、新規
  `libs/ui/nodes/KisNodeManagerNavigation.cpp`へ移した。実際の前後ノード探索への具体委譲は移動元の保護境界に
  残し、製品`kritaapplicationui`は新規`kritauinodemanageravigationobjects`の生成物を1回集約する。
- `libs/ui/nodes/kis_node_manager.h`の`activateNextSiblingNode`と`activatePreviousSiblingNode`を、新規
  `libs/ui/tests/KisNodeManagerNavigationContractTest.cpp`の1試験へ対応付けた。次・前の方向と両方で
  `siblingsOnly=true`を指定する規則を固定した。実装接続前のリンクは対象2 APIだけを未解決記号として
  診断した。
- 変更前の既存`KisNodeManagerTest`は1,811工程・3,621入力、直近の専用試験は5工程・17入力だった。製品未接続の
  赤試験は4工程・14入力、方向委譲対象は1工程・3入力、緑化後の試験は5工程・17入力に収めた。製品
  `kritaapplicationui`閉包は1,807工程・3,614入力から1,808工程・3,616入力、既存試験は1,812工程・3,623入力に
  なった。
- 方向委譲対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復は成功した。
  公開API契約検査と高速検査も成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,864件、
  未対応25,125件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 直前ノード切替え public API契約で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった直前ノードと親の有無による切替え判断を、既存
  `libs/ui/nodes/KisNodeManagerNavigation.cpp`へ移した。直前ノードと親状態の取得、非UI有効化への具体効果は
  移動元の保護境界に残し、既存`kritauinodemanageravigationobjects`を拡張して新しいCMake対象や依存を
  追加していない。
- `libs/ui/nodes/kis_node_manager.h`の`switchToPreviouslyActiveNode`を、既存
  `libs/ui/tests/KisNodeManagerNavigationContractTest.cpp`の1試験へ対応付けた。直前ノードなし、親なしの無操作と、
  親がある直前ノードの一度だけの有効化を固定した。実装接続前のリンクは`switchToPreviouslyActiveNode`だけを
  未解決記号として診断した。
- ナビゲーション対象は1工程・3入力、拡張後の試験は5工程・17入力、製品`kritaapplicationui`閉包は
  1,808工程・3,616入力、既存`KisNodeManagerTest`は1,812工程・3,623入力を維持した。対象CTestのmacOS
  単発実行と20回反復、元の`kis_node_manager.cpp`単体のコンパイル、公開API契約検査、高速検査は成功した。
  公開面は1,549ヘッダー、29,989 API、対応済み4,865件、未対応25,124件になった。
  製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b 前後ノード探索 public API契約と階層判断分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった前後ノードの階層探索、兄弟限定、非表示飛越し、ルート除外を、
  既存`libs/ui/nodes/KisNodeManagerNavigation.cpp`へ移した。現在ノード、隣接・親子関係、表示状態の具体取得と
  非UI有効化効果は移動元の保護境界に残し、既存`kritauinodemanageravigationobjects`を拡張して新しいCMake
  対象や依存を追加していない。
- `libs/ui/nodes/kis_node_manager.h`の`activateNextNode`と`activatePreviousNode`を、既存
  `libs/ui/tests/KisNodeManagerNavigationContractTest.cpp`の2試験へ対応付けた。現在ノードなし、次方向の入れ子への
  降下と親への退出、前方向の末子への進入と親の前兄弟への退出、兄弟限定、両方向の非表示飛越し、ルート
  除外を固定した。実装接続前のリンクは対象2 APIだけを未解決記号として診断した。
- ナビゲーション対象は1工程・3入力、拡張後の試験は5工程・17入力、製品`kritaapplicationui`閉包は
  1,808工程・3,616入力、既存`KisNodeManagerTest`は1,812工程・3,623入力を維持した。対象CTestのmacOS
  単発実行と20回反復、元の`kis_node_manager.cpp`単体のコンパイル、公開API契約検査、高速検査は成功した。
  公開面は1,549ヘッダー、29,989 API、対応済み4,867件、未対応25,122件になった。
  製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード前後順序変更 public API契約と実行判断分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった選択ノードの移動可否判断と前後順序変更の接続から、判断部分を
  新規`libs/ui/nodes/KisNodeManagerOrdering.cpp`へ移した。選択一覧と現在ノードの取得、移動可否の具体判定、
  undo名と一括順序変更効果は移動元の保護境界に残し、製品`kritaapplicationui`は新規
  `kritauinodemanagerorderingobjects`の生成物を1回集約する。
- `libs/ui/nodes/kis_node_manager.h`の`raiseNode`と`lowerNode`を、新規
  `libs/ui/tests/KisNodeManagerOrderingContractTest.cpp`の1試験へ対応付けた。移動不可時の無作用と、移動可能時に
  検査後の選択一覧を再取得し、現在ノードとともに各方向の一括順序変更へ渡す規則を固定した。実装接続前の
  リンクは対象2 APIだけを未解決記号として診断した。
- 変更前の既存`KisNodeManagerTest`は1,812工程・3,623入力、直近の専用試験は5工程・17入力だった。製品未接続の
  赤試験は4工程・14入力、順序判断対象は1工程・3入力、緑化後の試験は5工程・17入力に収めた。製品
  `kritaapplicationui`閉包は1,808工程・3,616入力から1,809工程・3,618入力、既存試験は1,813工程・3,625入力に
  なった。
- 順序判断対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復は成功した。
  公開API契約検査と高速検査も成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,869件、
  未対応25,120件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## R2-G19b ノード削除 public API契約と実行判断分離で完了した作業

- `libs/ui/nodes/kis_node_manager.cpp`にあった単一指定と現在選択によるノード削除判断を、新規
  `libs/ui/nodes/KisNodeManagerRemoval.cpp`へ移した。具体的な親取得、変更可否判定、現在ノード取得、undo名と
  一括削除効果は移動元の保護境界に残し、製品`kritaapplicationui`は新規
  `kritauinodemanagerremovalobjects`の生成物を1回集約する。
- `libs/ui/nodes/kis_node_manager.h`の`removeSingleNode`と`removeNode`を、新規
  `libs/ui/tests/KisNodeManagerRemovalContractTest.cpp`の2試験へ対応付けた。空値・親なし・変更不可の拒否、単一
  ノード一覧への変換、現在選択一覧の取得、許可された一覧と現在ノードによる削除効果を固定した。実装接続前の
  リンクは対象2 APIだけを未解決記号として診断した。
- 変更前の既存`KisNodeManagerTest`は1,813工程・3,625入力、直近の専用試験は5工程・17入力だった。製品未接続の
  赤試験は4工程・14入力、削除判断対象は1工程・3入力、緑化後の試験は5工程・17入力に収めた。製品
  `kritaapplicationui`閉包は1,809工程・3,618入力から1,810工程・3,620入力、既存試験は1,814工程・3,627入力に
  なった。
- 削除判断対象と元の`kis_node_manager.cpp`単体のコンパイル、対象CTestのmacOS単発実行と20回反復は成功した。
  公開API契約検査と高速検査も成功した。公開面は1,549ヘッダー、29,989 API、対応済み4,871件、
  未対応25,118件になった。製品`kritaapplicationui`のリンク、Linux、全ネイティブ検証は実行していない。

## 次の操作

同じ`libs/ui/nodes/kis_node_manager.h`に残る56 APIのうち、現在選択の複製を次の小単位とする。
`kis_node_manager.cpp`の`duplicateActiveNode`について、対象指定の変更なし計画、直接CMake依存、空構築閉包を
監査し、選択一覧と現在ノードを使う一括複製を具体undo効果から分けて挙動契約を追加する。

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
