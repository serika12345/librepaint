# LibrePaint作業スナップショット

- 更新日時: 2026-09-28 08:58 JST
- 状態: `in_progress`
- 現在の作業: [Issue #60](https://github.com/serika12345/librepaint/issues/60)のダイアログ寿命修正。表示直後に破棄したダイアログへ遅延位置調整がアクセスする不具合を修正し、PR提出の準備が完了した。
- ブランチ: `issue-60-dialog-lifetime`
- 開始コミット: `9bba5e336cc9557f3b7f20199ab275e4f919411b`。開始時の作業ツリーは変更なし。
- 検証状態: 変更前の`KoDialogContractTest`でSIGSEGVを確認。修正後は対象試験20回、`libs-widgets-`のCTest 26件、`verify-quick`（設計境界の試験17件、10責務、531公開ヘッダー、172プラグイン登録）、`git diff --check`が成功。
- 次の操作: Issue #60のPRをレビューする。続く製品作業は[Issue #59](https://github.com/serika12345/librepaint/issues/59)の矩形選択描画契約を独立ブランチで実施する。
- 未実施: 製品コードは共通Qt Widgetsの遅延処理だけを変更したため、各OSの製品再構築と実機操作は実施していない。
- 環境: 評価済みの`.direnv/flake-profile`、`build/tdd-macos`と共有コンパイラーキャッシュを継続利用する。
- 再開条件: ローカルFlakeの追加評価停止を継続する。未使用ソース増加の詳細は[Issue #66](https://github.com/serika12345/librepaint/issues/66)を参照する。Issue #60の実装はレビュー可能な状態にある。
