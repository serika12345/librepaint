# LibrePaint

[English](README.md)

LibrePaintは、Krita由来のコードベースを独立して開発・保守する、クロスプラットフォームのデジタルペイントアプリケーションです。共通のアプリケーション動作、UI、ワークフロー、ブランド、パッケージング、各OSとの統合を含め、コードベース全体をLibrePaintという一つの製品として改造し、デスクトップとモバイルの各プラットフォームへ提供することを目指します。

現在もっとも検証が進んでいるのはiPadOS版です。arm64実機向けの再現可能なビルドと配備、Apple Pencilとタッチ入力、Files連携、主要な描画ワークフローを実機で確認しています。他ターゲットのソースとパッケージング経路は、下表に示す段階まで整備されています。

> [!WARNING]
> LibrePaintは開発途中であり、完成度と検証範囲はプラットフォームごとに異なります。以下の表に、これまで実施した操作経路を具体的に記録しています。重要な作品は別の場所にもバックアップしてください。

## プロジェクトの方向性

| 項目 | 方針 |
|---|---|
| 製品の範囲 | 共通componentとplatform固有componentを持つ一つのアプリケーションとしてLibrePaintを開発・保守する |
| 対象プラットフォーム | Windows、macOS、Linux、Android（ChromeOS上のAndroid環境を含む）、iPadOS、およびKrita／Qtコードベースで対応可能な追加ターゲット |
| 共通開発 | 適切な機能とUXは共通コードで実装し、必要な箇所だけ各プラットフォーム固有の統合を行う |
| 互換性 | 作品、resource、安定した技術識別子を意図的に維持し、非互換migrationは明示する |
| 配布目標 | 各プラットフォームに適したbuild、package、検証、提供経路を整備する |

以下のplatform statusでは、roadmap上の対象と検証済みの提供範囲を分けて記載します。現在の完成度、機能範囲、提供経路はプラットフォームごとに異なります。

## プラットフォーム別の現在地

| プラットフォーム | 現在のリポジトリ上の状態 | 現在の検証状況 |
|---|---|---|
| iPadOS | 固定済みNix／Xcode環境、未署名IPA生成、AltStore／LiveContainer配備経路 | もっとも開発が進んでいる対象。iPadOS 17以降のarm64実機で詳細に検証中 |
| Android／ChromeOS | LibrePaint APK構成と[ローカルビルドガイド](README.android.md) | 次のgateはdependency prefixの準備とend-to-end実機検証 |
| Linux | [ローカルAppImage scriptとガイド](packaging/linux/appimage/README.md) | 次のgateはdependency prefixの準備、公開経路と署名の検証 |
| macOS | LibrePaint app bundleのNix recipeと既存のDMG packaging経路 | nixpkgsのLLVM toolchainとSDKによるarm64 clean buildとアプリケーション起動を確認済み。次は対話的UIと配布工程を検証 |
| Windows | LibrePaint executableとNSIS installerのpackaging経路 | 次のgateはclean build、installer、署名、end-to-end検証 |

現在のiOS対応範囲はiPadです。他のApple form factorや追加プラットフォームは、それぞれのUI、build、packaging、実機検証を経て対応状況を更新します。

既存のCMake target、設定directory、KRAのMIME／UTI、plugin ID、action IDは互換性契約として維持します。

## 現在のiPadOS開発状況

以下は2026年8月9日時点のiPadOS workstreamの状態です。iPadOSの進捗と実機検証結果の正本は[`TODO.md`](TODO.md)、iPadOSへ静的に組み込む機能の正本は[`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json)です。

### 実機で確認済み

| 分野 | 確認済みの範囲 |
|---|---|
| 起動と基本UI | AltStore経由のインストールと起動、LiveContainerへのIPA新規importと起動、main window、縦向き・横向きのsplash、初期縦向きと回転後のConfigure LibrePaint dialog |
| タッチUI | 非canvas領域でのスワイプによる慣性scrollとタップによる項目選択、combo boxのタップ確定、明示的な文字編集から始まるtext-entry focus |
| Apple Pencil | press・move・release、筆圧、傾き、初期選択済みbrushによる即時描画 |
| Pencil double tap | `eraser_preset_action`による、独立したペン側／消しゴム側brush presetの切り替え |
| ファイル | iPadOS Filesによるnative open/save、KRA・PNG・JPEG・ORAの保存、再読込、回収後の外部検査 |
| Brush engine | Pixel Brush、MyPaint presetの登録、Color Smudge、Spray、Hatching、Filter Brush（Invert）の描画 |
| ToolとDocker | 主要ToolのToolbox表示、主要Dockerのmenu表示、Layer Dockerなどの基本利用 |
| FilterとGenerator | 33 filterと従来の6 generatorの登録、layer作成、KRAのopen/save/reopen |
| Canvas表示 | DPR 2の高DPI表示、OpenGL ES 3.0によるcanvas描画 |
| App lifecycle | 同じprocessを維持したbackgroundからの復帰、canvas全面の復元、Pencil描画の再開、変更済みKRA 1文書のrecovery checkpoint作成 |

「確認済み」は記載した操作経路を特定の実機で通過したという意味です。検証範囲は、記録済みの設定、ファイル内容、端末、試験時間に限られます。

### ファイル形式

| 状態 | 形式と確認範囲 |
|---|---|
| 基本round trip確認済み | KRA、PNG、JPEG、ORA |
| 限定的な実機確認済み | WebPの保存。PSD、GIF、HEIF、JPEG XLの保存・再読込。TIFFはJPEG圧縮での保存・再読込を確認済み |
| 読込確認済み | PDF、Nikon Z7で作成したNEF（8288×5520、16-bit RGBAとして読込） |
| 同梱・追加検証中 | CSV、SVG、XCF、QML、TGA、Heightmap、Brush resource、Spriter、KRZ、RGBE、OpenEXR、JPEG 2000、Exif/IPTC/XMPなど |

PDFや確認済みのRAW sampleを含め、表にある個別の確認結果は記録済みのsampleと操作経路を対象とします。追加のvariant、圧縮方式、色空間、metadata、KRA filter構成は今後検証します。

### 同梱済みで操作確認中の機能

現在のiPad profileは162個の内部プラグインを静的登録しています。arm64での最終link、IPA検査、実機へのinstallとstartupまで確認済みです。次のUIと操作を引き続き検証します。

- SeExpr generatorとFill Layerの一連の操作
- LUT Dockerの表示とOpenColorIO LUTの適用
- Resource Managerによるbundleのimport/export
- Colorize Toolおよび各Tool/Dockerの詳細操作
- OpenEXRのround trip、JPEG 2000の読込、その他追加形式で実装済みのimport/export経路
- Pencil double tapの「直前のpreset」「palette」「何もしない」と、任意actionの設定UI
- iPad向けcanvas-only touch UI、Brush Library、Layer HUDの全操作と回帰試験

### 次のiPadOS検証項目

- Pencil描画と指gestureの完全な分離、およびundo/redo、pan、zoom、rotateの体系的な回帰試験
- Safe Area、Split View、Stage Manager、外部display、compact window geometry
- background/foregroundの反復、休止中の回転・文書close境界、失敗・期限切れ経路からの復旧
- Pencil hoverと外部keyboard
- Filesからのcold/warm launch、recent document、iCloud Drive、強制終了後のautosave recovery
- severe memory pressure、Jetsam、2K/4K/8K canvas上限、1時間連続描画、thermal/battery試験
- 小さなtouch target、modal dialogとsoftware keyboardの重なり

iOS向けのmemory policyとして、tile予算は物理RAMの25%かつ最大1 GiBを既定値とし、手動設定の上限を37.5%かつ最大1.5 GiBに制限しています。memory warning時のtile・pixmap cache解放も実装済みです。次の実機gateでは、強いmemory pressure下での復帰と未保存データ保持を検証します。

### 現行iPadOS profile

現在のiPadOS workstreamは、iPad向けtouch UIとAltStore／LiveContainerによるlocal配備を対象とします。iPhone向けUI調整、App Store配布、production signingは、それぞれ独立したplatform workとして扱います。

自己完結したbuildは、描画、同梱resource、local file workflowを中心に構成しています。次の統合機能は現行iPadOS profileの範囲外です。

- Python/PyQtとG'MIC
- Qt PrintSupportと印刷
- FFmpeg、MLT、SDLによる動画・音声のimport/export
- updater、外部processを使うbug reportや補助機能
- SVG Text Tool／Text Properties、Storyboard、Small Color Selector
- Recorderの動画exportとCompositionのanimation export

Animation UIはiPadOS向けの低優先度候補です。動画・音声exportは現行profileの範囲外です。他のLibrePaint向けplatform buildの機能表は、platformごとに整備します。

## ビルドと開発

macOSの標準buildは[`nix/macos/`](nix/macos/)で定義しています。Androidのビルド手順は[`README.android.md`](README.android.md)、LinuxのローカルAppImage経路は[`packaging/linux/appimage/README.md`](packaging/linux/appimage/README.md)に記載しています。各platformのpackagingは[`packaging/`](packaging/)にまとめています。

### macOSのNix build

Apple SiliconではmacOS packageをflakeのdefault outputに設定しています。リポジトリのrootから名前付きoutputをbuildします。

```sh
nix build .#librepaint-macos
```

app bundleは`result/bin/LibrePaint.app`に生成されます。起動する場合は次を実行します。

```sh
open result/bin/LibrePaint.app
```

同じ依存環境のdevelopment shellは次のコマンドで起動します。

```sh
nix develop .#librepaint-macos
```

clean buildで確認したtoolchainは次のとおりです。

| Component | 確認値 |
|---|---|
| Host | Apple Silicon macOS（`aarch64-darwin`） |
| Compiler | nixpkgsのLLVM Clang 21.1.8 |
| Linker／archive tool | nixpkgsのcctools／ld64 |
| SDK | Nix store内のApple SDK 14.4 |
| Qt | 6.11.1 |
| KDE Frameworks／ECM | 6.28.0 |
| Deployment target | macOS 14.0 |
| Architecture | arm64 |

固定済みのNix graphが宣言済みのbuild toolchainと依存setを供給します。Darwin向けtoolchainは、nixpkgsのLLVM Clang、cctools、SDK、open-sourceの`xcbuild` packageで構成しています。

native C++ desktop profileには、描画application、dynamic plugin、PopplerによるPDF import、LibRaw／KDcrawによるRAW import、KSeExpr generator、OpenColorIO、MLT/SDL audio-video support、FFmpeg／FFprobe、および[`nix/macos/krita.nix`](nix/macos/krita.nix)で宣言した画像形式libraryが含まれます。

次のmacOS dependency workでは、Python/PyQt scripting closureと埋込みruntime pathの統合を進めます。

このNix outputは、runtime libraryをNix closureに保持する再現可能な開発・checkpoint用bundleです。配布recipeでは、このbuildにstandalone bundling、DMG生成、署名、公証を積み重ねます。

### iPadOSのビルドとローカル配備

すべてのコマンドはリポジトリのrootから実行してください。通常のソース編集では、固定済みNix環境とfingerprintごとの永続Ninja treeを使うincremental workflowを推奨します。

#### 固定toolchain

現在の正確な固定値は[`packaging/ios/versions.env`](packaging/ios/versions.env)にあります。

| Component | 固定値 |
|---|---|
| Krita base revision | `7173825999953623d28777a163a65b42a3f26f0a` |
| Host | Apple Silicon macOS (`aarch64-darwin`) |
| Nix | 2.31以上 |
| Xcode | 26.6 (`17F113`) |
| iPhoneOS SDK | 26.5 (`23F81a`) |
| Apple Clang | 21.0.0 (`2100.1.1.101`) |
| Qt | 6.11.1 |
| KDE Frameworks / ECM | 6.28.0 |
| Deployment target | iPadOS 17.0 |
| Architecture | arm64 |

`iPhoneOS SDK`はAppleのSDK名であり、現在のbundleはiPad向けです。通常の開発では固定versionを維持し、version更新は別の検証作業として扱います。

#### 前提環境

- `/Applications/Xcode.app`に上表のXcodeがinstallされているApple Silicon Mac
- Nix 2.31以降とFlakesを利用できるNix daemon
- AltStoreで自動配備する場合は、iPadOS 17以降のiPad、USB接続、unlock、Macとのtrust、Developer Mode
- AltStoreで自動配備する場合は、Mac上のAltServer、iPad上で設定済みのAltStore、必要なlocal development signing環境、およびMacとiPad間のlocal network接続
- LiveContainerでインストールする場合は、iPadにLiveContainerがインストール・設定済みであること。実機確認済みのiOS 26構成ではJIT-Less modeを使用

Nix daemonはsandboxを有効、fallbackを無効にし、Xcodeだけを明示的なimpure host dependencyとして許可します。検証済みのnix-darwin設定は次のとおりです。

```nix
nix.settings.sandbox = true;
nix.settings.sandbox-fallback = false;
nix.settings.extra-allowed-impure-host-deps = [
  "/Applications/Xcode.app"
];
```

`sandbox-paths`にはXcodeを含めません。次のコマンドで環境を検査します。

```sh
nix develop --command packaging/ios/scripts/check-host.sh
```

このcheckはXcode、SDK、Clang、Nix、CMakeなどのversionに加え、Nix daemonのsandbox policyも検証します。

#### 最初のincremental build

新しいビルド構成では、一度だけbaselineを作成します。

```sh
packaging/ios/scripts/build-librepaint-incremental.sh path
packaging/ios/scripts/build-librepaint-incremental.sh bootstrap
```

wrapperがsource非依存の固定Nix profileを作成して再利用します。初回baselineは全面buildになるため時間がかかります。

依存closureとKF6のconsumer linkだけを先に検証したい場合は、次を実行できます。

```sh
nix build .#ios-dependencies --no-link
nix build .#kf6-consumer-check --no-link
```

#### 通常の開発build

変更後は、Ninjaが予定している処理を確認してからincremental buildします。

```sh
packaging/ios/scripts/build-librepaint-incremental.sh plan
packaging/ios/scripts/build-librepaint-incremental.sh build
```

`path`は選択中のbuild treeを表示します。通常の`build`と`deploy`は、意図しない全面再buildを事前に発見できるよう、既定で200 Ninja stepを超えるplanを拒否します。構成を意図的に大きく変えた場合は内容を確認し、`bootstrap`で新しいbaselineを構築してください。

日常のedit-build-test loopでは、このwrapperを使用します。直接の`cmake --preset`は構成作業、`nix build .#librepaint-ios-ipa`はclean checkpointに使用します。旧`build-krita-incremental.sh`と`krita-ios-*` entry pointは互換aliasとして維持します。

#### 再現可能なappと未署名IPA

clean checkpoint用のapp bundleとIPAはNixから構築できます。

```sh
nix build .#librepaint-ios-app \
  --out-link build-ios/nix-results/librepaint-ios-app
nix build .#librepaint-ios-ipa \
  --out-link build-ios/nix-results/librepaint-ios-ipa
```

成果物は次の場所に生成されます。

- `build-ios/nix-results/librepaint-ios-app/LibrePaint.app`
- `build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iPad-unsigned.ipa`

`librepaint-ios-ipa`は必要なappと依存も自動的に構築します。生成されるIPAは未署名です。署名情報、provisioning profile、Apple ID、device credentialはリポジトリの外で管理してください。

#### AltStoreで実機へ配備

前提環境を満たしてAltServerを起動した後、次のコマンドでincremental build、binary・plugin・runtime data検査、IPA生成、AltStoreによる署名とinstall、LibrePaintの起動、startup log回収まで行います。

```sh
packaging/ios/scripts/build-librepaint-incremental.sh deploy [device-id]
```

`device-id`を省略すると、最初に見つかった利用可能なCoreDeviceを選択します。接続端末は次のコマンドで確認できます。

```sh
xcrun devicectl list devices
```

timestamp付きIPAと回収した`librepaint.log`は`build-ios/deploy/`に保存されます。workflowは`packaging/ios/scripts/deploy-altstore.sh --skip-build`を内部handoffに使用し、現在の正確なbuild treeを渡します。

このworkflowは作者のlocal利用に必要なdevelopment signingを行います。

#### LiveContainerで実機へインストール

再現可能な未署名IPA `build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iPad-unsigned.ipa`は、LiveContainerへimportして利用することもできます。packaging workflowは、LiveContainerがapp bundleをpatch、起動、cleanupするために必要なarchive permissionを正規化します。LiveContainerのiOS 26 JIT-Less modeを使用した新規importと起動を実機で確認済みです。

以前の失敗したimportにより、LiveContainer内へread-onlyな一時`Payload`が残ることがあります。このstale state errorが発生した場合は、必要なapp dataを保護し、影響を受けたLiveContainerのstateをcleanupまたはresetしてから修正版IPAをimportしてください。正確なcleanup UIの実機確認が残る復旧項目です。現在のarchive permissionと復旧上の注意は[`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md)を参照してください。

#### Simulator smoke test

```sh
nix develop --command packaging/ios/scripts/build-smoke.sh simulator
```

このsmoke testはObjective-C++、UIKit、SDK、deployment target、bundle metadataを診断します。runtimeの受け入れ確認には実機試験を使用します。

#### 依存recipeを変更するmaintainer向け注意

`packaging/ios/scripts/bootstrap-ios-dependencies.sh --confirm-pinning-complete`はdependency recipe固定完了時の保守用手順です。実行時点は、すべての依存recipeを固定してcommitし、rootで保護されていない既存cache outputを破棄できる段階です。この手順は既知のlegacy GC rootを解放し、**Nixのfull garbage collectionを実行してから**最終aggregateを再構築します。通常のソース開発では上記のincremental workflowを使用します。

## 関連文書と出力

| Path | 内容 |
|---|---|
| [`TODO.md`](TODO.md) | iPadOSのmilestone、残作業、実機検証結果の正本 |
| [`README.android.md`](README.android.md) | 現在のAndroidローカルbuild経路とdependency prefixの制約 |
| [`packaging/linux/appimage/README.md`](packaging/linux/appimage/README.md) | 現在のLinuxローカルAppImage build経路と前提環境 |
| [`docs/ios/README.md`](docs/ios/README.md) | toolchain、依存build、cache設計の詳細 |
| [`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md) | AltStore配備、IPA permission、LiveContainer import時の注意事項 |
| [`packaging/ios/versions.env`](packaging/ios/versions.env) | 固定versionとdeployment target |
| [`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json) | iPad向け静的plugin profile |
| `build-ios/` | app、IPA、incremental build tree、Nix profile |
| `logs/ios/` | timestamp付きbuild log |

`build-ios/`、署名済み成果物、credential、private cache keyなどのlocal artifactはGitの外で管理してください。

## ライセンスとupstream

LibrePaintは[Krita](https://krita.org/)を基にした派生著作物であり、GNU General Public License Version 3の条件に従って配布されます。個々のファイルや同梱componentには、それぞれの互換licenseが適用されます。正確な条件は[`COPYING`](COPYING)と各ファイルのlicense表示を参照してください。

KritaはKrita Foundation、KDE、Krita contributorsによって開発されています。LibrePaintはLibrePaint contributorsが独立して保守しています。元projectとその履歴はupstreamの[graphics/krita](https://invent.kde.org/graphics/krita) repositoryを参照してください。
