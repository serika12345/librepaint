# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-09-19 23:54 JST
- 状態: `in_progress`
- 現在の検査段階: R2-G19c 利用者から観測できる振る舞いを守るテストへの整理
- 関連TODO: R2-G19a・R2-G19b完了、R2-G19cテスト整理、R2-G19d対象OS検証
- ブランチ: `develop`
- 開始コミット: `9f8373917d`。実行時検証を一つも持たない宣言形状専用試験251件の削除を確定済み。
- 目的: 実行時検証と混在する型特性・完全signature検査を除去し、利用者から観測できる結果を検証する部分だけを残す。
- 完了: 混在していた189件を再分類し、型特性しか検証しない91ファイルと空になった専用CMake対象を削除した。残した95ファイルから型特性検査、署名検査マクロ、空試験537件、未使用の型別名39件を除去した。`ContractTest.cpp`の`type_traits`利用は0件になった。
- 次の作業: 残った試験を構成要素ごとに読み、列挙値などの明示的な互換性契約と、状態・結果・副作用を検証する振る舞い契約を分ける。失敗時の利用者影響を説明できない試験を削除する。
- 検証: macOSのCMake再構成が成功し、CTest登録は933件になった。残存対象を含むCTest 96件が成功した。変更した試験対象はすべてコンパイル・リンクに成功した。

## 現在の変更範囲

最初の機械削除では、ファイル名が`ContractTest.cpp`で終わり、`type_traits`または型特性の
`static_assert`を含み、Qt Testの実行時検証を一つも持たない251ファイルを削除した。
次の機械整理では、混在ファイルから型特性の文だけを除去し、値、状態、所有、通知、変換結果、失敗条件を
検証する文を保持した。型特性の除去で空になった試験関数とファイル、専用CMake定義も削除した。
通常の数値変換やテンプレート処理で型特性を利用する通常試験14件は、宣言形状検査ではないため対象外とした。

`libs/resources/`、`libs/flake/`、`libs/widgets/`、`libs/image/`、`sdk/tests/`の所有対象から、
別のヘッダーが偶然提供する完全型への依存を除去する。値で公開するQt型は公開ヘッダーで完結させ、
共有ライブラリー境界を越えて利用する関数とクラスは所有ライブラリーから公開する。

`libs/image/tests/KisStandardUniformPropertiesFactoryContractTest.cpp`を削除し、
`libs/image/tests/kis_paintop_test.cpp`へ標準プロパティの利用者向け動作を集約する。
生成した寸法・不透明度・流量プロパティが設定へ値を書き戻し、設定変更通知後に最新値を読み直すことを検証する。

`libs/widgets/tests/KisWidgetConnectionStateContractTest.cpp`は、状態構造体の初期値と変換処理の検査から、
実際のQt画面部品とモデルの初期同期、双方向更新、範囲、有効状態、表示状態の反映へ置き換える。

次の試験を削除する。これらの独立した代替試験は追加せず、製品の利用側で検証する。

- `libs/image/tests/KisNodeVisitorContractTest.cpp`: 試験内visitorの仮想呼び出しと宣言形状の検査。
  製品ノードの訪問は同じディレクトリーの`kis_node_visitor_test.cpp`が扱う。
- `libs/image/tests/KisTransformMaskParamsInterfaceContractTest.cpp`: 試験内の変換設定・保持者の仮想呼び出し検査。
  変換結果は同じディレクトリーの`kis_transform_mask_test.cpp`が扱う。
- `libs/image/tests/KisPaintDeviceWriterContractTest.cpp`: 試験内writerの仮想呼び出し検査。
  製品からの出力は同じディレクトリーの`kis_paint_device_test.cpp`が扱う。
- `libs/flake/tests/KoShapeContainerModelContractTest.cpp`: 試験内modelの呼び出し履歴と宣言形状の検査。
  図形の所属・移動・変形は同じディレクトリーの`TestShapeContainer.cpp`が扱う。

- `libs/flake/tests/KoSvgTextCharacterResultValueContractTest.cpp`: 非公開の文字配置データ構造と初期値の検査。
  文字の配置・描画は同じディレクトリーの`TestSvgText.cpp`と`TestSvgTextShape.cpp`が扱う。

今回の本体修正は次の責務に分かれる。

- `libs/resources/KisResourceModelIndexResolver.h`と
  `libs/image/brushengine/kis_standard_uniform_properties_factory.h`は、利用側がリンクできる公開記号を提供する。
- `libs/flake/KoShape.h`は値で公開するQt幾何型を直接取り込み、単独で利用できる公開ヘッダーにする。
- `libs/flake/resources/`、`libs/widgets/`、`libs/image/`、`sdk/tests/`の各翻訳単位は、
  メンバー参照、値返却、共有ポインター操作に必要な完全型を所有ヘッダーから直接取り込む。

## 構築と検証

Nixの評価済み環境へ入る`./scripts/run-shared-test-env`を利用する。
`nix develop .#test`はローカルバイナリキャッシュへの接続待ちが発生している。

対象ごとの直接CMake依存と生成済みNinja定義の増分計画を確認した。
標準プロパティの検証は既に画像ライブラリーを利用する`kis_paintop_test`へ集約し、独立試験専用の
定義オブジェクトだけを使う経路を廃止した。これにより、共有ライブラリーから生成関数が公開されない不具合を
リンク時に検出し、本体の公開指定を復旧した。

実行コマンドは、評価済みNix環境内の`cmake --build --preset tdd-macos --target <対象>`、
`ctest --preset tdd-macos --output-on-failure -R <対象名の完全一致>`を用いた。
`kritaresourceui`、`kritaflake`、`kritawidgets`、`kritaimage`の全所有対象構築が成功した。
`libs-image-kis_paintop_test`は成功し、設定値の書込と通知後の再読込を確認した。
`libs-widgets-KisWidgetConnectionStateContractTest`は成功し、利用者入力とモデル更新の双方向反映を確認した。
以前共有ライブラリー欠落で起動できなかった`libs-flake-TestSvgTextShape`、
`libs-image-kis_node_visitor_test`、`libs-image-kis_paint_device_test`、
`libs-image-kis_transform_mask_test`は全件成功した。
`nm`でresource index resolverと標準プロパティ生成関数が各共有ライブラリーの外部記号として存在することを確認した。
`./scripts/run-shared-test-env ./scripts/verify-quick`は、運用検査39件、依存境界、公開ヘッダー、
プラグイン登録、文書、リンク、図の検証を含めて成功した。
型特性専用試験の削除後にも同じ高速検査が成功した。
`cmake --build --preset tdd-macos --target help`による再構成が成功し、
`ctest --preset tdd-macos -N`は933件を登録した。
混在試験の整理後に対象を含むCTest 96件がすべて成功した。残した95試験対象はコンパイル・リンクに成功した。
一括構築の依存先では、今回の差分外にある`libs/painting/strokes/move_stroke_strategy.cpp`の
`KritaUtils::filterContainer`未解決と、`libs/brush/kis_imagepipe_brush.cpp`の完全型不足が残っている。

## 残る課題と再開条件

機械的な型特性整理は完了した。次は残存試験を構成要素ごとに読み、実際の呼び出し側と永続形式から
互換性要件を確認する。列挙値や識別子の固定は保存データや外部連携の根拠がある場合だけ残し、
公開操作の結果を検証しないSchema名の試験は振る舞いへ置き換えるか削除する。

`KisResourceItemDelegateContractTest`の索引変換は試験内でresolverを再定義しているため、
実際の資源モデルが返す索引と描画結果による検証へ移す。
`KoDialog::showEvent()`の表示直後の破棄に関する既知不具合はTODOの独立項目で扱う。

主増分構築木`build/tdd-macos`と共有コンパイラーキャッシュを継続利用する。
Qt 5、Linux、Windows、Android、実タブレット入力と全ネイティブ試験は未実施である。
