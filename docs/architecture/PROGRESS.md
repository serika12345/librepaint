# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-09-19 22:23 JST
- 状態: `in_progress`
- 現在の検査段階: R2-G19c 利用者から観測できる振る舞いを守るテストへの整理
- 関連TODO: R2-G19a・R2-G19b完了、R2-G19cテスト整理、R2-G19d対象OS検証
- ブランチ: `develop`
- 目的: 宣言形状と内部構造の固定を除去し、入力、設定の保存、色変換、統計値の振る舞いを保護する。
- 完了: 共通値・色処理・ブラシ設定・入力の重複型検査と内部処理の固定を除去した。
  宣言形状専用試験と偽物だけを検証する試験を削除し、画像API集約試験を統計値試験へ縮小した。
- 次の作業: 残る画像・画面部品の宣言形状試験を、実装と利用側の意味論から整理する。
- 検証: 変更対象と直接関連する43件のCTestはすべて成功。既存実行物の依存ライブラリー欠落は下記の別課題とする。

## 現在の変更範囲

製品ソースと製品の責務境界を維持し、試験コードと所有CMake定義を整理する。
残す試験は、値、設定の読み書き、旧形式との互換性、所有と寿命、通知、変換結果を観測する。
色変換の作業用領域、一時設定オブジェクト、内部補助クラス、不要な呼び出し回数と順序の検査を除去した。
保存される設定キー、色の識別子、画素の並びに意味がある検査は継続する。

削除・置換の対応は次のとおり。

- `libs/input/ui/tests/TabletTestDialogSchemaContractTest.cpp`から
  `libs/input/ui/tests/TabletTestDialogTest.cpp`へ置き換える。
  公開アクセシビリティ情報を通じて接近・離脱ログ、Clear後の再入力、Close操作を検証する。
- `libs/input/ui/tests/KisExtendedModifiersMapperPluginInterfaceContractTest.cpp`と
  `libs/input/ui/tests/KisPopupWidgetInterfaceContractTest.cpp`を削除する。
  これらは試験内の偽物とQt・C++自体の振る舞いを検証していた。
  実際の修飾キー取得とプラグインの借用は既存の`KisExtendedModifiersMapperContractTest.cpp`で検証する。
- `libs/pigment/tests/KoColorValueSchemaContractTest.cpp`、
  `libs/pigment/tests/KoColorSpaceRegistrySchemaContractTest.cpp`、
  `libs/pigment/tests/KoColorSetSchemaContractTest.cpp`、
  `libs/pigment/tests/KoStopGradientSchemaContractTest.cpp`を削除する。
  同じディレクトリーの`TestKoColor.cpp`、`TestKoColorSpaceRegistry.cpp`、
  `TestKoColorSet.cpp`、`TestKisSwatchGroup.cpp`、`TestKoStopGradient.cpp`を動作試験の正本とする。
  色の不透明度・コピー・メタデータとグラデーション補間の観測を追加した。
- `libs/image/tests/KisImageTypesContractTest.cpp`から
  `libs/image/tests/KisImageStatisticsTest.cpp`へ置き換える。
  ヒストグラムとメモリー統計の初期値を検証し、画像API全体のsignature固定を除去する。

共通値・色処理の既存17試験とブラシ設定の既存18試験は同じ場所で整理した。
ブラシ設定の組み合わせは読み書きした値と失敗結果を、画面部品生成は公開factoryの結果を検証する。

## 構築範囲

TabletTestDialogの旧schema試験は4工程・8入力、近傍の`KoDialogEnumContractTest`は10工程・25入力である。
置換後は26工程・58入力で、実際の入力widget、設定、速度計算、dialogを直接構築する。
入力管理試験全体の製品接続は1,393工程・2,794入力になるため、対象に必要な依存を選択した。
Bezierサンプラー試験は実際の比較処理へ接続し、空構築閉包は6工程・13入力である。
画像統計試験は4工程で、画像API集約試験が必要としていた不要なヘッダー依存を除去した。

## 検証結果

対象構築・CTestは、Nixの評価済み環境へ入る`./scripts/run-shared-test-env`経由で実行する。

- 共通値・色処理17対象、ブラシ設定18対象、入力2対象、画像統計1対象、色・パレット5対象のCTest: 43/43成功。
- 既存の色・パレット・グラデーション5対象: 増分構築成功。追加した動作試験も関連CTestで成功。
- `ninja -C build/tdd-macos -j4 KisImageStatisticsTest`: 成功。対象CTestも成功。
- `ctest --preset tdd-macos --output-on-failure --timeout 120`
  `-R '^(libs-global-|libs-pigment-|plugins-paintops-libpaintop-)|TabletTestDialogTest$|KisExtendedModifiersMapperContractTest$'`:
  183/186成功。`KisCurveOptionDataTest`と`kis_linked_pattern_manager_test`は依存欠落で起動不可。
  `TestCompositeOpInversion`は実行時指定の120秒に到達した。
  `ctest --preset tdd-macos --output-on-failure -R 'TestCompositeOpInversion$'`の通常実行は262.19秒で成功した。
  関連186件の最終結果は184件成功・既存実行物の依存欠落2件である。
- `ctest --preset tdd-macos --output-on-failure`
  `-R '(kis_stroke_test|kis_histogram_test|kis_mask_generator_test|kis_iterators_ng_test|kis_clone_layer_test|KisImageStatisticsTest)$'`:
  統計試験成功。既存5件は依存欠落で起動不可。
- TabletTestDialogは20回反復成功。入力配送を一時的に外すと接近・離脱4ケースが失敗し、
  製品ソースを復元して再検証した。`KoDialogEnumContractTest`と`KisSpeedSmootherContractTest`も成功。

- `./scripts/run-shared-test-env ./scripts/verify-quick`: 成功。自己試験39件、境界、公開面、文書、生成図を確認した。
  `nix develop .#test --command ./scripts/verify-quick`はローカルバイナリキャッシュへの接続待ちとなったため、
  評価済みの同じNix環境を利用した。
- 新規画像統計試験と書き換えた設定組み合わせ・画面部品生成・接頭辞設定の
  `clang-format --dry-run --Werror`: 成功。
- `git diff --check`: 成功。

## 残る課題と継続利用する生成物

既存の入力2件、ブラシ設定2件、画像5件の実行物は、
`@rpath/libkritaresourceui.21.dylib`が欠落しているため起動できない。
入力2件は`KisInputManagerTest`と`KisToolProxyContractTest`である。
これらの製品構成は今回の変更範囲外であり、主構築木の依存復旧後に関連試験を再実行する。

`KoDialog::showEvent()`の遅延処理が表示直後の破棄に追従しない既知不具合をTODOへ記録した。
修正は共通dialogの寿命を検証する独立した変更として扱う。

主増分構築木`build/tdd-macos`と共有コンパイラーキャッシュを継続利用する。
削除した試験の実行物・object・自動生成物は除去済み。
回収量はTabletTestDialogの旧試験が1,243,673 bytes、今回の追加整理が11,002,225 bytesである。
Qt 5、Linux、Windows、Android、実タブレット入力と全ネイティブ試験は未実施である。
