# デスクトップ開発マニュアル

macOS、Linux、Windowsの成果物を構築し、配布前に検証するための手順です。
日常のソース編集と試験は[共通開発マニュアル](../architecture/DEVELOPMENT.md)の
評価済みNix環境と増分構築を使用します。以下のコマンドはリポジトリルートで実行します。

## macOSのNixビルド

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
| --- | --- |
| ホスト | Apple Silicon搭載macOS（`aarch64-darwin`） |
| コンパイラー | nixpkgsのLLVM Clang 21.1.8 |
| リンカー／アーカイブツール | nixpkgsのcctools／ld64 |
| SDK | Nixストア内のApple SDK 14.4 |
| Qt | 6.11.1 |
| KDE Frameworks／ECM | 6.28.0 |
| デプロイメントターゲット | macOS 14.0 |
| アーキテクチャ | arm64 |

固定済みのNixグラフが、宣言済みのビルドツールチェーンと依存関係一式を供給します。Darwin向けツールチェーンは、nixpkgsのLLVM Clang、cctools、SDK、オープンソースの`xcbuild`パッケージで構成しています。

ネイティブC++デスクトッププロファイルには、描画アプリケーション、動的プラグイン、PopplerによるPDFインポート、LibRaw／KDcrawによるRAWインポート、KSeExprジェネレーター、OpenColorIO、MLT／SDLによる音声・動画サポート、FFmpeg／FFprobe、および[`nix/macos/krita.nix`](../../nix/macos/krita.nix)で宣言した画像形式ライブラリーが含まれます。

次のmacOS依存関係対応では、Python／PyQtスクリプティングクロージャと埋め込みランタイムパスの統合を進めます。

このNix出力は、ランタイムライブラリーをNixクロージャに保持する、再現可能な開発・チェックポイント用バンドルです。配布レシピでは、このビルドにスタンドアロンバンドル化、DMG生成、署名、公証を重ねます。

## LinuxのNixビルド

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

## WindowsのNixクロスビルド

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

## LinuxのローカルAppImage作成

`packaging/linux/appimage/`のスクリプトは、準備済み依存配置からコンパイルと
AppImage組立を行います。Debian互換のLinux環境と通常のC／C++ツールチェーン、
`bash`、`cmake`、`dpkg`、`git`、`nproc`、`patchelf`、`realpath`、`rsync`を使用します。
恒久的な開発環境にないコマンドはNixの一時シェルで用意します。

`LIBREPAINT_DEPS_PATH`へアーキテクチャの一致する依存配置の絶対パスを指定します。
依存配置にはQt／KDE Frameworksの開発ファイル、Python／PyQt、画像・媒体ライブラリー、
翻訳、MIMEデータ、`linuxdeployqt`とAppImage実行時ファイルが必要です。
同変数が未設定の場合だけ旧名`KRITA_DEPS_PATH`を使用します。

```sh
LIBREPAINT_DEPS_PATH=/absolute/path/to/dependencies packaging/linux/appimage/build-krita.sh /absolute/path/to/appimage-work "$PWD"
LIBREPAINT_DEPS_PATH=/absolute/path/to/dependencies packaging/linux/appimage/build-image.sh /absolute/path/to/appimage-work "$PWD"
```

2番目のコマンドが作業ディレクトリーへ
`LibrePaint-<version>-<revision>-<architecture>.AppImage`を生成します。
依存配置はこの処理の前に準備し、署名と公開は完成した成果物に対して行います。

## 検証と保守

Nix定義を変更した場合は共通マニュアルのNix評価検査を実行し、該当する名前付き出力を
構築します。配布前には対象OSで起動、描画、保存と再読込、プラグイン読込を確認し、
署名・包装を変更した場合はその成果物を使って検証します。
Windowsの依存構造と残作業は[#68](https://github.com/serika12345/librepaint/issues/68)で管理します。

## Windows依存構造の設計方針

Windows依存構造の改善は[Issue #68](https://github.com/serika12345/librepaint/issues/68)で追跡します。
共通CMakeは供給元の配置から独立した名前付き対象を利用し、ホストで実行する生成ツールと
Windows向けヘッダー・ライブラリーを分離します。ホスト判定は`CMAKE_HOST_*`、対象判定は
`WIN32`とツールチェーン情報が所有します。配布機能の必須依存は構成時に検査します。
供給元固有の修正はパッケージ定義に置き、暫定回避策には対応Issueと削除条件を付けます。
依存物の更新は対象派生物とキャッシュの境界に従って行います。
