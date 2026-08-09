# iPadOS port: Android分岐再利用の棚卸し

## 適用範囲

Krita upstream本体へ加えるAndroid再利用差分は、既存のAndroid条件へ
iOSを追加する一行の条件変更に限定し、既存の処理本体をそのまま保つ。

許可する変更形式は次の同等形だけである。

```cpp
#ifdef Q_OS_ANDROID
```

を

```cpp
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
```

へ変更する。または、除外条件について

```cpp
#ifndef Q_OS_ANDROID
```

を

```cpp
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
```

へ変更する。CMakeについても、既存の`ANDROID`条件へ`IOS`を追加するだけの
同等な一行変更に限る。

次の変更はAndroid再利用差分の対象外とする。

- 共通helperの抽出、関数の移動、rename、class追加
- AndroidとiOSの処理を揃えるための既存処理本体の編集
- OS判定から新しいcapability abstractionへの移行
- guard変更に付随するnull check、ログ、同期方法などの改善
- upstream本体の広範な非同期化

一行の条件追加で成立する機能だけをAndroid流用として扱う。それ以外は
iOS adapterを独立ファイルへ隔離するか、現状のiOS専用分岐で扱う。

## 調査範囲

Krita基準revision
`7173825999953623d28777a163a65b42a3f26f0a`から、棚卸し前HEAD
`b05bf0486821a82764f9b3e9e72a39548d874205`までの82コミット・309ファイルを
対象とした。

このうち123ファイルは`nix/ios`、44ファイルは`packaging/ios`、10ファイルは
`docs/ios`であり、差分量の大半はKrita本体処理ではなくiOSビルド・配備境界で
ある。

## 判定1: Android条件へiOSを追加するだけで流用できるもの

次の変更は既存Android/非Android処理本体を変更せず、条件だけを拡張している。
現在の方針に適合する。

| 対象 | 条件変更 | 状態 |
|---|---|---|
| Video Animation import | `!Q_OS_ANDROID`へ`!Q_OS_IOS`を追加 | 実装済み。両OSのprofileはprocess起動型FFmpeg機能を対象外とする。 |
| Main Windowのdesktop widget style列挙 | `!Q_OS_ANDROID`へ`!Q_OS_IOS`を追加 | 実装済み。mobile profileはapplication側で選択したwidget styleを使う。 |
| TabletRelease後のmouse event再許可 | macOS/Android条件へiOSを追加 | 実装済み。処理本体は共通。 |
| pickerが返したURLへの拡張子後付け禁止 | macOS/Android条件へiOSを追加 | 実装済み。pickerが返したsecurity-scoped URLを維持する。 |
| `krita_version`補助実行ファイル | `NOT ANDROID`へ`NOT IOS`を追加 | 実装済み。mobile bundleはapplication executableだけを収録する。 |
| Small Color Selectorの除外 | `NOT ANDROID`へ`NOT IOS`を追加 | 実装済み。既存plugin本体は現状を維持する。 |

現在のAndroid固有ブロックに対する棚卸し結果は、上記6件の実装で完結して
いる。

## 判定2: platform-neutralな共通実装

次はAndroid分岐の流用ではなく、元からplatform-neutralなKrita pluginまたは
処理である。iOS側は主として依存構築、静的リンク、登録、runtime data同梱を
担当する。

- Brush、PaintOp、Tool、Docker、Filter、通常Generator
- KRA、PNG、JPEG、ORA
- CSV、SVG、QImageIO、XCF、PSD、QML、TGA、Heightmap、Brush、Spriter、
  KRZ、RGBE
- TIFF、OpenEXR、JPEG 2000、WebP
- GIF、HEIF、JPEG XL、RAW、PDF
- MyPaint、metadata、flake shapes、color-space extensions、Resource Manager
- SeExpr generator

これらの機能本体はplatform-neutralな実装を維持し、iOS固有処理を依存構築、
静的リンク、登録、runtime data同梱に限定する。

## 判定3: platform固有実装が必要なもの

| 対象 | Android側 | iOS側 | 判定 |
|---|---|---|---|
| Application pause/autosave通知 | Activity、JNI、foreground service | UIApplication通知、background task | native adapterをplatform別に維持し、保存処理には既存application APIを使う。 |
| Native file picker | Storage Access Framework URI | UIDocumentPicker、security-scoped URL | iOS adapterがMIME selectorとsecurity-scoped URL lifecycleを担当する。 |
| Plugin loading | ABI別`lib_krita*`動的load | 単一実行ファイルへの静的登録 | iOSは静的plugin registryを使う。 |
| Stylus初回接触 | Android QPA/S Pen event順序 | hover/EnterなしのApple Pencil press | iOS branchがApple Pencil固有の初回press順序を処理する。 |
| 補助stylus action | S Pen plugin/JNI | `UIPencilInteraction` | 各native bridgeが共通のKrita actionを呼び出す。 |
| Kinetic scrolling | Android touchとJNI long-press timeout | synthesized left-mouseとviewport gesture | iOS branchがviewport gestureを構成する。 |
| Memory pressure | Android adapterは未実装 | UIKit memory warningとjetsam向け上限 | 現行上限はiOSのUIKit／jetsam policyとして維持する。 |
| Fill Layer modal dialog | Androidでは同期`exec()`が成立 | Qt iOSの同期QPA配送で再入する | iOS非同期分岐がnested event loopを避ける。 |
| OpenColorIO/LUT Docker | Android buildでは明示的に除外 | desktop実装をiOS GLES向けに構築 | 現行のiOS限定build設定でdesktop実装を構築する。 |
| Recorder/animation export | Android media encoder | iOS encoderなし | iOS backendの設計までdeferredとする。 |
| SVG Text | Android window/IME処理あり | Qt Quick/QML未搭載、iOS IME未検証 | Qt Quick/QML導入とiOS IME検証を前提とするdeferred項目。 |
| Text Properties | Android/desktop Qt Quick UI | Qt Quick/QML未搭載 | Qt Quick/QML導入を前提とするdeferred項目。 |
| Storyboard/printing | Android/desktop PrintSupport | iOS Qt profileにPrintSupportなし | 現行iOS profileの対象外。 |
| Splash、Preferences、ComboBox、item-view keyboard | Androidとは異なるQt/UI挙動 | UIKit scene、UIPicker、iOS IME | UIKit／iOS IME固有分岐を維持する。 |

## 判定4: upstream提案として扱うcleanup

次の共通化・一般化はlocal port patchの範囲外とし、必要に応じて独立した
upstream提案として扱う。

- Android/iOS pause保存処理の共通関数化
- Android/iOS MIME selectorやfile picker state machineの統合
- Pencil/S Pen action dispatcherの抽出
- Fill Layer非同期処理の全platform共通化
- LUT Dockerの`IOS`判定を新しいGLES capability判定へ変更
- `KisOpenGLIOSCompat.h`のAndroid/GLES一般化
- iOS UI workaroundのmobile共通policy化
- node creationのqueued connection削除または再構成

LibRawのheap化、plugin factory symbol、IPTC初期化、MyPaint登録、CMake Find module
など、移植中に見つかったplatform-neutralな問題は、必要な現行patchを維持する
範囲に留める。一般修正への拡張は別途upstreamへ提案し、upstream側の変更として
取り込む。

## 今後の変更判定手順

新しい候補は次をすべて満たす場合だけ実装する。

1. upstreamに既存のAndroid分岐がある。
2. iOSでも分岐内の処理本体を同一に保てる。
3. 分岐内の処理がAndroid固有のJava/JNI/API、path、lifecycle、event orderingから独立している。
4. 変更が条件行への`Q_OS_IOS`追加だけで完結する。
5. iOS実機確認とAndroidの挙動同一性を検証できる。

条件外の候補は専用adapterまたはdeferred項目として扱い、この文書と`TODO.md`へ
理由を記録する。

## 検証状態

この棚卸しは実装境界を確定する。実機検証の状態は別に管理し、非同期Fill Layer、
LUT Docker、SeExprの操作確認を引き続き`TODO.md`で追跡する。実機ビルド
`20260805102358`でnode creationのqueued connectionだけを除くと、Pencil release中の
`QGestureManager::getState()`でクラッシュが再現した。したがって、queued connectionは
node／UI変更をtablet/mouse配送完了後へ送るために必要であり、nested event loopを除く
非同期Fill Layer処理とは異なる再入区間を防ぐ。
