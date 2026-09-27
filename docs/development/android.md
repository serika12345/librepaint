# Android開発マニュアル

x86_64 Linux上でAndroid向けのAPK／AABを構築し、端末で検証する手順です。
[共通開発マニュアル](../architecture/DEVELOPMENT.md)でNix環境を準備し、
コマンドをリポジトリルートで実行します。

## 構成と固定バージョン

`arm64-v8a`を実機向け、`x86_64`を実機とWaydroidでの診断向けに用意します。
NixはQt、KDE Frameworks、C／C++依存物、LibrePaint、APK包装を別々に構築します。
SDK／NDKは固定した配布物を使用し、Gradleは記録済みの応答から依存物を取得します。

| 項目 | 固定値 |
| --- | --- |
| Qt／KDE Frameworks | 6.11.1／6.28.0 |
| Android NDK | 27.3.13750724 |
| compile SDK／target SDK | 35 |
| SDK Build-Tools | 35.0.0 |
| minimum SDK | 28 |
| JDK／Gradle／Android Gradle Plugin | 17／8.13／8.12 |
| C++ | C++17、共有LLVM C++実行時ライブラリー |

正本は`nix/android/`、ソース情報を共有するiOS依存マニフェスト、`flake.lock`、
`nix/android/gradle-deps.json`です。バージョン更新時は固定ハッシュを更新し、両ABIを再検証します。
パッチと構築フラグは構築メタデータとして共有します。依存物は固定レシピから構築し、
同一のNix派生物をバイナリキャッシュから復元できます。

## 増分構築

共通開発環境で、ABIごとの永続Ninja木とコンパイラーキャッシュを利用します。

```sh
build-incremental android configure
build-incremental android plan krita
build-incremental android build krita
build-incremental android package-product
build-incremental android-x86_64 configure
build-incremental android-x86_64 build krita
```

初回に個別の開発シェルが必要な場合は、次の入口を使います。

```sh
nix develop path:.#librepaint-android
nix develop path:.#librepaint-android-x86_64
```

CMakeはQt 6の実行対象を`qt_finalize_executable()`で確定し、
`QT_ANDROID_PACKAGE_SOURCE_DIR`で`packaging/android/apk`を`androiddeployqt`へ渡します。
`create-apk-krita`と`create-aab-krita`がAPKとAABを生成します。

## 再現可能な成果物

増分構築と端末検証の完了後に、製品全体を構築します。

```sh
nix build path:.#librepaint-android
nix build path:.#librepaint-android-x86_64
```

各コマンドの`result`には、そのABIの`LibrePaint-<ABI>.apk`と
`LibrePaint-<ABI>.aab`が生成されます。各構築はABI、ELF機械種別、16 KiBの
ロードセグメント整列、単一の共有C++実行時ライブラリー、Qt 6／KF6、Androidプラグイン、
資源、Manifest、最小／対象SDKを監査し、Qt 5／KF5と旧Java名前空間の混入を診断します。

依存物だけの診断やキャッシュ準備には次の出力を使います。

```sh
nix build path:.#android-source-dependencies
nix build path:.#qtbase-android
nix build path:.#android-kf6
nix build path:.#android-application-dependencies
nix build path:.#android-dependencies
```

x86_64版は順に`android-x86_64-source-dependencies`、`qtbase-android-x86_64`、
`android-x86_64-kf6`、`android-x86_64-application-dependencies`、
`android-x86_64-dependencies`です。製品ソースの編集ではこれらの依存物を再利用します。

## Qt Testと端末への導入

Qt TestはABIに対応する共有ライブラリーとして構築し、選択した一対象と推移的な実行時
ライブラリーをAPKに収めます。依存物固定表、構成指紋、Ninja木、コンパイラーキャッシュ、
APKをABIごとに分離します。試験の`data/`はAPKのassetsに収め、Activityがアプリ専用
外部領域へ展開します。試験はその領域を作業ディレクトリーとして使用します。

一つのQt Testを監査済み未署名APKにまとめ、接続先で実行します。

```sh
build-incremental android-x86_64 package-test KisCurveOptionModelTest
adb connect <Waydroidまたは実機の接続先>
build-incremental android-x86_64 run-test KisCurveOptionModelTest [adb-serial]
```

ARM64実機では次を使います。

```sh
build-incremental android run-test KisCurveOptionModelTest [adb-serial]
```

実行処理は端末ABIを検査し、リポジトリのキャッシュ領域で作った試験専用鍵により実行用複製へ
署名します。導入、Qt 6 Activity起動、xUnit XMLとlogcat回収、停止、試験パッケージ削除まで
実施し、結果を選択中の構築木の`test-results/<target>/`へ保存します。
未署名の監査済みAPKは元の状態で保持します。

製品のリリース署名は再現可能な構築の後段で行います。配布・導入の権限を持つ鍵で署名した
APKを、固定環境のADBで導入します。

```sh
adb install -r <signed-apk>
adb logcat
```

アプリ識別子`org.krita`とネイティブ対象名`krita`は互換性のため維持します。

## 依存物と互換性の保守

Gradle依存物の更新は、同じAndroid Gradle Plugin、AndroidX、SDKを使う最小プロジェクトで
応答とハッシュを更新します。最小リリースAPKの包装まで実行するため、包装時に解決される
依存物も記録します。その後、通常のネットワークなし構築を検証します。

```sh
gradle_update_script="$(nix build --no-link --print-out-paths \
  path:.#librepaint-android.gradleDepsUpdate)"
"$gradle_update_script"
```

Android ActivityはQt初期化前に`QT_ANDROID_DISABLE_ACCESSIBILITY=1`を設定します。
Qt 6.11.1では、アクセシビリティ照会がQt事象ループを待つ間に新規文書ダイアログなどの
別のOpenGL画面を作ると異常終了するため、この互換性設定を維持します。
根本原因は[QTBUG-140490](https://qt-project.atlassian.net/browse/QTBUG-140490)、
先行修正は[QTBUG-140674](https://qt-project.atlassian.net/browse/QTBUG-140674)、
修正案は[Gerrit 735089](https://codereview.qt-project.org/c/qt/qtbase/+/735089)に対応します。
R5のアクセシビリティ検査段階で最低Qt版への修正取り込みを確認し、設定を解除して
実機のダイアログとアクセシビリティ操作を反復検証します。
