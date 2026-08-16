# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-08-16 13:35 JST
- 状態: `in_progress`
- 現在の検査段階: R1-G2 公開面と主要クラスの棚卸し
- 関連TODO: `docs/architecture/TODO.md`の「R1: コードパッケージングの改善」
- ブランチ: `r1-g2b-public-header-inventory`
- 目的: R1-G2bとして`kritaui`と`kritaimage`の公開ヘッダーを全件採取し、所有者、
  利用元、公開根拠を5構成のCMakeターゲット台帳へ接続する。

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

## R1-G2aで完了した作業

- `docs/architecture/public-surface-inventory.json`に、`kritaui`と`kritaimage`の
  代表公開ヘッダー3件、主要クラス3件、PNG読込プラグイン1件を記録した。
- 公開ヘッダーを所有ターゲット、公開マクロ、別ターゲットからの直接include、
  対応プラットフォーム、責務、根拠へ接続した。
- 主要クラスを宣言、実装、所有ターゲット、公開ヘッダー、利用元へ接続し、
  プラグインをCMakeターゲット、JSONメタデータ、登録マクロ、実行時利用元へ接続した。
- `check_public_surface_inventory.py`が、パス、決定的な整列、5台帳のターゲット実体、
  ソース所属、公開マクロ、include、クラス宣言、メタデータ、登録を高速検査で確認する。
- Nixの独立した運用検査を、Nix提供のBash、明示的なスクリプト処理系、認証局証明書で
  完結させ、macOSとLinuxで同じ検査を構築できるようにした。
- 台帳の目的、構成、検査方法、保守手順をアーキテクチャガイドと開発手順へ記録した。

## 検証状態

- 初回契約検査は公開面検査器の欠落を診断し、統合契約は高速検査に検査器が未接続で
  あることを診断した。Linuxの独立Nix検査は`/usr/bin/env`への依存と認証局証明書の
  欠落を順に診断し、処理系と軽量な検査入力を修正した後に成功した。
- `nix develop .#test --command ./scripts/verify-quick`: 34件の単体試験、公開面検査、運用検査、
  シェル検査、文書検査、リンク検査、D2再生成検査が成功した。
- `nix flake check --no-build --all-systems`: 全出力、検査、開発シェル、整形器の
  評価が成功した。
- `nix build --no-link .#checks.aarch64-darwin.governance`と、`ssh nixos`上の
  `nix build --no-link .#checks.x86_64-linux.governance`が成功した。
- `nix develop .#test --command ./scripts/architecture/verify_cmake_graphs.py
  --remote-host nixos --remote-repository
  /home/masato/worktrees/librepaint-r1-g2a-verify`: macOS、Linux、iOS、Android、Windowsの
  5台帳と差分行列が同一コミットの実構成に一致した。
- 製品C++とCMakeターゲットを変更していないため製品全体の再構築は実行していない。
  Nix変更は全システム評価、両ホストの独立運用検査、5構成の実構成で検証した。

## 次の操作

R1-G2bとして`kritaui`と`kritaimage`の公開ヘッダーを全件台帳化する。

1. この文書の状態を`in_progress`へ変更し、R1-G2aの代表項目を受理例として保持する。
2. 両ターゲットの公開マクロを持つヘッダーと別ターゲットからの直接includeを機械採取し、
   現行の公開面候補を固定データへ記録する。
3. 各候補の所有ターゲット、対応プラットフォーム、利用元、公開根拠を5台帳と実ソースで
   検証し、生成物または内部専用項目を根拠付きで分類する。
4. 欠落、重複、誤った所有者、根拠のない公開指定を診断する契約試験を追加する。
5. `publicHeaders`を両ターゲットの全件へ広げ、高速検査、macOSとLinuxの独立運用検査、
   5構成の同時検証を成功させる。

## R1-G2完了条件

- 公開ヘッダーが所有ターゲット、利用元、公開根拠へ対応付く。
- `KisApplication`、`KisDocument`、`KisImportExportManager`と`libs/ui/tool`の主要
  クラスが実際の責務へ分類される。
- 各プラグインがCMakeターゲット、メタデータ、登録先、機能所有者へ対応付く。
- 台帳が機械可読で、決定的な整列と有効な参照を高速検査で確認できる。
- 5構成の実体と台帳の一致を同時検証し、進捗をR1-G3へ更新する。
