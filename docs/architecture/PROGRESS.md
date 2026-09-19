# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-09-19 22:42 JST
- 状態: `in_progress`
- 現在の検査段階: R2-G19c 利用者から観測できる振る舞いを守るテストへの整理
- 関連TODO: R2-G19a・R2-G19b完了、R2-G19cテスト整理、R2-G19d対象OS検証
- ブランチ: `develop`
- 開始コミット: `0367088969`。入力・設定・色処理の最初の整理を確定済み。
- 目的: 画像・図形・SVG文字・画面状態の試験から、宣言形状と内部処理の固定を除去する。
- 完了: 型検査専用の試験関数と偽物の仮想関数呼び出し試験を除去し、公開操作の結果へ整理した。
- 次の作業: 標準ブラシ設定の生成処理と画面操作について、実際の所有者との接続を検証する。
- 検証: macOSの対象26試験が成功。関連13試験中9件が成功、4件は既存の共有ライブラリー欠落により失敗。高速検査は成功。

## 現在の変更範囲

`libs/image/tests/`、`libs/flake/tests/`、`libs/widgets/tests/`の既存試験と所有CMake定義を整理する。
型の別名、完全なsignature、継承、オーバーロードの固定を除去し、値と操作結果を維持する。
SVG/CSSの保存は文字列を用いるため、内部列挙値の連番とプロパティ識別子の連続性を固定する試験を除去する。
CSSキーワード、長さと単位、字体の機能指定、変換結果は既存の動作試験で保護する。

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

既存ファイル内の主な置換は次のとおり。

- `libs/image/tests/KisBezierGradientMeshContractTest.cpp`: 内部補間関数を呼ぶ試験から、
  公開メッシュの分割後の色、端点、パッチの四隅、コピーの独立性へ置き換える。
- `libs/image/tests/KisImageAnimSettingCommandContractTest.cpp`: 設定取得・更新関数の呼び出し履歴から、
  再実行・取り消し・結合後のフレームレートと再生範囲へ置き換える。
- `libs/image/tests/KisOptimizedByteArrayContractTest.cpp`: 領域の共有・再利用アドレスと確保回数から、
  配列内容、コピー後の独立性、必要な記憶域、確保領域の解放へ置き換える。
- `libs/image/tests/KisImageBarrierLockContractTest.cpp`: 内部呼び出し回数から、
  所有状態、読み取り専用の選択、試行結果、スコープ終了後の解放へ置き換える。

## 構築と検証

Nixの評価済み環境へ入る`./scripts/run-shared-test-env`を利用する。
`nix develop .#test`はローカルバイナリキャッシュへの接続待ちが発生している。

対象ごとの直接CMake依存と、生成済みNinja定義の増分計画を確認した。
グラデーションメッシュ試験は既存の曲線・パッチ処理を直接利用する8工程の空構築閉包へ変更した。
比較対象の共通メッシュ試験と同じ責務の構築単位を用いる。

実行コマンドは、評価済みNix環境内の`cmake --build --preset tdd-macos --target <対象>`、
`ctest --preset tdd-macos --output-on-failure -R <対象名の完全一致>`を用いた。
対象26件は全件成功。関連13件ではCSS変換・字体設定・図形操作など9件が成功した。
グラデーション補間を一時的に無効化すると分割試験が失敗し、復元後に成功することも確認した。
製品コードの差分はない。
`./scripts/run-shared-test-env ./scripts/verify-quick`は成功した。
運用検査の自己試験39件、依存境界、公開ヘッダーとプラグイン登録、文書・リンク・図の検証を含む。

## 残る課題と再開条件

標準ブラシ設定の生成処理の既存試験は内部の定義取得関数を呼んでいる。
`libs/image/tests/KisStandardUniformPropertiesFactoryContractTest.cpp`の整理は、
実際の設定所有者に接続する構築範囲と、設定との双方向同期の意味論を確認して進める。
画面状態の値変換は整理済みで、画面部品の接続処理は実際の公開操作による追加検証を要する。

既存の入力・ブラシ設定・画像の統合試験は、主構築木で
`@rpath/libkritaresourceui.21.dylib`が欠落しているため一部を起動できない。
今回の`kis_node_visitor_test`、`kis_paint_device_test`、`kis_transform_mask_test`は起動時に失敗する。
`TestSvgTextShape`も同じ欠落で矩形プラグインの読み込みに失敗し、矩形生成時に異常終了する。
CTestと同じ環境で`QT_DEBUG_PLUGINS=1`を付け、読み込み失敗を確認した。
依存を復旧した後、これら4件の統合試験を再実行する。
`KoDialog::showEvent()`の表示直後の破棄に関する既知不具合はTODOの独立項目で扱う。

削除した5試験の実行ファイル・自動生成物・専用オブジェクト約7.8 MiBを除去した。
主増分構築木`build/tdd-macos`と共有コンパイラーキャッシュを継続利用する。
Qt 5、Linux、Windows、Android、実タブレット入力と全ネイティブ試験は未実施である。
