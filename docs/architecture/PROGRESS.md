# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-08-16 12:19 JST
- 状態: `planned`
- 現在の検査段階: R1-G2 公開面と主要クラスの棚卸し
- 関連TODO: `docs/architecture/TODO.md`の「R1: コードパッケージングの改善」
- ブランチ: `r1-g1-cmake-graph-foundation`
- 目的: 公開ヘッダー、主要クラス、プラグインの所有者、利用元、実際の責務を、
  全プラットフォームのCMakeターゲット台帳へ接続できる機械可読な台帳に記録する。

## 再開環境

- direnvが`test`開発シェルと`build-incremental`、`run-test`、`verify-quick`、
  `verify`を読み込む。
- DarwinホストがmacOSとiOS、`ssh nixos`で接続するx86_64 NixOSホストがLinux、
  Android、Windowsの構成を担当する。
- 全プラットフォームの実構成差分は、両ホストの清浄な作業ツリーを同じコミットへ
  揃え、`scripts/architecture/verify_cmake_graphs.py`から5構成を並行検証する。
- CMake File API `codemodel-v2`の台帳は
  `docs/architecture/cmake-targets-<platform>.json`、全構成の比較結果は
  `docs/architecture/cmake-target-matrix.json`に記録する。

## R1-G1で完了した作業

- 固定File API応答と抽出器が、ターゲット名、種別、リポジトリ相対の定義場所、
  `linkLibraries`による直接CMakeターゲット依存を決定的に整列する。
- 再生成器が各プラットフォームの増分構築入口から永続構築木を取得し、問い合わせ、
  構成同期、台帳更新、バイト単位の差分検査を実行する。
- macOS 624件、Linux 639件、iOS 558件、Android 564件、Windows 594件の
  ターゲットを同じ形式で記録した。
- 差分行列が542件の共通ターゲット、119件の条件付きターゲット、244件の構成差を
  持つターゲットを記録した。
- 絶対パスから名前が変わる翻訳補助ターゲットを固定応答の契約で除外し、異なる
  作業ツリーから同じ台帳を再生成できるようにした。
- 全プラットフォーム同時検証入口が、Darwinとx86_64 Linuxの担当構成、同一コミット、
  清浄な作業ツリーを構成開始前に検査する。遠隔の並行Nix評価は個別の評価処理を使う。
- Windowsのソース準備処理を通常のNix構築と増分構築で共有し、CMakeの試行生成物を
  Ninja構築木へ収める契約を固定した。
- 台帳の範囲、差分行列、ホスト割り当て、更新手順、同時検証コマンドを
  アーキテクチャガイドと開発手順へ記録した。

## 検証状態

- 初回契約検査は、5台帳と差分行列の欠落、全構成を受け取らない再生成器、同時検証
  入口の欠落を診断した。Windowsの初回実構成は`substituteInPlace: command not found`
  を診断し、修正後に構成と台帳生成が成功した。
- `nix develop .#test --command ./scripts/verify-quick`: 27件の単体試験、運用検査、
  シェル検査、文書検査、リンク検査、D2再生成検査が成功した。
- `nix flake check --no-build --all-systems`: 全出力、検査、開発シェル、整形器の
  評価が成功した。
- `nix develop .#test --command ./scripts/architecture/verify_cmake_graphs.py
  --remote-host nixos --remote-repository
  /home/masato/worktrees/librepaint-r1-g1-verify`: macOS、Linux、iOS、Android、Windowsの
  5台帳と差分行列が同一コミットの実構成に一致した。
- 製品C++を変更していないため製品全体の再構築は実行していない。WindowsのNix変更は
  全システム評価と実構成により検証し、実行形式の完全構築は配布検査地点に残る。

## 次の操作

R1-G2の最初の小単位として、公開面と主要クラスを記録する台帳契約を作る。

1. この文書の状態を`in_progress`へ変更する。
2. `libs/ui`と`libs/image`のインストール規則、公開ヘッダー、別ターゲットからの
   include、主要クラスを調査し、所有者と利用元を判定する証拠を固定する。
3. 公開ヘッダー、主要クラス、プラグインの受理例を含む最小固定データを作る。
4. 台帳の各要素がパス、種別、所有ターゲット、利用元、実際の責務、根拠を持つ
   契約試験を追加し、期待する初回診断を記録する。
5. 最小の台帳検査器と、`kritaui`、`kritaimage`の代表項目を実装して契約試験を
   成功させる。
6. 高速検査と全プラットフォーム同時検証を成功させ、調査範囲を全公開ヘッダー、
   主要クラス、プラグインへ広げる。

## R1-G2完了条件

- 公開ヘッダーが所有ターゲット、利用元、公開根拠へ対応付く。
- `KisApplication`、`KisDocument`、`KisImportExportManager`と`libs/ui/tool`の主要
  クラスが実際の責務へ分類される。
- 各プラグインがCMakeターゲット、メタデータ、登録先、機能所有者へ対応付く。
- 台帳が機械可読で、決定的な整列と有効な参照を高速検査で確認できる。
- 5構成の実体と台帳の一致を同時検証し、進捗をR1-G3へ更新する。
