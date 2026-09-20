# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-09-21 00:08 JST
- 状態: `complete`
- 現在の検査段階: R2-G19l センサーパックインターフェース依存の直接化（完了）
- 関連TODO: R2-G19a・R2-G19b・R2-G19c・R2-G19f・R2-G19g・R2-G19h・R2-G19i・R2-G19j・R2-G19k・R2-G19l完了、R2-G19d-a・R2-G19d-b・R2-G19eは`planned`
- ブランチ: `issue-44-direct-dependencies`
- 開始コミット: `b8c3277221`。作業開始時点の作業ツリーは変更なし。
- 目的: センサーパックの公開インターフェースが共有データ、文字列、センサー一覧を推移的取込みから得る状態を解消し、複製・比較・設定入出力の既存契約を保ったまま利用要件を明示する。
- 範囲固定: `plugins/paintops/libpaintop/KisSensorPackInterface.{h,cpp}`と、`plugins/paintops/libpaintop/CMakeLists.txt`の`kritapaintopsensorpackinterfaceobjects`を変更する。`plugins/paintops/libpaintop/tests/KisSensorPackInterfaceContractTest.cpp`と同CMake定義は閲覧・実行のみとし、公開API、共有データ複製、借用ポインター、操作結果と順序を維持する。
- 調査: `direnv exec . build-incremental native plan kritapaintopsensorpackinterfaceobjects`は変更なし計画とmacOSパッケージ境界1723対象の成功を確認した。Ninja command closureは1件、CMakeは`Qt6::Core`を直接列挙しFile APIの構築依存は0件である。変更前の`misc-include-cleaner`は実装の`QString`に対して1件の診断を報告した。
- 完了: 公開ヘッダーが`std::vector`、`QSharedData`、`QString`、公開記号を所有ヘッダーから直接得るようにし、実装も定義に使う`QString`を直接取得する。CMakeのQt Core利用要件、前方宣言、複製・設定入出力の責務は維持し、診断は0件になった。
- 検証: `direnv exec . build-incremental native build kritapaintopsensorpackinterfaceobjects`と`kritalibpaintop`は成功し、macOSパッケージ境界1723対象を確認した。`direnv exec . run-test KisSensorPackInterfaceContractTest`は1件成功した。
- 残るリスク: センサーデータオブジェクトとその実利用側は未監査である。実行検証はmacOS・Qt 6.11.1であり、Qt 5、Linux、Windows、Android、iOSはIssue #44のプラットフォーム監査で扱う。
- 次の作業: Issue #44の順序に従い、`plugins/paintops/libpaintop/CMakeLists.txt`の`kritapaintopsensordataobjects`を次の有限な監査単位とし、センサーデータ実装の所有ヘッダーと直接依存を測定する。
- 目的: 設定UIから分離済みの`kritapaintopruntime`が、`kritalibbrush`と`kritapainting`の推移的な取込み・リンク閉包から実行に必要な型と記号を得る状態を解消する。`kritapaintopruntime_LIB_SRCS`の30実装と同対象のCMake依存を範囲とし、テストソース、公開API、描画結果、保存形式は変更しない。
- 調査: `direnv exec . build-incremental native plan kritapaintopruntime`は変更なし計画とmacOSパッケージ境界1723対象の成功を確認した。変更前の直接依存は`kritalibbrush`、`kritapainting`、`kritapaintopsensordataobjects`、`kritapaintoptextureoptionioobjects`の4対象である。Clang 21の`misc-include-cleaner`を3実装へ試行し、Qt値型、共有ポインター型、安全検査マクロ、ダブ生成APIの所有ヘッダー不足と未使用取込みを再現した。
- 完了: `kritapaintopruntime`の全30実装を`misc-include-cleaner`で監査した。センサー実装は曲線設定ヘッダー経由で得ていたデータ型を`KisSensorData.h`へ直接接続し、数学関数、Qt値型、検査マクロ、不透明度定数、合成ID、共有ポインター補助の所有ヘッダーを追加した。未使用・重複取込みを除去し、輪郭計算は`KisOpacityOption.h`経由で得ていた`KisSizeOption`を`KisStandardOptions.h`から直接得る。`KisNode`は`dynamic_cast`入力側の完全型に必要なため、検査の未使用診断よりコンパイラー診断を優先して実装取込みを維持した。
- 完了: `kritapaintopruntime`は、全体基盤、画像、ブラシ、描画、undo、色、資源、Qt Core・Gui・Widgets・Xml、KDE翻訳、Boostを公開利用要件として直接列挙した。オブジェクトを利用する`kritalibpaintop`のリンクまで成功し、集約対象の推移的リンク閉包へ依存しない構築経路を確認した。
- 検証: `direnv exec . build-incremental native build kritapaintopruntime`と`kritalibpaintop`は成功し、macOSパッケージ境界1723対象を確認した。`run-test`で`kis_paintop_test`、`kis_linked_pattern_manager_test`、`KisTextureOptionDataIOContractTest`、`KisTextureOptionLodContractTest`は各1件成功し、`libpaintop`の残る29件も成功した。`direnv exec . ./scripts/verify-quick`は45個の方針試験、10責務、533公開ヘッダー、172プラグイン登録、文書・リンク・図を含めて成功した。完全native検査は対象を含む878件が成功し、変更外の`KisSafeDocumentLoaderTest`だけが並列時に通知数1対2で失敗した。同試験は直後の単独再実行で19.50秒・1件成功し、再度の並列実行では同じ競合を再現した。
- 残るリスク: `KisSafeDocumentLoaderTest`の並列競合は今回のテスト固定方針により変更せず、後続の試験安定化対象として引き渡す。実行検証はmacOS・Qt 6.11.1であり、Qt 5、Linux、Windows、AndroidはR2-G19dの対象である。`kritapaintopruntime`以外の`libpaintop`オブジェクト対象と設定UI実装は未監査である。
- 次の作業: `plugins/paintops/libpaintop/CMakeLists.txt`の`kritapaintopdynamicsensorfactoryobjects`を次の有限な監査単位とし、2実装の所有ヘッダーと直接依存を測定する。
- 目的: 画像ノード、画素ブラシ、複製paint-op、グラデーションツールの実装が、別ヘッダーや別CMake対象の推移的依存から型・記号・リンク対象を得る状態を解消する。既存の利用者向け試験は固定し、実装側の所有者だけを明示する。
- 範囲固定: `libs/image/kis_node.h`・`libs/image/kis_node.cpp`、`plugins/paintops/defaultpaintops/brush/KisDabRenderingQueue.h`・`KisDabRenderingQueue.cpp`、`plugins/paintops/defaultpaintops/duplicate/kis_duplicateop.h`・`kis_duplicateop.cpp`・`kis_duplicateop_settings.h`・`kis_duplicateop_settings.cpp`、`plugins/tools/basictools/kis_tool_gradient.h`・`kis_tool_gradient.cc`を、各ファイルが使う標準・Qt・製品型の所有ヘッダーへ直接接続する。対応する`kritapixelbrush`、`kritadefaultpaintops_static`、`kritadefaulttools_static`のCMake依存を直接記載する。テストソース、公開動作、保存形式は変更しない。
- 完了: `kis_node`は`QList`、標準`optional`、型登録、既定境界、レイヤー、ポインター変換、投影更新フラグの所有ヘッダーを直接取り込む。`KisDabRenderingQueue`は標準探索・数値上限、Qt削除補助、固定描画装置、共有型と色空間宣言を直接取り込む。複製paint-opは標準オプション、描画情報、合成ID、均一プロパティ、設定、ポインター変換の所有ヘッダーへ接続し、設定画面経由の取込み、重複取込み、未使用宣言を除去した。グラデーションツールは値型、ツール工場、翻訳、画像signal、既定境界、描画装置、undo表示名の所有ヘッダーへ接続し、数学関数を標準名前空間から使う。
- 完了: `plugins/paintops/defaultpaintops/CMakeLists.txt`の`kritapixelbrush`と`kritadefaultpaintops_static`は、画像、全体基盤、ブラシ、色、paint-op、描画、Qt、翻訳の利用対象を直接列挙した。`plugins/tools/basictools/CMakeLists.txt`の`kritadefaulttools_static`は、キャンバス、画像、資源、部品、Qt、KDE Frameworksの利用対象を直接列挙した。公開ヘッダーが必要とする依存と実装専用依存を分け、誤っていた資源UI対象名は既存CTestのリンク診断から`kritaresourceui`へ修正した。
- 検証: `direnv exec . build-incremental native build kritapixelbrush`、`kritadefaultpaintops_static`、`kritadefaulttools_static`は成功し、各回のmacOSパッケージ境界検査は1723対象を確認した。`direnv exec . run-test KisDabRenderingQueueTest`、`kis_node_test`、`MoveSelectionStrokeTest`は各1件成功した。`direnv exec . ./scripts/verify-quick`は45個の方針試験、10責務、533公開ヘッダー、172プラグイン登録、文書・リンク・図を含めて成功した。`direnv exec . ./scripts/verify`はnative CTest 879件を300.89秒で全件成功した。
- 残るリスク: 実行検証はmacOS・Qt 6.11.1であり、Qt 5、Linux、Windows、Androidの実行確認はR2-G19dへ引き渡す。今回の完了範囲外にある実装ファイルとCMake対象の推移的依存は未監査である。
- 範囲固定: ブラシプリセット設定群は、`plugins/paintops/libpaintop/tests/CMakeLists.txt`で実際の`KisCurveOptionData`・`KisKritaSensorPack`の保存・復元へ直接つながるデータ試験に限定した。`KisCurveOptionDataCommonContractTest.cpp`、`KisCurveOptionDataContractTest.cpp`、`KisKritaSensorPackContractTest.cpp`、`KisSizeOptionDataContractTest.cpp`、`KisMirrorOptionDataContractTest.cpp`、`KisSharpnessOptionDataContractTest.cpp`、`KisScatterOptionDataContractTest.cpp`、`KisSpacingOptionDataContractTest.cpp`、`KisPrefixedOptionDataWrapperContractTest.cpp`の監査を完了した。曲線・標準値・旧センサー・ミラー・シャープネス・散布・間隔の保存結果は既存または新設の実設定試験へ維持・統合し、構築既定値、内部ポインター、演算の写し、偽の設定ストアだけを固定する試験は削除した。`KisAirbrushOptionDataContractTest.cpp`、`KisColorOptionDataContractTest.cpp`、`KisColorSourceOptionDataContractTest.cpp`、`KisCompositeOpOptionDataContractTest.cpp`、`KisPaintingModeOptionDataContractTest.cpp`、`KisFilterOptionDataContractTest.cpp`は共通曲線・センサー保存経路を共有しないため、所有実装を変更するときに監査する後続対象とする。
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
- 完了: `KoSvgTextPropertyDataContractTest.cpp`を削除し、`KisTextPropertiesManagerContractTest.cpp`へ置き換えた。段落・文字範囲の混在する選択が文字プロパティdockerの状態となり、dockerの設定・解除がSVGテキストツールの選択へ反映されることを検証する。
- 完了: `KoSvgTextPropertiesInterfaceContractTest.cpp`を削除した。試験内の仮想呼出しとsignal順序ではなく、文字範囲選択のsignal、継承プロパティ、span状態、設定・解除の利用者向け結果を`KisTextPropertiesManagerContractTest.cpp`へ統合した。
- 完了: `KoShapeAnchorEnumContractTest.cpp`を削除した。アンカーの位置・基準・方式の整数値は、保存形式、SVG/XML、設定、プラグイン、スクリプト、外部識別子で利用されていない。製品の利用場面も確認できないため、数値順序を固定する専用CTestを維持せず、代替試験も追加しない。
- 完了: `KoShapeAnchorContractTest.cpp`を、図形のインライン化による位置遷移、文字位置の借用、配置戦略の置換・破棄という公開状態と寿命の契約へ縮小した。参照同一性、仮想メソッド呼出し、変更できない既定値を固定する検証は削除した。
- 完了: `KoShapeEnumContractTest.cpp`を削除し、形状プラグインが登録したテンプレートの識別子、表示情報、作成プロパティが形状生成へ渡ることを既存の`TestKoShapeFactory`へ統合した。空初期化と浅いコピーの検証は削除した。
- 完了: `KoShapeLoadingContextSchemaContractTest.cpp`を削除した。追加属性の値型と登録簿は、製品のSVG/XML読込、設定、プラグイン、スクリプト、外部識別子から参照されていない。文字列保持と比較演算子だけを固定する専用CTestは維持せず、代替試験も追加しない。
- 完了: `KoShapeSavingContextSchemaContractTest.cpp`を削除した。保存オプションのビット値は、既定値を設定する実装以外で読まれず、SVG/XML、設定、プラグイン、スクリプト、外部識別子に変換されない。整数値とQtフラグ演算だけを固定する専用CTestは維持せず、代替試験も追加しない。
- 完了: `KoShapeReorderCommandSchemaContractTest.cpp`を削除した。既存の`TestShapeReorderCommand`が、前後移動、最前面・最背面、子図形、重なり、変更不能時の結果を実際のz順序で検証している。操作種別の整数値だけを固定する専用CTestは維持しない。
- 完了: `KoSnapGuideSchemaContractTest.cpp`を削除し、`TestSnapStrategy`へ統合した。スナップ設定は`KisSnapConfig`の真偽値として保存され、利用者が選ぶ補助線種別の整数値や内部優先度は保存形式・XML・プラグイン・スクリプト・外部識別子に使われていない。キャンバス操作では、有効化した対象がポインター位置を変え、Shiftで一時的に吸着を回避できること、同時に候補があると点への吸着が近い線への吸着より優先されることを利用結果で検証する。
- 完了: `GimpBumpMapSchemaContractTest.cpp`を削除し、`kis_layer_style_projection_plane_test`のバンプマップ処理へ統合した。`bumpmap_vals_t`はベベル・エンボス実装だけが構築し、既定値や`BumpmapType`の整数値は保存形式、設定、プラグイン、スクリプト、外部識別子に使われていない。同じ高さマスクに対しエンボス方向を反転すると、傾斜の選択値が変化し、反対方向では明暗が反転することを画素結果で検証する。
- 完了: `KisKeyframeChannelSchemaContractTest.cpp`を削除した。スカラー曲線の利用者はアニメーションdockerと不透明度などのキーフレーム編集であり、既存の`kis_keyframing_test`が実チャンネルへの追加と制限変更後の再生値を検証している。KRA保存のチャンネル名は互換性対象だが、既存の`kis_kra_loader_test`が保存済みアニメーションの`content`チャンネルを復元する。専用試験が固定していた範囲値型の並びと未使用の抽象プローブは維持しない。
- 完了: `KisNodeCommandsAdapterSchemaContractTest.cpp`を削除した。専用試験は、色ラベルを参照する内部キャッシュの比較用値型とリスト所有方式だけを固定しており、ノード編集アダプターの操作結果を検証していなかった。レイヤーdocker、フィルターダイアログ、拡張機能はアダプターでノード操作を実行し、既存の`kis_node_commands_adapter_test`が追加後の表示ツリーとundo、画像再束縛、選択マスク移動後の有効状態を検証している。色ラベルの統合結果はツール内部で一時利用され、保存形式、XML、スクリプト、外部識別子としての根拠は確認されなかった。
- 完了: `KisPaintOpUtilsSchemaContractTest.cpp`を削除し、既存の`kis_paintop_test`へ利用結果を統合した。フリーハンド・液状化ツールは直前の異なるカーソル位置から輪郭方向を決め、ブラシ実装は自動間隔と最小ダブ判定でストローク密度を決める。試験はカーソル移動後に使う位置、異方・等方ブラシの実効間隔、不可視ダブの抑制を検証する。保存済みブラシプリセットとpaint-opプラグインが使うマスキングブラシのIDと設定キーは`KisPaintopSettingsIdsCompatibilityTest.cpp`へ明示的に分離し、既存プリセットを復元できることを守る。
- 完了: `KoCompositeOpSchemaContractTest.cpp`を削除した。専用試験が固定していた`ParameterInfo`の生ポインター、既定値、コピー、平均不透明度のキャッシュは、合成処理中だけの内部バッファであり、保存形式や外部連携の根拠は確認されなかった。既存の`TestKoColorSpaceAbstract`がチャンネル制限を含む色空間をまたぐ合成後の画素を、`TestCompositeOpInversion`が各合成モードの描画結果を検証している。合成モードIDはKRAの`compositeop`属性とOpenRasterの`composite-op`属性へ保存されるため、`KoCompositeOpIdsCompatibilityTest.cpp`として明示的に維持する。
- 完了: `KisResourceModelEnumContractTest.cpp`を整理した。リソース選択画面、リソース管理拡張、各dockerは列・役割・絞り込みを列挙子で参照し、値そのものを保存、XML、プラグイン、スクリプト、外部識別子へ渡していない。`TestResourceModel`は、無効化したリソースを全件・無効リソース絞り込みで再選択でき、無効化したストレージのリソースを全ストレージ絞り込みで再表示できることを検証する。`resourcecache.sqlite`の`storage_types`名と`storages.storage_type_id`は既存データの読込と一時リソース削除に使われるため、種別ID・非翻訳名だけを`KisResourceStorageTypeCompatibilityTest.cpp`へ明示的に分離した。
- 完了: `KisTagModelSchemaContractTest.cpp`を整理した。タグ選択部品は、`SelectedTags`設定に保存されたURLで再起動後の選択を復元するため、`All`と`All untagged`の予約URLだけを`KisTagPseudoUrlCompatibilityTest.cpp`で維持する。擬似行ID、列番号、タグ・ストレージ絞り込み値の数値は保存形式、XML、プラグイン、スクリプト、外部識別子に使われていない。`TestTagModel`は、無効化したタグを全件・無効タグ絞り込みで再選択でき、無効化したストレージのタグを全ストレージ絞り込みで再表示できることを検証する。
- 完了: `KisAnimUtilsSchemaContractTest.cpp`を削除した。試験が固定していた`FrameItem`の初期値、比較、ハッシュ、関数の宣言だけは、タイムラインdocker内部の移動要求を表す実装詳細であり、保存形式、設定、プラグイン、スクリプト、外部識別子の根拠はない。既存の`kis_animation_utils_test`は、同一レイヤーの循環移動、レイヤーをまたぐ移動、フレーム入替後の各時刻の画素とundo後の復元を検証する。
- 完了: `KisAnimTimelineFramesModelSchemaContractTest.cpp`を削除した。試験が固定していたタイムライン選択値型の等値・ハッシュと、メニュー項目の名前・ダミー保持は、docker内部の一時データであり、保存形式、設定、プラグイン、スクリプト、外部識別子の根拠はない。タイムラインの固定状態はKRAの`intimeline`属性として既存の`kis_kra_saver_test`が保存往復を検証する。`timeline_model_test`は、既存レイヤーを選ぶとタイムラインに追加され、アクティブ化され、候補メニューから除かれる結果を検証する。この試験で、非同期のダミー更新前にアクティブ行を設定して別レイヤーを選択したままにする不具合を発見し、行への反映後に選択するよう修正した。共有の空ノード試験補助は、使用する`KisPaintDevice`の完全型を直接取り込む。
- 完了: `KXMLGUIClientSchemaContractTest.cpp`を削除した。試験が固定していた状態反転列挙子の整数値と状態変更リストの初期化・コピーは、XMLGUI内部で一時的に使う実装詳細である。XMLGUIの状態要素、状態変更の製品側呼出し、状態反転値を使う保存形式、設定、プラグイン、スクリプト、外部識別子は確認されなかった。XMLGUIの実際のアクションIDはメニュー定義、入力、拡張機能で別途利用されるが、この試験の対象ではないため、根拠のない互換性試験や代替試験を追加しない。
- 完了: `KisCurveOptionSchemaContractTest.cpp`を削除した。`ValueComponents`の初期値と曲線ウィジェットのフラグ値は、曲線計算・画面構成だけが使う内部表現であり、ブラシプリセット、設定、XML、プラグイン、スクリプト、外部識別子に使われていない。既存の`KisCurveOptionDataTest`、`KisKritaSensorPackCompatibilityTest`、`KisCurveOptionModelTest`が、プリセットの曲線・センサー保存と復元、画面の有効状態・強度範囲・曲線選択を検証する。対象の構築では、ブラシ実行・設定画面ライブラリーが完全型と`KisMpl`を推移的インクルードへ依存し、実行時オブジェクトが共有ライブラリーへ取り込まれない不具合を検出した。使用する型・ユーティリティを直接取り込み、実行時の下位オブジェクトを最終ライブラリーにも組み込むよう修正した。
- 完了: macOS全体構築で露出した直接依存漏れを、値型・テンプレート・MOCが必要とするQt型と所有型を各利用元が直接取り込む形へ修正した。`KisDabRenderingQueue.cpp`は使用する`kismpl::mem_less`の所有ヘッダー`KisMpl.h`を直接取り込む。実ブラシ操作を使う`FreehandStrokeContractTest`は`kritapixelbrush`と`kritalibpaintop`の構築閉包へ接続し、リンク時に必要なpaint-op実装を確実に取り込む。XML色の保存読込試験は、Qt 6.11で妥当な`#RRGGBBAA`色を無効とする旧期待値を除き、保存した色が同じ色として復元される利用結果を検証する。
- 完了: `KisPaintOpFactorySchemaContractTest.cpp`を削除した。`AUTO`、`ALWAYS`、`NEVER`の列挙値は、ファクトリー内部の未使用状態に初期化されるだけで、設定、プリセット、XML、プラグイン、スクリプト、外部識別子には使われていない。paint-op IDはプリセットの`paintop`プロパティとして保存され、プラグイン登録、ブラシ選択、描画、ライブプレビューが解決するが、専用試験はその利用結果を検証していなかった。根拠のない互換性試験や重複した代替試験を追加せず、専用CTestと広いinclude・compile definitionを削除した。
- 完了: `KisPlaybackEngineSchemaContractTest.cpp`を`KisPlaybackEngineContractTest.cpp`へ置き換えた。再生統計の初期値とコピーは、タイムラインdockerが表示する値型の実装詳細であり、設定、保存形式、XML、プラグイン、スクリプト、外部識別子の互換性根拠はない。アニメーションdockerの再生制御モデルは、フレームを落とす設定の値と変更signalへ接続する。実際のQt再生エンジンに対し、切替後の読取値とsignal引数が一致し、同値の再設定では通知しないことを検証する。
- 完了: `KisReferenceImagesDecorationSchemaContractTest.cpp`を削除した。専用試験は参照画像を操作せず、ガイド装飾の内部登録名`guides-decoration`だけを固定していた。この名前はガイドマネージャー内の生成・検索で使われ、設定、保存形式、XML、プラグイン、スクリプト、外部識別子には使われていない。参照画像は作成操作後に参照画像レイヤーへ追加され、専用ツールへ切り替わり、失敗時には入力元を示す通知を表示する。この利用結果は既存の`KisNodeManagerReferenceImageContractTest`が検証する。
- 完了: `KisDlgPreferencesEnumContractTest.cpp`を削除した。色空間用のボタングループID、設定ページと各タブの整数値は、設定、保存形式、XML、プラグイン、スクリプト、外部識別子へ渡らない。設定ダイアログの再表示位置は`KisDlgPreferences/CurrentPage`の文字列として保存される。選択アクションパネルは一時的なページ要求で一般設定のツールタブを開くため、実際の`KisMainWindow`を使う`kis_view_signals_test`へ統合し、設定アクション後に表示されるページとタブを検証する。専用CTest、広い依存定義、生成UIヘッダーの専用登録を削除した。統合先試験は変換マスクを生成する補助コードが使う完全型を直接取り込む。
- 完了: `KisActionEnumContractTest.cpp`を`KisActionCompatibilityTest.cpp`へ改名した。コアと29個のプラグインの`.action`定義は`activationFlags`と`activationConditions`を2進数文字列で保存し、`KisActionManager`が基数2で復元する。操作登録と拡張機能はこの値で有効状態を決めるため、各ビットの互換性要件と、変更時に影響する利用者を試験に明記した。Qtのフラグ演算を重複して検証していた文は削除した。
- 完了: `KisConfigEnumContractTest.cpp`を`KisConfigCompatibilityTest.cpp`へ置き換えた。既存の`kritarc`は入力、色採取、起動、色管理、背景、選択アクションバー、レイヤー表示、補助線描画の各モードを整数で保存するため、その値を明示的な互換性要件として維持した。`ColorSamplerPreviewStyle::Count`は保存値ではないため除外した。キャンバス色管理、表示ビット深度、ルート表示形式は`kritarc`と`kritadisplayrc`の文字列で保存するため、実際の保存値と再読込後のモードを検証する。専用の広い依存定義を、設定実装を提供する`kritaapplication`への直接依存へ縮小した。
- 完了: `KoDialogEnumContractTest.cpp`を`KoDialogContractTest.cpp`へ置き換えた。`ButtonCode`、`ButtonPopupMode`、見出しフラグの数値は保存形式、XML、プラグイン、スクリプト、外部識別子で使われていない。インポート、書出し、設定、復旧の各ダイアログは記号名のボタンとsignalを使い、利用者はボタンの表示状態、選択後のsignalと受理・取消結果、詳細領域の表示を観測する。実クリックでこれらを検証し、数値、レイアウトヒント、内部スロット呼出し、遅延破棄の固定を削除した。直接オブジェクト対象のリンクには、必要なヘルプ実装を加えた。
- 完了: `KColorSchemeEnumContractTest.cpp`を`KColorSchemeThemeCompatibilityTest.cpp`へ置き換えた。色集合、背景・前景・装飾・濃淡の数値は製品側で記号名として使われ、保存形式、XML、プラグイン、スクリプト、外部識別子に渡らない。9個の同梱`.colors`テーマと利用者テーマは、`Colors:View`などのグループ名と色キーを保存しており、テーマ管理、各画面の配色、拡張機能のエラー表示が読込結果を利用する。全画面種別の背景・前景色、フィードバック・装飾色、境界の明暗を利用結果で検証する。Qt 6では`KF6::ColorScheme`が実装を提供するため、試験は製品ライブラリーと同じ実依存を明示する。
- 完了: `SvgTextCursorEnumContractTest.cpp`を削除し、型組版の基線選択を既存の`SvgTextCursorTest`へ統合した。カーソル移動と型組版ハンドルの整数値は保存形式、XML、プラグイン、スクリプト、外部識別子へ渡らない。既存試験は横書き、右横書き、縦書きの文字・語・行・段落移動の結果を検証している。Shiftで表意文字基線ハンドルを選ぶと、選択テキストの支配・配置基線プロパティが更新されることを追加で検証する。統合先の実装ライブラリー構築で、値として保持する`QPointF`と利用する画像、ノード、スナップガイドの完全型を推移的インクルードへ依存していたため、各所有者を直接取り込むよう修正した。
- 完了: 削除済みの`KisDisplayConfigSchemaContractTest`と`KisDitherWidgetSchemaContractTest`の名前を使う、`libs/ui/tests/CMakeLists.txt`のUI生成定義を削除した。同じUIは`libs/ui`、`libs/application`、`libs/impex`の製品側生成定義が各利用者へ提供する。テスト専用の重複生成は利用者向け契約を持たないため、代替試験は追加しない。
- 完了: `KoSvgTextFontMetricsValueContractTest.cpp`を削除した。フォント計測、背景・線、textPath、下線位置の構造体について、初期値、全メンバーのコピー、等値比較、列挙値だけを固定しており、保存形式・設定・プラグイン・スクリプト・外部識別子の互換性根拠はない。`TestSvgText`は実フォント計測とtextPathの開始位置・方向・伸張・装飾をSVG描画結果で、`TestSvgTextShape`は文字形状と輪郭形状の操作結果で保護する。専用CTestとそのQt・公開ヘッダー依存を削除し、重複した代替試験は追加しない。
- 完了: `KisMetaDataValueContractTest.cpp`を、メタデータ編集、文書複製、統合、言語別値の利用結果へ整理した。XMP入出力とメタデータ編集は、値種別と`xml:lang`修飾子を使う。異種値への編集は失敗として元の値を保ち、配列の有効な拡張編集は成功として返す。統合は完全な最新日時を保持し、時刻のミリ秒を正しく繰り上げる。言語修飾子を比較対象に含め、文書複製後の値は独立して編集できるようにした。`KisEntryEditorContractTest`は、構造体フィールドの編集後も他のフィールドを保存することを実ストアと画面部品で検証する。この試験で判明した構造体全体を単一フィールドで置換する不具合を修正した。両試験は実製品ライブラリーへ直接リンクし、編集部品試験はアプリケーション全体ではなく必要なメタデータ・Qt依存だけで閉じる。
- 完了: `KisMetaDataTypeInfoContractTest.cpp`を、EXIFスキーマで検証するスカラー、配列、構造体、言語別コメント、XMP文字列解析の利用結果へ整理した。以前のPropertyType整数値、型情報キャッシュ、私有初期状態、選択肢のコピー、借用スキーマ参照には、保存形式、外部API、プラグイン、スクリプトの互換性根拠がない。実スキーマを使う検証器は不正な配列要素を`INVALID_TYPE`、閉じた選択肢の未定義値を`INVALID_VALUE`として報告する。言語別配列が`asArray()`の対象外であるため`xml:lang`のない要素を見逃す不具合を修正し、値の所有者が全要素を非公開に検証する。XMP入力の整数、有理数、日時はスキーマに対応する値へ解析され、検証器が受理する。試験は製品ライブラリーと標準スキーマ資産だけへ直接依存する。
- 完了: `kis_meta_data_test.cpp`を、メタデータストアの追加・重複拒否・遅延作成・複製・削除と、検証器が未知項目、型不正、閉じた選択肢の値不正を区別して報告する利用結果へ縮小した。旧試験の値型初期化・等値・コピー、私有`TypeInfo`ファクトリー、型情報キャッシュ、スキーマ内部構造は利用者向け保証を持たず、専用補助ヘッダーとともに削除した。XMP読込が利用する解析結果は`KisMetaDataTypeInfoContractTest`へ統合した。外部の`Document::exportImage()`が指定する`Anonymizer`フィルターIDは`KisMetaDataAnonymizerCompatibilityTest.cpp`で明示的に維持し、Dublin CoreとPhotoshopの全個人情報項目を実ストアから除去することを検証する。製品側以外に`Parser`を実装する利用者は確認できないため、仮想呼出しと破棄だけを確認する`KisMetaDataParserContractTest.cpp`と専用CTestを削除した。
- 完了: `KisMetaDataTagsContractTest.cpp`を削除した。EXIF、RAW、TIFFの読込実装は一部のタグ番号で形式固有の変換・除外を選ぶが、専用試験は利用されないタグを含む表全件の数値だけを固定していた。`KisExifTest`を通常のmacOS/Linux CTestにし、実カメラ画像からTIFF/EXIF値、日時、OECF、CFAパターンを復元する結果と、不正なOECF/CFAデータを拒否して他のメタデータを保持する結果を検証する。動的に読み込む`kritaexif`をCTest対象の直接構築依存にし、実行に必要な資源初期化を`KISTEST_MAIN`へ移した。EXIF日時タグは文字列のまま読込まれて日時スキーマの検証に失敗していたため、標準日時文字列を`QDateTime`として復元するよう修正した。Windowsの既知の未対応状態はbroken testとして明示する。
- 完了: `KisMetaDataIOBackendContractTest.cpp`を削除した。専用試験のバックエンドは実際のプラグインを使わず、`BackendType`と`HeaderType`の数値、仮想呼出し、生ポインターの転送、仮想破棄だけを固定していた。EXIF、XMP、IPTCは同梱プラグインとして登録され、画像・KRAの入出力利用者が各形式の保存・読込結果を使う。共通インターフェースの形状そのものに保存形式、外部拡張、スクリプトの互換性根拠は確認できず、EXIFの実保存・読込結果は`KisExifTest`で保護されるため、根拠のない代替試験は追加しない。
- 完了: `KisMetaDataMergeStrategyContractTest.cpp`を、レイヤー統合の実ストア結果へ置き換えた。Layers dockerが選ぶ削除、先頭優先、一致のみ、Smartの各戦略は、メタデータを残さない、下側レイヤーを優先する、同値の項目だけを残す、重みの高い値・加重レーティング・作成者一覧を統合する結果を利用者へ示す。偽戦略のID・説明文・ポインター・呼出し回数・破棄の検査を削除し、製品ライブラリーへ直接リンクした。`OnlyIdenticalMergeStrategy`が各ソースの値ではなく最初の値だけを比較して競合値を残していた不具合を修正した。
- 完了: `KisAsynchronousStrokeUpdateHelperContractTest.cpp`を、移動・フリーハンドの定期更新と終了時の強制更新、変形ツールの初期化前終了時の強制更新、取消後に更新を停止する状態遷移へ整理した。更新ジョブを受けるストロークIDと強制更新の結果を実`KisStroke`と更新窓口で検証し、ジョブの逐次性・排他性、複製、QObject破棄、未使用のカスタム工場、私有スロット呼出しを固定する検証を削除した。試験はテスト専用オブジェクトではなく`kritapainting`へ直接リンクする。
- 完了: `KisResourceStorageTypeCompatibilityTest.cpp`を、`StorageType`列挙子の整数値と変換関数を直接固定する試験から、実際の`resourcecache.sqlite`の`storage_types`行を読む互換性試験へ置き換えた。`storages.storage_type_id`は同表の外部キーとして保存され、起動時の一時リソース削除とストレージ一覧は保存済みIDと非翻訳名を使う。隔離したキャッシュを初期化し、既存キャッシュと同じ7個のID・名称の組をSQLite結果として検証する。試験は`kritaresources`、SQL、テスト実行環境、翻訳ライブラリーへ直接リンクし、ヘッダーだけを検査する依存を除去した。
- 完了: `KoCanvasResourceIdsContractTest.cpp`を削除した。キャンバスリソースのIDはツール、docker、描画処理が同一プロセス内で記号名として読取・変更通知に使う鍵であり、保存形式、設定、XML、プラグイン記述子、スクリプト、外部連携で数値を使う根拠はない。`TestResourceManager`が色、単位、派生リソースの読取と変更通知を実際の`KoCanvasResourceProvider`で検証するため、整数表だけを固定する専用CTestとCMake定義を維持しない。
- 完了: `KisResourceTypesContractTest.cpp`を`KisResourceTypesCompatibilityTest.cpp`へ改名した。リソース種別キーはリソースバンドルのMANIFEST、タグの`ResourceType`、`resourcecache.sqlite`、配置パスに保存されるため、既存のリソースを読込・分類する利用者の明示的な互換性対象である。全種別の保存キーを維持し、リソース選択画面が種別を対応する表示名へ解決する結果を検証する。保存形式・拡張利用の根拠がないサブ種別の文字列、ログ分類名、同一カテゴリ取得の検査を削除した。
- 完了: `KisTagPseudoUrlCompatibilityTest.cpp`を、擬似URL関数の戻り値を直接比較する試験から、実際の`KisTagModel`が保存済みURLを擬似タグとして解決する互換性試験へ置き換えた。タグ選択部品は`SelectedTags`設定へURLを保存し、タグモデルとフィルターは`All`と`All untagged`を全件・未分類表示として扱う。隔離したキャッシュで両URLを復元し、選択対象の有効状態、種別、URL、表示名を検証する。試験はテスト専用のヘッダー定義ではなく`kritaresources`へ直接リンクする。
- 完了: `KoCanvasResourcesInterfaceContractTest.cpp`を削除した。専用試験は偽実装への仮想呼出し、共有ポインターの所有、仮想破棄だけを固定しており、保存形式、プラグイン、スクリプト、外部APIの互換性根拠はない。実利用者は`KoCanvasResourceProvider`または局所ストロークの実装から値を読む。`TestResourceManager`が色・単位・派生リソースの読取と変更通知を、`TestPaintingBoundary`が局所ストロークのリソーススナップショットを検証するため、専用CTestとCMake定義を維持しない。
- 完了: `KoLocalStrokeCanvasResourcesContractTest.cpp`を削除し、`TestPaintingBoundary`へ局所ストロークの利用結果を統合した。ペイントプリセットの複製は必要なキャンバス値を局所ストレージへ格納し、実行時スナップショットはその値を読む。パターンとグラデーションを取得後に選択を置換しても、開始済みスナップショットは元の署名を保持し、次のスナップショットだけが置換後の署名を使うことを検証する。任意キーの格納・置換、コピー・代入、共有ポインター破棄は利用者向け契約ではないため維持しない。
- 完了: `KisResourceMetaDataModelContractTest.cpp`は、リソース選択部品、依存リソースの欠損判定、メタデータ絞込みがSQLiteの保存値を個別に読む結果を検証する。リソースID・表・キーに一致するBase64化`QVariant`だけを復元し、欠損または空の値は無効値として扱う。問い合わせの破棄後に表を再作成できることは内部資源管理であるため、専用検証を削除した。
- 完了: `KisDatabaseTransactionLockContractTest.cpp`は、リソースキャッシュの初期化・同期・削除が複数のSQL更新を中断したとき全てを取消し、成功後に明示的に確定したとき全てを保持することを検証する。内部アダプターへの直接呼出し、重複呼出し、ロック所有フラグは利用者の観測結果ではないため削除した。
- 完了: `KisSqlQueryLoaderContractTest.cpp`は、リソースキャッシュの初期化・移行・同期がSQL資源を順に実行し、単一文の値束縛と一括実行で全ての値を保存することを検証する。失敗時は診断がSQL資源と文番号、ファイルを開けない原因を示す。例外構造体の直生成・コピーと問い合わせ参照の同一性は内部形状であるため削除した。
- 完了: `KisTemporaryResourceStorageLockContractTest.cpp`を削除し、`TestResourceLocator`へ実ロケーターを使う一時ストレージの利用結果を統合した。同名のレイヤースタイルを同時に編集すると別々の一時ストレージが登録され、一方を閉じても他方の依存リソースは選択可能なまま残り、最後の編集終了後に消える。偽ロケーターへのアダプター呼出し、固定した接尾辞、ロック所有フラグは維持しない。
- 完了: `KoResourceCacheInterfaceContractTest.cpp`は、ペイントスナップショットが古いブラシ・キャンバス状態のキャッシュを拒否する所有cookieと、プリセットのバックグラウンド更新結果をUIへ渡すQt型登録を検証する。偽キャッシュへの仮想呼出し、仮想破棄、共有ポインター型との直接比較は利用者向け契約ではないため削除した。実装キャッシュと接頭辞付きキャッシュの読取・格納試験を併せて実行する。
- 完了: `KoResourceCachePrefixedStorageWrapperContractTest.cpp`は、主ブラシとマスキングブラシが同じ論理キーで輪郭を保存しても、接頭辞付きキャッシュにより別々の値を読むことを実装キャッシュで検証する。偽キャッシュの転送記録、空接頭辞、共有ポインターの寿命は利用者向け契約ではないため削除した。実装キャッシュの安全アサートを使うため、対象にその実装オブジェクトを最小の直接依存として追加した。
- 完了: `KoResourceCacheStorageContractTest.cpp`は、ブラシ準備が未生成のキャッシュを無効値として扱い、別々のキーの準備済み値を独立して読むことを検証する。同じキーへの重複格納はキャッシュ別名の回復可能なエラーとして報告する。内部マップの条件式、仮想破棄、無効操作後に残る上書き値は利用者向け契約ではないため削除した。
- 完了: `KoEmbeddedResourceValueContractTest.cpp`を削除した。KPP読込の`KisPaintOpPresetTest`が実際の埋込みリソースを有効なMD5とともに復元し、破損したMD5を除外し、同名で同一・異なる内容のパターンを正しく再利用または分離する。`TestResourceLocator`はリソースの保存・読出しで同じMD5を維持する。署名値型の既定値・比較・デバッグ書式、装置読取位置、単体ハッシュ関数の重複検証は維持しない。
- 完了: `KoResourceLoadResultContractTest.cpp`を、資源解決の利用結果へ縮小した。ペイントプリセット・フィルターが資源DBから既存依存を解決すると、その型付き資源と署名を使える。ローカルストロークのスナップショットとKRA読込は埋込み資源の内容と署名を取込み、未解決リンクは利用者が復旧できる署名付きの失敗として返る。既存の`KisPaintOpPresetTest`は実KPPの埋込み・未解決・再発見結果を検証する。値型のコピー時の共有所有、代入、診断書式は利用者向け契約ではないため削除した。
- 完了: `KoResourceContractTest.cpp`を、資源の入出力と依存資源の利用結果へ縮小した。選択したファイルから資源を作ると読込パスと表示名を得る。存在しない・空のファイルは読込に失敗し、保存は内容を置換して下位書込みの失敗を呼出し側へ返す。内容から生成したMD5と明示的なMD5を資源解決・重複排除用の署名に使い、リンク・埋込み資源は順に返し、移管したサイドロード資源は消去する。下位書込み失敗を`save()`が成功として返していた不具合を修正し、パレット編集などが保存失敗を通知できるようにした。初期メンバー値、コピー、仮想破棄、デバッグ書式、値型の比較演算子は利用者向け契約ではないため削除した。
- 完了: `KisStoragePluginContractTest.cpp`を削除し、ストレージ時刻の利用結果を`TestResourceStorage`へ統合した。フォルダー・バンドルの実ストレージはファイルシステムの更新時刻を返し、文書ローカルのメモリーストレージは生成時刻を維持するため、資源キャッシュ同期は変更を検出し、未変更の一時ストレージを繰り返し走査しない。既存のフォルダー・メモリー・バンドル試験が実資源の読込、版管理、列挙、入出力、MD5を検証する。偽プラグインのローダー探索・呼出し回数、既定値、仮想破棄、任意メタデータ転送を固定する専用CTestとCMake定義は削除した。
- 完了: `KisResourcesInterfaceContractTest.cpp`を削除し、資源解決の利用結果を`KisLocalStrokeResourcesContractTest`へ統合した。局所ストロークの資源集合は、活動中で署名が一致する資源を優先し、古い文書のファイル名検索をbest-matchだけで行い、厳密照合は不一致のMD5を拒否する。未解決リンクは復旧用の署名を保ち、型付き取得は要求した資源型を返す。実KPP読込も埋込み・未解決・再発見の結果を検証する。インターフェースの内部ソースキャッシュ、私有状態、仮想破棄、偽アダプターの転送を固定する専用CTestとCMake定義は削除した。
- 完了: `KisLocalStrokeResourcesContractTest.cpp`を、実行中ストロークが資源スナップショットから得る解決結果へ縮小した。レイヤースタイル画面が追加する複製済みの勾配・パターンと、ペイントプリセット・フィルターが使う資源は、型別に検索される。活動中の厳密一致が優先され、旧文書のファイル名候補はbest-matchだけで利用でき、未解決リンクは復旧に必要な署名を保つ。追加・除去後の候補更新も同じ試験で確認する。初期リストのnull除去、重複した内部リスト、回復可能アサートの回数、`clone()`後のコンテナーと共有ポインターの所有形状は利用者向け契約ではないため削除した。
- 完了: `KisGlobalResourcesInterfaceContractTest.cpp`を削除した。全体資源インターフェースはGUIスレッドの資源モデルを型別sourceへ接続し、パレット・パターン選択、KPP・PSD・ブラシ読込がその解決結果を使う。実KPP読込とPSD読込の試験が資源を取得できる結果を検証する。複数スレッドで同一singletonを返すこと、sourceアダプターの同一アドレス、偽モデルsourceの生成回数は、外部利用を持たない内部初期化・キャッシュの形状であるため、専用CTestとCMake定義を削除した。
- 完了: `KisStorageFilterProxyModelContractTest.cpp`を削除し、資源一覧の絞込み結果を既存の`TestStorageFilterProxyModel`へ統合した。資源選択画面とバンドル管理画面は、ファイル名、対応するストレージ種別、活動状態で候補を絞込み、条件を切り替えると新しい候補だけを表示する。試験で判明した再評価漏れを修正し、`setFilter()`は行フィルターを更新する。Qt 6.10以降では行フィルター変更APIを使い、旧Qtでは既存の無効化APIを使う。列挙子の整数値、親所有、内部source indexへの変換、偽ストレージ探索を固定する専用CTestとCMake定義は削除した。
- 完了: `KisResourceThumbnailCacheContractTest.cpp`を削除した。資源一覧、プリセット選択、ツールチップは、要求サイズと変形方法に合うサムネイル画像を受け取り、選択状態を含めて描画する。既存の`KisResourceThumbnailPainterContractTest`と資源一覧ビュー・一覧ウィジェットの契約試験が画像のサイズ・色・選択枠・ツールチップへの結果を検証する。private挿入フック、ストレージ位置の正規化回数、内部キャッシュキー、返却値の所有期間、singletonポインターを固定する専用CTestとCMake定義は削除した。
- 完了: `KisDatabaseTransactionLockContractTest.cpp`は利用者向けのトランザクション契約を既に検証しているため保持した。資源キャッシュの初期化・同期・掃除は、未承認の複数変更をスコープ終了または明示取消で残さず、全工程の成功後にcommitした変更だけを返す。部分的なキャッシュが資源選択画面へ現れることを防ぐSQLiteの実状態を検証する。ロック所有、内部接続、呼出し順序、適合ロックの継承形状は固定しない。
- 完了: `KisResourceMetaDataModelContractTest.cpp`は利用者向けのメタデータ照会契約を既に検証しているため保持した。資源選択、依存判定、メタデータフィルターは、対象テーブル・資源・キーに一致する直列化値だけを読み、欠落・空値を利用不可として扱う。別の資源またはテーブルの値で選択・警告を誤ることを防ぐSQLiteの実状態を検証する。内部SQL行、モデル索引、準備済み問い合わせのキャッシュ形状は固定しない。
- 完了: `KoResourceBundleManifestContractTest.cpp`を、バンドル編集と読込が観測する資源・型・タグ・失敗結果へ縮小した。資源の追加・削除後にバンドルローダーが残存する型別ファイルとタグを受け取り、壊れたまたは利用不能なマニフェストは失敗して古い資源を残さない。`KoResourceBundleManifestCompatibilityTest.cpp`は、`KoResourceBundle`が`META-INF/manifest.xml`へ保存し、バンドル読込が利用するXMLの名前空間、根エントリー、型、パス、MD5、タグを保存形式互換性として検証する。資源参照の既定値・構築子、マップ順序、仮想破棄、デバイスを開く回数とモードは固定しない。
- 完了: `KisResourceTypesCompatibilityTest.cpp`を、保存済み型キーだけの互換性試験へ縮小した。資源バンドル、タグ、`resourcecache.sqlite`の`resource_types.name`は同じ型キーで資源を照会するため、既存キーを保持する。`KisResourceTypesContractTest.cpp`は、資源型選択、バンドル概要、欠落資源警告が型キーを利用者向け表示名へ変換する結果を検証する。キー列挙の順序、内部表示名map、翻訳値型の所有形状は固定しない。
- 完了: `KisResourceThumbnailPainterContractTest.cpp`を、資源管理・取込画面が観測するサムネイル描画結果へ縮小した。要求サイズのプレビューは資源画像の色を保ち、選択した項目は選択色の枠内に画像を描画する。項目デリゲートの試験も同じ描画結果を検証する。親QObjectによる破棄通知は画面利用者の契約ではないため削除した。キャッシュへの挿入は描画入力の準備であり、呼出し結果を固定しない。
- 完了: `KisIconToolTipContractTest.cpp`を削除し、資源一覧ビュー・一覧ウィジェットの既存契約試験へツールチップの利用結果を集約した。資源選択画面と資源管理画面は固定した縮小サイズでも画像の色を保ち、透明なパターン・グラデーションではチェッカー有効時だけ透明部分を可視化する。親QObjectと内部`QTextDocument`の所有・破棄は画面利用者が観測する契約ではないため維持しない。設定保存、XML、プラグイン、スクリプト、外部識別子による互換性要件は確認されなかった。
- 完了: `KisResourceItemListViewContractTest.cpp`を、資源選択画面の表示・選択・通知・入力結果へ整理した。資源管理、プリセット、パレットの選択画面は初期サムネイル格子、表示方式と項目寸法の切替、現在の資源を保つ厳密選択、選択・クリック・文脈メニュー通知、運動スクロール中のカーソル表示を利用する。Qtの既定プロパティ、内部resize呼出し、未発火signal、QObject破棄を固定する検証と試験用friendを削除した。サイズ変更signalは資源選択部品に接続されるが、その親部品が自身のresizeで表示更新するため、利用者影響を確認できない通知回数を契約にしない。
- 完了: `KisResourceItemListWidgetContractTest.cpp`を、バンドル作成画面が観測する複数選択と表示切替へ整理した。選択済みのブラシ・パターンをサムネイル格子で複数選択でき、保存済みのサムネイル／詳細設定を適用しても資源セルの寸法を保つ。`ListViewMode`の整数値は保存せず、資源管理拡張が設定の`0`／`1`を表示方式へ変換するため、列挙値の互換性試験は追加しない。未使用の厳密選択、ツールチップ設定、signal、内部resize、スクロール、QObject所有の検証と試験用friendを削除した。直接オブジェクト構成で常に所有するツールチップ・サムネイル経路をリンクする解決関数と安全アサートの試験実行用定義は、振る舞いを検証せずに維持する。
- 完了: `KisResourceItemViewContractTest.cpp`を削除した。表形式資源ビューはリポジトリー内で生成、UI登録、signal接続、設定保存、XML、プラグイン、スクリプト、外部識別子として利用されていない。専用試験が固定していた表ヘッダー、列挙値、スクロール方針、signal回数、内部resize、QObject所有、ツールチップ内部文書には、利用者向けまたは互換性の根拠がない。
- 完了: `KisResourceItemDelegateContractTest.cpp`を、資源選択画面とバンドル作成画面が観測するセル寸法、詳細表示のサムネイル配置、選択枠、実資源プレビューへ整理した。バンドル作成ではローカル行の資源型・IDから全体資源モデルの同じ資源を引き、描画結果が直接その全体モデルを描いた結果と一致する。試験内でresolver、ストレージ位置変換、privateキャッシュ挿入、安全アサートを再定義していた検証と、QObject親子破棄の検証を削除した。実資源DB・ローダーの既存フィクスチャを使うため、CTestは`kritaresources`、`kritaglobal`、`kritaplugin`、`kritatestsdk`の実装閉包を直接リンクする。設定保存、XML、プラグイン識別子、スクリプト、外部識別子の互換性要件は確認されなかった。
- 完了: `KisResourceItemChooserSyncContractTest.cpp`を、同期済み資源選択画面の初期プレビュー寸法、利用可能範囲への丸め、画面更新へ渡すsignalの値へ整理した。プリセット、既定ブラシ、ガマットマスクの選択画面はこの値をセル寸法として利用する。singletonの同一アドレスとQObject破棄、同じ値を再設定した時の通知回数は利用者向け契約ではないため削除した。`KisViewManager`は`baseLength`を書き込むが、再読込する利用側、XML、プラグイン識別子、スクリプト、外部識別子の互換性要件は確認されなかった。
- 完了: `TestResourceUiContract.cpp`を、偽の構築・表示・選択・入力・ボタン部品への内部転送を固定する20件の検証から、実際の資源選択画面の利用結果へ置き換えた。ブラシプリセット選択画面は、可視の実資源を選ぶと同じ資源を返して所有画面へ通知する。同期を有効にした二つの選択画面は、共有プレビュー寸法の変更後に同じセル寸法となる。実資源DB・ローダーと`kritaresourceui`を使う実装閉包へ更新し、列挙値、内部呼出し、偽ポインター、QObject所有、内部設定値を固定する検証を削除した。保存形式、XML、プラグイン識別子、スクリプト、外部識別子の互換性要件は確認されなかった。
- 完了: `ResourceListViewModesContractTest.cpp`を削除した。資源管理の`ResourceItems*.viewMode`設定は`0`／`1`を格子・詳細表示へ直接変換しており、`ListViewMode`の整数値を保存しない。横ストリップを含む表示切替とセル寸法は`KisResourceItemListViewContractTest`、バンドル作成画面の格子・詳細切替は`KisResourceItemListWidgetContractTest`が利用結果として検証する。列挙順、数値、相違比較だけを固定する専用CTestとCMake定義を維持しない。
- 完了: `KisResourceUiDescriptorContractTest.cpp`を削除した。記述子の型文字列は実際のブラシプリセット選択画面がその型の資源を表示・選択する結果として`TestResourceUiContract`で確認する。プレビュー方針は呼出し側が画面構成へ渡す内部値であり、保存形式、XML、プラグイン識別子、スクリプト、外部識別子の互換性要件は確認されなかった。単なる文字列・真偽値の保持と既定値を固定する専用CTestとCMake定義を維持しない。
- 完了: `KisTagLabelContractTest.cpp`を、バンドルのタグプレビューが選択済みタグを表示から除くために使う文字列照会へ整理した。`WdgTagPreview`はタグ名と同じラベルを取り除くため、表示したタグ名を返すことを検証する。親QObjectの所有・破棄は利用者が観測する契約ではないため削除した。設定保存、XML、プラグイン識別子、スクリプト、外部識別子の互換性要件は確認されなかった。
- 完了: `KisStorageChooserDelegateContractTest.cpp`を、資源選択ポップアップのセル寸法とストレージのサムネイル・有効状態の描画結果へ整理した。資源管理の利用者が同じストレージを有効・無効にすると、サムネイルを保ったままセル表示が変わることを検証する。Qt style primitiveとcheckboxの呼出し回数・状態フラグ、無効索引の早期return、QObject所有を固定する検証を削除した。フォールバックアイコンを含む実際の描画を使うため、CTestは`kritawidgetutils`へ直接依存する。設定保存、XML、プラグイン識別子、スクリプト、外部識別子の互換性要件は確認されなかった。
- 完了: `KisStorageChooserWidgetContractTest.cpp`を削除し、表示中のバンドルをクリックするとそのストレージだけの有効状態が反転し、再クリックで復元される試験を`TestResourceUiContract.cpp`へ統合した。既存の`TestStorageModel`は実DBの有効状態遷移を、`TestStorageFilterProxyModel`はストレージ種別の絞り込み結果を保護する。専用試験が固定していた偽モデル、内部slot、子QObject所有、アイコン寸法、行数を削除した。資源選択ウィジェットの公開APIに保存形式、XML、プラグイン識別子、スクリプト、外部識別子の互換性要件は確認されなかった。
- 完了: `KisResourceUserOperationsContractTest.cpp`を削除し、実製品ライブラリーの`TestResourceUiContract.cpp`へ上書き確認、重複名称の取消、読込失敗通知を統合した。肯定・取消した上書き確認は利用者の判断を返し、重複名称の取消は保存済み資源名を維持し、存在しない読込元は失敗警告と空の結果を返す。実際の読込・追加・名称変更・更新のDB結果は既存の`TestResourceModel`が保護する。偽の資源・モデル・保存・質問・警告経路、private関数の置換、内部呼出し回数を削除した。公開操作に保存形式、XML、プラグイン識別子、スクリプト、外部識別子の互換性要件は確認されなかった。
- 完了: `KisTagChooserWidgetContractTest.cpp`を削除し、実製品ライブラリーの`TestResourceUiContract.cpp`へ、カスタムタグの追加・選択、選択signal、現在選択、`SelectedTags`設定への保存を統合した。資源選択画面は保存済みURLを読み直して選択を復元する。擬似タグURLの設定互換性は、利用者と保存対象を明記した既存の`KisTagPseudoUrlCompatibilityTest`が維持する。専用試験が固定していた偽モデル、追加・選択・保存関数、内部slot、子QObject所有、アイコン更新回数を削除した。
- 完了: `TagActionsContractTest.cpp`を削除し、実製品ライブラリーの`TestResourceUiContract.cpp`へ、資源選択画面の文脈メニューから既存タグへ割り当てる操作、現在タグから解除する操作、新規タグを作成して割り当てる操作を統合した。タグ操作ボタンから名前を入力して新規タグを作成する画面経路も、実際のタグモデルへ反映される結果で検証する。既存の`TestTagResourceModel`は実DBのタグ付け・解除とモデル通知を保護する。専用試験が固定していた偽のタグ・資源・入力部品、アイコン名、QObject所有、privateコールバック、比較補助の状態を削除した。`TagActions`の利用側は資源選択画面、タグ選択部品、タグ操作ボタンだけであり、保存形式、XML、プラグイン識別子、スクリプト、外部識別子として維持すべき互換性要件は確認されなかった。
- 完了: `StoreDebugContractTest.cpp`を`StoreDebugCompatibilityTest.cpp`へ置き換えた。Log Docker は保存済みの`LogDocker/file_41008`設定を`krita.lib.store`のログ規則として適用し、ファイル入出力診断を有効化する。この外部設定との互換性を、実際に同規則を適用してストレージのデバッグ出力が有効になる結果で検証する。singleton参照の同一性と既定の重大度は保存規則の利用結果ではないため固定しない。
- 完了: `WidgetsDebugContractTest.cpp`を削除した。`krita.lib.widgets`はウィジェット実装の診断出力だけで使われ、保存設定、Log Docker、プラグイン、スクリプト、外部診断設定で安定識別子として参照されない。参照の同一性、カテゴリ文字列、既定の重大度を固定する専用CTestとCMake定義を維持しない。診断ヘッダーを利用する既存の`zoomhandler_test`は増分構築と実行に成功した。
- 完了: `FlakeDebugContractTest.cpp`を`FlakeDebugCompatibilityTest.cpp`へ置き換えた。Log Docker は保存済みの`LogDocker/tools_41003`設定を`krita.lib.flake`のログ規則として適用し、ツール診断を有効化する。この外部設定との互換性を、実際に同規則を適用してFlakeのデバッグ出力が有効になる結果で検証する。singleton参照の同一性と既定の重大度は保存規則の利用結果ではないため固定しない。
- 完了: `DebugPigmentContractTest.cpp`を`DebugPigmentCompatibilityTest.cpp`へ置き換えた。Log Docker は保存済みの`LogDocker/pigment`設定を`krita.lib.pigment`のログ規則として適用し、色管理診断を有効化する。この外部設定との互換性を、実際に同規則を適用してPigmentのデバッグ出力が有効になる結果で検証する。singleton参照の同一性と既定の重大度は保存規則の利用結果ではないため固定しない。
- 完了: `ResourceDebugCompatibilityTest.cpp`を追加し、Log Docker の保存済み`LogDocker/resources_30009`規則が資源管理のデバッグ出力を有効にすることを検証する。試験は実装が`krita.lib.resource`を返すため失敗し、同設定が適用する`krita.lib.resources`へ資源ライブラリーのカテゴリを修正した。既存のリソース種別互換性試験と新しい互換性試験は修正後に成功し、資源管理診断を有効にしても出力されない不具合を修復した。
- 完了: `KisDebugContractTest.cpp`から、関数ポインターで全21カテゴリの名称を固定する検査を除いた。Log Docker の保存済み規則が使うグローバルカテゴリは`KisDebugCompatibilityTest.cpp`へ分離し、実際に各規則を適用してデバッグ出力が有効になる結果を検証する。保存規則外のDB移行、Android、ロケールカテゴリには互換性根拠がない。試験により、保存済み描画規則`krita.grender`とタブレット規則`krita.tablet`が実装の別名へ向き、診断を有効にできない不具合を発見した。各カテゴリを保存済み規則へ合わせ、既存のメソッド名整形・バックトレース試験も成功した。
- 完了: `KoStoreDeviceContractTest.cpp`を削除し、偽の`KoStore`が固定していた自動オープン状態、借用所有権、失敗後の開放モード、内部`seek()`回数を廃止した。KRA、OpenRaster、参照画像の保存読込は、開いたアーカイブ項目を`KoStoreDevice`経由で追加の`open()`なしにXMLとして読み書きするため、実ZIPアーカイブでXML文書とレイヤー名が往復する`TestResourceStorageArchiveContract`へ統合した。Qt XMLへの試験依存は、製品の`QDomDocument`利用と同じ保存結果を検証するために限定した。
- 完了: `KoUnitContractTest.cpp`から、単位列挙と一覧オプションの整数値、型数、換算定数、代入と等値比較、デバッグ出力を固定する検査を除いた。整数値を読む保存形式、設定、プラグイン、スクリプト、外部識別子は見つからず、長さ・角度文字列の解析にも製品側の呼び出しはない。単位入力、文書単位メニュー、選択範囲操作、キャンバス・形状・スクリーントーンの変換、変形後のピクセル値を利用者が観測する結果として検証する。選択項目が対応する単位へ戻ること、ピクセル非表示時の選択結果、物理長の変換と表示値の再入力、変形後の寸法を維持する。
- 完了: `KisZugContractTest.cpp`を削除した。キャスト、乗算、比較、丸め、タプル変換の検査はzug内部の変換器を直接固定しており、保存形式、設定、スクリプト、外部識別子の利用根拠はない。未使用の否定比較・小比較・タプル変換器は代替試験を要しない。角度反転、曲線強度範囲、即時プレビュー可否は、それぞれ描画角度センサー、曲線オプション、LOD可用性の既存モデル試験が状態更新として検証する。
- 保留: `KisPredefinedBrushModel`の明るさ・コントラスト百分率、調整有効状態、ライトネスマップ状態には利用者向け契約が必要である。モデルは非公開で専用オブジェクト対象を持たず、現在の`kritalibpaintop`構築閉包は4,230入力に及ぶ。試験だけのために製品CMakeへ重複した構築対象を加える前に、設定UIを分離した所有者と最小の構築閉包を設計する。
- 完了: `KisCurveOptionModelTest.cpp`から、範囲・強度・ラベル状態の完全型固定、完全な初期データ比較、内部範囲モデルとQObject破棄を固定する検証を除いた。曲線オプション画面は、共有曲線を最初のセンサーに表示し、外部有効状態とチェック状態を保存値へ反映し、強度スライダーに有効範囲を示し、共有曲線と個別センサー曲線を分け、選択センサーの曲線・長さ・ラベルを表示する。試験専用の`KisPropertiesConfiguration`再定義を削除して実ライブラリーへ接続した。残る`RangeProbe`は、注入された範囲モデルの選択入力だけを記録するカテゴリ4の補助であり、曲線計算・設定保存・永続化を再実装しない。実際の設定保存と復元は`KisCurveOptionDataTest`と互換性試験が保護する。実ライブラリーを使う構築閉包は4,236入力である。
- 完了: `KisCurveOptionDataCommonContractTest.cpp`を削除し、構築既定値、等値比較、センサーポインター列挙、所有、null入力、偽の設定ストアを固定する検証を廃止した。曲線オプションは`KisCurveOptionDataTest`へ統合し、実際の設定で共有曲線・強度・有効センサーを保存して復元すること、複数の接頭辞付きオプションと無関係なプリセット値が共存すること、無効にしたセンサーが無効のまま復元すること、センサー定義がない場合に筆圧の既定曲線を使うことを検証する。既存の実ライブラリー対象を使うため、新しい対象・依存は追加していない。対象の構築閉包は4,236入力である。
- 完了: `KisCurveOptionDataContractTest.cpp`を削除し、曲線データ種別の構築既定値、センサー列挙、強度範囲、チェック状態、偽の設定ストアを固定する検証を廃止した。標準オプションのIDは`KisKritaSensorPack`がプリセット設定のキーを組み立てるため、保存済みプリセットが利用する互換性要件である。`KisStandardOptionDataCompatibilityTest.cpp`は実設定へ強度を保存し、同じ既存キーだけを持つ設定から各標準オプションを復元する。互換性試験は標準の不透明度、流量、比率、硬さ、回転、色調整、速度、テクスチャ強度、明度強度の各キーを対象にする。新しい試験は既存の実ライブラリー構築閉包を使い、対象の入力は4,236件である。
- 完了: `KisKritaSensorPackContractTest.cpp`を削除し、`Checkability`の数値、センサー宣言順、内部ポインター集合、等値比較、複製、安全断言回数、偽の設定ストアとXML出力順を固定する検証を廃止した。`KisKritaSensorPackCompatibilityTest.cpp`は実際の設定を使い、保存済みブラシプリセットの旧`SizeSensor` XMLに含まれる16種類の入力IDが、選択した入力と曲線を復元することを検証する。`KisCurveOptionDataTest`は時間入力の曲線、長さ、周期設定が保存後にも復元することを検証する。画面上の選択センサーの長さは`KisCurveOptionModelTest`、実ストロークの動的入力は`FreehandStrokeContractTest`が保護する。新しい互換性試験は既存の実ライブラリー構築閉包を使い、対象の入力は4,236件である。
- 完了: `KisSizeOptionDataContractTest.cpp`を削除し、サイズID、接頭辞、構築既定値、内部の制限IDだけを固定する検証と偽の設定実装を廃止した。ブラシプリセットは`SizeValue`でサイズ曲線強度を保存するため、`KisStandardOptionDataCompatibilityTest`が実設定の保存・読込結果でこのキーを守る。ブラシ編集画面はSize曲線のFuzzy入力で即時プレビューを注意状態にし、Fade入力で利用不可にするため、`KisCurveOptionDataTest`が実ライブラリーの制限結果を検証する。状態を画面表示へ反映する規則は既存の`KisLodAvailabilityContractTest`が保護する。新しい対象や依存は追加せず、既存の実ライブラリー構築閉包4,236入力へ統合した。
- 完了: `KisMirrorOptionDataContractTest.cpp`を削除し、構築既定値、接頭辞の保持、等値比較、偽の設定実装を固定する検証を廃止した。主ブラシとマスキングブラシのプリセットは`HorizontalMirrorEnabled`と`VerticalMirrorEnabled`を保存し、ブラシ実装とマスキング設定がこれらを読んでダブの反転方向を決める。`KisMirrorOptionDataCompatibilityTest.cpp`は実設定で主ブラシの水平反転を保存・読込してダブの反転結果を検証し、マスキングブラシの`MaskingBrush/Preset/`配下の垂直反転設定が埋込み設定へ復元することを検証する。最終描画は既存の`kis_brushop_test`が保護する。新しい試験は既存の実ライブラリー構築閉包を使い、対象の入力は4,236件である。
- 完了: `KisSharpnessOptionDataContractTest.cpp`、`KisScatterOptionDataContractTest.cpp`、`KisSpacingOptionDataContractTest.cpp`を削除した。これらが固定していた構築既定値、データ等値比較、内部曲線、散布乱数、間隔演算、偽の設定ストアには利用者向け互換性根拠がなかった。`KisBrushPresetDynamicsCompatibilityTest.cpp`は実際の設定でシャープネスの輪郭整列と旧`Sharpness/factor`、散布の軸設定と旧`ScatterAmount`、間隔の`SpacingValue`・等方性・ダブ間隔設定を保存・復元し、シャープネスのダブ座標と散布無効時の位置、間隔係数を検証する。保存済みブラシプリセットを開いた利用者が同じ動的設定とダブ結果を得ることを保護する。
- 完了: `KisPrefixedOptionDataWrapperContractTest.cpp`を削除した。偽の設定マップと架空のオプション値型が接頭辞操作を再実装していたためである。主ブラシとマスキングブラシの実設定を通す`KisMirrorOptionDataCompatibilityTest.cpp`へ統合し、`MaskingBrush/Preset/`配下の設定が埋込みプリセットへ復元する結果を保護する。
- 完了: ブラシプリセット設定群のMock、Fake、Stub、試験専用派生、設定ストア、モデル、DB、直列化処理を監査した。曲線・標準値・旧センサー・ミラー・シャープネス・散布・間隔の保存試験はすべて実ライブラリーと実設定を使い、カテゴリ1の製品ロジック再実装、カテゴリ2の内部呼出し回数・順序固定、カテゴリ3の外部副作用隔離は残らない。`KisCurveOptionModelTest.cpp`の`RangeProbe`だけがカテゴリ4の入力記録補助として残り、限定理由を試験コメントへ記載した。試験専用の設定ストアと製品メソッド再定義は削除した。
- 完了: `TestTagFilterResourceProxyModel`へ、実DB上でタグを選択し、表示中の資源を無効化してから`ShowAllResources`へ切り替える契約を追加した。画面と同じ`KisTagFilterResourceProxyModel`がタグ選択時に通す`KisTagResourceModel`経路で、同じ資源IDが再表示されることを検証する。
- 後続一覧（高リスク優先監査）: R2-G19eは、`KisPropertiesConfiguration`の製品メソッドを再定義して設定ストアを再実装する`KisAirbrushOptionDataContractTest.cpp`、`KisColorOptionDataContractTest.cpp`、`KisColorSourceOptionDataContractTest.cpp`、`KisCompositeOpOptionDataContractTest.cpp`、`KisFilterOptionDataContractTest.cpp`、`KisPaintingModeOptionDataContractTest.cpp`、`KisPrecisionOptionContractTest.cpp`、`KisTextureOptionDataIOContractTest.cpp`の8件に限定する。実設定の保存・復元または公開操作の結果へ置き換え、完了後にリポジトリ全体の逐次監査へ拡大しない。
- 後続一覧（所有実装変更時の監査）: `KisPredefinedBrushModel`は、所有する設定UIまたはpaint-opの変更に合わせて利用者向け保存・描画結果へ整理する。
- 後続一覧（現状の振る舞い維持）: `KisCurveOptionDataTest.cpp`、`KisCurveOptionModelTest.cpp`、`KisStandardOptionDataCompatibilityTest.cpp`、`KisKritaSensorPackCompatibilityTest.cpp`、`KisMirrorOptionDataCompatibilityTest.cpp`、`KisBrushPresetDynamicsCompatibilityTest.cpp`、`TestTagFilterResourceProxyModel.cpp`は、現在の利用場面を実ライブラリーと実設定または実DBで保護するため維持する。
- 検証: macOSで`cmake --build --preset tdd-macos --target KisBrushPresetDynamicsCompatibilityTest KisCurveOptionModelTest`、`cmake --build --preset tdd-macos --target TestTagFilterResourceProxyModel`、ブラシ設定6件と資源管理1件のCTest、削除済み4件のCTest登録0件、`python3 scripts/architecture/check_test_contracts.py`、`git diff --check`、`./scripts/verify-quick`が成功した。ブラシ設定の実ライブラリー構築閉包は4,236入力である。
- 検証: macOSで`cmake --build --preset tdd-macos --target kritapixelbrush FreehandStrokeContractTest -- -j1`、`libs-ui-FreehandStrokeContractTest`、`plugins-paintops-defaultpaintops-brush-KisDabRenderingQueueTest`、`plugins-tools-basictools-MoveSelectionStrokeTest`のCTest 3件、`./scripts/verify-quick`、`./scripts/verify`が成功した。完全検査は879/879件成功した。
- 未実施OS: R2-G19d-aはQt 5、Linux、Windows、Androidでブラシプリセット設定保存・復元と資源管理プロキシ切替を実行する。R2-G19d-bはAndroid crash handler、Windows/MSVC互換操作、Linux DBus・colord構成を実行環境で検証する。ソース検査は同一リビジョンの構築ホストで実行し、各OSまたは端末は実行時契約を検証する。
- 次の作業: R2-G19d-aで各OSとQt 5の構築閉包・CTest実行可能性を確定し、移植済みのブラシ設定6試験と資源管理1試験を実行する。
- 検証: macOSで`TestAngleSelector`と全依存の構築、ガイド・格子設定試験、色役割試験が成功した。対象試験の反復実行と`verify-quick`も成功した。
- 再発防止検証: macOSで`KisSignalCompressorContractTest`、`KisBezierPatchContractTest`、
  `KStandardActionCompatibilityTest`の構築とCTestが成功した。新しい検査を含む運用検査45件と
  `verify-quick`が成功した。
- 検証: macOSで`KoDialogContractTest`の構築と`libs-widgets-KoDialogContractTest`のCTestが成功した。
- 検証: macOSで`KColorSchemeThemeCompatibilityTest`の構築と`libs-widgetutils-KColorSchemeThemeCompatibilityTest`のCTestが成功した。
- 検証: macOSで`SvgTextCursorTest`の構築と`plugins-tools-svgtexttool-SvgTextCursorTest`のCTestが成功した。
- 検証: macOSで`kritaapplicationui`の再構成を含む増分構築と`libs-ui-KisPlaybackEngineContractTest`のCTestが成功した。`check_test_contracts.py`と`verify-quick`も成功した。
- 検証: macOSで`TestSvgText`の構築、`libs-flake-TestSvgTextShape`のCTest、削除済み`KoSvgTextFontMetricsValueContractTest`の登録件数0が成功した。`TestSvgText`の直接実行は、現行のバンドル実行パスを解決できず初期化前に失敗する既知のbroken testである。
- 検証: macOSで`KisMetaDataValueContractTest`、`KisEntryEditorContractTest`、`kritametadataeditor`、`kis_meta_data_test`、`KisMetaDataTypeInfoContractTest`の構築と4件のCTestが成功した。
- 検証: macOSで`KisMetaDataTypeInfoContractTest`、`kis_meta_data_test`、`KisMetaDataValueContractTest`の増分構築と3件のCTestが成功した。言語修飾子がない配列要素を有効と数える初期失敗を再現後、`check_test_contracts.py`と`git diff --check`が成功した。
- 検証: macOSで`kis_meta_data_test`、`KisMetaDataAnonymizerCompatibilityTest`、`KisMetaDataTypeInfoContractTest`、`KisMetaDataValueContractTest`の増分構築と4件のCTestが成功した。削除済みの`KisMetaDataParserContractTest`は再構成後のCTest登録に存在しない。

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

`libs/flake/tests/KoSvgTextPropertyDataContractTest.cpp`は、実際の`KoSvgTextProperties`を試験内で
再定義し、プロパティデータの既定値、コピー、等値比較、メタ型名、デバッグ出力順序を固定していた。
文字プロパティdockerとSVGテキストツールは、`KisTextPropertiesManager`とキャンバス資源プロバイダーを通じて
混在選択、設定、解除を利用する。`KisTextPropertiesManagerContractTest.cpp`は段落・文字範囲で異なる太さの
選択から混在状態を通知し、dockerが選んだ太さと解除操作を実際のツール境界へ渡すことを検証する。メタ型名と
デバッグ書式に保存形式・プラグイン・スクリプトの互換性根拠はない。

`libs/flake/tests/KoSvgTextPropertiesInterfaceContractTest.cpp`は、`KoSvgTextProperties`を再定義して試験用の
仮想メソッドとsignalの呼出し順序を確認していた。実際のSVG文字ツールは文字選択signal、継承プロパティ、
span選択状態を`KisTextPropertiesManager`へ渡すため、同じ管理者契約で文字範囲の選択変更、表示状態、設定、
解除を検証する。親子関係と試験用実装の呼出し順序は、ツール利用者が観測する契約ではない。

`libs/flake/tests/KoShapeAnchorEnumContractTest.cpp`は、アンカーの位置、基準、方式の列挙値を整数値へ
固定していた。`libs`、`plugins`、`sdk`の製品利用箇所、保存、SVG/XML、設定、外部識別子を確認しても、
これらの型と列挙子を消費する利用者は見つからない。値の並びは図形編集の結果、保存結果、表示結果を表さないため、
試験と専用CTest定義を削除する。

`libs/flake/tests/KoShapeAnchorContractTest.cpp`は、利用側のない参照同一性、仮想メソッドの呼出し回数、
変更手段のない既定値を固定していた。`KoShapeAnchor`のヘッダーが定める公開状態と寿命は、インライン図形の
文字相対位置、テキスト文書が所有する位置情報の存続、置換された配置戦略の破棄である。この3つを利用者の操作と
観測できる結果として保持する。

`libs/flake/tests/KoShapeEnumContractTest.cpp`は、`KoShapeTemplate`の空初期化と浅いコピーを固定していた。
形状プラグインのテンプレートは、識別子、表示名、分類、説明、アイコン、作成プロパティを形状選択と作成へ渡す。
`TestKoShapeFactory.cpp`は登録後に公開されるテンプレートと、そのプロパティで生成される形状を検証する。

`libs/flake/tests/KoShapeLoadingContextSchemaContractTest.cpp`は、追加属性の値型が文字列を保持し、名前だけで
比較されることを固定していた。追加属性の登録簿と取得結果を製品の読込処理は参照しておらず、SVG/XML、設定、
プラグイン、スクリプト、外部識別子への変換もないため、専用試験とCTest定義を削除する。

`libs/flake/tests/KoShapeSavingContextSchemaContractTest.cpp`は、保存オプションの整数値とQtフラグ演算を
固定していた。各オプションは既定値を設定する実装以外から読まれず、保存結果や外部連携へ変換されない。
SVG/XMLの保存結果は既存の保存試験で保護し、専用試験とCTest定義を削除する。

`libs/flake/tests/KoShapeReorderCommandSchemaContractTest.cpp`は、前後移動・最前面・最背面を表す列挙値の
整数値を固定していた。既存の`TestShapeReorderCommand.cpp`は、図形選択の並べ替え後に利用者が見るz順序、
子図形の順序、重なり、変更不能時の結果を検証するため、専用試験と広いCMake定義を削除する。

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
`KisTextPropertiesManagerContractTest`の増分構築とCTestがmacOSで成功した。段落・文字範囲のプロパティ状態を
実際のキャンバス資源とツール境界へ渡す所有ライブラリー`kritaapplicationui`だけを直接依存とした。生成済み
Ninjaグラフの依存閉包は70327行であり、同じUI所有ライブラリーを使う既存の文書状態試験（72368行）より小さい。

主増分構築木`build/tdd-macos`と共有コンパイラーキャッシュを継続利用する。
Qt 5、Linux、Windows、Android、実タブレット入力と全ネイティブ試験は未実施である。
