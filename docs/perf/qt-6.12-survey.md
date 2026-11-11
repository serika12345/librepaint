# Qt を nixpkgs の版（6.12）に寄せるための事前調査

2026-11-11 に行った、実装前の調査の記録である。目的は、Qt を nixpkgs の標準の版に寄せて公開のバイナリキャッシュを使い、Windows はプラットフォームプラグイン（qwindows.dll）だけを改変して作る案（利用者が上策を選んだ）が成り立つかを確かめることである。実装はしていない。

## nixpkgs の版

| 項目 | 固定している nixpkgs（a5cbcfe954、2026-08-02） | nixpkgs-unstable（4fe25e12eb、2026-11-07） |
| --- | --- | --- |
| Qt | 6.11.1（LibrePaint は overlay で 6.11.3 に差し替え） | 6.12.0（67c8881f91、2026-10-08 に 6.11.2 から上がった） |
| Python | 3.14.6 | 3.14.8 |
| PyQt6 | — | 6.11.0 |
| KDE Frameworks | 6.28.0 | 6.31.0 |
| GCC（Linux と Windows の MinGW の両方） | 15.3.0 | 16.2.0 |
| CMake | 4.3.4 | 4.4.4 |

nixpkgs では GCC 16 への対応の修正が 2026-09 末から続き、Qt 6.12.0 はその後に入った。Qt 6.12 を持つ nixpkgs は、どれも GCC 16 になる。

## 6.11.3 に上げた理由の 2 つの不具合

- QTBUG-147854（Linux X11 で描くたびに 1 フレーム止まる）：修正の文（「Always re-apply the swap interval ... See QTBUG-147854」）が v6.12.0 の qglxintegration.cpp にある。
- QTBUG-147602（macOS で文書を開くと落ちる）：v6.12.0 の qimage_darwin.mm の履歴に、修正（9bf3e90fe3「Fix regression in QImage::toCGImage() with custom color space」、2026-06-22）がある。v6.11.3 の履歴には、同じ修正を移したもの（08e464f855）がある。
- どちらも 6.12.0 で直っている。

## 公開キャッシュ（cache.nixos.org）

nixpkgs-unstable の標準の定義での出力の有無：

| パッケージ | x86_64-linux | aarch64-darwin |
| --- | --- | --- |
| qtbase、qtdeclarative、qtsvg、qttools、qtmultimedia、qtimageformats | ある | ある |
| qtwayland | ある | — |
| PyQt6 | ある | ない |
| KDE Frameworks（kcoreaddons、ki18n で確認） | ある | ない |

- LibrePaint は、構築の時間を減らすために、nixpkgs の定義を変えている（nix/qt/overlay.nix の qtmultimedia から Qt Quick 3D を外す、qtdeclarative のパッチの取捨、nix/kde/overlay.nix の Python の接続部品を外す、nix/pyqt6.nix の PyQt6 の機能を絞る）。定義を変えると出力の識別子が変わり、公開キャッシュを使えない。キャッシュを使うには、これらの変更を外して標準の定義に戻す必要がある。外すと、使わない部品（Qt Quick 3D など）も入力に入るが、キャッシュから取るので構築の時間は増えない。配布物に入るかどうかは未確認。
- Windows（MinGW）の qtbase は、nixpkgs の標準の定義のままでは評価できない（libglvnd が Windows で使えないため）。nixpkgs の公式の構築でも作られず、公開キャッシュにない。今の設定の変更（libGL と vulkan-loader を外す）は 6.12 でも要り、Windows の qtbase 6.12 は手元で 1 回作る。
- iOS の Qt は、LibrePaint が版と取得元の hash を監査した定義（nix/ios/packages/qtbase.nix など、6.11.3）で作る。6.12.0 に合わせて定義を更新する必要がある。Android の Qt は overlay の qtbase の版に従う。どちらも公開キャッシュはない。

## Windows のプラグインのパッチ

- Krita のフォークの 051139ed72（Wintab の対応範囲）と 7bb46828ae（相対モードの無効化）は、v6.12.0 の src/plugins/platforms/windows/qwindowstabletsupport.cpp と .h にずれなく当たる（patch --dry-run）。このファイルは 6.11.3 と 6.12.0 で同じである。

## LibrePaint が使う Qt の内部用の API

LibrePaint のソースの 23 個のファイルが Qt の内部用のヘッダーを使う。主なヘッダーの 6.11.3 と 6.12.0 の差：

| ヘッダー | 変わった行 |
| --- | --- |
| qpa/qplatformnativeinterface.h、qguiapplication_p.h、qpa/qplatformscreen.h、qkeymapper_p.h、qeventpoint_p.h | 1 行（先頭の注記） |
| qpa/qplatformintegration.h | 4 行 |
| qxkbcommon_p.h | 0 行 |
| qpa/qplatformwindow_p.h | 27 行。QCocoaWindow の内部用の接続の版が 2 から 3 に上がり setContentBorderEnabled が消えた。HarmonyOS の追加。LibrePaint は QCocoaWindow を使っていない。使っている QWaylandWindow の部分は変わっていない |

qtwayland の内部用のヘッダー（qwaylandintegration_p.h、qwaylanddisplay_p.h）は、調べた場所が誤っていて比べられていない。

## 手戻りの危険

- GCC 15.3 から 16.2 への上がり：LibrePaint（Krita 由来の大きな C++ のコード）と依存の自前の構築（Windows の MinGW の全依存、Android、iOS の一部）が、新しいコンパイラーで新たな警告やエラーを出す可能性がある。量は、実際に構築するまで分からない。Windows のコンパイラーの包み（mcfgthread など）と Krita の依存の Windows 向けパッチも、GCC 16 で当て直しが要る可能性がある。
- Qt 6.12 と KDE Frameworks 6.31 への上がりで、これまでの段の確認と計測（Q1、Q4 の段 1〜3、10、L1、L2）をやり直す必要がある。
