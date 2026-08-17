# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-08-17 12:33 JST
- 状態: `planned`
- 現在の検査段階: R1-G6b 描画実行境界
- 関連TODO: `docs/architecture/TODO.md`の「R1: コードパッケージングの改善」
- ブランチ: `r1-g6a-resource-ui-boundary`
- 目的: 描画特性契約を追加し、`libs/ui/tool/strokes`を`libs/painting/strokes`へ、
  `libs/command`の画像・キャンバス向け取り消し処理を`libs/painting/undo`へ、
  `libs/metadata`の画像メタデータ型を`libs/painting/metadata`へ移す。

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

## 次の操作

`libs/ui/tool/strokes`、`libs/command`、`libs/metadata`を起点として、画像状態、ストローク順序、
取り消し、投影の特性契約と公開画像ヘッダーの構築契約を先に追加する。続いて
`libs/painting/strokes`、`libs/painting/undo`、`libs/painting/metadata`へ所有権を移し、
`kritapainting`を構築する。描画から文書寿命への95件の逆方向includeと、`kritaimage`の
内部ヘッダー29件に対する593件のパッケージ外参照をゼロへ縮小する。

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
