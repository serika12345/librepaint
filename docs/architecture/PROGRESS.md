# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-08-16 11:52 JST
- 状態: `in_progress`
- 現在の検査段階: R1-G1 依存グラフ採取基盤
- 関連TODO: `docs/architecture/TODO.md`の「R1: コードパッケージングの改善」
- ブランチ: `r1-g1-cmake-graph-foundation`
- 目的: macOS、Linux、iOS、Android、WindowsのCMakeターゲットと直接リンク
  依存を同じ契約で採取し、全構成を同時に検証する。

## 再開環境

- DarwinホストがmacOSとiOSの構成を担当する。
- `ssh nixos`で接続するx86_64 NixOSホストがLinux、Android、Windowsの構成を
  担当する。
- 両ホストの清浄な作業ツリーを同じコミットへ揃え、
  `scripts/architecture/verify_cmake_graphs.py`から5構成を並行検証する。
- CMake File API `codemodel-v2`の台帳は
  `docs/architecture/cmake-targets-<platform>.json`、全構成の比較結果は
  `docs/architecture/cmake-target-matrix.json`に記録する。

## R1-G1で完了した作業

- 固定File API応答と抽出器が、ターゲット名、種別、リポジトリ相対の定義場所、
  `linkLibraries`による直接CMakeターゲット依存を決定的に整列する。
- 再生成器が各プラットフォームの増分構築入口から永続構築木を取得し、問い合わせ、
  構成同期、台帳更新、バイト単位の差分検査を実行する。
- macOS 626件、Linux 641件、iOS 560件、Android 566件、Windows 596件の
  ターゲットを同じ形式で記録した。
- 差分行列が542件の共通ターゲット、125件の条件付きターゲット、244件の構成差を
  持つターゲットを記録した。
- 全プラットフォーム同時検証入口が、Darwinとx86_64 Linuxの担当構成、同一コミット、
  清浄な作業ツリーを構成開始前に検査する。
- Windowsのソース準備処理を独立した実行ファイルとし、通常のNix構築と増分構築で
  同じ置換処理を利用できる契約を固定した。
- 台帳の範囲、差分行列、ホスト割り当て、更新手順、同時検証コマンドを
  アーキテクチャガイドと開発手順へ記録した。

## 検証状態

- 初回契約検査は、5台帳と差分行列の欠落、全構成を受け取らない再生成器、同時検証
  入口の欠落を診断した。
- Windowsの初回実構成は`substituteInPlace: command not found`を診断し、修正後に
  x86_64 NixOS上で構成と台帳生成が成功した。
- macOSとiOSをDarwin、Linux、Android、Windowsをx86_64 NixOSで構成し、5台帳を
  生成した。
- `nix develop .#test --command python3 -m unittest discover -s scripts/tests -p
  'test_*cmake_graph*.py'`: 15件成功。
- `nix develop .#test --command ./scripts/verify-quick`: 26件の単体試験、運用検査、
  シェル検査、文書検査、リンク検査、D2再生成検査が成功した。
- `nix flake check --no-build --all-systems`: 全出力、検査、開発シェル、整形器の
  評価が成功した。
- 同一コミットによる5構成同時差分検査は次の操作で実行する。

## 次の操作

1. 現在の変更をコミットし、同じコミットの清浄な作業ツリーをNixOSホストへ用意する。
2. `verify_cmake_graphs.py`から5構成を同時に構成し、各台帳と差分行列の一致を確認する。
3. 検証結果をこの文書へ記録し、状態を`planned`、次の検査段階をR1-G2へ更新する。
4. コミットをpushし、既存PRの目的、成果物、検証結果を更新する。

## R1-G1完了条件

- 5構成のCMakeターゲット、種別、定義場所、直接リンク依存を同じ形式で再生成できる。
- 全構成の共通ターゲット、条件付きターゲット、構成差を決定的に比較できる。
- Darwinとx86_64 Linuxの同一コミットから5構成を同時に検証できる。
- 固定応答の契約検査、高速検査、Nix全システム評価、実構成差分検査が成功する。
