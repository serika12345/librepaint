# LibrePaint開発マニュアル

開発・検証コマンドと作業の進め方をまとめます。作業状態・前提・完了条件は GitHub Issue が正本です。

## 環境

- 作業・ビルド・検証は Linux 開発サーバー nixos で行う。
- Nix が依存とツールチェーンを定義し、direnv が `test` シェルを読込む。
- `direnv allow` 後、リポジトリに入ると以下のコマンドが直接使える：

  ```sh
  build-incremental native build [target]
  run-test <target> [ctest-regex]
  verify-quick
  verify
  ```

- `build-incremental` は永続 Ninja ツリーとプラットフォーム別コンパイラーキャッシュを選ぶ。
  nixos の native プリセットは `build/tdd-linux` を使う。
- 各プラットフォームの一巡は `path` `configure` `plan` `build` `bootstrap` `cache-stats`
  の各操作で公開する。iOS は固定デバイス環境、Android と Windows は x86_64 Linux ビルドホストの
  ソース非依存プロファイルを使う。
- 意図的に開発シェルを再評価する場合のみ `nix develop .#test` を使う。
  ソース編集ごとに `nix develop` や `direnv exec` で包まない。

## 作業の進め方

- Issue 本文を読んで再開する。進捗・次操作・検証結果は Issue へ反映する。
- 一気にスクラップビルドし、最後に利用者が集中監査する。PR レビュー・マージは行わない。
- 監査可能性を実装中に蓄積する（緑の検証、通常経路への接続、旧経路の撤去、一貫した履歴）。
- 機能を移行したら旧経路を同じ単位で撤去する。

## 検証の層

| 段階 | コマンド | 使う場面 |
| --- | --- | --- |
| 局所テスト | `run-test <target> [regex]` | 実装中、次の変更の判断に必要な疑問を解消 |
| 迅速検査 | `verify-quick` | パッケージ依存・公開ヘッダー可視性・プラグイン登録の変更 |
| 全ネイティブ | `verify` | まとまった受入れ・広い置換の時点 |

- `verify` は `verify-quick` を含むため、同じ入力への連続実行を避ける。
- プラットフォーム・サニタイザー・性能・デバイス試験は該当する最終受入れで行う。

## 構造・文書

- 設計境界は `docs/architecture/README.md`、パッケージ境界方針は
  `docs/architecture/package-boundaries.json`（実構成から生成）が正本。
- 図は `docs/architecture/*.d2` が編集元で、`scripts/docs/render-architecture.sh` が SVG を生成する。
