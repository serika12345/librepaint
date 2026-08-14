# LibrePaint

[英語版](README.md)

LibrePaintは、Krita由来のコードベースを独立して開発・保守する、クロスプラットフォームのデジタルペイントアプリケーションです。共通のアプリケーション動作、ユーザーインターフェース、ワークフロー、ブランド、パッケージング、各OSとの統合を含め、コードベース全体をLibrePaintという一つの製品として改造し、デスクトップとモバイルの各プラットフォームへ提供することを目指します。

現在もっとも検証が進んでいるのはiPadOS版です。arm64実機向けの再現可能なビルドと配備、Apple Pencilとタッチ入力、「ファイル」アプリとの連携、主要な描画ワークフローを実機で確認しています。ほかのターゲット向けのソースとパッケージング経路は、下表に示す段階まで整備されています。

> [!WARNING]
> LibrePaintは開発途中であり、完成度と検証範囲はプラットフォームごとに異なります。以下の表に、これまで実施した操作経路を具体的に記録しています。重要な作品は別の場所にもバックアップしてください。

## プロジェクトの方向性

| 項目 | 方針 |
|---|---|
| 製品の範囲 | 共通コンポーネントとプラットフォーム固有コンポーネントを持つ一つのアプリケーションとしてLibrePaintを開発・保守する |
| 対象プラットフォーム | Windows、macOS、Linux、Android（ChromeOS上のAndroid環境を含む）、iPadOS、およびKrita／Qtコードベースで対応可能な追加ターゲット |
| 共通開発 | 適切な機能とユーザー体験は共通コードで実装し、必要な箇所だけ各プラットフォーム固有の統合を行う |
| 互換性 | 作品、リソース、安定した技術識別子を意図的に維持し、互換性のない変更は明示する |
| 配布目標 | 各プラットフォームに適したビルド、パッケージ化、検証、提供経路を整備する |

以下のプラットフォーム別の状況では、ロードマップ上の対象と検証済みの提供範囲を分けて記載します。現在の完成度、機能範囲、提供経路はプラットフォームごとに異なります。

## プラットフォーム別の現在地

| プラットフォーム | 現在のリポジトリ上の状態 | 現在の検証状況 |
|---|---|---|
| iPadOS | 固定済みNix／Xcode環境、未署名IPA生成、AltStore／LiveContainer配備経路 | もっとも開発が進んでいる対象。iPadOS 17以降のarm64実機で詳細に検証中 |
| Android／ChromeOS | LibrePaint APK構成と[ローカルビルドガイド](README.android.md) | 次の課題は依存関係のプレフィックスの準備とエンドツーエンドの実機検証 |
| Linux | Nixの依存関係、完成ビルド、AppImageレシピと[ローカルAppImageスクリプトおよびガイド](packaging/linux/appimage/README.md) | 次の課題はランタイム動作、公開経路、署名の検証 |
| macOS | LibrePaintアプリバンドルのNixレシピと既存のDMGパッケージング経路 | nixpkgsのLLVMツールチェーンとSDKによるarm64クリーンビルドとアプリケーション起動を確認済み。次は対話型ユーザーインターフェースと配布工程を検証 |
| Windows | x86_64 Linuxから64ビットWindows向けの可搬ディレクトリーとZIPアーカイブをクロスビルドするNixレシピ。[依存構造TODO](docs/windows/TODO.md)で標準パッケージへの移行を管理 | 次の課題はWindowsでの動作、インストーラー、署名、エンドツーエンド検証 |

現在のiOS対応範囲はiPadです。ほかのAppleフォームファクターや追加プラットフォームについては、それぞれのユーザーインターフェース、ビルド、パッケージング、実機検証を経て対応状況を更新します。

既存のCMakeターゲット、設定ディレクトリ、KRAのMIME／UTI、プラグインID、アクションIDは互換性契約として維持します。

## 現在のiPadOS開発状況

以下は2026年8月9日時点のiPadOSワークストリームの状態です。iPadOSの進捗と実機検証結果の正本は[`TODO.md`](TODO.md)、iPadOSへ静的に組み込む機能の正本は[`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json)です。

### 実機で確認済み

| 分野 | 確認済みの範囲 |
|---|---|
| 起動と基本画面 | AltStore経由のインストールと起動、LiveContainerへのIPA新規インポートと起動、メインウィンドウ、縦向き・横向きのスプラッシュ画面、初期縦向きと回転後のLibrePaint設定ダイアログ |
| タッチ操作 | キャンバス外でのスワイプによる慣性スクロールとタップによる項目選択、コンボボックスのタップ確定、明示的な文字編集から始まるテキスト入力フォーカス |
| Apple Pencil | 押下・移動・解放、筆圧、傾き、初期選択済みブラシによる即時描画 |
| Pencilのダブルタップ | `eraser_preset_action`による、独立したペン側／消しゴム側ブラシプリセットの切り替え |
| ファイル | iPadOSの「ファイル」アプリによるネイティブな読み込みと保存、KRA・PNG・JPEG・ORAの保存、再読み込み、回収後の外部検査 |
| ブラシエンジン | ピクセルブラシ、MyPaintプリセットの登録、カラースマッジ、スプレー、ハッチング、フィルターブラシ（反転）の描画 |
| ツールとドッカー | 主要ツールのツールボックス表示、主要ドッカーのメニュー表示、レイヤードッカーなどの基本利用 |
| フィルターとジェネレーター | 33個のフィルターと従来の6個のジェネレーターの登録、レイヤー作成、KRAの読み込み・保存・再読み込み |
| キャンバス表示 | デバイスピクセル比2の高DPI表示、OpenGL ES 3.0によるキャンバス描画 |
| アプリのライフサイクル | 同じプロセスを維持したバックグラウンドからの復帰、キャンバス全面の復元、Pencil描画の再開、変更済みKRA文書1件のリカバリーチェックポイント作成 |

「確認済み」は記載した操作経路を特定の実機で通過したという意味です。検証範囲は、記録済みの設定、ファイル内容、端末、試験時間に限られます。

### ファイル形式

| 状態 | 形式と確認範囲 |
|---|---|
| 基本ラウンドトリップ確認済み | KRA、PNG、JPEG、ORA |
| 限定的な実機確認済み | WebPの保存。PSD、GIF、HEIF、JPEG XLの保存・再読み込み。TIFFはJPEG圧縮での保存・再読み込みを確認済み |
| 読み込み確認済み | PDF、Nikon Z7で作成したNEF（8288×5520、16ビットRGBAとして読み込み） |
| 同梱済み・追加検証中 | CSV、SVG、XCF、QML、TGA、ハイトマップ、ブラシリソース、Spriter、KRZ、RGBE、OpenEXR、JPEG 2000、Exif／IPTC／XMPなど |

PDFや確認済みのRAWサンプルを含め、表にある個別の確認結果は記録済みのサンプルと操作経路を対象とします。追加のバリエーション、圧縮方式、色空間、メタデータ、KRAフィルター構成は今後検証します。

### 同梱済みで操作確認中の機能

現在のiPadプロファイルは、162個の内部プラグインを静的に登録しています。arm64での最終リンク、IPA検査、実機へのインストールと起動まで確認済みです。次のユーザーインターフェースと操作を引き続き検証します。

- SeExprジェネレーターと塗りつぶしレイヤーの一連の操作
- LUTドッカーの表示とOpenColorIO LUTの適用
- リソースマネージャーによるバンドルのインポート／エクスポート
- カラー化ツールおよび各ツール／ドッカーの詳細操作
- OpenEXRのラウンドトリップ、JPEG 2000の読み込み、その他の追加形式で実装済みのインポート／エクスポート経路
- Pencilのダブルタップにおける「直前のプリセット」「パレット」「何もしない」の各動作と、任意アクションの設定画面
- iPad向けのキャンバス専用タッチ操作画面、ブラシライブラリー、レイヤーHUDの全操作と回帰試験

### 次のiPadOS検証項目

- Pencil描画と指ジェスチャーの完全な分離、およびアンドゥ／リドゥ、パン、ズーム、回転の体系的な回帰試験
- セーフエリア、Split View、Stage Manager、外部ディスプレイ、コンパクトウィンドウのジオメトリー
- バックグラウンド／フォアグラウンドの反復、休止中の回転、文書を閉じる際の境界、失敗・期限切れ経路からの復旧
- Pencilホバーと外部キーボード
- 「ファイル」アプリからのコールド／ウォーム起動、最近使ったドキュメント、iCloud Drive、強制終了後の自動保存データ復旧
- 厳しいメモリープレッシャー、Jetsam、2K／4K／8Kキャンバス上限、1時間連続描画、温度／バッテリー試験
- 小さなタッチターゲット、モーダルダイアログとソフトウェアキーボードの重なり

iOS向けのメモリーポリシーとして、タイル予算は物理RAMの25%、かつ最大1 GiBを既定値とし、手動設定の上限を37.5%、かつ最大1.5 GiBに制限しています。メモリー警告時のタイル・ピックスマップキャッシュ解放も実装済みです。次の実機検証では、強いメモリープレッシャー下での復帰と未保存データ保持を検証します。

### 現行iPadOSプロファイル

現在のiPadOSワークストリームは、iPad向けタッチ操作画面とAltStore／LiveContainerによるローカル配備を対象とします。iPhone向けの画面調整、App Store配布、製品版署名は、それぞれ独立したプラットフォーム作業として扱います。

自己完結したビルドは、描画、同梱リソース、ローカルファイルのワークフローを中心に構成しています。次の統合機能は現行iPadOSプロファイルの範囲外です。

- Python/PyQtとG'MIC
- Qtの印刷サポートと印刷
- FFmpeg、MLT、SDLによる動画・音声の読み込みと書き出し
- アップデーター、外部プロセスを使うバグ報告や補助機能
- SVGテキストツール／テキストプロパティ、ストーリーボード、スモールカラーセレクター
- 録画機能の動画エクスポートと合成機能のアニメーションエクスポート

アニメーション画面はiPadOS向けの低優先度候補です。動画・音声のエクスポートは現行プロファイルの範囲外です。ほかのLibrePaint向けプラットフォームビルドの機能表は、プラットフォームごとに整備します。

## ビルドと開発

macOS、Linux、Windowsの標準Nixビルドは、[`nix/macos/`](nix/macos/)、[`nix/linux/`](nix/linux/)、[`nix/windows/`](nix/windows/)で定義しています。Androidのビルド手順は[`README.android.md`](README.android.md)、LinuxのローカルAppImageワークフローは[`packaging/linux/appimage/README.md`](packaging/linux/appimage/README.md)に記載しています。各プラットフォームのパッケージング定義は[`packaging/`](packaging/)にまとめています。

### macOSのNixビルド

Apple SiliconではmacOSパッケージをNixフレークのデフォルト出力に設定しています。リポジトリのルートから名前付き出力をビルドします。

```sh
nix build .#librepaint-macos
```

アプリバンドルは`result/bin/LibrePaint.app`に生成されます。起動する場合は次を実行します。

```sh
open result/bin/LibrePaint.app
```

同じ依存環境の開発シェルは、次のコマンドで起動します。

```sh
nix develop .#librepaint-macos
```

クリーンビルドで確認したツールチェーンは、次のとおりです。

| コンポーネント | 確認値 |
|---|---|
| ホスト | Apple Silicon搭載macOS（`aarch64-darwin`） |
| コンパイラー | nixpkgsのLLVM Clang 21.1.8 |
| リンカー／アーカイブツール | nixpkgsのcctools／ld64 |
| SDK | Nixストア内のApple SDK 14.4 |
| Qt | 6.11.1 |
| KDE Frameworks／ECM | 6.28.0 |
| デプロイメントターゲット | macOS 14.0 |
| アーキテクチャ | arm64 |

固定済みのNixグラフが、宣言済みのビルドツールチェーンと依存関係一式を供給します。Darwin向けツールチェーンは、nixpkgsのLLVM Clang、cctools、SDK、オープンソースの`xcbuild`パッケージで構成しています。

ネイティブC++デスクトッププロファイルには、描画アプリケーション、動的プラグイン、PopplerによるPDFインポート、LibRaw／KDcrawによるRAWインポート、KSeExprジェネレーター、OpenColorIO、MLT／SDLによる音声・動画サポート、FFmpeg／FFprobe、および[`nix/macos/krita.nix`](nix/macos/krita.nix)で宣言した画像形式ライブラリーが含まれます。

次のmacOS依存関係対応では、Python／PyQtスクリプティングクロージャと埋め込みランタイムパスの統合を進めます。

このNix出力は、ランタイムライブラリーをNixクロージャに保持する、再現可能な開発・チェックポイント用バンドルです。配布レシピでは、このビルドにスタンドアロンバンドル化、DMG生成、署名、公証を重ねます。

### LinuxのNixビルド

x86_64 Linux向けNixフレークには、ソースに依存しない依存関係クロージャと、ラッパーを含む完成済みLibrePaintビルドを用意しています。まず次のコマンドで依存関係クロージャだけをビルドし、LibrePaintソースの変更に左右されずローカルまたは設定済みのバイナリキャッシュを準備します。

```sh
nix build .#linux-dependencies --no-link
```

同じ依存関係レシピでアプリケーションをビルドします。

```sh
nix build .#librepaint-linux
```

生成物は`result/bin/LibrePaint`です。Kritaとの互換デスクトップ識別子とMIME識別子は維持しつつ、表示ブランドはLibrePaintとしています。完成ビルドはnixpkgsのKrita本体／ラッパー構成に従い、G'MICプラグインとQt／GLibランタイムラッパーを含みます。対応する開発シェルは、次のコマンドで開けます。

```sh
nix develop .#librepaint-linux
```

パッケージングの最終段階として、タイプ2 AppImageをビルドできます。

```sh
nix build .#librepaint-linux-appimage \
  --out-link LibrePaint-1.0.2-x86_64.AppImage
```

出力シンボリックリンクは、エントリーポイントを`LibrePaint`とする自己完結型AppImageです。アプリケーションを再ビルドせず、完成済みNixクロージャを埋め込みます。実行にはLinuxユーザー名前空間が必要です。

開発時およびNixOS上でのローカル利用では、AppImageではなく通常のNixパッケージを使います。

```sh
nix run .#librepaint-linux
```

AppImageは配布用アーティファクトです。Nixクロージャ型AppImageのアップストリームランタイムには、NixOS以外でのOpenGL移植性に既知の制約があり、nixGL形式のラッパーが必要になる場合があります。配布前に、必ず対象システムとGPUで検証してください。

### WindowsのNixクロスビルド

Windows向けレシピは、x86_64 Linuxから64ビットWindows用の`x86_64-w64-mingw32`ビルドをクロスコンパイルします。LibrePaintのソースに依存しないターゲット依存関係グラフとアプリケーションビルドを分離しているため、変更のない依存関係はバイナリキャッシュから利用できます。

```sh
nix build .#windows-dependencies --no-link
nix build .#librepaint-windows
```

生成物は、`result/bin/LibrePaint.exe`を含む可搬ディレクトリーです。パッケージング段階で、ターゲットDLL、QtプラグインとQMLモジュール、Python／PyQt、G'MIC、FFmpeg／FFprobe、MLTデータ、翻訳、Fontconfigの設定とフォント、`qt.conf`を実行ファイルの隣へ配置します。対応するZIPアーカイブは次のコマンドで生成します。

```sh
nix build .#librepaint-windows-archive
```

アーカイブは`result/LibrePaint-1.0.2-x86_64-windows.zip`として生成されます。

このレシピは、Python／PyQtスクリプト、Qt Quick／QML画面、PDFインポート、G'MIC、KSeExpr、FFTW、OpenColorIO、MLT／SDLによる音声・映像対応、FFmpeg／FFprobe、DrMingwのクラッシュ記録、HDR画面情報、およびGIF、HEIF、JPEG XL、TIFF、WebPの各ワークフローを含む、upstreamのWindows版と同等の機能一式を有効にします。

### iPadOSのビルドとローカル配備

すべてのコマンドは、リポジトリのルートから実行してください。通常のソース編集では、固定済みNix環境とフィンガープリントごとの永続Ninjaビルドツリーを使うインクリメンタルワークフローを推奨します。

#### 固定ツールチェーン

現在の正確な固定値は[`packaging/ios/versions.env`](packaging/ios/versions.env)にあります。

| コンポーネント | 固定値 |
|---|---|
| Kritaベースリビジョン | `7173825999953623d28777a163a65b42a3f26f0a` |
| ホスト | Apple Silicon搭載macOS（`aarch64-darwin`） |
| Nix | 2.31以上 |
| Xcode | 26.6 (`17F113`) |
| iPhoneOS SDK | 26.5 (`23F81a`) |
| Apple Clang | 21.0.0 (`2100.1.1.101`) |
| Qt | 6.11.1 |
| KDE Frameworks／ECM | 6.28.0 |
| デプロイメントターゲット | iPadOS 17.0 |
| アーキテクチャ | arm64 |

`iPhoneOS SDK`はAppleのSDK名であり、現在のバンドルはiPad向けです。通常の開発では固定バージョンを維持し、バージョン更新は別の検証作業として扱います。

#### 前提環境

- `/Applications/Xcode.app`に上表のXcodeがインストールされているApple Silicon搭載Mac
- Nix 2.31以降とフレークを利用できるNixデーモン
- AltStoreで自動配備する場合は、iPadOS 17以降のiPad、USB接続、ロック解除、Macへの信頼設定、開発者モード
- AltStoreで自動配備する場合は、Mac上のAltServer、iPad上で設定済みのAltStore、必要なローカル開発用署名環境、およびMacとiPad間のローカルネットワーク接続
- LiveContainerでインストールする場合は、iPadにLiveContainerがインストール・設定済みであること。実機確認済みのiOS 26構成ではJITなしモードを使用

Nixデーモンではサンドボックスを有効、フォールバックを無効にし、Xcodeだけを明示的な外部ホスト依存関係として許可します。検証済みのnix-darwin設定は次のとおりです。

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

このチェックでは、Xcode、SDK、Clang、Nix、CMakeなどのバージョンに加え、Nixデーモンのサンドボックスポリシーも検証します。

#### 最初のインクリメンタルビルド

新しいビルド構成では、一度だけベースラインを作成します。

```sh
packaging/ios/scripts/build-librepaint-incremental.sh path
packaging/ios/scripts/build-librepaint-incremental.sh bootstrap
```

ラッパーがソースに依存しない固定Nixプロファイルを作成して再利用します。初回ベースラインはフルビルドになるため、時間がかかります。

依存関係クロージャとKF6を利用する側のリンクだけを先に検証したい場合は、次を実行できます。

```sh
nix build .#ios-dependencies --no-link
nix build .#kf6-consumer-check --no-link
```

#### 通常の開発ビルド

変更後は、Ninjaが予定している処理を確認してからインクリメンタルビルドします。

```sh
packaging/ios/scripts/build-librepaint-incremental.sh plan
packaging/ios/scripts/build-librepaint-incremental.sh build
```

`path`は選択中のビルドツリーを表示します。通常の`build`と`deploy`は、意図しないフルリビルドを事前に発見できるよう、既定で200個のNinjaステップを超える`plan`を拒否します。構成を意図的に大きく変えた場合は内容を確認し、`bootstrap`で新しいベースラインを構築してください。

日常の編集・ビルド・テストのサイクルでは、このラッパーを使用します。直接の`cmake --preset`は構成作業、`nix build .#librepaint-ios-ipa`はクリーンチェックポイントに使用します。旧`build-krita-incremental.sh`と`krita-ios-*`のエントリーポイントは、互換エイリアスとして維持します。

#### 再現可能なアプリと未署名IPA

クリーンチェックポイント用のアプリバンドルとIPAは、Nixから構築できます。

```sh
nix build .#librepaint-ios-app \
  --out-link build-ios/nix-results/librepaint-ios-app
nix build .#librepaint-ios-ipa \
  --out-link build-ios/nix-results/librepaint-ios-ipa
```

成果物は次の場所に生成されます。

- `build-ios/nix-results/librepaint-ios-app/LibrePaint.app`
- `build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iPad-unsigned.ipa`

`librepaint-ios-ipa`は必要なアプリと依存関係も自動的に構築します。生成されるIPAは未署名です。署名情報、プロビジョニングプロファイル、Apple ID、デバイス認証情報はリポジトリの外で管理してください。

#### AltStoreで実機へ配備

前提環境を満たしてAltServerを起動した後、次のコマンドでインクリメンタルビルド、バイナリ・プラグイン・ランタイムデータの検査、IPA生成、AltStoreによる署名とインストール、LibrePaintの起動、起動ログの回収まで行います。

```sh
packaging/ios/scripts/build-librepaint-incremental.sh deploy [device-id]
```

`device-id`を省略すると、最初に見つかった利用可能なCoreDeviceを選択します。接続端末は次のコマンドで確認できます。

```sh
xcrun devicectl list devices
```

タイムスタンプ付きIPAと回収した`librepaint.log`は`build-ios/deploy/`に保存されます。このワークフローは`packaging/ios/scripts/deploy-altstore.sh --skip-build`を内部ハンドオフに使用し、現在の正確なビルドツリーを渡します。

このワークフローは、作者のローカル利用に必要な開発用署名を行います。

#### LiveContainerで実機へインストール

再現可能な未署名IPA `build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iPad-unsigned.ipa`は、LiveContainerへインポートして利用することもできます。パッケージングワークフローは、LiveContainerがアプリバンドルへパッチを適用し、起動、クリーンアップするために必要なアーカイブ権限を正規化します。LiveContainerのiOS 26 JITなしモードを使用した新規インポートと起動を実機で確認済みです。

以前の失敗したインポートにより、LiveContainer内へ読み取り専用の一時`Payload`が残ることがあります。この古い状態によるエラーが発生した場合は、必要なアプリデータを保護し、影響を受けたLiveContainerの状態をクリーンアップまたはリセットしてから修正版IPAをインポートしてください。正確なクリーンアップ画面の実機確認が残る復旧項目です。現在のアーカイブ権限と復旧上の注意は[`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md)を参照してください。

#### シミュレーターのスモークテスト

```sh
nix develop --command packaging/ios/scripts/build-smoke.sh simulator
```

このスモークテストはObjective-C++、UIKit、SDK、デプロイメントターゲット、バンドルメタデータを診断します。ランタイムの受け入れ確認には実機試験を使用します。

#### 依存関係レシピを変更するメンテナー向け注意

`packaging/ios/scripts/bootstrap-ios-dependencies.sh --confirm-pinning-complete`は、依存関係レシピの固定完了時に使う保守手順です。実行するのは、すべての依存関係レシピを固定してコミットし、ルートで保護されていない既存キャッシュ出力を破棄できる段階です。この手順は既知の旧GCルートを解放し、**Nixのフルガベージコレクションを実行してから**最終アグリゲートを再構築します。通常のソース開発では、上記のインクリメンタルワークフローを使用します。

## 関連文書と出力

| パス | 内容 |
|---|---|
| [`TODO.md`](TODO.md) | iPadOSのマイルストーン、残作業、実機検証結果の正本 |
| [`README.android.md`](README.android.md) | 現在のAndroidローカルビルド経路と依存関係プレフィックスの制約 |
| [`packaging/linux/appimage/README.md`](packaging/linux/appimage/README.md) | 現在のLinuxローカルAppImageビルド経路と前提環境 |
| [`docs/ios/README.md`](docs/ios/README.md) | ツールチェーン、依存関係ビルド、キャッシュ設計の詳細 |
| [`docs/ios/altstore-deployment.md`](docs/ios/altstore-deployment.md) | AltStore配備、IPA権限、LiveContainerインポート時の注意事項 |
| [`packaging/ios/versions.env`](packaging/ios/versions.env) | 固定バージョンとデプロイメントターゲット |
| [`packaging/ios/manifests/initial-plugin-profile.json`](packaging/ios/manifests/initial-plugin-profile.json) | iPad向け静的プラグインプロファイル |
| `build-ios/` | アプリ、IPA、インクリメンタルビルドツリー、Nixプロファイル |
| `logs/ios/` | タイムスタンプ付きビルドログ |

`build-ios/`、署名済み成果物、認証情報、プライベートキャッシュキーなどのローカルアーティファクトはGitの外で管理してください。

## ライセンスとアップストリーム

LibrePaintは[Krita](https://krita.org/)を基にした派生著作物であり、GNU一般公衆利用許諾書第3版の条件に従って配布されます。個々のファイルや同梱コンポーネントには、それぞれ互換性のあるライセンスが適用されます。正確な条件は[`COPYING`](COPYING)と各ファイルのライセンス表示を参照してください。

KritaはKrita Foundation、KDE、Kritaコントリビューターによって開発されています。LibrePaintはLibrePaintコントリビューターが独立して保守しています。元プロジェクトとその履歴は、[アップストリームのリポジトリ](https://invent.kde.org/graphics/krita)を参照してください。
