# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-08-16 02:21 JST
- 状態: `planned`
- 現在の検査段階: R1-G1 依存グラフ採取基盤
- 関連TODO: `docs/architecture/TODO.md`の「R1: コードパッケージングの改善」
- ブランチ: `master`
- 目的: CMakeターゲットの名前、種別、定義場所、直接依存を同じ入力から再生成できるようにする。

## 再開環境

- direnvが`test`開発シェルと`build-incremental`、`run-test`、`verify-quick`、
  `verify`を読み込む。
- macOSとLinuxはホスト別の試験プリセット、iOS、Android、Windowsは依存構成
  ごとの固定Nix環境、全対象は永続Ninja木とプラットフォーム別`ccache`を使用する。
- Darwin上のネイティブ構成・反復計画とiOS構成・計画が成功している。
- AndroidとWindowsの開発環境、構成指紋、クロスコンパイル定義は全システムの
  Nix評価を通過している。構成・構築はx86_64 Linuxの対応検査段階で実行する。
- `direnv exec . ./scripts/verify-quick`は10件の単体試験、運用検査、シェル検査、
  文書検査、リンク検査、D2再生成検査を完了している。
- `nix flake check --no-build --all-systems`は全パッケージ、検査、開発シェルの
  評価を完了している。

## 成果物

- CMake File API `codemodel-v2`の固定試験データ
- 抽出器の単体試験
- ターゲット台帳のJSON形式
- 再生成コマンド
- 現行ターゲット台帳
- 再現性検査

R1-G1は現状採取と再生成を担当する。R1-G3が依存方針を定義し、R1-G4が
循環、内部ヘッダー参照、依存方向を継続検査へ接続する。

## 次の操作

最初の小単位として、CMake File APIの最小固定データと抽出器の契約試験を作る。

1. この文書の状態を`in_progress`へ変更する。
2. `scripts/tests/fixtures/cmake-file-api/`へ、実行形式、共有ライブラリー、
   プラグインと直接依存を含む最小`codemodel-v2`固定データを置く。
3. `scripts/tests/test_extract_cmake_graph.py`を作り、出力JSONの各要素が
   `name`、`type`、`sourceDirectory`、整列済み`dependencies`を持つ
   契約を記述する。
4. 契約試験を実行し、期待する初回診断を記録する。
5. `scripts/architecture/extract_cmake_graph.py`を最小実装し、契約試験を
   成功させる。

この小単位の完了後、`build/tdd-<platform>/.cmake/api/v1/query/codemodel-v2`を
作成する再生成処理、現行台帳、決定的な並び順の差分検査を順に実装する。

## 完了条件

- 固定データの受理例と診断例の単体試験が成功する。
- 同じCMake応答からバイト単位で同じ台帳を生成する。
- `kritaui`、`kritaimage`、プラグイン、実行形式の直接依存を台帳で確認できる。
- 再生成差分検査が`./scripts/verify-quick`に接続される。
- スクリプト、試験、生成台帳、アーキテクチャ文書の変更としてR1-G1を完了する。
- 検証結果を記録し、進捗をR1-G2へ更新する。
