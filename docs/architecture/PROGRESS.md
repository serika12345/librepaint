# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-08-16 11:26 JST
- 状態: `planned`
- 現在の検査段階: R1-G2 公開面と主要クラスの棚卸し
- 関連TODO: `docs/architecture/TODO.md`の「R1: コードパッケージングの改善」
- ブランチ: `r1-g1-cmake-graph-foundation`
- 目的: 公開ヘッダー、主要クラス、プラグインの所有者、利用元、実際の責務を
  機械可読な台帳へ記録する。

## 再開環境

- direnvが`test`開発シェルと`build-incremental`、`run-test`、`verify-quick`、
  `verify`を読み込む。
- macOSのネイティブ試験構成は`tdd-macos`、永続Ninja木は
  `build/tdd-macos`を使用する。
- R1-G1でCMake File API `codemodel-v2`から明示的ビルドターゲットと直接リンク
  依存を再生成する基盤が完成している。
- `docs/architecture/cmake-targets-macos.json`がmacOS構成の626ターゲットを記録し、
  `krita`、`kritaui`、`kritaimage`とモジュール型プラグインを含む。

## R1-G1で完了した作業

- 実行形式、共有ライブラリー、モジュール型プラグイン、直接依存、外部ターゲット、
  CMake生成ターゲットを含む固定File API応答を追加した。
- `scripts/architecture/extract_cmake_graph.py`がターゲット名、種別、リポジトリ相対の
  定義場所、`linkLibraries`による直接CMakeターゲット依存を決定的に整列する。
- `scripts/architecture/regenerate_cmake_graph.py`がFile API問い合わせの作成、
  macOS構成の同期、台帳更新、バイト単位の差分検査を一つの入口から実行する。
- 固定応答の受理例、欠落診断、対応File API版、決定的な直列化、差分診断と、
  macOS台帳の主要入口を18件の高速検査へ接続した。
- 台帳の範囲、依存の意味、更新コマンド、検査コマンドをアーキテクチャガイドと
  開発手順へ記録した。

## 検証状態

- `nix develop .#test --command python3 -m unittest discover -s scripts/tests -p
  'test_*cmake_graph.py'`: 8件成功。
- `nix develop .#test --command
  ./scripts/architecture/regenerate_cmake_graph.py macos --check`: macOS実構成との
  バイト単位の一致を確認した。
- `nix develop .#test --command ./scripts/verify-quick`: 18件の単体試験、運用検査、
  シェル検査、文書検査、リンク検査、D2再生成検査が成功した。
- 製品C++、CMake定義、Nix出力を変更していないため、製品構築とNix全システム評価は
  この変更の検証範囲外とする。

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

この小単位の完了後、全公開ヘッダー、主要クラス、プラグインへ範囲を広げ、
所有者がない項目、複数責務、別ターゲットからの内部ヘッダー参照を診断する。

## R1-G2完了条件

- 公開ヘッダーが所有ターゲット、利用元、公開根拠へ対応付く。
- `KisApplication`、`KisDocument`、`KisImportExportManager`と`libs/ui/tool`の主要
  クラスが実際の責務へ分類される。
- 各プラグインがCMakeターゲット、メタデータ、登録先、機能所有者へ対応付く。
- 台帳が機械可読で、決定的な整列と有効な参照を高速検査で確認できる。
- 調査結果と検証を記録し、進捗をR1-G3へ更新する。
