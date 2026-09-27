# iOS／iPadOS開発マニュアル

すべてのコマンドは、リポジトリのルートから実行してください。通常のソース編集では、固定済みNix環境と構成指紋ごとの永続Ninjaビルドツリーを使う増分ワークフローを推奨します。

## 固定ツールチェーン

現在の正確な固定値は[`packaging/ios/versions.env`](../../packaging/ios/versions.env)にあります。

| コンポーネント | 固定値 |
| --- | --- |
| Kritaベースリビジョン | `7173825999953623d28777a163a65b42a3f26f0a` |
| ホスト | Apple Silicon搭載macOS（`aarch64-darwin`） |
| Nix | 2.31以上 |
| Xcode | 26.6 (`17F113`) |
| iPhoneOS SDK | 26.5 (`23F81a`) |
| Apple Clang | 21.0.0 (`2100.1.1.101`) |
| Qt | 6.11.1 |
| KDE Frameworks／ECM | 6.28.0 |
| デプロイメントターゲット | iOS／iPadOS 17.0 |
| アーキテクチャ | arm64 |

バンドルはAppleの`iPhoneOS` SDKを使用し、iPhoneとiPadの両方を対象にします。通常の開発では固定バージョンを維持し、バージョン更新は別の検証作業として扱います。

## 前提環境

- `/Applications/Xcode.app`に上表のXcodeがインストールされているApple Silicon搭載Mac
- Nix 2.31以降とフレークを利用できるNixデーモン
- AltStoreで自動配備する場合は、iOS／iPadOS 17以降のiPhoneまたはiPad、USB接続、ロック解除、Macへの信頼設定、開発者モード
- AltStoreで自動配備する場合は、Mac上のAltServer、端末上で設定済みのAltStore、必要なローカル開発用署名環境、およびMacと端末間のローカルネットワーク接続
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
nix develop .#librepaint-ios --command packaging/ios/scripts/check-host.sh
```

このチェックでは、Xcode、SDK、Clang、Nix、CMakeなどのバージョンに加え、Nixデーモンのサンドボックスポリシーも検証します。

## 最初の増分ビルド

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

## 通常の開発ビルド

変更後は、Ninjaが予定している処理を確認してから増分ビルドします。

```sh
packaging/ios/scripts/build-librepaint-incremental.sh plan
packaging/ios/scripts/build-librepaint-incremental.sh build
```

`path`は選択中のビルドツリーを表示します。通常の`build`と`deploy`は、意図しないフルリビルドを事前に発見できるよう、既定で200個のNinjaステップを超える`plan`を拒否します。構成を意図的に大きく変えた場合は内容を確認し、`bootstrap`で新しいベースラインを構築してください。

日常の編集・ビルド・テストのサイクルでは、このラッパーを使用します。直接の`cmake --preset`は構成作業、`nix build .#librepaint-ios-ipa`はクリーンチェックポイントに使用します。旧`build-krita-incremental.sh`と`krita-ios-*`のエントリーポイントは、互換エイリアスとして維持します。

## 再現可能なアプリと未署名IPA

クリーンチェックポイント用のアプリバンドルとIPAは、Nixから構築できます。

```sh
nix build .#librepaint-ios-app \
  --out-link build-ios/nix-results/librepaint-ios-app
nix build .#librepaint-ios-ipa \
  --out-link build-ios/nix-results/librepaint-ios-ipa
```

成果物は次の場所に生成されます。

- `build-ios/nix-results/librepaint-ios-app/LibrePaint.app`
- `build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iOS-unsigned.ipa`

`librepaint-ios-ipa`は必要なアプリと依存関係も自動的に構築します。生成されるIPAは未署名です。署名情報、プロビジョニングプロファイル、Apple ID、デバイス認証情報はリポジトリの外で管理してください。

## AltStoreで実機へ配備

前提環境を満たしてAltServerを起動した後、次のコマンドで増分ビルド、バイナリ・プラグイン・ランタイムデータの検査、IPA生成、AltStoreによる署名とインストール、LibrePaintの起動、起動ログの回収まで行います。

```sh
packaging/ios/scripts/build-librepaint-incremental.sh deploy [device-id]
```

`device-id`を省略すると、最初に見つかった利用可能なCoreDeviceを選択します。接続端末は次のコマンドで確認できます。

```sh
xcrun devicectl list devices
```

タイムスタンプ付きIPAと回収した`librepaint.log`は`build-ios/deploy/`に保存されます。このワークフローは`packaging/ios/scripts/deploy-altstore.sh --skip-build`を内部ハンドオフに使用し、現在の正確なビルドツリーを渡します。

このワークフローは、作者のローカル利用に必要な開発用署名を行います。

## LiveContainerで実機へインストール

再現可能な未署名IPA `build-ios/nix-results/librepaint-ios-ipa/LibrePaint-iOS-unsigned.ipa`は、LiveContainerへインポートして利用することもできます。パッケージングワークフローは、LiveContainerがアプリバンドルへパッチを適用し、起動、クリーンアップするために必要なアーカイブ権限を正規化します。LiveContainerのiOS 26 JITなしモードを使用した新規インポートと起動をiPad実機で確認済みです。

以前の失敗したインポートにより、LiveContainer内へ読み取り専用の一時`Payload`が残ることがあります。この古い状態によるエラーが発生した場合は、必要なアプリデータを保護し、影響を受けたLiveContainerの状態をクリーンアップまたはリセットしてから修正版IPAをインポートしてください。正確なクリーンアップ画面の実機確認が残る復旧項目です。現在のアーカイブ権限と復旧上の注意は[`docs/ios/altstore-deployment.md`](../../docs/ios/altstore-deployment.md)を参照してください。

## シミュレーターのスモークテスト

```sh
nix develop .#librepaint-ios --command packaging/ios/scripts/build-smoke.sh simulator
```

このスモークテストはObjective-C++、UIKit、SDK、デプロイメントターゲット、バンドルメタデータを診断します。ランタイムの受け入れ確認には実機試験を使用します。

## 依存関係レシピを変更するメンテナー向け注意

`packaging/ios/scripts/bootstrap-ios-dependencies.sh --confirm-pinning-complete`は、依存関係レシピの固定完了時に使う保守手順です。実行するのは、すべての依存関係レシピを固定してコミットし、ルートで保護されていない既存キャッシュ出力を破棄できる段階です。この手順は既知の旧GCルートを解放し、**Nixのフルガベージコレクションを実行してから**最終アグリゲートを再構築します。通常のソース開発では、上記の増分ワークフローを使用します。

## 依存物の診断と部分構築

日常の開発環境と検証は[共通開発マニュアル](../architecture/DEVELOPMENT.md)を参照します。
依存物の責務とリンク契約は[iOS依存構築の設計](../ios/dependency-design.md)に記載します。
以下の個別診断は、初回に開いたiOS開発シェル内で続けて実行します。

```sh
nix develop .#librepaint-ios
packaging/ios/scripts/check-host.sh
```

利用者キャッシュに書き込めない環境では、初回のシェル起動に
`XDG_CACHE_HOME="$PWD/.cache/nix" nix develop .#librepaint-ios`を使用します。

ソースからの依存構築と利用側のリンクを明示的に検証する場合は、代替取得を無効にします。

```sh
nix build .#ios-dependencies --no-link --no-substitute
nix build .#kf6-consumer-check --no-link --no-substitute
```

個別の依存構築スクリプトでは、プラットフォームの後ろに名前を渡すと対象とその依存物を、
名前を省略するとマニフェストの全対象を構築します。

```sh
packaging/ios/scripts/build-dependencies.sh device harfbuzz
packaging/ios/scripts/build-dependencies.sh device
packaging/ios/scripts/probe-dependencies.sh device
```

実機とシミュレーターの依存配置は別々です。静的アーカイブの全要素について
アーキテクチャとAppleプラットフォームを検査し、成功した構築だけを記録します。
依存グラフと設定は`packaging/ios/deps/dependencies.json`、検証範囲は
[M2検証記録](../ios/validation-m2.md)にあります。

QtとQt依存ライブラリーは次の順に構築・検査します。

```sh
packaging/ios/scripts/build-dependencies.sh device
packaging/ios/scripts/build-qt.sh device
packaging/ios/scripts/build-dependencies.sh device
packaging/ios/scripts/probe-qt.sh device
```

Qt構築はソース、Xcode／SDK、レシピの構成指紋で再利用を決め、再利用時も静的アーカイブを
検査します。入力変更後にQt専用の構築ディレクトリーを再作成する場合は、
`packaging/ios/scripts/build-qt.sh device --clean`を使います。
Qtの試験用アプリはCore、Gui、Widgets、Xml、Network、Svg、Concurrent、Sql、OpenGL、
OpenGLWidgets、Core5Compat、iOSプラットフォーム、静的補助プラグイン、QuaZipをリンクします。
PrintSupportはiOSプロファイルの対象外のため、必須依存への混入を検査します。

続いてECM／KF6を構築します。

```sh
packaging/ios/scripts/build-frameworks.sh device
packaging/ios/scripts/probe-frameworks.sh device
```

`kconfig_compiler_kf6`はmacOS上で実行し、KF6ライブラリーはiOS向けに構築します。
試験用アプリはホスト側の設定生成とConfig、WidgetsAddons、Codecs、Completion、CoreAddons、
GuiAddons、I18n、ItemViews、ColorSchemeのリンクを検証します。
正本は`packaging/ios/frameworks/frameworks.json`です。

実機用の最小アプリによるUIKit／SDK診断は、同じシェルで実行できます。

```sh
packaging/ios/scripts/build-smoke.sh device
```

## 出力、ログ、プラグイン目録

| パス | 内容 |
| --- | --- |
| `build-ios/` | アプリ、IPA、実機／シミュレーター構築木、Nixプロファイル |
| `build-ios/deploy/` | 配備したIPAと起動ログ |
| `logs/ios/` | タイムスタンプ付きのコマンドログ |
| `packaging/ios/manifests/plugins.json` | プラグイン目録 |
| `packaging/ios/manifests/dependencies.json` | 依存物目録 |
| `packaging/ios/manifests/initial-plugin-profile.json` | 静的プラグインの採用設定 |

プラグイン対象を追加・削除した場合は、開発シェルで目録を再生成します。

```sh
python3 packaging/ios/scripts/inventory-plugins.py
```

ローカル成果物、署名済み成果物、認証情報、キャッシュ秘密鍵はGitの外で管理します。

## キャッシュの保守

Nixストアを通常の構築キャッシュとして使用します。ストアのごみ収集と独立して保持する
ローカルバイナリキャッシュへ保存・復元する場合は、次の入口を使います。

```sh
packaging/ios/scripts/publish-nix-cache.sh .#zlib-ios
packaging/ios/scripts/restore-nix-cache.sh .#zlib-ios
```

既定の保存先はGit対象外の`build-ios/nix-binary-cache`です。
`KRITA_IOS_NIX_CACHE_URI`で`nix copy`対応の私有ストアURIを指定できます。
ファイル以外の宛先には`KRITA_IOS_NIX_CACHE_SIGNING_KEY`も必要です。
専用の送信方式を使うサービスでは、そのサービスのクライアントを使用します。

複数Macで共有する場合は、Nix設定に私有キャッシュと信頼する公開鍵を登録します。
Apple SDK由来の成果物は私有キャッシュで管理し、署名用の認証情報はキャッシュ対象から
外します。Xcode、SDK、Apple Clangの正確な版が構築入力になり、構成ごとにキャッシュを分離します。

```sh
nix path-info --recursive .#devShells.aarch64-darwin.librepaint-ios
```

増分配備は選択したCMake／Ninja木を`maintain-build-cache.sh`へ渡し、そのグラフが参照する
Nix入力を保護します。空き容量が閾値未満の場合、または明示した強制実行時に、保護後の
ごみ収集を行います。依存レシピ固定作業と通常配備のキャッシュ保守は別の操作として扱い、
依存レシピの固定中は旧開発シェルや構築入力の保護・配備保守を開始しません。
先述の初期再構築は、レシピを固定・コミットしたクリーンな状態で、削除範囲への承認を得て
実行する保守操作です。通常の編集では評価済みプロファイルと増分構築木を維持します。

## 実機検証と保守資料

構築成功、IPA検査、実機操作の成功を別々に記録します。対象の操作、端末、設定、ファイル、
試験時間を固定し、描画、保存と再読込、入力、画面回転、前景復帰を変更範囲に応じて確認します。
署名、IPA権限、LiveContainerの復旧手順は
[配備手順](../ios/altstore-deployment.md)、機能範囲は
[機能定義](../ios/feature-scope.md)、残作業は[プラットフォームTODO](../../TODO.md)を参照します。

### 機能別の操作検証

以下は2026年8月9日時点の検証対象です。
実施済み範囲と次の対象は[プラットフォームTODO](../../TODO.md)と
[現在の作業状況](../architecture/PROGRESS.md)で確認します。

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
