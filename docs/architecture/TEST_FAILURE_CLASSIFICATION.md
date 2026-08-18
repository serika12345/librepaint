# ネイティブ試験失敗の一時分類

## 目的

この文書は、R1-G6d完了後のmacOSネイティブ試験で継続している失敗を、試験契約、
製品実装、依存・実行時資源契約、非同期試験の同期方法へ分類し、修正単位と起点ファイルを
確定するための一時台帳である。分類と修正を別のレビュー単位にし、期待値の変更だけで
製品不具合を覆わないために使用する。

分類対象の修正が完了し、macOSとLinuxの全ネイティブ試験が成功した時点でこの文書を削除する。
削除と同時に`docs/architecture/PROGRESS.md`を通常のR1-G6進行へ戻す。

## 観測条件

- 対象コミット: `142583963dc2da80c285154f745d2f00e090ea2f`
- macOS: arm64、macOS 26.5.2、Qt 6.11.1、LittleCMS 2.19、libtiff 4.7.2
- Linux: x86_64 NixOS、Qt 6.11.1、LittleCMS 2.19、libtiff 4.7.2
- macOS全件実行:
  `nix develop .#test --command ctest --preset tdd-macos --output-on-failure -j 1`
- macOS結果: 327件中290件成功、37件失敗
- 直前の並列全件実行: 327件中292件成功、35件失敗
- Linux比較: `/home/masato/librepaint-r1-g6b-verify`を対象コミットへ揃え、
  `QT_QPA_PLATFORM=offscreen`でmacOS失敗37対象を逐次実行した。

37件のうち35件は実行方式にかかわらず継続する。図形選択履歴と安全な外部文書再読込の
2件は、並列実行では成功し、逐次実行またはLinux対象実行では失敗する。Linuxの対象実行は、
試験対象だけを構築したため一部の実行時プラグインを欠く。その対象は比較不能として扱い、
同じ製品・試験依存が揃った対象だけを分類根拠に用いる。

## 分類基準

- **試験契約**: 現在の対応プラットフォームで成立しない前提、固定件数、または不完全な
  実行時準備を試験が要求している。修正対象は試験入力、観測方法、試験用実行環境である。
- **製品実装**: 通知順序、幾何計算、状態遷移、寿命、または入力解釈が製品契約を満たさない。
  期待値は保持し、製品コードを修正する。
- **依存・実行時資源契約**: 現在固定しているQt、LittleCMS、libtiff、生成器または
  実行時資源の結果を、製品や基準データが正しく扱えていない。先に意味上の契約を固定し、
  互換処理、資源配置または基準を更新する。
- **非同期同期**: 実処理の完了条件ではなく固定時間の待機で結果を観測している。完了通知か
  決定的な待機条件へ置き換える。

## 全対象の一次分類

| 一次分類 | 件数 | 対象機能と試験 |
| --- | ---: | --- |
| 試験契約 | 15 | 資源保管場所の初期件数、JPEG・PDF・Qt画像・ORA・PSD・QML・TGA・ブラシ・GIF・WebP・JPEG XL・RGBEの読取専用出力、画面信号の実行時準備、OCIO表示変換の資源準備 |
| 製品実装 | 6 | 図形一括変更通知、曲線内外判定、図形レイヤー更新、絵コンテモデル通知、アニメーション変形状態、SVG文字カーソル移動 |
| 依存・実行時資源契約 | 14 | SVG解析3方式、CMYK変換、色変換経路、ケージ変形、変形マスク、色プロファイル登録・PQ色空間・プロファイル生成、SeExpr画像、KRA読込・保存、TIFF読込 |
| 非同期同期 | 2 | 図形選択履歴、安全な外部文書再読込 |

KRA保存は`rec2100 PQ 203 nits`色プロファイルの実行時登録と読取専用出力の二つの原因を含む。
全件表では、製品動作を妨げるプロファイル登録を一次分類とし、読取専用出力契約にも修正対象として
含める。

## 試験契約

### 読取専用ファイルへの出力

- 起点ファイル: `sdk/tests/filestest.h`
- 利用試験: `plugins/impex/libkra/tests/kis_kra_saver_test.cpp`、
  `plugins/impex/jpeg/tests/kis_jpeg_test.cpp`、`plugins/impex/pdf/tests/KisPdfTest.cpp`、
  `plugins/impex/qimageio/tests/KisQImageIOTest.cpp`、`plugins/impex/ora/tests/KisOraTest.cpp`、
  `plugins/impex/psd/tests/kis_psd_test.cpp`、`plugins/impex/qml/tests/KisQmlTest.cpp`、
  `plugins/impex/tga/tests/KisTgaTest.cpp`、`plugins/impex/brush/tests/KisBrushTest.cpp`、
  `plugins/impex/gif/tests/KisGifTest.cpp`、`plugins/impex/webp/tests/kis_webp_test.cpp`、
  `plugins/impex/jxl/tests/kis_jpegxl_test.cpp`、`plugins/impex/rgbe/tests/kis_rgbe_test.cpp`
- 原因: 共通補助は書込可能なディレクトリー内の既存ファイルから書込ビットだけを外し、
  出力失敗を期待する。macOSでは一時ファイルからの原子的な置換が許可されるため、13形式が
  正常に保存を完了する。これは出力本体の失敗ではなく、ディレクトリー権限を含まない試験入力の
  移植性不備である。
- 修正境界: 出力先ディレクトリーを含めて拒否を再現するか、上書きと置換を区別した
  ファイルシステム接続面を用いる。成功を失敗へ変える製品側のmacOS分岐は導入しない。
- 修正状態: `sdk/tests/filestest.h`が形式ごとの一時ディレクトリーと出力ファイルから書込権限を
  除き、終了時に元の権限へ復元する。共通補助を利用する有効な17形式で、macOSとLinuxの
  読取専用出力契約が成功した。この項目は全分類対象の修正完了時に台帳とともに削除する。

### 資源保管場所の固定件数

- 起点ファイル: `libs/resources/tests/TestResourceLocator.cpp`
- 関連実装: `libs/resources/KisResourceLocator.cpp`
- 原因: 初期化後の保管場所を4件と固定しているが、現在の登録契約は5件を作成する。
  macOSとLinuxで同じ5件を観測する。
- 修正境界: 必須保管場所の識別子と属性を検査し、実装詳細である総件数を期待値から外す。

### 画面信号試験の実行時準備

- 起点ファイル: `libs/ui/tests/kis_view_signals_test.cpp`
- 関連構成: `libs/ui/tests/CMakeLists.txt`
- 原因: 主画面、表示、ツール、資源を同時に初期化する試験が、必要な実行時登録の成立を
  確認する前に画面を生成し、初期化中に空参照へ到達する。macOSとLinuxの双方で
  `initTestCase`中に同じ空番地参照を起こす。
- 修正境界: 信号接続に必要な最小の登録と資源を明示してから画面を構築し、初期化前提を
  個別の検査で診断する。

### OCIO表示変換試験の資源準備

- 起点ファイル: `plugins/dockers/lut/tests/kis_ocio_display_filter_test.cpp`
- 関連実装: `libs/resources/KisResourceLocator.cpp`
- 原因: 単純試験入口で画像と表示変換器を直接生成し、SQL駆動とメモリー保管場所を準備して
  いない。資源検索が空の保管場所を返した後、安全表明で終了する。macOSとLinuxで同じ
  準備不足を観測する。
- 修正境界: 表示変換の契約に必要な最小資源を試験入口で登録し、資源初期化の成否を
  表示変換の検査より先に確定する。

## 製品実装

### 図形一括変更通知

- 起点ファイル: `libs/flake/KoShapeBulkActionLock.cpp`
- 契約試験: `libs/flake/tests/TestDependentShapes.cpp`
- 原因: macOSで一括変更対象の従属図形が収集されず、移動した2図形の変更通知が空になる。
  Linuxでは同じ行が成功するため、複数継承された図形接続面の判定経路にコンパイラーまたは
  ABI依存がある。試験が期待する二つの通知は一括更新の意味上必要である。
- 修正境界: 具象型への実行時型判定に依存せず、一括変更接続面を所有図形から安定して取得する。

### 曲線の内外判定

- 起点ファイル: `libs/global/kis_algebra_2d.cpp`
- 契約試験: `libs/image/tests/kis_algebra_2d_test.cpp`
- 原因: ベジェ円の中心から境界へ引く複数候補線が、交点を端点と判定して次候補へ送られ、
  macOSでは最終的に交点数0となる。Linuxでは中心を内部と判定する。中心を外部へ変える根拠は
  なく、浮動小数点交点の退化を処理できない製品側の幾何計算である。
- 修正境界: 曲線を横切る半直線と頂点の重複規則を決定的にし、コンパイラーに依存しない
  境界・内部・外部の契約を追加する。

### 図形レイヤーの更新と複製

- 起点ファイル: `libs/ui/flake/kis_shape_layer.cc`
- 関連接続: `libs/ui/flake/kis_shape_layer_canvas.cpp`
- 契約試験: `libs/ui/tests/kis_shape_layer_test.cpp`
- 原因: 150の不透明度を持つ図形レイヤーの描画結果が基準画像の255と一致せず、拡大縮小、
  下方統合、複製のいずれかが環境に応じて300秒待機へ入る。macOSとLinuxで停止する試験行が
  入れ替わるため、基準画像だけでなく遅延更新と画像処理完了の状態遷移に競合がある。
- 修正境界: 不透明度をどの段階で適用するかを画素契約で固定し、遅延図形更新と画像処理の
  完了条件を一つの所有者に集約する。

### 絵コンテモデルの変更通知

- 起点ファイル: `plugins/dockers/storyboarddocker/StoryboardModel.cpp`
- 契約試験: `plugins/dockers/storyboarddocker/tests/StoryboardModelTest.cpp`
- 原因: 行追加・削除中にレイアウト変更通知を重ね、Qtのモデル検査器が進行中の変更種別の
  不一致を検出する。macOSとLinuxで同じ診断になる。
- 修正境界: 一つの構造変更を対応する開始・終了通知の組として発行し、レイアウト変更と
  行変更を同じ更新に重ねない。

### アニメーション変形の状態と寿命

- 起点ファイル: `plugins/tools/tool_transform2/KisAnimatedTransformMaskParamsHolder.cpp`
- 契約試験:
  `plugins/tools/tool_transform2/tests/test_animated_transform_parameters.cpp`
- 原因: 時刻切替、遅延キーフレーム作成、取り消しを連続すると、通常実行では空番地参照、
  デバッガー実行では完了待ちへ入る。期待値比較より前に終了するため、試験値ではなく
  非同期状態と保持対象の寿命に属する製品不具合である。
- 修正境界: 時刻切替完了、変形引数の所有、取り消し後の保持状態を明示し、各遷移を
  決定的な契約で固定する。

### SVG文字カーソルの単語移動

- 起点ファイル: `libs/flake/text/KoSvgTextShape.cpp`
- 呼出し元: `plugins/tools/svgtexttool/SvgTextCursor.cpp`
- 契約試験: `plugins/tools/svgtexttool/tests/SvgTextCursorTest.cpp`
- 原因: 単語の左右移動が文章方向だけを見て単語先頭・末尾へ写像し、縦書き方向と視覚移動を
  使用していない。右から左の文でも一つ手前へずれ、縦書きでは通常の左右移動と異なる位置へ
  到達する。Qt 6.11.1を使うmacOSとLinuxで同じ結果になる。
- 修正境界: 文章方向、書字方向、論理移動、視覚移動の組合せを明示し、単語境界の意味を
  保持したまま製品側の写像を修正する。

## 依存・実行時資源契約

### ICC実行時資源と登録順序

- 起点ファイル: `libs/pigment/KoColorSpaceRegistry.cpp`、
  `plugins/color/lcms2engine/LcmsEnginePlugin.cpp`、`krita/data/profiles/CMakeLists.txt`
- 影響試験: `libs/pigment/tests/TestColorConversionSystem.cpp`、
  `plugins/color/lcms2engine/tests/TestColorSpaceRegistry.cpp`、
  `plugins/color/lcms2engine/tests/TestLcmsRGBP2020PQColorSpace.cpp`、
  `plugins/color/lcms2engine/tests/TestProfileGeneration.cpp`、
  `plugins/impex/libkra/tests/kis_kra_saver_test.cpp`
- 原因: `p2020PQProfile()`は「Krita Rec. 2100 Perceptual Quantizer (203cd/m²)」という
  登録済み資源を名前で取得するが、`krita/data/profiles`に該当ICCはなく、PQのその場生成も
  `profileFor()`で意図的に無効化されている。色変換経路と色プロファイル別名の2試験は空値を
  検査せず参照してセグメンテーション違反になる。プロファイル生成試験ではRec.2020とRec.709が
  ICC実装として登録されない行もあり、試験入口での色管理プラグインとICC資源の成立順序が
  契約化されていない。
- 修正境界: 203 nit PQを配布資源にするか決定的に生成するかを一つに定め、名前だけが残る
  状態を解消する。色管理プラグインとICC資源をレジストリー照会前に準備し、空プロファイルを
  公開しない契約を追加する。

### LittleCMS 2.19の変換値とICCタグ検証

- 起点ファイル: `plugins/color/lcms2engine/IccColorSpaceEngine.cpp`、
  `plugins/color/lcms2engine/colorprofiles/LcmsColorProfileContainer.cpp`
- 影響試験: `libs/pigment/tests/TestKoColorSpaceAbstract.cpp`、
  `plugins/impex/libkra/tests/kis_kra_loader_test.cpp`
- 原因: CMYKからRGBへの7行はmacOSとLinuxで同じ現在値を返すが、試験は旧変換値を完全一致で
  保持している。KRA読込資料のICCにはLittleCMS 2.19が不正と診断する`cicp`タグがあり、
  原色値を復元できない。`mluc`タグの厳格化診断も色管理試験の実行時に観測する。
- 修正境界: 外部ICCの不正タグを診断付きで扱い、有効な色度情報からの復元規則を固定する。
  CMYK変換は許容差と意味上の色契約を先に追加してから現在値を採用する。

### Qt 6描画結果と画像・幾何基準

- 起点ファイル: `libs/flake/tests/TestSvgParser.cpp`、
  `libs/flake/tests/SvgParserTestingUtils.h`、`libs/image/kis_cage_transform_worker.cpp`、
  `libs/image/kis_transform_mask.cpp`
- 影響試験: SVG解析の通常・複製・往復3方式、
  `libs/image/tests/kis_cage_transform_worker_test.cpp`、
  `libs/image/tests/kis_transform_mask_test.cpp`
- 原因: SVGメッシュ勾配と画像拡縮は現在の描画結果に対して最大17階調、画像の一箇所で
  1階調ずれ、ICC取得回数は1回に対して2回となる。ケージ変形は固定画像、変形マスクは矩形の
  幅と高さが各1画素ずれる。いずれも旧描画・丸め結果を固定した基準であり、現在の依存構成に
  対する意味上の許容条件がない。
- 修正境界: 代表画素、端点、面積、変換往復などの意味上の契約を先に追加する。製品誤差が
  否定された結果だけ基準画像または許容差へ反映する。

### SeExpr生成画像

- 起点ファイル: `plugins/generators/seexpr/tests/kis_seexpr_generator_test.cpp`
- 関連実装: `plugins/generators/seexpr/generator.cpp`
- 原因: 固定スクリプトの現在出力が先頭画素から基準画像と異なる。生成器の数値結果へ依存する
  基準画像だけが契約で、生成値の範囲、決定性、折返し境界を独立に検査していない。
- 修正境界: 固定入力での決定性と主要な数値特性を先に検査し、生成器の正当性を確認してから
  基準画像を採用する。

### libtiff 4.7.2のJPEG圧縮TIFF

- 起点ファイル: `plugins/impex/tiff/tests/kis_tiff_test.cpp`
- 関連実装: `plugins/impex/tiff/kis_tiff_import.cc`
- 原因: JPEG圧縮されたストリップ形式とタイル形式の2資料だけが画像を生成できない。
  他のTIFF資料と読取専用出力は成功するため、TIFF全体ではなくlibtiffのJPEG符号器接続または
  該当資料の互換性に限定される。
- 修正境界: libtiffが返す符号器診断を保持し、対応機能の有無を試験前提として明示する。
  符号器が有効な構成では2資料の画素契約を維持する。

## 非同期同期

### 図形選択履歴

- 起点ファイル: `libs/ui/tests/kis_shape_selection_test.cpp`
- 関連実装: `libs/ui/flake/kis_shape_selection.cpp`
- 原因: 図形の削除と選択投影の再計算後を固定200ミリ秒と画像処理待機だけで観測する。
  逐次macOSでは選択矩形が更新前の値に残るが、直前の並列全件とLinux単独では成功する。
- 修正境界: 図形選択の投影更新完了通知または世代番号を待ち、履歴操作ごとの最終状態を検査する。

### 安全な外部文書再読込

- 起点ファイル: `libs/impex/ui/kis_safe_document_loader.cpp`
- 契約試験: `libs/ui/tests/KisSafeDocumentLoaderTest.cpp`
- 原因: `QFileSystemWatcher`、100ミリ秒の再接続、1秒の消失検査、10秒の消失判定、
  500ミリ秒の再読込圧縮に対し、試験は1.5秒から15秒の固定待機で信号数を観測する。
  macOSとLinuxで欠落する信号と時点が変わる。
- 修正境界: ファイル監視を注入可能な接続面へ分け、消失、再出現、内容変更を仮想時刻と
  明示イベントで進める。

## 修正と削除の順序

1. この分類だけを独立したPRとしてレビューし、対象、起点ファイル、修正境界を確定する。
2. 試験契約、依存・実行時資源契約、製品実装、非同期同期を原因単位のPRで修正する。
3. 各修正で対象試験、影響部品、macOS全件、Linux全件を順に成功させる。
4. 327件を含む両ネイティブ試験集合が成功した変更でこの文書を削除し、
   `docs/architecture/PROGRESS.md`の次の操作をR1-G6e文書寿命境界の計画へ戻す。

この順序はテスト正常化の範囲だけを定める。UIから利用事例を登録する専用移行と、
ドメイン計算からI/Oを分離する専用移行の開始判断には使用しない。
