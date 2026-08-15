# Windows依存構造TODO

この文書は、LibrePaintのWindowsビルドからKrita固有の依存ライブラリ改変とパッケージ供給元固有の回避策を除去するための実装計画である。最終状態では、vcpkg、nixpkgs、MSYS2のいずれかが供給する未改変の依存ライブラリを、共通のCMake依存契約を通じて利用できるようにする。

## 目標

- Krita向けQtフォーク固有APIをLibrePaintから削除し、未改変のQtで同等機能を提供する別実装へ移行する。
- Linux上で動く生成ツールとWindows向けライブラリを、CMake上で明確に分離する。
- vcpkg、nixpkgs、MSYS2の標準パッケージが公開するCMakeターゲットまたはpkg-config情報から依存関係を解決する。
- 公式Windows機能構成を機械可読にし、依存不足による暗黙の機能欠落を構成時エラーにする。
- パッケージの不具合は可能な限り各パッケージ供給元または依存ライブラリのupstreamで修正し、LibrePaint内の複製レシピとパッチを削除する。

## 基本方針

- LibrePaintのCMakeコードは、Nix store、vcpkgの配置、MSYS2のprefixを直接参照しない。
- 依存ライブラリは名前付きのimported targetでリンクし、ファイル名や取込ライブラリの配置を仮定しない。
- ホストOSの判定には`CMAKE_HOST_*`を、Windows対象の判定には`WIN32`とtoolchain情報を使用する。
- 配布向け機能プロファイルでは、必要な依存関係を任意扱いにしない。
- ローカル回避策には削除条件を設け、恒久的な依存実装として扱わない。

## P0: クロスビルド境界の再設計

- [ ] ホスト用Python、SIP、PyQtメタデータ取得処理と、Windows対象用Pythonヘッダー・取込ライブラリを別のCMake変数とターゲットに分離する。
- [ ] `moc`、`rcc`、`uic`、`lrelease`、QML生成ツールに明示的なホストツール経路を用意し、対象Qtの設定処理と同じ環境へ混在させない。
- [ ] パス区切りや実行可能形式の判定から対象側の`WIN32`依存を除去し、ホスト側の判定へ置き換える。
- [ ] vcpkg toolchain、NixのMinGW toolchain、MSYS2のMinGW/UCRT toolchainで同じCMake入力を使用できるようにする。
- [ ] `nix/windows/krita.nix`にある`FindPythonLibrary.cmake`、`FindPyQt6.cmake`、`FindSIP.cmake`、`SIPMacros.cmake`の書換えを削除する。

### 完了条件

- [ ] ホスト用実行ファイルが対象ライブラリのprefixから探索されない。
- [ ] 対象用Windows実行ファイルを構成中または生成中に実行しない。
- [ ] パッケージ供給元ごとのPython・Qt探索コードをLibrePaint本体に持たない。

## P0: PyKrita/SIP API境界の修正

- [ ] Pythonから通常呼び出す`DockWidget::canvas()`を、C++側でも意図が一致する公開APIとして定義する。
- [ ] `canvasChanged()`を派生Pythonクラス向け仮想コールバックとして定義し、SIPの生成する橋渡しクラスから安全に呼び出す。
- [ ] MinGWを含む全対象で同じSIP公開範囲を生成する。
- [ ] `protected=public`と`SIP_PROTECTED_IS_PUBLIC`によるヘッダー全体のアクセス指定書換えを廃止する。
- [ ] `cmake/modules/SIPMacros.cmake`のMinGW向け暫定修正を削除する。

### 完了条件

- [ ] PyKritaの生成コードが保護メソッドを基底クラスポインターから直接呼ばない。
- [ ] コンパイラー定義によるアクセス指定の書換えなしでWindows向けPyKritaを構築できる。

## P0: Krita Qtフォーク固有APIの撤去

- [ ] `setTextureColorSpace()`など、Krita向けQtフォークだけが提供するAPIの利用箇所を全件列挙する。
- [ ] 画面色空間、HDR出力、テクスチャ色空間設定をLibrePaint側の描画・画面情報抽象へ集約する。
- [ ] フォーク固有APIを使わず、標準QtとWindowsの公開APIから同等機能を提供する実装へ置き換える。
- [ ] DXGI、標準Qt、必要に応じて独立した描画バックエンドを組み合わせ、HDRと広色域表示を維持する。
- [ ] `dimula73/qtbase`の固定、Qt版情報の書換え、フォーク専用パッチを`nix/windows/krita.nix`から削除する。
- [ ] 独自ANGLE構築が不要になった時点で`nix/windows/google-angle.nix`を削除し、Qtまたはパッケージ供給元が提供する描画バックエンドへ移行する。

### 完了条件

- [ ] LibrePaintのソースがKrita Qtフォーク固有のシンボルを参照しない。
- [ ] vcpkg、nixpkgs、MSYS2の未改変QtでHDR、広色域表示、Windowsタブレット入力を構築できる。
- [ ] Qtモジュールの版情報を書き換えず、同一リリース系列のQt一式を利用できる。

## P1: パッケージ供給元に依存しないCMake契約

- [ ] WebP、HEIF、JPEG XL、TIFF、OpenEXR、OpenColorIO、Poppler、FFmpeg、MLT、G'MIC、Python/PyQtを名前付きCMakeターゲットで解決する。
- [ ] package configを優先し、必要な場合だけ保守された`Find*.cmake`とpkg-configへ段階的にフォールバックする。
- [ ] WebPのmux/demuxなど、部品名と公開ターゲット名の対応をCMake側で正規化する。
- [ ] 絶対パス、ライブラリファイル名、特定prefixのディレクトリー構造に依存するリンク指定を除去する。
- [ ] 同じ機能名と依存ターゲット名をvcpkg、nixpkgs、MSYS2で共有する。

### 完了条件

- [ ] パッケージ供給元の切替えでLibrePaintのソースや`CMakeLists.txt`を変更しない。
- [ ] 各依存の探索結果と有効機能を構成要約として出力する。
- [ ] 必須ターゲットが不足する場合は、該当機能名と不足ターゲットを示して構成時に失敗する。

## P1: 公式Windows機能プロファイル

- [ ] 公式Windows版で必要な機能を、CMake presetまたは機械可読な機能プロファイルとして定義する。
- [ ] Python/PyQt、G'MIC、FFmpeg/MLT、OpenColorIO、Poppler、HDR、ANGLE相当の描画経路、主要画像形式を必須機能として列挙する。
- [ ] 必須依存が見つからない場合のプラグイン自動除外を禁止する。
- [ ] Nixレシピに並ぶ`CMAKE_REQUIRE_FIND_PACKAGE_*`を、LibrePaint側の公式プロファイルへ移す。
- [ ] vcpkg、nixpkgs、MSYS2の各構成が同じ機能プロファイルを満たすことを比較可能な構成要約で示す。

### 完了条件

- [ ] 「構築成功だが公式版より機能が少ない」配布物を構成段階で拒否できる。
- [ ] 公式機能集合の変更が一つのプロファイル差分として確認できる。

## P1: 依存パッケージ側へ移す修正

- [ ] MinGW版libwebpの完全なmux取込ライブラリを、libwebpまたは各パッケージ供給元で生成する。完了後、`nix/windows/krita.nix`のDLL公開表からの再生成処理を削除する。
- [ ] G'MICのWindows pthread依存とホストQt翻訳ツール依存をG'MICまたは各パッケージ定義で宣言する。完了後、`nix/windows/default.nix`の追加依存と専用Qt Toolsラッパーを削除する。
- [ ] `nix/windows/giflib-mingw.patch`と`nix/windows/fontconfig-mingw.patch`の修正をupstreamまたは各パッケージ供給元へ反映し、ローカルパッチを削除する。
- [ ] DrMingwをvcpkg、nixpkgs、MSYS2の標準パッケージから取得できるようにし、`nix/windows/drmingw.nix`を削除する。
- [ ] 独自に固定している依存ライブラリを一覧化し、標準パッケージへ置換できたものからローカルレシピを削除する。

## P1: Qt 5からQt 6への実行時互換性棚卸し

Qt 6でコンパイルが通ることだけを移行完了条件にしない。今回、Windowsの既定style名が
`windowsvista`から`windows11`へ変わったことを判定できず、暗色paletteとnative styleが競合して
文字色と境界線が崩れた。このような、APIには現れにくい識別子、既定値、plugin、描画結果の
変化をQt 5版の上流Kritaと比較して洗い出す。

- [ ] style、platform、theme、icon、image format、TLS、SQL、multimediaなど、名前で分岐または探索するQt pluginと`objectName()`の利用箇所を全件列挙し、Qt 5/6およびWindows版ごとの実値を記録する。
- [ ] palette、style、font、icon theme、high-DPI scaling、画面DPI、native dialogの既定値と適用順序を比較し、light/darkおよびWindowsの表示倍率ごとに画面差分を採取する。
- [ ] Qt 6で変更・廃止されたenum、property、signal/slot、event、入力座標、wheel/tablet/touch処理、正規表現、文字コード、locale/time zone、URL/path処理の利用箇所を静的検索し、暗黙の互換処理を明文化する。
- [ ] Qt 5のOpenGL経路とQt 6のOpenGL/RHI/ANGLE経路について、surface format、color space、alpha、swap interval、device loss、software fallbackの既定挙動を比較する。
- [ ] 設定値、window state、shortcut、recent files、resource database、clipboard、drag and drop、MIME dataについて、Qt 5で保存した状態をQt 6で読み込むupgrade試験とrollback試験を行う。
- [ ] 起動、splash、初回cache/database生成、終了処理、クラッシュハンドラーについて、上流KritaとLibrePaintの時系列ログ、応答状態、スクリーンショットを同条件で比較する。
- [ ] Qtのminor/patch更新時にも同じ比較を再実行できるよう、採取する環境情報、ログ、代表画面、入力シナリオを自動検証レシピにする。

### 完了条件

- [ ] Qt 5依存の識別子・既定動作・互換分岐が、該当ソース、Qt 6での期待値、検証方法とともに一覧化されている。
- [ ] 起動直後、workspace、設定画面、file dialog、canvas、主要dock、light/dark、100%/高DPIの基準スクリーンショットに重大な意図しない差分がない。
- [ ] Qt 6またはQt pluginの更新で比較試験が失敗し、今回の`windows11` style判定漏れと同種の退行を配布前に検出できる。

## P2: Qt依存セットの版管理

- [ ] qtbase、qtdeclarative、qtsvg、qttools、qttranslationsを一つの対応表またはlock情報で管理する。
- [ ] 異なるQtパッチ版のモジュール混在を構成時に拒否する。
- [ ] Qtフォーク廃止までの移行期間は、フォークの基点と全Qtモジュールの版を一致させる。
- [ ] パッケージ供給元が提供する単一Qtリリースをそのまま利用できる状態を最終条件とする。

## 最終完了条件

- [ ] vcpkg、nixpkgs、MSYS2の各標準依存セットから、ローカルな依存ソース構築・修復パッチなしで公式Windows機能プロファイルを構成できる。
- [ ] パッケージ供給元固有の処理はtoolchainまたはパッケージ定義に閉じ、LibrePaintのアプリケーションコードと共通CMakeコードへ侵入しない。
- [ ] Krita Qtフォーク、`google-angle.nix`、WebP取込ライブラリ再生成、G'MIC専用Qt Toolsラッパー、SIPアクセス指定書換えを削除できる。
- [ ] Windows依存の更新が、個別依存の局所的な再構築またはキャッシュ取得で完了し、Qtや依存ツリー全体の不要な再構築を誘発しない。

