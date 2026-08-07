# LibrePaint iPadOS Port

[English](README.md)

LibrePaintは、現在Krita由来のコードを基盤としてarm64の実機iPadで動作する、非公式かつ実験的なiPadOSペイントアプリです。Apple Pencil、タッチ操作、iPadOSのFilesを使い、Android版に近い実用的な描画環境をローカルで構築することを目標にしています。

> [!WARNING]
> LibrePaintはKrita FoundationまたはKDEによる公式iPad版ではありません。現在は開発途中であり、データ保全、長時間動作、すべてのファイル形式やupstream Krita機能との互換性を保証しません。重要な作品は別の場所にもバックアップしてください。

## 対象範囲

| 項目 | 方針 |
|---|---|
| 対象デバイス | arm64の実機iPad |
| 最低OS | iPadOS 17.0 |
| 入力 | Apple Pencilとタッチ |
| 受け入れ対象 | 実機。Simulatorはtoolchainやbundleの診断専用 |
| インストール | AltStoreまたはLiveContainerによるローカルsideload |
| 主な到達目標 | Android版に近い主要描画機能、内部プラグイン、Files連携 |
| 対象外デバイス | **iPhoneはサポート対象外** |
| 対象外の配布 | App Store、公式代替マーケットプレイス、公証、一般向け配布 |

Python/PyQt、G'MIC、印刷、動画・音声機能、自動更新、外部プロセスを起動する機能、外部の第三者プラグインも現在の対象外です。ブラシ、ツール、Docker、色管理、画像入出力を構成するKrita内部プラグインは、選択したものを静的リンクして登録します。

## プロジェクトの位置づけと将来構想

LibrePaintは、作者個人の制作環境でKrita相当の機能をiPadから利用することを主な目的としています。現在はupstream Kritaとの差分を可能な限り限定していますが、将来もupstreamへの追従や変更の取り込みやすさを保証するものではありません。個人の制作フローとiPad上の操作性を優先し、UI、機能、内部構造にupstream Kritaから大きくかけ離れた改造を施す可能性があります。

長期的な最終像としては、Krita由来のGPLコードに依存しない独立実装へ移行し、**MIT Licenseで提供する独立プロジェクト**として成立させることを構想しています。これは将来構想であり、現在のKrita派生コードをMIT Licenseへ変更する宣言ではありません。現リポジトリに適用されるライセンスについては[ライセンスとupstream](#ライセンスとupstream)を参照してください。

## 現在のサポート状況

以下は2026年8月6日時点の状態です。進捗と実機検証結果の正本は[`TODO.md`](TODO.md)、静的に組み込む機能の正本は[`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json)です。

### 実機で確認済み

| 分野 | 確認済みの範囲 |
|---|---|
| 起動と基本UI | AltStore経由のインストールと起動、LiveContainerへのIPA新規importと起動、main window、縦向き・横向きのsplash、初期縦向きと回転後のConfigure LibrePaint dialog |
| タッチUI | 非canvas領域のスワイプと慣性scroll、scroll中の誤選択抑止、combo boxのタップ確定、設定画面やbrush一覧での不要なsoftware keyboard抑止 |
| Apple Pencil | press・move・release、筆圧、傾き、初期brushを選び直さずに行う描画 |
| Pencil double tap | `eraser_preset_action`による、独立したペン側／消しゴム側brush presetの切り替え |
| ファイル | iPadOS Filesによるnative open/save、KRA・PNG・JPEG・ORAの保存、再読込、回収後の外部検査 |
| Brush engine | Pixel Brush、MyPaint presetの登録、Color Smudge、Spray、Hatching、Filter Brush（Invert）の描画 |
| ToolとDocker | 主要ToolのToolbox表示、主要Dockerのmenu表示、Layer Dockerなどの基本利用 |
| FilterとGenerator | 33 filterと従来の6 generatorの登録、layer作成、KRAのopen/save/reopen |
| Canvas表示 | DPR 2の高DPI表示、OpenGL ES 3.0によるcanvas描画 |

「確認済み」は記載した操作経路を特定の実機で通過したという意味です。すべての設定、ファイル内容、端末、長時間動作を網羅した互換性保証ではありません。

### ファイル形式

| 状態 | 形式と確認範囲 |
|---|---|
| 基本round trip確認済み | KRA、PNG、JPEG、ORA |
| 限定的な実機確認済み | WebPの保存。PSD、GIF、HEIF、JPEG XLの保存・再読込。TIFFはJPEG圧縮での保存・再読込を確認済み |
| 読込確認済み | PDF、Nikon Z7で作成したNEF（8288×5520、16-bit RGBAとして読込） |
| 同梱・追加検証中 | CSV、SVG、XCF、QML、TGA、Heightmap、Brush resource、Spriter、KRZ、RGBE、OpenEXR、JPEG 2000、Exif/IPTC/XMPなど |

PDFや確認済みのRAW sampleを含め、表にある個別の確認結果は形式全体の互換性を保証しません。未検証のvariant、圧縮方式、色空間、metadata、KRA filter構成が残っています。

### 同梱済みで操作確認中の機能

現在のiPad profileは161個のKrita内部プラグインを静的登録しています。arm64での最終link、IPA検査、実機へのinstallとstartupまでは確認済みですが、各プラグインのすべてのUIと操作を確認したわけではありません。主な確認継続項目は次のとおりです。

- SeExpr generatorとFill Layerの一連の操作
- LUT Dockerの表示とOpenColorIO LUTの適用
- Resource Managerによるbundleのimport/export
- Colorize Toolおよび各Tool/Dockerの詳細操作
- OpenEXRのround trip、JPEG 2000の読込、その他追加形式で実装済みのimport/export経路
- Pencil double tapの「直前のpreset」「palette」「何もしない」と、任意actionの設定UI

### 主な未完了・未検証項目

- Pencil描画と指gestureの完全な分離、およびundo/redo、pan、zoom、rotateの体系的な回帰試験
- Safe Area、Split View、Stage Manager、外部display、compact window geometry
- background/foreground時のOpenGL surfaceとresourceの破棄・復元
- Pencil hoverと外部keyboard
- Filesからのcold/warm launch、recent document、iCloud Drive、強制終了後のautosave recovery
- severe memory pressure、Jetsam、2K/4K/8K canvas上限、1時間連続描画、thermal/battery試験
- 小さなtouch target、modal dialogとsoftware keyboardの重なり

iOS向けのmemory policyとして、tile予算は物理RAMの25%かつ最大1 GiBを既定値とし、手動設定の上限を37.5%かつ最大1.5 GiBに制限しています。memory warning時のtile・pixmap cache解放も実装済みですが、強いmemory pressure下での復帰と未保存データ保持はまだ実機検証中です。

### 明示的に含めない機能

- iPhone対応
- App Store向け配布、notarization、production signing
- Python/PyQt、G'MIC、Qt PrintSupportと印刷
- FFmpeg、MLT、SDLを必要とする動画・音声のimport/export
- updater、外部processに依存するbug reportや補助機能
- SVG Text Tool／Text Properties、Storyboard、Small Color Selector
- Recorderの動画exportとCompositionのanimation export

Animation UI自体は将来の低優先度候補ですが、動画・音声exportは対象外です。

## ビルド

すべてのコマンドはリポジトリのrootから実行してください。通常のソース編集では、固定済みNix環境とfingerprintごとの永続Ninja treeを使うincremental workflowを推奨します。

### 固定toolchain

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

`iPhoneOS SDK`はAppleのSDK名です。iPhoneをサポート対象に含めるという意味ではありません。通常の開発で固定versionを暗黙に上書きしないでください。version更新は別の検証作業として扱います。

### 前提環境

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

Xcodeを`sandbox-paths`へ追加しないでください。環境を検査します。

```sh
nix develop --command packaging/ios/scripts/check-host.sh
```

このcheckはXcode、SDK、Clang、Nix、CMakeなどのversionに加え、Nix daemonのsandbox policyも検証します。

### 最初のincremental build

新しいビルド構成では、一度だけbaselineを作成します。

```sh
packaging/ios/scripts/build-krita-incremental.sh path
packaging/ios/scripts/build-krita-incremental.sh bootstrap
```

wrapperがsource非依存の固定Nix profileを作成して再利用するため、事前に`nix develop`へ入る必要はありません。初回baselineは全面buildになるため時間がかかります。

依存closureとKF6のconsumer linkだけを先に検証したい場合は、次を実行できます。

```sh
nix build .#ios-dependencies --no-link
nix build .#kf6-consumer-check --no-link
```

### 通常の開発build

変更後は、Ninjaが予定している処理を確認してからincremental buildします。

```sh
packaging/ios/scripts/build-krita-incremental.sh plan
packaging/ios/scripts/build-krita-incremental.sh build
```

`path`は選択中のbuild treeを表示します。通常の`build`と`deploy`は、意図しない全面再buildを事前に発見できるよう、既定で200 Ninja stepを超えるplanを拒否します。構成を意図的に大きく変えた場合は内容を確認し、`bootstrap`で新しいbaselineを構築してください。

日常のedit-build-test loopでは、直接`cmake --preset`を呼び出したり、毎回`nix build .#krita-ios-ipa`を実行したりせず、このwrapperを使用してください。

### 再現可能なappと未署名IPA

clean checkpoint用のapp bundleとIPAはNixから構築できます。

```sh
nix build .#krita-ios-app \
  --out-link build-ios/nix-results/krita-ios-app
nix build .#krita-ios-ipa \
  --out-link build-ios/nix-results/krita-ios-ipa
```

成果物は次の場所に生成されます。

- `build-ios/nix-results/krita-ios-app/krita.app`
- `build-ios/nix-results/krita-ios-ipa/LibrePaint-iPad-unsigned.ipa`

IPAだけが必要な場合は`krita-ios-ipa`だけをbuildすれば、必要なappと依存も自動的に構築されます。生成されるIPAは意図的に未署名です。署名情報、provisioning profile、Apple ID、device credentialをリポジトリへ保存しないでください。

### AltStoreで実機へ配備

前提環境を満たしてAltServerを起動した後、次のコマンドでincremental build、binary・plugin・runtime data検査、IPA生成、AltStoreによる署名とinstall、LibrePaintの起動、startup log回収まで行います。

```sh
packaging/ios/scripts/build-krita-incremental.sh deploy [device-id]
```

`device-id`を省略すると、最初に見つかった利用可能なCoreDeviceを選択します。接続端末は次のコマンドで確認できます。

```sh
xcrun devicectl list devices
```

timestamp付きIPAと回収した`krita.log`は`build-ios/deploy/`に保存されます。`packaging/ios/scripts/deploy-altstore.sh --skip-build`はworkflow内部のhandoff専用であり、古いbuild treeを手動指定するために使用しないでください。

この手順は作者のlocal利用を目的とするdevelopment signingです。App Store提出や一般配布用の署名pipelineではありません。

### LiveContainerで実機へインストール

再現可能な未署名IPA `build-ios/nix-results/krita-ios-ipa/LibrePaint-iPad-unsigned.ipa`は、LiveContainerへimportして利用することもできます。packaging workflowは、LiveContainerがapp bundleをpatch、起動、cleanupするために必要なarchive permissionを正規化します。LiveContainerのiOS 26 JIT-Less modeを使用した新規importと起動を実機で確認済みです。

修正版IPAを使っても、以前の失敗したimportによってLiveContainer内に残ったread-onlyな一時`Payload`は削除できません。このstale state errorが発生する場合は、必要なapp dataを保護したうえで、影響を受けたLiveContainerのstateをcleanupまたはresetしてから再度importしてください。正確なcleanup UIは実機確認中です。現在のarchive permissionと復旧上の注意は[`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md)を参照してください。

### Simulator smoke test

```sh
nix develop --command packaging/ios/scripts/build-smoke.sh simulator
```

このsmoke testはObjective-C++、UIKit、SDK、deployment target、bundle metadataの診断用です。署名やinstallは行わず、実機試験の代わりにはなりません。

### 依存recipeを変更するmaintainer向け注意

`packaging/ios/scripts/bootstrap-ios-dependencies.sh --confirm-pinning-complete`は通常の初回buildコマンドではありません。すべての依存recipeを固定してcommitした後に、既知のlegacy GC rootを解放し、**Nixのfull garbage collectionを実行してから**最終aggregateを再構築する保守用手順です。通常のソース編集やcacheを温存したい状況では実行しないでください。

## 出力と関連文書

| Path | 内容 |
|---|---|
| [`TODO.md`](TODO.md) | milestone、残作業、実機検証結果の正本 |
| [`docs/ios/README.md`](docs/ios/README.md) | toolchain、依存build、cache設計の詳細 |
| [`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md) | AltStore配備、IPA permission、LiveContainer import時の注意事項 |
| [`packaging/ios/versions.env`](packaging/ios/versions.env) | 固定versionとdeployment target |
| [`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json) | iPad向け静的plugin profile |
| `build-ios/` | app、IPA、incremental build tree、Nix profile |
| `logs/ios/` | timestamp付きbuild log |

`build-ios/`、署名済み成果物、credential、private cache keyなどのlocal artifactをGitへcommitしないでください。

## 開発時の原則

- iOS固有の挙動は原則として`Q_OS_IOS`でguardし、意図しないAndroid、desktop、macOS側の変更を避けます。
- UIKit連携は薄いObjective-C++ bridgeへ閉じ込め、可能な限り既存のKrita actionやsubsystemを再利用します。
- plugin profileへ含まれていることと、実機で機能検証済みであることを区別します。
- touch、Pencil、Files、rotation、lifecycle、memoryに関する変更はcompile成功だけで完了扱いにしません。
- milestoneまたは実機検証状態が変わった場合は`TODO.md`を更新します。
- certificate、provisioning profile、Apple ID、署名secret、device credential、署名済みIPAをcommitしません。

## ライセンスとupstream

現在の本リポジトリは[Krita](https://krita.org/)を基にした派生物であり、MIT Licenseではありません。Krita全体にはGNU General Public License Version 3が適用され、個々のファイルや同梱componentにはそれぞれの互換licenseが適用されます。正確な条件は[`COPYING`](COPYING)と各ファイルのlicense表示を参照してください。

KritaはKrita Foundation、KDE、Krita contributorsによって開発されています。upstream repositoryは[graphics/krita](https://invent.kde.org/graphics/krita)、利用方法は[Krita User Manual](https://docs.krita.org/)を参照してください。本ポート固有の問題は、upstreamでも再現することを確認しない限りupstream Kritaの問題として扱わないでください。

将来のMIT License構想は、Krita由来のGPLコードを含まない独立実装として成立した後継プロジェクトを対象とします。現在のfork、現在のbuild成果物、Krita由来コードのライセンス条件を変更するものではありません。
