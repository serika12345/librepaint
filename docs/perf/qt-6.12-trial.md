# nixpkgs-unstable（Qt 6.12、GCC 16）での試しの構築の結果

2026-11-11 に行った試しの構築の記録である。目的は、nixpkgs の Qt 6.12.0 と GCC 16.2.0 に寄せたときに壊れる箇所を、直さずに数えることである。作業はブランチ `trial/nixpkgs-unstable`（`flake.lock` と `nix/qt/overlay.nix` の 1 コミット `c4c75edc5b`）で行い、`feature/pen-latency` にはマージしていない。

## 方法

- nixpkgs を `4fe25e12eb`（nixpkgs-unstable、2026-11-07）に固定した。他の入力は変えていない。
- `nix/qt/overlay.nix` の Qt の版の差し替え（6.11.3）を外し、nixpkgs の Qt 6.12.0 を使った。Qt、KDE、PyQt6 のモジュールの取捨は変えていない。iOS と Android の Qt の定義は対象外とした。
- Linux は `.#librepaint-linux` を、Windows は `.#librepaint-windows-upstream-qt-archive` を、`nix build --keep-going -L` で 1 本ずつ構築した。失敗した派生物で止まらず、壊れた派生物を集めるためである。

## 結果

| # | 派生物 | 失敗の段 | エラーの件数 | 代表的なエラーの文 | 原因の見立ての分類 |
| --- | --- | --- | --- | --- | --- |
| 1 | `librepaint-linux-unwrapped-1.0.3` | compile | 1（uic の拒否 1 件。これに伴い翻訳単位 1 件が失敗） | `AssistantsToolOptions.ui: Invalid class name: " QWidget"` / `File '.../plugins/assistants/Assistants/AssistantsToolOptions.ui' is not valid` / `kis_assistant_tool.h:22:10: fatal error: ui_AssistantsToolOptions.h: No such file or directory` | Qt 6.12 のツールの変化（uic の検証の強化） |
| 2 | `krita-plugin-gmic-3.7.4.1` | 構築前（依存先の失敗） | 0 | `Reason: 1 dependency failed.` | その他（依存先の失敗） |
| 3 | `librepaint-linux-1.0.3` | 構築前（依存先の失敗） | 0 | `Reason: 2 dependencies failed.` | その他（依存先の失敗） |
| 4 | `librepaint-windows-upstream-qt-archive-1.0.3` | 評価 | 1 | `error: function 'anonymous lambda' called with unexpected argument 'python3Packages'`（`pkgs/by-name/gl/glib/package.nix:1:1`）、`python3.14-pyqt6-6.11.0` の `requiredPythonModules` の計算中、`nix/windows/krita.nix:87` の `hostPythonPath` から `flake.nix:844` の glib の上書きへ | その他（nixpkgs の glib が `python3Packages` 引数を受けなくなった） |

壊れた箇所は合計 4 件（Linux 3 件、Windows 1 件）である。うち自分自身の構築で失敗したのは LibrePaint 本体の 1 件だけで、残る 3 件はその失敗の波及か、評価の段の失敗である。

### 1. LibrePaint 本体（Linux）

- `plugins/assistants/Assistants/AssistantsToolOptions.ui` の 287 行目の `class=" QWidget"` を、Qt 6.12.0 の uic が受け付けない。Qt 6.11.3 では同じファイルで構築できているため、uic のクラス名の検証が厳しくなったと見られる。
- uic が `ui_AssistantsToolOptions.h` を作らないため、`kritaassistanttool_static_autogen/mocs_compilation.cpp` の 1 翻訳単位が失敗し、`make` が 59% で止まった。
- 本体のコンパイルのエラーは、この 1 翻訳単位だけである。この 1 件を直すと次のエラーが出るかは、直さない方針のため未確認である。
- 参考として、GCC 16.2.0 の新しい警告 `-Wsfinae-incomplete` が `libs/libkis/View.h` で 1 件、`-Wmaybe-uninitialized` が `libs/psd/psd_layer_record.cpp` で出た。どちらも警告で、構築は止まっていない。

### 2. 依存の構築（Linux）

Qt 6.12.0 ほか 57 個のパスは公開キャッシュから取得した。実際に構築したのは次の 13 派生物で、`librepaint-linux-unwrapped` の前までに 10 個が成功した。

- 成功：`python3-3.14.8-env`、`opencolorio-2.5.2`、KDE Frameworks 6.31.0 の 6 個（`kcoreaddons`、`kguiaddons`、`kcrash`、`kcolorscheme`、`kwidgetsaddons`、`kcompletion`）、`python3.14-pyqt6-6.11.0`、`qtmultimedia-6.12.0`。
- 失敗：`librepaint-linux-unwrapped-1.0.3`。
- 失敗の波及：`krita-plugin-gmic-3.7.4.1`（依存先 1 件の失敗）、`librepaint-linux-1.0.3`（依存先 2 件の失敗）。

KDE Frameworks 6.31.0、PyQt6 6.11.0、`qtmultimedia` 6.12.0 は、Qt 6.12.0 と GCC 16.2.0 の組で構築できた。

### 3. Windows

評価の段で `python3.14-pyqt6-6.11.0` の `requiredPythonModules` の計算に失敗したため、Windows の構築は動かしていない。したがって Windows 側のコンパイルの壊れ方は未計測である。原因は `flake.nix:844` の glib の上書きが `python3Packages` を渡していることで、nixpkgs-unstable の `glib` はこの引数を受け取らない。

## 所要時間

Linux の構築は 19:40:53 に始まり 21:28:24 に終わった（1 時間 47 分）。上限の 4 時間には達していない。

## 直していないこと

この試しでは何も直していない。`flake.lock` と `nix/qt/overlay.nix` 以外のファイルは変えていない。結果の判断は監査役が行う。
