# nixpkgs-unstable（Qt 6.12、GCC 16）での試しの構築の結果

nixpkgs を nixpkgs-unstable（`4fe25e12eb`、2026-11-07）に上げ、Qt を nixpkgs の
6.12.0 に差し替えた状態で、Linux と Windows の構築がどこで壊れるかを、直さずに
数えた。試しのブランチは `trial/nixpkgs-unstable`（基点 `feature/pen-latency`）で、
変更は `flake.lock` と `nix/qt/overlay.nix` の 2 ファイル（コミット `c4c75edc5b`）に
限る。iOS と Android の Qt の定義は対象外である。

## 条件

- Linux は `.#librepaint-linux` の完全な構築を `nix build --keep-going -L` で行った。
  2026-11-11 19:40 に開始し、21:28 に失敗して終わった（約 1 時間 48 分）。4 時間の
  上限には達していない。
- Windows は `.#librepaint-windows-upstream-qt-archive` の評価が通らないため、
  構築を動かしていない。評価の失敗を 1 件として数える。
- qtbase 6.12.0 と KDE Frameworks 6.31.0 は差し替えで取得し、この構築では
  作り直していない。
- 数え方は、失敗した派生物ごとに、段、エラー件数、代表的なエラーの文、原因の
  見立ての分類である。

## 失敗した派生物

Linux の構築は 13 の派生物を対象にした。うち 10 が成功し、3 が失敗した。自身の
構築で壊れたのは 1 件で、残る 2 件は依存の失敗により構築に進まなかった。
Windows の評価の失敗と合わせ、壊れた箇所は 2 件である。

| 派生物 | 段 | エラー件数 | 分類の見立て |
| --- | --- | --- | --- |
| `librepaint-linux-unwrapped-1.0.3` | compile（`.ui` の処理で失敗し、生成ヘッダが無いまま翻訳単位のコンパイルが止まる） | 2 | Qt 6.12 の API の変化（`uic`） |
| `krita-plugin-gmic-3.7.4.1` | 未構築（依存 1 件の失敗） | 0 | その他（依存の失敗） |
| `librepaint-linux-1.0.3` | 未構築（依存 2 件の失敗） | 0 | その他（依存の失敗） |
| `librepaint-windows-upstream-qt-archive-1.0.3` | 評価 | 1 | Python |

エラー件数は、ninja が最初のエラーで止まった時点で観測できた数であり、下限である。

### `librepaint-linux-unwrapped-1.0.3`

configure は通り、compile の 59% で止まった。`uic` が
`plugins/assistants/Assistants/AssistantsToolOptions.ui` を不正として受け付けず、
`ui_AssistantsToolOptions.h` が生成されない。そのヘッダを
`plugins/assistants/Assistants/kis_assistant_tool.h:22` が取り込むため、
`kritaassistanttool_static` の翻訳単位がコンパイルできない。止まった翻訳単位の数は
1（`kritaassistanttool_static_autogen/mocs_compilation.cpp`）である。

代表的なエラーの文（3 つ）:

```
AssistantsToolOptions.ui: Invalid class name: " QWidget"
File '.../plugins/assistants/Assistants/AssistantsToolOptions.ui' is not valid
kis_assistant_tool.h:22:10: fatal error: ui_AssistantsToolOptions.h: No such file or directory
```

同じ `.ui` の処理で、`verticalLayout` の重複を defaults で解消する警告も出ている。
`.ui` のファイル自体は試しのブランチで変更していない。エラーは 2 件（`.ui` の処理
1 件、翻訳単位のコンパイル 1 件）である。構築は 59% まで進み、ninja が止まった。
残りは未確認である。この派生物で `error:` で始まる GCC の
診断はこの 1 件だけで、GCC 16 の新しいエラーと C++20 の既定化による失敗は
現れていない。

### `krita-plugin-gmic-3.7.4.1`

`librepaint-linux-unwrapped` に依存するため、構築に進まずに止まった。

```
error: Cannot build '...-krita-plugin-gmic-3.7.4.1.drv'.
       Reason: 1 dependency failed.
```

### `librepaint-linux-1.0.3`

`librepaint-linux-unwrapped` と `krita-plugin-gmic` に依存するため、構築に進まずに
止まった。

```
error: Cannot build '...-librepaint-linux-1.0.3.drv'.
       Reason: 2 dependencies failed.
```

### `librepaint-windows-upstream-qt-archive-1.0.3`（評価）

評価の段で、`python3.14-pyqt6-6.11.0` の `requiredPythonModules` の計算が失敗する。
エラーは 1 件である。

```
error: function 'anonymous lambda' called with unexpected argument 'python3Packages'
at .../pkgs/by-name/gl/glib/package.nix:1:1
```

経路は `nix/windows/krita.nix:87` の `hostPythonPath` から `flake.nix:844` の glib の
上書きへ至り、glib の定義が `python3Packages` の引数を受け取らなくなっている。

## 成功した派生物

Linux で構築を終えた 10 の派生物は次のとおりである。`python3.14-pyqt6` 6.11.0 は
Linux では Qt 6.12.0 に対して構築を終えている。Windows の評価の失敗は、この
組み合わせではなく glib の上書きの経路で起きている。

- `python3-3.14.8-env`
- `python3.14-pyqt6-6.11.0`
- `kguiaddons-6.31.0`
- `kcolorscheme-6.31.0`
- `kwidgetsaddons-6.31.0`
- `kcompletion-6.31.0`
- `kcoreaddons-6.31.0`
- `kcrash-6.31.0`
- `qtmultimedia-6.12.0`
- `opencolorio-2.5.2`

## 範囲

この試しでは何も直していない。結果は `trial/nixpkgs-unstable` にだけ置き、
`feature/pen-latency` にはマージしない。
