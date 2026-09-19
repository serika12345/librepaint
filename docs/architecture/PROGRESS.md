# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-09-20 08:43 JST
- 状態: `in_progress`
- 現在の検査段階: R2-G19c 利用者から観測できる振る舞いを守るテストへの整理
- 関連TODO: R2-G19a・R2-G19b完了、R2-G19cテスト整理、R2-G19d対象OS検証
- ブランチ: `develop`
- 開始コミット: `bd08b8556f`。宣言形状テストの再発防止検査を導入済み。
- 目的: 残存するSchema試験を利用場面から判定し、根拠のない列挙値・内部型・役割番号の固定を除去する。意味論があるAPIは利用者が渡す値と観測できる結果で保護する。
- 完了: 意味論を持たないSchema試験8件を削除した。角度選択APIは列挙値の順序からスクリプト文字列変換を分離した。ガイドと格子の設定は線種から描画ペンへの変換とXML往復へ統合し、Qt 6.4以降で色を復元できなかった不具合を修正した。マウスボタンから前景・背景色への対応試験は振る舞いを表す名称へ変更した。契約試験への型特性、コンパイル時形状検査、完全署名別名の再追加を拒否し、明示的な互換性試験には利用者と維持対象の記載を要求する高速検査を追加した。
- 完了: `KisToolSelectUiBaseSchemaContractTest.cpp`は、選択ツールの利用側が列挙値の整数値へ依存せず、設定は`sampleAllLayers`などの文字列で保存され、既存の`TestToolSettingsUiContract`が設定の往復結果を検証していることを確認した。利用者向け結果を持たない専用Schema試験と、その専用CTest・広いinclude・compile definition・UI生成定義を削除した。
- 完了: PSD書出しの内部オフセット構造体を固定する`PSDLayerRecordSchemaContractTest.cpp`を削除した。PSD保存後の再読込、画素結果、透明マスクは既存の`kis_psd_test`が検証する。
- 完了: 入力プロファイルは操作種別を16進数の数値で保存するため、`KisToolInvocationActionSchemaContractTest.cpp`を`KisToolInvocationActionCompatibilityTest.cpp`へ置き換えた。保存済みのTool InvocationおよびAlternate Invocationの各modeが、設定画面で同じ操作名へ解決されることを検証する。
- 完了: 移動ストロークの内部ジョブデータを確認していた`KisFilterStrokeStrategySchemaContractTest.cpp`を、`MoveStrokeStrategyContractTest.cpp`へ置き換えた。レイヤーのドラッグ完了時の移動量と、途中取消後の位置復元を実際のストロークで検証する。
- 完了: `KisPNGConverterSchemaContractTest.cpp`のオプション構造体既定値・コピー検査を削除した。既存のPNG実行試験から、HDR画素、CICP/ICCプロファイル、旧HDRプロファイルの読込結果を通常のCTestとして実行する。
- 完了: `KisDlgImportVideoAnimationSchemaContractTest.cpp`を`KisVideoFrameImportContractTest.cpp`へ置き換えた。連番フレームと重複除去後フレームが、それぞれ連番配置とタイムスタンプ配置を選ぶ状態を検証する。
- 完了: `KisFFMpegWrapperSchemaContractTest.cpp`を`KisFFMpegWrapperContractTest.cpp`へ置き換えた。外部エンコーダーの成功・失敗について、戻り値、signal、診断、コマンドログを検証する。
- 完了: `KoFFWWSConverterSchemaContractTest.cpp`を`KoFFWWSConverterContractTest.cpp`へ置き換えた。SVG文字の一般フォント名が利用可能なフォント分類へ解決され、未知の名前を解決しないことを検証する。
- 完了: `KoFontGlyphModelSchemaContractTest.cpp`を`KoFontGlyphModelCompatibilityTest.cpp`へ置き換えた。Glyph Palette QMLが使う`openType`、`glyphLabel`、`childCount`のモデルrole名を明示的な互換性要件として検証する。
- 完了: `KoToolBaseSchemaContractTest.cpp`を削除した。ツールボックス区分は実行時に同じ定数を参照して並べ替える内部情報であり、保存形式・拡張記述子・スクリプトの互換性根拠はない。
- 完了: `KoDocumentResourceManagerSchemaContractTest.cpp`を`KoDocumentResourceManagerContractTest.cpp`へ置き換えた。図形コントローラーが使う文書解像度とキャンバス領域の読取・変更通知、および形状ハンドルの安全な最小選択範囲を検証する。
- 完了: `KoSvgTextEnumContractTest.cpp`を`KoSvgTextFontStretchContractTest.cpp`へ置き換えた。SVG/CSSの9種類の`font-stretch`キーワードについて、読込後の幅と再保存時のキーワードを検証する。
- 完了: `KoSvgTextShapeMarkupConverterSchemaContractTest.cpp`を`KoSvgTextWrappingContractTest.cpp`へ置き換えた。SVGテキストの`white-space`と`inline-size`を文書編集後にも維持し、`pre-wrap`に有効な幅がない場合は`pre`へ正規化することを検証する。
- 完了: `KoSvgTextFontSelectionValueContractTest.cpp`を削除し、既存のフォント読込試験を`KoSvgTextFontImportContractTest.cpp`へ分離した。SVGの`font-family`、幅、太さ、style、variant、装飾を解析すると、編集部品が読む解決済み文字属性へ反映されることを検証する。
- 次の作業: 残存Schema試験を利用場面から監査する。次の`KoSvgTextPropertyDataContractTest.cpp`では、試験内で再定義した内部プロパティとコピー・既定値を、文書編集とSVG保存で観測できる結果から分離する。R2-G19cの高速検査と対象CTestを再実行する。列挙値と内部識別子の固定は、保存形式や外部連携の根拠がある場合だけ保持する。
- 検証: macOSで`TestAngleSelector`と全依存の構築、ガイド・格子設定試験、色役割試験が成功した。対象試験の反復実行と`verify-quick`も成功した。
- 再発防止検証: macOSで`KisSignalCompressorContractTest`、`KisBezierPatchContractTest`、
  `KStandardActionCompatibilityTest`の構築とCTestが成功した。新しい検査を含む運用検査45件と
  `verify-quick`が成功した。

## 現在の変更範囲

最初の機械削除では、ファイル名が`ContractTest.cpp`で終わり、`type_traits`または型特性の
`static_assert`を含み、Qt Testの実行時検証を一つも持たない251ファイルを削除した。
次の機械整理では、混在ファイルから型特性の文だけを除去し、値、状態、所有、通知、変換結果、失敗条件を
検証する文を保持した。型特性の除去で空になった試験関数とファイル、専用CMake定義も削除した。
通常の数値変換やテンプレート処理で型特性を利用する通常試験14件は、宣言形状検査ではないため対象外とした。

列挙値、内部Traits、モデル役割番号だけを固定していた次のSchema試験を削除した。

- `libs/ui/tests/KisDlgCreateNewDocumentSchemaContractTest.cpp`
- `libs/flake/tests/KoSvgTextAddRemoveShapeCommandsSchemaContractTest.cpp`
- `libs/resources/storage/tests/KoStoreSchemaContractTest.cpp`
- `libs/canvas/tests/KisCoordinatesConverterSchemaContractTest.cpp`
- `libs/widgets/tests/KisPaletteModelSchemaContractTest.cpp`

`libs/widgets/tests/KisAngleSelectorSchemaContractTest.cpp`が固定していた列挙値にはスクリプト文字列との対応という
利用者向け意味論があった。`libs/libkis/AngleSelector.cpp`の変換を列挙値の順序から分離し、
`libs/libkis/tests/TestAngleSelector.cpp`で有効値の往復と無効値の無視を検証する。

ガイド線種の数値だけを固定していた`libs/ui/tests/KisGuidesConfigSchemaContractTest.cpp`は削除し、
既存の`kis_grid_config_test.cpp`で実際のペン種別、色、XML保存後の復元結果を検証する。
この試験により、Qt 6.4以降で`QColor::fromString()`の戻り値を捨てて色を復元していなかった不具合を修正した。
既定ショートカット種別のビット集合だけを固定していた`KKeySequenceWidgetSchemaContractTest.cpp`も削除した。
`KisAcsTypesSchemaContractTest.cpp`は前景・背景色の選択という利用者向け結果を検証しているため、
`KisAcsTypesTest.cpp`へ改名して保持した。

`libs/ui/tests/KisGridConfigValueContractTest.cpp`は、格子種別と線種の整数値および内部描画計算用
`TrigoCache`のメンバー配置だけを固定していたため削除した。既存の`kis_grid_config_test.cpp`で、
実線、破線、点線、非表示が実際の`QPen`へ反映され、色、格子種別とともにXML往復後も維持されることを検証する。

`scripts/architecture/check_test_contracts.py`を高速検査へ追加した。`ContractTest`で型特性、
コンパイル時形状検査、完全署名別名を使用すると失敗する。宣言形状を明示的な互換性として保護する
`CompatibilityTest`には、利用者と維持対象を示す`// Compatibility requirement:`行を必須とする。
既存のsignal compressorに残っていた列挙値の相違だけを調べる`static_assert`と、Bezier patchおよび
standard action試験に残っていた完全署名別名を除去し、新しい検査条件を既存ソースへ適用した。

`libs/ui/tests/KisToolSelectUiBaseSchemaContractTest.cpp`は、`SampleAllLayers`、
`SampleCurrentLayer`、`SampleColorLabeledLayers`の整数値だけを固定していたため削除した。
選択ツールの利用側は列挙子との比較で採取対象を選び、永続設定は
`libs/tools/ui/kis_selection_tool_config_widget_helper.cpp`の文字列へ変換される。
この意味論は`libs/tools/ui/tests/TestToolSettingsUiContract.cpp`の設定往復試験で保護する。

`libs/ui/tests/KisCollapsibleButtonGroupSchemaContractTest.cpp`は、LOD設定構造体の既定値・等値演算と、
長押し機能の内部property文字列を固定していた。LODの利用者は描画エンジン設定画面であり、
しきい値による即時プレビュー可否、設定保存、未知の描画エンジン設定での状態保持を
`KisLodAvailabilityContractTest.cpp`で検証する。長押しの利用者はコンテキストメニューを持つ画面部品であり、
有効時のメニュー発生と無効時のメニュー抑止を`KisLongPressEventFilterContractTest.cpp`で検証する。
内部実装は共有ライブラリーの非公開記号のため、試験は公開API化せず、所有する実装ソースを試験対象へ組み込み、
必要な`kritaimage`と`kritawidgetutils`へ直接依存する。LODの設定読込は描画エンジン登録簿を利用するため
`kritaimage`の依存閉包を保持し、長押し試験は`kritawidgetutils`だけで閉じる。

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

`libs/ui/tests/KisCanvas2SchemaContractTest.cpp`は、InfinityManagerの内部登録名だけを固定していた。
この名前はCanvas2内部の装飾登録・検索・表示切替にだけ使われ、保存形式、設定、プラグイン、スクリプト、
外部連携の利用は確認できなかったため、意味論を持つ代替試験を追加せず、試験ファイルと専用CTest定義を削除した。

`libs/widgetutils/tests/KStandardActionEnumContractTest.cpp`は、`KStandardAction::StandardAction`の
整数値が連続することだけを固定していた。整数値を保存・通信・外部APIで利用する証拠はなく、
`KStandardAction::name()`の値は`krita/kritamenu.action`、製品のアクション検索、タッチUIプラグインで
外部識別子として利用されることを確認した。試験を`KStandardActionCompatibilityTest.cpp`へ改名し、
識別子の一意性と主要な外部識別子、QAction生成結果、起動通知を実装ライブラリーで検証する。

`libs/psdutils/tests/PSDLayerRecordSchemaContractTest.cpp`は、PSD書出し中だけに使う
`ChannelWritingInfo`の初期値とフィールドを固定していた。保存結果はPSD形式であり、
`plugins/impex/psd/tests/kis_psd_test.cpp`が保存・再読込後の画素と透明マスクを検証するため、
専用試験と広いCMake依存を削除した。

`libs/input/ui/tests/KisToolInvocationActionSchemaContractTest.cpp`は、入力操作の列挙値を固定していた。
`krita/data/input/*.profile`と利用者の入力プロファイルは、Tool InvocationおよびAlternate Invocationの
modeを16進数値で保存する。`KisToolInvocationActionCompatibilityTest.cpp`は保存形式を読み込み、
そのmodeが設定画面の同じ操作名へ解決されることを確認する。

`libs/painting/tests/KisFilterStrokeStrategySchemaContractTest.cpp`は、移動ストロークの内部ジョブ種別、
フィールド、LOD複製を固定していた。`MoveStrokeStrategyContractTest.cpp`は実際にストロークを開始し、
ドラッグ完了後のレイヤー位置と取消後の位置復元を確認する。

`libs/impex/tests/KisPNGConverterSchemaContractTest.cpp`は、PNG変換オプション構造体の既定値とコピーを
固定していた。オプションはPNG書出しプラグインが設定値へ変換する内部入力であり、構造体の値そのものは
保存形式ではない。`plugins/impex/png/tests/kis_png_test.cpp`からHDR画素の往復、CICP/ICCプロファイルの
往復、旧HDRプロファイル読込を`KisPngHdrAndColorProfileContractTest`として通常のCTestへ登録した。

`libs/impex/tests/KisDlgImportVideoAnimationSchemaContractTest.cpp`は、動画情報構造体の初期値とコピーを
固定していた。動画取込では`RenderedFrames`がタイムスタンプの有無を`KisMainWindow`へ渡し、連番か
位置再配置かを選択する。`KisVideoFrameImportContractTest.cpp`はこの利用側の状態判定を検証する。

`libs/impex/tests/KisFFMpegWrapperSchemaContractTest.cpp`は、FFmpeg設定構造体とエラー番号の値を固定していた。
これらは外部ファイルやスクリプトの互換性識別子ではない。`KisFFMpegWrapperContractTest.cpp`は自身を短い
子プロセスとして実行し、動画入出力の成功時に開始・完了通知とログを返し、失敗時に失敗結果と診断を返す
ことを検証する。

`libs/flake/tests/KoFFWWSConverterSchemaContractTest.cpp`は、フォント分類用の内部構造体のフィールド、
既定値、コピーを固定していた。SVG文字の利用者はCSS一般フォント名を指定するため、
`KoFFWWSConverterContractTest.cpp`は`serif`、`sans-serif`、`monospace`の分類結果と未知の名前を解決しない
結果を検証する。

`libs/flake/tests/KoFontGlyphModelSchemaContractTest.cpp`は、glyph種別とモデルroleの番号を固定していた。
Glyph Palette QMLは`openType`、`glyphLabel`、`childCount`というrole名でモデルを読むため、
`KoFontGlyphModelCompatibilityTest.cpp`はその文字列を、QML利用者を明記した互換性要件として検証する。

`libs/flake/tests/KoToolBaseSchemaContractTest.cpp`は、ツールボックスの内部区分文字列を固定していた。
区分は`KoToolManager`の初期ツール選択と`KoToolBox`の配置にだけ使われ、全利用側が同じ定数を参照する。
設定、XML、拡張記述子、スクリプトへの保存または外部公開はないため、専用試験とCTest定義を削除した。

`libs/flake/tests/KoDocumentResourceManagerSchemaContractTest.cpp`は、文書リソースキーの番号を固定していた。
各利用側は列挙子で参照し、保存・通信・拡張境界に番号を渡さない。`KoDocumentResourceManagerContractTest.cpp`は、
図形コントローラーと編集部品が利用する文書解像度・画素領域の更新、読取、`resourceChanged`通知、
ハンドル選択範囲の下限を検証する。

`libs/flake/tests/KoSvgTextEnumContractTest.cpp`は、SVG文字の内部値構造、比較演算、CSSキーワード配列の添字を
固定していた。CSSの`font-stretch`はSVG読込・保存で利用者が観測する形式であるため、
`KoSvgTextFontStretchContractTest.cpp`は全キーワードを実際に読込み、解決したQt幅と保存後のキーワードを検証する。
他の構造体既定値・コピー・flag構成は、SVG/CSS変換と描画の既存試験で十分に表現できる内部詳細として削除した。

`libs/flake/tests/KoSvgTextShapeMarkupConverterSchemaContractTest.cpp`は、文書内で一時的に使う折返し種別と
`QTextFormat` property番号を固定していた。PSD変換、SVGテキスト編集、再保存は`white-space`と`inline-size`を
観測するため、`KoSvgTextWrappingContractTest.cpp`はSVGから`QTextDocument`への変換と再保存を行い、
`pre`、有効な`pre-wrap`、幅のない`pre-wrap`の保存結果を検証する。

`libs/flake/tests/KoSvgTextFontSelectionValueContractTest.cpp`は、フォント分類軸、OpenType機能の
内部既定値・コピー、および描画器へ渡すタグ文字列を固定していた。SVG/CSSのフォント指定は読込時の
解決済み文字属性として利用者が観測するため、既存のフォント読込試験を
`KoSvgTextFontImportContractTest.cpp`へ分離して通常のCTestへ登録した。`font-variant-*`のCSS値と
OpenType機能の変換・再保存は既存の`KoSvgTextEnumConversionContractTest`が検証する。フォント機能の
画素結果は`TestSvgText::testCssFontVariants()`が担うが、macOSの現行Fontconfig構成では比較基準と一致せず、
従来どおり隔離した画像試験として維持する。

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
`cmake --build --preset tdd-macos --target help`による再構成と、
`ctest --preset tdd-macos -N`による対象CTestの登録確認が成功した。
混在試験の整理後に、対象CTestの実行と残した試験対象のコンパイル・リンクが成功した。
今回の`TestAngleSelector`構築では`kritalibkis`の完全な依存閉包を構築し、各翻訳単位が利用するQt事象型、
画像型、設定型、領域型を直接取り込むように修正した。`kritaimpexui`の内側へ入れた補助オブジェクトが
オブジェクトライブラリー境界を越えて伝播しなかったため、所有先の`kritaapplicationui`へ明示的に組み込んだ。
その後、`TestAngleSelector`の構築と3回反復が成功した。
`./scripts/run-shared-test-env ./scripts/verify-quick`は運用検査39件、依存境界、公開ヘッダー、
プラグイン登録、文書、リンク、図の検証を含めて成功した。

## 残る課題と再開条件

機械的な型特性整理は完了した。次は残存Schema試験を利用場面から監査し、
実際の呼び出し側と永続形式から互換性要件を確認する。列挙値や識別子の固定は保存データや外部連携の
根拠がある場合だけ残し、公開操作の結果を検証しない試験は振る舞いへ置き換えるか削除する。

`KisResourceItemDelegateContractTest`の索引変換は試験内でresolverを再定義しているため、
実際の資源モデルが返す索引と描画結果による検証へ移す。
`KoDialog::showEvent()`の表示直後の破棄に関する既知不具合はTODOの独立項目で扱う。

今回の対象では、`TestToolSettingsUiContract`の構築と
`libs-tools-ui-TestToolSettingsUiContract`のCTestがmacOSで成功した。
`KisLodAvailabilityContractTest`と`KisLongPressEventFilterContractTest`の構築および
`libs-ui-KisLodAvailabilityContractTest`、`libs-ui-KisLongPressEventFilterContractTest`のCTestがmacOSで成功した。
新しい試験の増分依存閉包は、生成済みNinjaグラフでそれぞれ約27053行と6449行であり、
`kritaapplicationui`全体への依存を避けて、LODは`kritaimage`、長押しは`kritawidgetutils`へ限定した。
`KStandardActionCompatibilityTest`の構築と`libs-widgetutils-KStandardActionCompatibilityTest`のCTestもmacOSで成功した。
実装ライブラリーを利用する依存閉包は生成済みNinjaグラフで約6443行であり、既存の
`KisDialogStateSaverTest`と同程度である。
`python3 scripts/architecture/check_test_contracts.py`と`git diff --check`も成功した。
Canvas2の削除後にInfinityManager識別子の利用箇所を再検索し、Canvas2とInfinityManagerの内部実装だけであることを確認した。
`plugins-impex-psd-kis_psd_test`、`libs-input-ui-KisToolInvocationActionCompatibilityTest`、
`libs-input-ui-KisToolProxyContractTest`、`libs-painting-MoveStrokeStrategyContractTest`、
`libs-painting-TestPaintingBoundary`の構築とCTestがmacOSで成功した。入力プロファイルと移動ストロークの
新しい試験は、それぞれ既存の実装ライブラリー試験と同じNinja依存閉包（42588行、28413行）に収まる。
`plugins-impex-png-KisPngHdrAndColorProfileContractTest`、
`libs-impex-KisVideoFrameImportContractTest`、`libs-impex-KisFFMpegWrapperContractTest`の構築とCTestが
macOSで成功した。動画取込とFFmpegの新しい試験は、既存の`TestImportExportUiBoundary`と同じ
Ninja依存閉包（70327行）に収まる。PNG-suite全体は、ICCプロファイルを持つ16ビットグレースケール7件の
比較基準と色管理経路の不一致を理由に従来どおり隔離し、成功するHDR・プロファイル契約だけを通常のCTestで
維持する。
`libs-flake-KoFFWWSConverterContractTest`と`libs-flake-KoFontGlyphModelCompatibilityTest`の構築とCTestが
macOSで成功した。`libs-flake-KoDocumentResourceManagerContractTest`も成功し、生成済みNinjaグラフの
依存閉包は既存の`TestResourceManager`と同程度（12931行、12934行）である。`KoToolBaseSchemaContractTest`の
削除後、CMake再構成で削除済みCTest登録がないことを確認する。
`libs-flake-KoSvgTextFontStretchContractTest`と`libs-flake-KoSvgTextWrappingContractTest`の構築とCTestが
macOSで成功した。いずれも実装ライブラリーの`kritaflake`だけを追加依存とし、既存の文書リソース試験と
同程度のNinja依存閉包（各12931行）に収まる。削除したSVG文字のSchema CTest登録も再構成後に残っていない。
`KoSvgTextFontImportContractTest`の増分構築とCTestがmacOSで成功した。SVG読込の実装ライブラリーである
`kritaflake`へ依存を限定し、フォント登録を伴う隔離済みの`TestSvgText`全体を通常CTestへ追加しない。
生成済みNinjaグラフの依存閉包は12931行であり、既存のSVG文字契約試験と同程度に収まる。
`testCssFontVariants`は同じ環境でFontconfig設定を読込めず、6件の文字画像比較基準と不一致になるため、
この実行環境では通常CTestへ昇格できない。

主増分構築木`build/tdd-macos`と共有コンパイラーキャッシュを継続利用する。
Qt 5、Linux、Windows、Android、実タブレット入力と全ネイティブ試験は未実施である。
