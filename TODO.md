# LibrePaint iPadOS TODO

この文書は、KritaをiPadOS実機で起動し、Android版に近い主要描画機能を使える状態にするための実装計画である。

## 目標

- iPadOS 17以降のarm64実機で起動する。
- Apple Pencilとタッチで実用的に描画できる。
- KRA/ORA/PNG/JPEGの読み書きを行える。
- Android版にある主要なブラシ、ツール、Docker、フィルタを利用できる。
- 同一revisionから再現可能にビルドできる。

## 前提と非目標

- 最終ターゲットはiPad実機。Simulatorは初期診断用途に限る。
- Qt 6系を使用する。
- NixはOSS依存物とホストツールの固定に使い、XcodeはApple ClangとiOS SDKを供給する。ローカル開発署名と実機更新にはAltStore/AltServerも利用できる。
- 実機起動に必要な開発署名はAltStoreに任せる。署名情報をリポジトリへ保存せず、公証は行わない。
- 初期版ではPython/PyQt、G'MIC、動画・音声、印刷、自動更新、外部プロセス依存機能を対象外とする。
- 「プラグインを捨てる」は外部拡張機能を対象とする。ブラシ、ツール、画像入出力、DockerなどKrita本体機能を構成する内部プラグインは必要なものを静的リンクする。
- App Store、公式代替マーケットプレイス、一般配布、iPhone対応は対象外。

## 優先度

- **P0**: 次のマイルストーンを成立させるために必須。
- **P1**: Android版相当の主要利用体験に必要。
- **P2**: 初期版成立後に追加する機能または改善。

## マイルストーン一覧

| ID | マイルストーン | 成果物 | 目安 |
|---|---|---|---:|
| M0 | 方針固定とベースライン | 決定記録、機能表、既存ビルド確認 | 2～4日 |
| M1 | 再現可能なビルド基盤 | `flake.nix`、toolchain、Xcode検査 | 5～10日 |
| M2 | iOS向け最小依存セット | arm64/iOS静的ライブラリ群 | 10～20日 |
| M3 | Krita最小アプリのリンク | 起動可能な最小`.app` | 8～15日 |
| M4 | 内部プラグイン静的化 | 自動登録基盤、最小機能セット | 12～25日 |
| M5 | 実機起動と描画 | Pencilで描画できる実機ビルド | 10～20日 |
| M6 | ファイルとライフサイクル | Files連携、保存、復帰 | 8～15日 |
| M7 | Android版相当機能 | 主要ブラシ・ツール・Docker | 15～30日 |
| M8 | 安定化と性能 | 長時間利用可能な候補ビルド | 15～30日 |
| M9 | 再現可能な引き渡し | 1コマンドビルド、運用文書 | 5～10日 |

日数は単純加算しない。M2、M4、M5、M8がクリティカルパスである。

---

## M0: 方針固定とベースライン

### タスク

- [x] **P0** 対象ブランチ、Qt/KFのrevision、最低iPadOSバージョンを記録する。
- [x] **P0** Xcode、iOS SDK、Nixの要求バージョンを決める。
- [x] **P0** macOSまたは既存Androidビルドの手順を確認し、移植前の既知エラーを分離する。
- [x] **P0** Android版の機能を「必須・後回し・削除」に分類する。
- [x] **P0** 173件の`MODULE`定義（具体的な172ターゲットとテスト用テンプレート1件）をカテゴリ別に棚卸しし、初期版の静的リンク対象を選ぶ。
- [x] **P0** 必須依存と任意依存の一覧を機械可読な形で作る。
- [x] **P0** ADR（Architecture Decision Record）を作り、Nix/Xcode境界と静的リンク方針を記録する。
- [x] **P1** 既存Android固有コードのうち、iPadOSで再利用できるUI・タッチ対応を特定する（基準revisionからの全82コミット・309ファイルを再棚卸しし、`docs/ios/android-reuse-audit.md`に記録済み。upstream追従のため、本体での再利用は既存Android条件へ`Q_OS_IOS`を追加する一行変更だけに限定し、処理本体の共通化は行わない）。

### 完了条件

- [x] 初期機能セットと削除機能が明文化されている。
- [x] 必須依存、内部プラグイン、プラットフォームAPIの三つのリスク表がある。
- [x] Qt/KF/Xcode/iPadOSの組み合わせが一つに固定されている。

---

## M1: NixとXcodeによる再現可能なビルド基盤

### タスク

- [x] **P0** `flake.nix`と`flake.lock`を追加する。
- [x] **P0** `aarch64-darwin`用dev shellを作る。
- [x] **P0** CMake、Ninja、Python、pkg-config等のホストツールを固定する。
- [x] **P0** macOSホストツールとarm64/iOSターゲットライブラリを明確に分離する。
- [x] **P0** `CMAKE_SYSTEM_NAME=iOS`を設定するtoolchain/presetを追加する。
- [x] **P0** XcodeとiOS SDKのversionを検査し、不一致時に早期失敗させる。
- [x] **P0** deployment target、architecture、bitcode、visibility、dead stripping方針を固定する。
- [x] **P0** 実機用とSimulator用の出力ディレクトリを分離する。
- [x] **P1** ローカルNix binary cacheの利用手順を用意する。
- [x] **P1** CIなしでも実行できるビルドログ収集スクリプトを追加する。

### 完了条件

- [x] クリーンなshellから同じコンパイラ、SDK、CMake設定が選択される。
- [x] Mac用バイナリとiOS用ライブラリの混入を検査できる。
- [x] `nix develop`から最小のiOS Hello Worldをビルドできる。

---

## M2: iOS向け最小依存セット

### タスク

- [x] **P0** Qt 6 for iOSを取得またはソースビルドし、revisionと構成を固定する。
- [x] **P0** Qt Core/Gui/Widgets/Xml/Network/Svg/Concurrent/Sql/OpenGL系を検証する。
- [x] **P0** iOSで利用できないQt PrintSupportを必須依存から外す。
- [x] **P0** ECMと必須KDE Frameworksを静的ビルドする。
- [x] **P0** KF Config、WidgetsAddons、Codecs、Completion、CoreAddons、GuiAddons、I18n、ItemViews、ColorSchemeを個別検証する。
- [x] **P0** PNG、zlib、Boost、Immer、Zug、Lagerを構築する。
- [x] **P0** Eigen、Exiv2、LCMS2、xsimd、QuaZipを構築する。
- [x] **P0** FreeType、HarfBuzz、Fontconfig、libunibreakを構築する。
- [x] **P0** `try_run()`、ホスト実行コード生成器、pkg-config誤検出を修正する。
- [x] **P0** 各成果物を`file`、`lipo`、`otool`で検査し、iOS arm64以外を拒否する。
- [x] **P1** JPEGを追加する。
- [x] **P1** WebP/TIFFを追加する（依存をNixで固定し、iOS静的プラグインのリンクと実機保存を確認済み。TIFFはJPEG圧縮での保存・再読み込みも確認済み）。
- [ ] **P2** OpenEXR/GIF/HEIF/JPEG XL/RAW/Poppler/OpenColorIO/KSeExprを個別に再評価する（OpenEXRは依存固定・静的プラグイン同梱まで完了。残る7機能向けの16個の個別derivationを58依存aggregateへ統合し、KF6 consumer link、Krita arm64最終リンク、161件の静的登録、IPA検査、実機インストール・起動まで完了。GIF、HEIF、JPEG XLの保存・再読込とPDF読込はビルド`20260805070711`で実機確認済み。Nikon Z7のNEFはlibkdcrawがスタック上に確保した`LibRaw`でスタック上限を超えたためヒープ化し、修正ビルド`20260805080339`で同一NEFを8288×5520、16bit RGBA画像として実機読込済み。LUT Docker表示／適用とSeExpr generatorの実機確認を継続する）。

### 完了条件

- [x] 必須依存がすべてiOS arm64向けにリンク可能である。
- [x] Homebrewやホスト側`/usr/local`への暗黙依存がない。
- [x] 同じlock fileから依存物を再生成できる。

### 技術ゲート G1

KDE FrameworksまたはQt Widgets/OpenGLがiOS上で成立しない場合、Krita全体の作業を進めず、代替構成または対象機能縮小を判断する。

判定: **通過**。Qt Widgets/OpenGL、ECM、必須KF6、KConfigホストコード生成器を1本のiOS arm64アプリへ静的リンクできた。

---

## M3: Krita最小アプリのconfigure・compile・link

### タスク

- [x] **P0** `APPLE`分岐を`IOS`と`APPLE AND NOT IOS`へ分離する。
- [x] **P0** macOSパッケージング、RPATH、`.icns`、`-mmacosx-version-min`をiOSから除外する。
- [x] **P0** `qt_add_executable()`または同等のiOS bundle生成へ切り替える。
- [x] **P0** iOS用Info.plist、Bundle ID、向き、デバイス要件、アイコンを追加する。
- [x] **P0** `krita_version`等の補助実行ファイルをiOSビルドから外す。
- [x] **P0** PrintSupport、QProcess、Python、アップデータ、外部実行機能を条件付き無効化する。
- [x] **P0** `libs/macosutils`とmacOS Objective-C++コードをiOSから分離する。
- [x] **P0** 共有ライブラリを静的ライブラリまたはiOS対応frameworkへ変換する。
- [x] **P0** 最小main windowとリソースを含む`.app`を生成する。
- [ ] **P1** 起動ログをOSLogまたは標準的な実機ログへ転送する。

### 完了条件

- [x] 未署名またはad-hocの中間`.app`がリンクまで完了する。
- [x] iOS SDKに存在しないAPIやmacOS frameworkへのリンクがない。
- [x] 起動前の静的初期化でクラッシュしない（AltStore署名で実機検証済み）。

---

## M4: Krita内部プラグインの静的化

### タスク

- [x] **P0** `kis_add_library(... MODULE ...)`をiOS時に`STATIC`へ変換するCMake基盤を作る。
- [x] **P0** KPlugin factoryを衝突なく静的登録する仕組みを作る。
- [x] **P0** JSONメタデータをバイナリまたはリソースへ埋め込む。
- [x] **P0** `Q_IMPORT_PLUGIN`相当の登録コードを自動生成する。
- [x] **P0** linker dead strippingからfactoryとメタデータを保護する。
- [x] **P0** 有効なプラグインターゲット一覧から登録コードとリンク対象を生成する。
- [x] **P0** 最小セットのKRA import/exportを有効化する。
- [x] **P0** 最小セットのPNG import/exportを有効化する。
- [x] **P0** 最小セットのPixel Brushを有効化する。
- [x] **P0** 最小セットの基本Toolを有効化する。
- [x] **P0** 最小セットのLayer Dockerを有効化する。
- [x] **P0** 起動に必須のLittleCMSカラーマネジメントエンジンを有効化する。
- [x] **P0** 静的Qtリソースと実行時データの欠落を機械検査する。
- [x] **P1** 主要Tool/Dockerを46個の静的プラグイン構成へ拡張し、実機メニューとToolboxで確認する。
- [ ] **P1** JPEG/ORA、主要Brush、主要Tool、Brush Presets、Color Selectorを追加する。
  - [x] JPEG/ORAのimport/export 4プラグインをiOS向け共通変換ライブラリとともに静的リンクする（50プラグイン、ビルド`20260802145518`）。
  - [x] JPEGをネイティブFiles経由で保存・再読み込みし、USB回収した2480×3508、8-bit RGB画像を外部デコード・目視確認する（ビルド`20260802150920`、`NativeSaveTest2.jpg`）。
  - [x] ORAをネイティブFiles経由で保存・再読み込みし、USB回収後に`mimetype`、`stack.xml`、2レイヤーPNG、サムネイル、統合画像を含む全6エントリを検証する（ビルド`20260802150920`、`NativeSaveTest.ora`）。
- [x] **P1** プラグイン機能群ごとにON/OFFできるiOS feature profileを作る。
- [ ] **P2** 任意フィルタとDockerを段階的に追加する。

### 完了条件

- [x] 起動時に選択したプラグインだけが列挙・ロードされる。
- [x] Pixel Brush、基本Tool、Layer Dockerが実機で利用可能である。
- [x] KRA/PNG I/Oが実機で利用可能である（ビルド`20260802143848`でネイティブFiles経由の保存・再読み込みを確認済み）。
- [x] 新しい静的プラグインを一覧へ追加するだけで組み込める。

### 技術ゲート G2

最小プラグインセットを静的ロードできなければ、個別factory登録またはコアへの直接組み込みへ方針変更する。

---

## M5: iPad実機起動と描画

### タスク

- [x] **P0** AltStore/AltServerによるローカル開発署名を設定する。
- [x] **P0** ビルド、検査、IPA生成、実機更新、起動、ログ取得をスクリプト化する。
- [x] **P0** アプリ起動とmain window表示を確認する（追加依存16件と161件の静的プラグインを含むビルド`20260805070711`を実機確認済み。NEF修正とFill Layerの初回回避策を含む`20260805080339`、Fill Layer非モーダル化後の`20260805081535`（git `0e3c512`）も実機インストール・起動・ログ取得済み）。
- [ ] **P0** 正常終了と再起動を確認する。
- [ ] **P0** 最小キャンバスを作成し、指でストロークを描く。
- [x] **P0** Apple Pencilの位置、筆圧、傾き、方位、接触状態を記録・検証する（iPad8,1実機で`QTabletEvent`のpress/move/release、pressure、xTilt/yTiltを確認済み）。
- [x] **P0** 起動・ファイル表示直後から、ツールの再選択なしで最初のApple Pencilストロークを描画できることを実機検証する（初期Enter・Pencil入力デバイス切替を実装し、実機確認済み）。
- [ ] **P0** Pencil描画と指ジェスチャーを分離する。
- [ ] **P0** undo/redo、pan、zoom、rotateを実装・確認する。
- [ ] **P0** 高DPI、Safe Area、画面回転を修正する（キャンバスの高DPIは、iPad8,1実機でDPR 2と論理サイズの2倍の描画viewportを診断後、診断コードを除いたビルド`20260805122451`をインストールしてOpenGL ES 3.0動作と表示の鮮明化を確認済み。Safe Areaと回転後の再検証は未完了）。
- [ ] **P0** OpenGL/描画surfaceの作成、破棄、再作成を検証する（AltStore起動時のsuspended状態ではprobeを延期し、active遷移後に作成できることをiPad8,1実機で確認済み。Issue #6対応として、非Active中のQOpenGLWidget resize／Show／DPR変更、描画、projection uploadを保留し、復帰後にcontextを検証してresize・設定・画像全体を再送するiOS専用guardを実装し、arm64最終リンクまで完了。ビルド`20260806104631`で変更済みKRAを開いたままHome→Procreate→Kritaを往復し、PID `6682`維持、キャンバス全面復帰、Pencil描画再開、復元確認なし、新規IPSなしを実機確認済み。画面回転と休止中に文書を閉じる境界は未検証）。
- [ ] **P1 Apple Pencilダブルタップ対応**: KritaのQt iOS viewへ`UIPencilInteraction`を登録し、ダブルタップを既存のKritaアクションへ橋渡しする（Qtソースpatch不要。最小ブリッジを実装し、消しゴム切り替えを実機確認済み）。
  - [ ] iPadOSの`preferredTapAction`を読み、少なくとも「消しゴム切り替え」「直前のプリセットへ切り替え」「カラーパレット表示」「何もしない」を対応する。
  - [x] 消しゴム切り替えを、ペン先／消しゴム側の別プリセットを保持する`eraser_preset_action`へ接続する（実機確認済み）。
  - [ ] 直前プリセット切り替えを`previous_preset`へ接続する（実装済み・実機確認待ち）。任意アクション設定が必要になった場合のみ既存S-Pen設定のaction-name mapを再利用する。
  - [ ] Apple Pencil第2世代の実機で、起動後の初回接触前と通常描画後の両方について、1回のダブルタップにつきアクションが1回だけ発火し、描画中のストローク、筆圧、傾き、指ジェスチャーへ回帰がないことを確認する。
- [ ] **P1** hover対応iPadでPencil hoverを検証する。
- [ ] **P1** キーボードショートカットを確認する。

### 完了条件

- [ ] 実機で新規キャンバスを作成し、Pencil筆圧付きで描画できる。
- [ ] Apple PencilダブルタップがiPadOSの選択内容に対応するKritaアクションを実行する。
- [ ] pan/zoom/rotateと描画が競合しない。
- [ ] 10分間の連続描画でクラッシュや入力停止がない。

### 技術ゲート G3

主要対象iPadで安定したPencilイベントまたは描画surfaceが得られない場合、Qt patchまたはiOS native input bridgeを実装する。

---

## M6: ファイルアクセスとアプリライフサイクル

### タスク

- [x] **P0** Qt iOSの`UIDocumentPicker`をKritaのopen/saveダイアログとして有効化し、実機でopen/saveの両方を確認する（ビルド`20260802143848`でFilesから`Untitled.kra`を開き、`NativeSaveTest.kra`をFilesへ保存して確認済み）。
- [ ] **P0** open/import/export/save/save-asの動線をiPadOS向けに整理する。
- [x] **P0** security-scoped URLの開始、終了、bookmark保持をQt iOSの標準file engineへ委譲する。`UIDocumentPicker`とFilesからの`QFileOpenEvent`が同じengineにURLを登録し、Qt 6.11.1がアクセス期間と永続bookmarkを管理する。
- [x] **P0** ユーザーファイルの既定位置を現在のアプリコンテナの`Documents`へ固定し、AltStore再インストール後の古い絶対パスから自動回復する。
- [x] **P0** `UIFileSharingEnabled`と`LSSupportsOpeningDocumentsInPlace`を有効化し、デプロイ時に両設定を機械検査する。
- [ ] **P0** inbox、temporary、Documents、cacheの用途を完全に分離する。
- [x] **P0** KRA/PNG/JPEG/ORAの読み書きを実機で検証する（ネイティブFiles経由の保存・再読み込み、USB回収後の外部検証、クラッシュレポート非生成を確認済み）。
- [x] **P0** 新規KRAを実機の`Documents`へ保存し、USBで回収したKRAのZIP構造と主要エントリを検証する（ビルド`20260802143408`）。
- [x] **P0** ネイティブSaveで保存したKRAをUSBで回収し、全ZIPエントリが破損していないことを検証する（ビルド`20260802143848`、`NativeSaveTest.kra`、13エントリ）。
- [x] **P0** PNGをネイティブSaveで`Documents`へ保存し、USBで回収後に2480×3508、8-bit RGBAとしてデコード・目視確認し、FilesからKritaへ再読み込みする（ビルド`20260802143848`、`NativeSaveTest.png`）。
- [x] **P0** JPEG保存後のサムネイル生成でQt同梱版と外部版の静的libjpegが衝突するクラッシュを、iOSではQt同梱版へ統一して修正する。JPEGの保存・再読み込み、USB回収後の外部デコード、クラッシュレポート非生成を確認する（ビルド`20260802150920`、`NativeSaveTest2.jpg`）。
- [ ] **P0** background移行時に全変更documentのrecovery checkpointを非同期かつ直列に作成し、既存saveへの合流、保存中の追記、失敗、background task期限切れを処理する（Issue #6対応として同期`exportDocumentSync()`を完了signal型APIへ置換し、UIKitの4段階通知とbackground taskを実装してarm64最終リンクまで完了。ビルド`20260806104631`で文書0件の完了に加え、変更済みKRA 1件を`.Untitled.kra-autosave.kra`へ216,501 bytesで保存し、checkpoint `success: true`を実機確認済み。通常autosave中／保存中の追記／失敗・期限切れ／複数documentの実機検証待ち）。
- [x] **P0** foreground復帰時にcanvas/GPU/resourceを復元する（OpenGL contextがcurrentになるまでqueued retryし、保留resize・renderer設定・全画像projectionを復元する実装と、iOS操作UIの非Active中layout抑止を追加。ビルド`20260806104631`で変更済みKRAを開いたままKrita→Home→Procreate→Kritaを往復し、PID `6682`維持、4段階通知、キャンバス全面復帰、Pencil描画再開、復元確認なし、新規IPSなしを実機確認済み）。
- [x] **P0** `UIApplicationDidReceiveMemoryWarningNotification`を受け、使用中でないtileをswapへ退避してtile allocatorとQt pixmapのcacheを解放する。
- [ ] **P0** 強制終了後のautosave recoveryを検証する。
- [ ] **P1** Filesから「共有/開く」でKritaへ渡すDocument Type（KRA/ORA/PNG/JPEG）を設定し、実機でcold/warm launchの双方を確認する（実装済み、実機確認待ち）。
- [ ] **P1** iCloud Drive上のファイルで競合・遅延を検証する。

### 完了条件

- [ ] FilesからKRAを開き、編集して安全に保存できる。
- [ ] background/foregroundを20回繰り返してデータ消失やクラッシュがない。
- [ ] 強制終了後にautosaveから復旧できる。

---

## M7: Android版相当の主要機能

### 必須機能セット

- [ ] **P0** Pixel Brush、Eraser、基本Brush Presets。
- [ ] **P0** Layer追加・削除・並べ替え・可視性・opacity・blend mode。
- [ ] **P0** Undo/Redo、selection、move、transform、crop、fill、gradient、textの基本動作。
- [x] **P0** KRA/ORA/PNG/JPEG import/export（基本的な保存・再読み込みをビルド`20260802150920`で実機確認済み。KRA内の任意フィルター互換性は次項で継続する）。
- [ ] **P1** Android版の追加ファイル形式・メタデータ（CSV、SVG、XCF、PSD、QML、TGA、Heightmap、Brush、Spriter、KRZ、RGBE、EXIF/IPTC/XMP）を実機でopen/save/reopenする（WebP、TIFF、JPEG 2000、OpenEXRを追加済み。GIF、HEIF、JPEG XL、RAW、PDFも161件の静的profileへ追加し、58依存bootstrap、arm64最終リンク、IPA検査、実機インストール・起動まで完了。PSDとTIFFのJPEG圧縮保存・再読込、GIF・HEIF・JPEG XLの保存・再読込、PDF読込は実機確認済み。libkdcrawのスタック上限超過修正後、同一Nikon Z7 NEFの8288×5520、16bit RGBA読込も実機確認済み。その他形式の確認を継続する）。
- [x] **P0** KRAが参照する調整レイヤー・フィルターを棚卸しし、Android版相当として残す内部フィルタープラグインを決める（追加依存なしで構築できる33 filterと6 generatorはarm64リンク・登録・ランタイムデータ検査、および`invert`を含む実機KRAの保存・再起動・再読込を確認済み。7番目のSeExpr generatorもKSeExpr依存、静的登録、arm64最終リンク、実機起動まで完了し、generatorの実操作確認を残す）。
- [ ] **P0** Layer、Brush Presets、Tool Options、Advanced Color Selector Docker。
- [ ] **P0** canvas-only modeまたはiPad向け省スペース配置（iOS専用の静的`kritaiostouchui`を追加し、4本指タップで切り替わる既存canvas-only actionに連動する上部ツールバー、左側のbrush size／opacity、Brush Library、Layer HUDを実装した。Canvas-only中のiPad操作UIはViewメニュー、上部の非表示ボタン、Safe Area内の48pt復帰ボタンで切り替えられ、設定画面から「前回状態を復元」「常にiPad操作UI」「常に完全キャンバス」を選択できる。162件の静的profile、arm64コンパイル・最終リンク・factory登録、action registryのIPA梱包をビルド`20260806094342`で機械検査し、実機へ更新インストール・起動して最新sessionに新規error／assertがないことを確認済み。canvas／overlay上の4本指切替、描画、回転、Brush／Smudge／Eraser／Layers／Color／Undo／Redo、UI表示切替と3つの入場設定、およびpanel dismissal時の誤描画がないことを確認待ち）。
- [ ] **P1** Clone、Filter Brush、Colorize、Assistant等の主要ブラシ・ツール（MyPaintを含む14 paintopを静的プロファイルへ追加し、arm64リンク・登録・画像リソースを機械検査済み。MyPaint presetはクリーンインストール後の登録を実機確認済み。Color Smudge、Spray、Hatching、Filter Brush（Invert）を含む主要engineの実機描画を確認済み。Colorize Toolの実機機能確認を継続）。
- [ ] **P1** 基本フィルタとgenerator（33 filterと従来の6 generatorについて、構築設定・静的登録・パッケージデータ、および実機でのフィルター／generatorレイヤー作成とKRA open/save/reopenを確認済み。SeExpr generatorもKSeExpr依存、静的登録、arm64最終リンク、実機起動まで完了。PencilからFill Layerを作成する操作は、iOSのみノード作成をqueued connectionにしても、`QDialog::exec()`のネストしたイベントループが未完了のtouch処理へ再入し、ビルド`20260805080339`の`QGestureManager::getState`で再度クラッシュした。iOSの作成ダイアログを非同期`QDialog::open()`へ変更し、OK時の設定・名称変更・stroke確定とCancel／破棄時のstroke取消を保った。責務確認のためqueued connectionを除いたビルド`20260805102358`は、KRA読込後のPencil releaseで再び`QGestureManager::getState`のnull参照によりクラッシュしたため、ノード／UI変更の遅延と非同期dialogは別々の再入区間を防ぐことを確認した。queued connectionを復元したビルド`20260805102943`を実機配備済み。Fill Layer作成、SeExpr選択、KRA save/reopenの再確認待ち）。
- [ ] **P1** OpenColorIO依存のLUT Dockerを静的profileへ追加する（OpenColorIOと推移依存の個別derivation、config-mode検索、GLES経路、静的登録、arm64最終リンク、実機起動まで完了。実機の`設定 → ドッキングパネル → LUT Management`からDockerを表示できるか、LUTを適用できるかの確認待ち）。
- [ ] **P1** resource bundleのimport/export（Android相当のResource ManagerをiOS静的プロファイルへ追加し、実機確認待ち）。
- [ ] **P1** Bluetooth/USBキーボード操作。
- [ ] **P2** アニメーションUI。ただし動画・音声exportは対象外。

### UI調整

- [ ] **P0** Android版のタッチ用設定をiOS profileとして再利用する。
- [x] **P0** `QComboBox`の選択をタップで確定・閉鎖できるようにする（UIKit pickerをiOSで無効化し、実機検証済み）。
- [x] **P0** 起動時の画面向き確定後にスプラッシュを画面内へ縮小・再配置する（縦向き・横向きとも実機検証済み）。
- [x] **P0** `Configure LibrePaint`起動時に検索欄へ自動フォーカスせず、ソフトウェアキーボードを抑止する（実機検証済み）。
- [ ] **P0** `Configure LibrePaint`を初期画面向き・画面回転・Split View後の利用可能領域へ再配置し、横向きと縦向きの双方で切れないことを実機検証する（初回縦向き・回転後は実機検証済み、Split View確認待ち）。
- [x] **P0** 全`QAbstractScrollArea`を指のスワイプと慣性スクロールに対応させ、ドラッグ中の項目選択を抑止する（設定画面とブラシ選択を実機検証済み）。
- [x] **P0** ブラシなど編集可能な一覧項目の選択だけではソフトウェアキーボードを表示せず、明示的な文字編集時だけ有効化する（実機検証済み）。
- [x] **P0** Android版相当の主要ToolとDockerを静的リンクし、実機のToolboxとDockerメニューで表示を確認する（46プラグイン、ビルド`20260802140547`）。
- [ ] **P0** 小さすぎるmenu、dialog、slider、spinboxのtouch targetを修正する。
- [ ] **P0** modal dialogとソフトウェアキーボードの重なりを修正する。
- [ ] **P1** Split ViewとStage Managerでlayoutを検証する。
- [ ] **P1** 外部ディスプレイ接続時の挙動を確認する。

### 完了条件

- [ ] 定義したAndroid版相当のP0機能チェックリストをすべて通過する。
- [ ] 主要操作にマウスを必要としない。
- [ ] 初回起動から保存までの操作に行き止まりがない。

---

## M8: 安定化、性能、容量

### タスク

- [ ] **P0** Instrumentsでmemory、CPU、GPU、hangを計測する。
- [ ] **P0** 2K/4K/8Kキャンバスと複数レイヤーの上限を記録する。
- [ ] **P0** tile/cache/thread数をデバイスメモリに合わせて調整する。
  - [x] iOSで物理RAMを取得し、tile予算の既定値を25%・最大1 GiB、手動上限を37.5%・最大1.5 GiBへ制限する。旧設定の過大値は起動時にclampして保存する。
  - [ ] `os_proc_available_memory()`と実測値を使ったデバイス別の動的調整を評価する。
- [ ] **P0** memory pressure時の段階的cache削減を実装する。
- [ ] **P0** 実機でmemory pressureを発生させ、警告受信後のtile退避、描画の一時停止時間と復帰、未保存データの保持、Jetsam終了の有無を検証する。
- [ ] **P0** 起動時間、初回brush表示、KRA保存時間の基準値を作る。
- [ ] **P0** 1時間連続描画テストを実行する。
- [ ] **P0** suspend/resume、回転、Split View、低ストレージの回帰テストを作る。
- [ ] **P0** static pluginとresourceを削減し、アプリ容量を確認する。
- [ ] **P0** crash logと再現手順を保存する運用を作る。
  - [ ] 収集したcrash log、Jetsam report、`krita.log`を実機検証記録へ関連付けて保存する。
- [ ] **P1** Address Sanitizer/Undefined Behavior SanitizerをSimulatorまたは対応構成で実行する。
- [ ] **P1** Kritaの非GUI単体テストをiOS互換範囲で実行する。
- [ ] **P2** battery/thermal throttlingを長時間試験する。

### 完了条件

- [ ] 対象デバイスで1時間の描画・保存を完走する。
- [ ] 既知のデータ消失バグがない。
- [ ] P0回帰テストを連続3回通過する。
- [ ] サポートするキャンバスサイズとメモリ上限が文書化されている。

---

## M9: 再現可能な自前ビルドと運用

### タスク

- [x] **P0** 新規checkoutからのbootstrap手順を自動化する。
  - [x] 固定中は旧closureを保護せず、全依存の固定・commit後にだけ既知のrepository-local GC rootを解除、full GC、通常Git flakeの`ios-dependencies` build、成功したaggregateのroot化を順番に行うスクリプトを追加する。
  - [x] commit `e8ba4dc`から`bootstrap-ios-dependencies.sh --confirm-pinning-complete`を実行し、full GC（1,808 store paths、4,627.38 MiB削除）後に通常Git flakeから31依存aggregateを再構築し、KF6 consumerの完全なiOSコード生成・リンク検査を通して最終aggregateだけをroot化することを確認する。
- [x] **P0** `nix build`で依存物を再生成できるようにする。
  - [x] zlibを独立した`zlib-ios` derivationへ移し、Xcode/SDK/Clangの完全なbuild identity、arm64/IOS archive検査、決定的再ビルド、ローカルbinary cache投入を検証する。
  - [x] libpngを`libpng-ios` derivationへ移し、zlibへのstore依存、決定的再ビルド、binary cache復元を検証する。
  - [x] FreeTypeを`freetype-ios` derivationへ移し、zlib/libpngの複数store依存、固定feature contract、決定的再ビルド、3-path binary cache復元を検証する。
  - [x] Expatを`expat-ios` derivationへ移し、XML feature contract、CMake/pkg-config consumer、決定的再ビルド、binary cache復元を検証する。
  - [x] HarfBuzzを`harfbuzz-ios` derivationへ移し、FreeTypeの推移依存、CoreText bridge、再配置可能なCMake export、4-path binary cache復元を検証する。
  - [x] Little CMSを`lcms2-ios` derivationへ移し、thread対応、Xcode SDK pathを含まないCMake export、consumer、決定的再ビルドを検証する。
  - [x] Eigenを`eigen-ios` derivationへ移し、header-only CMake targetを使うiOS C++ consumerと決定的再ビルドを検証する。
  - [x] Fontconfigを`fontconfig-ios` derivationへ移し、共通Autotools sandbox、Expat/FreeTypeの推移依存、pkg-config consumer、決定的再ビルド、5-path binary cache復元を検証する。
  - [x] xsimdを`xsimd-ios` derivationへ移し、header-only CMake targetを使うiOS SIMD consumer、決定的再ビルド、binary cache復元を検証する。
  - [x] libunibreakを`libunibreak-ios` derivationへ移し、KritaのCMake検索契約、UTF-8改行APIを使うconsumer、決定的再ビルド、binary cache復元を検証する。
  - [x] libjpeg-turboを`libjpeg-turbo-ios` derivationへ移し、2つのstatic target、arm64 NEON、決定的なbuild identity、独立consumer、binary cache復元を検証する。
  - [x] Exiv2を`exiv2-ios` derivationへ移し、library-only feature、PNG support、JPEG/Exif/文字コード変換APIのcompile/link、zlibの推移依存、決定的再ビルド、binary cache復元を検証する。
  - [x] BoostをXcode非依存の`boost-ios` header derivationへ移し、relocatable CMake targets、Apple Clang iOS consumer、決定的再ビルド、binary cache復元を検証する。
  - [x] ImmerとZugをXcode非依存のpure header derivationへ移し、固定version metadata、plain CMake targets、C++14契約、C++17 Zug skip経路、決定的再ビルド、binary cache復元を検証する。
  - [x] Lagerを依存伝播対応のpure header derivationへ移し、欠落していたBoost依存、C++17/plain target/version契約、Krita実利用API、決定的再ビルド、3-path単体closureを検証する。
  - [x] libintlをgettext-runtime内の最小static runtime derivationへ移し、固定cross cache、SDK iconv/CoreFoundationを使う実リンク、決定的再ビルド、binary cache復元を検証する。
  - [x] FriBidiを共通Meson sandboxの最初の`fribidi-ios` derivationへ移し、7個のnative macOS generatorと18-object iOS runtimeの分離、deprecated API互換、Kritaの`FindFriBidi.cmake`を使うconsumer、決定的再ビルド、binary cache復元を検証する。
  - [x] 残るC/C++依存をパッケージ単位のderivationへ移し、18個の基礎依存を`ios-dependencies`で統合する。
  - [x] Qt 6/QuaZipの4 derivationと必須KF6の9 derivationを段階化し、31依存aggregateへ統合する。
  - [x] GIF、HEIF、JPEG XL、RAW、PDF、OpenColorIO、KSeExpr向けの16 derivationを追加し、58依存aggregateへ統合する（exact-set、source lock、推移依存を検査し、`bootstrap-ios-dependencies.sh --confirm-pinning-complete`によるaggregate構築・root化、KF6 consumer link、161件の静的profileを含むKrita arm64最終リンクを完了。NEFプレビュー用`LibRaw`のヒープ化patch追加後も、58依存aggregateの再bootstrap、KF6 consumer link、`libkdcraw-ios`の決定的再ビルド、Krita arm64最終リンクを再確認済み）。
  - [x] Krita本体と未署名IPAを段階的なderivationへ移す。
    - [x] 固定済み31依存の上で、初期静的プラグイン50 targetとruntime dataを含むarm64/iOS 17.0アプリを`krita-ios-app`として構築し、SDK 26.5、plist、未署名状態、リソース、build/Xcode path非混入を検査する。
    - [x] 分離したQtSvg prefixから`QSvgPlugin`と`QSvgIconPlugin`をKritaへ明示リンクし、旧実機ビルドと同じ59個の静的プラグイン集合をNix install checkで保証する。
    - [x] 修正版Nix IPAを実機へ手動インストールし、Toolboxの全Tool、ツールバーのSVGアイコン、Dockerメニューが旧実機ビルド相当であることを確認する（ビルド`20260804040455`で実機確認済み）。
    - [x] timestampとentry順を正規化した`krita-ios-ipa`を追加し、ZIP整合性、必須ファイル、署名/Finder metadata非混入を検査して87 MiBのIPA生成を確認する。
    - [x] Nix Storeの`0555`/`0444`がIPAへ漏れてLiveContainerの一時`Payload`を削除不能にする問題を修正する。Nix版と増分deploy版は同じcheckerを使い、stageをdirectory `0755`、data file `0644`、main executable `0755`へ正規化する。symlink/special file、Finder/signing/ZIP extra metadata、unsafe path、Unix type/mode、DOS read-only属性、stage/archive inventory差を拒否する軽量回帰checkも追加した。旧IPAが同じFoundation errorを再現すること、両packaging coreの修正版をpermission保持展開した後は`Payload`を削除できることをhost上で確認済み。
    - [x] 権限修正版Nix IPA（SHA-256 `3eca1fb633daaeff836314b8cd72f57252befc4673cd8f74f7ba69a9672ac0f7`）をAltStore経由で実機へ直接インストールし、署名後bundle `org.krita.ipad.port.PUDY4GHY3Y` version `6.1.0`の起動と`krita.log`取得を確認する（2026-08-06）。これはLiveContainer import検証とは別。
    - [ ] LiveContainer内に旧失敗importのread-onlyな一時`Payload`が残っている場合は、そのstale stateの安全な解消手順を実機で確認する。修正版IPAの新規importとLiveContainer側のiOS 26 JIT-Less設定後の起動は、`Payload`削除エラーなしで実機成功済み（2026-08-06）。旧stale stateの安全な解消と同一bundle IDの再importは未確認。
    - [x] Nix recipe、生成物、移植文書、TODOをKrita compilation sourceから除外し、IPA/文書変更がKrita本体の再ビルドへ波及しないcache境界を固定する。
  - [x] Darwin daemonの`allowed-impure-host-deps`へXcodeだけを追加し、derivationの`__impureHostDeps`宣言、`sandbox = true`、cache-miss再ビルドの順に有効化する。
  - [ ] 署名付きprivate binary cacheを設定し、別の隔離storeまたはMacから復元確認する。
- [x] **P0** 現在のビルドツリーからconfigure、build、検査、AltStore更新、起動、ログ取得を1コマンド化する。
- [x] **P0** Nixで固定した依存・ツールチェーンを使う永続Ninjaツリーを追加し、通常のKritaソース変更を増分ビルドできるようにする。
  - [x] ソース非依存の`krita-ios-incremental`環境を固定profileとして記録し、通常編集ごとのflake再評価と約575 MiBのKritaソースsnapshot生成をなくす。`krita-ios-app`固有phase hookのstring contextがprofileからsourceを保持していた漏れも除去し、profile closure全体の`*-krita-ios-source`不在を作成時・再利用時に検査する。修正後のprofile closureは2.7 GiBから2.1 GiBへ587.7 MiB縮小した。
  - [x] 初回3370工程のbaseline構築後、無変更時0工程、単一pluginソース変更時は予定5工程・実行4工程（対象object、archive、最終app link）となり、無関係なarchiveが更新されないことを確認する。
  - [x] 200工程を超える予期しない再ビルドを標準で拒否し、初回・構成変更時だけ明示的な`bootstrap`を要求する。純粋な`nix build .#krita-ios-ipa`はcheckpoint/release検証用として残す。
  - [x] Nix app recipeと同じCMake cache契約を増分configureでも検査し、`KoConfig.h`のbuild pathを`/build`へ正規化してローカル絶対パスを最終binaryへ埋め込まない。既存baselineは一度だけ1958工程で補正し、以後は正規化後の内容が同じならheader timestampを維持して再configureによる再発を防ぐ。
  - [x] デプロイ後のcache保守をdirty flakeの評価から分離し、CMake/Ninja graphが参照する53個のStore入力だけを検証・root化することで、編集後の実機更新でもKritaソースsnapshotを生成しない。
- [x] **P0** 旧`Krita-iPad-*`と現`LibrePaint-iPad-*`を共通のbundle-version順で扱って直近3件を残すIPA整理と、runtimeおよびcache-deployment closureをGC rootで保護した低容量時Nix GCを自動化する。debug logとscreenshotは整理対象外とする回帰テストを追加した。
- [ ] **P0** configure、build、development sign、install、log取得を個別コマンドにする。
- [ ] **P0** Xcode/SDK更新時の検証手順を作る。
- [ ] **P0** upstream追従時のrebase/checklistを作る。
- [ ] **P0** 使用パッチ、削除機能、既知制約を文書化する。
- [ ] **P0** GPL/LGPL対象ソース、patch、build recipeを保持する。
- [x] **P0** iOSの非コードdata installを、機能設定、明示ライセンスの31 ICC profile、帰属を同梱した監査済みresource bundleへ限定し、CC-BY/CC-BY-SA/CC0/GPL/LGPL/ICC本文と既存帰属資料を`share/doc/librepaint/non-code-licenses`へ同梱する。曖昧な3 ICCとsponsor logoのQRC組込みを除外し、既定bundle全281 fileをCC0 244件・CC-BY-3.0 37件へ分類した。さらに静的依存resourceを明示ライセンスの7群253 fileへ固定し、Qt標準の未改変`aboutQt` logoは正当な依存帰属として保持し、未使用`kcharselect_data`群だけをiOS linkから除外するexact-set audit gateを追加した。2026-08-07に実機へのインストールと初回起動を確認済み。`aboutQt`表示を含む各機能画面は未確認。
- [x] **P0** 1,289件のrepository-owned QRC画像と32件のinstall-only画像を閉集合検査する。作者本人がCC0-1.0を選択したLibrePaint原本4件をハッシュ固定し、使用中のアプリケーション、文書、Qt runtime、log、PDF fallback、resource bundle preview、4K/HD splashの各ブランド面へ派生した。不要なupstream branding、fundraising、sponsor、news、disabled-tool画像はresource/UI参照とともに削除し、QRC内の白placeholderとzero-byte aliasを0件にした。現在のQRC分類は、直接CC-BY-SA-4.0を確認した703件、追加根拠を固定したCC-BY-SA-4.0機能画像93件、Breeze/Oxygen LGPL画像319件、Android画像2件、KXmlGui画像1件、project-wide GPL fallback機能画像148件、LibrePaint CC0ブランド画像23件で、未分類は0件。install-only 32件は19個のCMake定義から再発見し、未分類0件を維持する。source-wide white manifestの置換対象も0件で、macOS DMG背景はCC0のLibrePaintスプラッシュを流用する。未到達のbug-report／About KDE実装とAbout KDE画像を削除し、iOSで消費しないLinux desktop alias 24件と非搭載Storyboard docker用workspace 1件をruntime installから除外した。arm64最終リンク後に配布手順と同じローカル`.app`を再ステージし、204件のruntime dataとの一致とdesktop alias不在を機械検証した。ビルド`20260807151823`を2026-08-08に実機へインストール・起動し、QuickTime captureで単一表示のLibrePaintロゴ、About画面、Dock上の新アイコンを確認済み。今回の追加削除後は2026-08-09にhost上の3,453工程baseline build、arm64最終リンク、runtime再ステージまで再確認済みで、実機buildと全画面の網羅的な目視検査は継続する。
- [x] **P0** C/C++/Objective-C++のgettext抽出msgid、翻訳対象`.ui`文字列、`.action`/XMLGUIの表示要素、plugin JSONの表示フィールドを対象に、旧ブランドの再混入を拒否するユーザー表示監査gateを追加する。例外理由はinlineの`upstream-attribution`と`format-history`だけに固定し、増分buildとNix `postConfigure`の双方へ組み込む。
- [ ] **P0** KRA/MIMEとplugin/action/config IDは互換契約として維持する。ユーザー表示監査と分離した次段階で、`krita` target・executable・`share` pathなどのbuild/package内部名を棚卸しし、互換契約でないものだけを段階的にrenameする。必要な箇所はaliasまたはmigrationを先行させる（未着手）。
- [x] **P0** CC-BY-NC-ND-4.0のNetflix JPEG XL source fixture、派生expected result、専用test case、dataset固有license fileを削除し、制限付きdatasetへの参照をなくす。
- [ ] **P1** source-onlyのtest/benchmark fixtureを含むREUSE/DEP5相当の資産台帳を作り、残る権利対応が不明なfixtureと個別対応のないUI iconを除去・再作成・正確な帰属維持のいずれかに分類する。
- [ ] **P1** dependency SBOMとライセンス一覧を生成する。
- [ ] **P1** private binary cacheの復旧手順を記録する。

### 完了条件

- [ ] クリーンなforkから文書どおりに実機ビルドを再現できる。
- [ ] 別の開発セッションでも手作業の未記録操作を必要としない。
- [ ] 既知問題、対象外機能、対応端末条件がREADMEに記載されている。

---

## 初期プラグイン候補

正確なtarget名はM0で棚卸しして確定する。

### P0で残す

- KRA、ORA、PNG、JPEG import/export
- Pixel BrushとEraserに必要なpaintop
- Freehand、Line、Rectangle、Ellipse、Move、Transform、Crop、Fill、Gradient、Selection系tool
- Layer、Brush Presets、Tool Options、Advanced Color Selector docker
- 基本色管理とresource loader
- KRAで使用する基本filter/generator

### 初期版から外す

- Python/PyQt
- G'MIC
- PrintSupport
- FFmpeg/MLT/SDLと動画・音声export
- AppImage updater等の更新機能
- bug reportの外部process起動
- Qt Designer plugin
- X11/Wayland/Windows/macOS/Android固有platform plugin
- SVG Text Tool／Text PropertiesとStoryboard（依存追加とリンクだけでは成立せず、Krita本体側のplatform対応・PrintSupport分離が必要）
- 外部実行ファイルに依存する機能

## 横断的な完了基準

各タスクを完了扱いにするには、原則として次を満たす。

- [ ] コードまたは設定がforkにcommit可能な形で存在する。
- [ ] 再現コマンドが記録されている。
- [ ] 成功ログまたはテスト結果がある。
- [ ] 新しい手動前提がREADME/ADRに記録されている。
- [ ] macOS/Android側への意図しない回帰がない、または影響が明記されている。
- [ ] 一時的な回避策には削除条件と追跡TODOがある。

## 直近の実行順序

1. M0の対象version・機能セット・プラグイン棚卸しを確定する。
2. M1でflakeとiOS Hello Worldを成立させる。
3. M2では全依存を一度に作らず、Qt → KF → Krita必須C/C++ライブラリの順で追加する。
4. M3でプラグインなしの最小Krita shellをリンクする。
5. M4でKRA/PNG、Pixel Brush、Layer Dockerだけを静的登録する。
6. G1～G3を通過してからAndroid版相当機能を追加する。
