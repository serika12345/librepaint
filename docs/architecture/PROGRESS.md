# LibrePaint作業スナップショット

- 更新日時: 2026-09-28 00:17 JST
- 状態: `planned`
- 現在の作業: [Issue #61](https://github.com/serika12345/librepaint/issues/61)から次のR2作業を選定。
- ブランチ: `develop`
- 開始コミット: `93209943ba9e9f06e5d28dd3bc09e76608b6712a`（直近の文書整理の開始点）。次の作業開始時にHEADと作業ツリーを確認する。
- 検証状態: macOSで`./scripts/run-shared-test-env ./scripts/verify-quick`が成功（運用試験57件、責務・公開契約、テキスト、文書、生成図）。現存文書9件の書式・内部リンク・見出し参照、削除パスの参照除去、`git diff --check`も成功。文書整理の範囲と結果は[Issue #66](https://github.com/serika12345/librepaint/issues/66)を参照する。
- 次の操作: [Issue #61](https://github.com/serika12345/librepaint/issues/61)から前提を満たすR2作業を選ぶ。開発操作は[開発マニュアル](DEVELOPMENT.md)、現行設計は[アーキテクチャガイド](README.md)を参照する。
- 受入れ済み: [Issue #56](https://github.com/serika12345/librepaint/issues/56)のiPad実機確認は利用者受入れにより完了。
- 環境: 評価済みの`.direnv/flake-profile`、`build/tdd-macos`と共有コンパイラーキャッシュを継続利用する。
- 環境の保守条件: ローカルFlakeの追加評価を停止し、評価済み環境を使う。未使用ソース増加の詳細は[Issue #66](https://github.com/serika12345/librepaint/issues/66)を参照する。
