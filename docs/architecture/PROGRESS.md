# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-09-20 11:15 JST
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
- 完了: `KisCurveOptionSchemaContractTest.cpp`を削除した。`ValueComponents`の初期値と曲線ウィジェットのフラグ値は、曲線計算・画面構成だけが使う内部表現であり、ブラシプリセット、設定、XML、プラグイン、スクリプト、外部識別子に使われていない。既存の`KisCurveOptionDataTest`、`KisKritaSensorPackContractTest`、`KisCurveOptionModelTest`が、プリセットの曲線・センサー保存と復元、画面の有効状態・強度範囲・曲線選択を検証する。対象の構築では、ブラシ実行・設定画面ライブラリーが完全型と`KisMpl`を推移的インクルードへ依存し、実行時オブジェクトが共有ライブラリーへ取り込まれない不具合を検出した。使用する型・ユーティリティを直接取り込み、実行時の下位オブジェクトを最終ライブラリーにも組み込むよう修正した。
- 完了: `KisPaintOpFactorySchemaContractTest.cpp`を削除した。`AUTO`、`ALWAYS`、`NEVER`の列挙値は、ファクトリー内部の未使用状態に初期化されるだけで、設定、プリセット、XML、プラグイン、スクリプト、外部識別子には使われていない。paint-op IDはプリセットの`paintop`プロパティとして保存され、プラグイン登録、ブラシ選択、描画、ライブプレビューが解決するが、専用試験はその利用結果を検証していなかった。根拠のない互換性試験や重複した代替試験を追加せず、専用CTestと広いinclude・compile definitionを削除した。
- 完了: `KisPlaybackEngineSchemaContractTest.cpp`を`KisPlaybackEngineContractTest.cpp`へ置き換えた。再生統計の初期値とコピーは、タイムラインdockerが表示する値型の実装詳細であり、設定、保存形式、XML、プラグイン、スクリプト、外部識別子の互換性根拠はない。アニメーションdockerの再生制御モデルは、フレームを落とす設定の値と変更signalへ接続する。実際のQt再生エンジンに対し、切替後の読取値とsignal引数が一致し、同値の再設定では通知しないことを検証する。
- 完了: `KisReferenceImagesDecorationSchemaContractTest.cpp`を削除した。専用試験は参照画像を操作せず、ガイド装飾の内部登録名`guides-decoration`だけを固定していた。この名前はガイドマネージャー内の生成・検索で使われ、設定、保存形式、XML、プラグイン、スクリプト、外部識別子には使われていない。参照画像は作成操作後に参照画像レイヤーへ追加され、専用ツールへ切り替わり、失敗時には入力元を示す通知を表示する。この利用結果は既存の`KisNodeManagerReferenceImageContractTest`が検証する。
- 完了: `KisDlgPreferencesEnumContractTest.cpp`を削除した。色空間用のボタングループID、設定ページと各タブの整数値は、設定、保存形式、XML、プラグイン、スクリプト、外部識別子へ渡らない。設定ダイアログの再表示位置は`KisDlgPreferences/CurrentPage`の文字列として保存される。選択アクションパネルは一時的なページ要求で一般設定のツールタブを開くため、実際の`KisMainWindow`を使う`kis_view_signals_test`へ統合し、設定アクション後に表示されるページとタブを検証する。専用CTest、広い依存定義、生成UIヘッダーの専用登録を削除した。統合先試験は変換マスクを生成する補助コードが使う完全型を直接取り込む。
- 完了: `KisActionEnumContractTest.cpp`を`KisActionCompatibilityTest.cpp`へ改名した。コアと29個のプラグインの`.action`定義は`activationFlags`と`activationConditions`を2進数文字列で保存し、`KisActionManager`が基数2で復元する。操作登録と拡張機能はこの値で有効状態を決めるため、各ビットの互換性要件と、変更時に影響する利用者を試験に明記した。Qtのフラグ演算を重複して検証していた文は削除した。
- 完了: `KisConfigEnumContractTest.cpp`を`KisConfigCompatibilityTest.cpp`へ置き換えた。既存の`kritarc`は入力、色採取、起動、色管理、背景、選択アクションバー、レイヤー表示、補助線描画の各モードを整数で保存するため、その値を明示的な互換性要件として維持した。`ColorSamplerPreviewStyle::Count`は保存値ではないため除外した。キャンバス色管理、表示ビット深度、ルート表示形式は`kritarc`と`kritadisplayrc`の文字列で保存するため、実際の保存値と再読込後のモードを検証する。専用の広い依存定義を、設定実装を提供する`kritaapplication`への直接依存へ縮小した。
- 完了: `KoDialogEnumContractTest.cpp`を`KoDialogContractTest.cpp`へ置き換えた。`ButtonCode`、`ButtonPopupMode`、見出しフラグの数値は保存形式、XML、プラグイン、スクリプト、外部識別子で使われていない。インポート、書出し、設定、復旧の各ダイアログは記号名のボタンとsignalを使い、利用者はボタンの表示状態、選択後のsignalと受理・取消結果、詳細領域の表示を観測する。実クリックでこれらを検証し、数値、レイアウトヒント、内部スロット呼出し、遅延破棄の固定を削除した。直接オブジェクト対象のリンクには、必要なヘルプ実装を加えた。
- 次の作業: 残存Schema試験を利用場面から監査する。次の`KColorSchemeEnumContractTest.cpp`では、配色役割の値が保存形式、画面状態、プラグイン、スクリプト、外部識別子で必要か確認する。R2-G19cの高速検査と対象CTestを再実行する。内部値型と初期化だけの固定は、保存形式や外部連携の根拠がある場合だけ保持する。
- 検証: macOSで`TestAngleSelector`と全依存の構築、ガイド・格子設定試験、色役割試験が成功した。対象試験の反復実行と`verify-quick`も成功した。
- 再発防止検証: macOSで`KisSignalCompressorContractTest`、`KisBezierPatchContractTest`、
  `KStandardActionCompatibilityTest`の構築とCTestが成功した。新しい検査を含む運用検査45件と
  `verify-quick`が成功した。
- 検証: macOSで`KoDialogContractTest`の構築と`libs-widgets-KoDialogContractTest`のCTestが成功した。

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
