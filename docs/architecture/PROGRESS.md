# LibrePaint作業スナップショット

- 更新日時: 2026-09-28 09:11 JST
- 状態: `in_progress`
- 現在の作業: [Issue #59](https://github.com/serika12345/librepaint/issues/59)の矩形選択描画契約。選択内の画素結果、投影一致、正確な描画領域と選択外不変を固定し、PR提出の準備が完了した。
- ブランチ: `issue-59-selection-stroke-contract`
- 開始コミット: `9bba5e336cc9557f3b7f20199ab275e4f919411b`。開始時の作業ツリーは変更なし。
- 検証状態: 変更前の対象閉包2,181工程を画素ブラシ実行オブジェクトへ分離して1,309工程へ縮小。矩形選択の初回診断で`QRect(225, 225, 100, 100)`と画素ハッシュを採取した。固定後は自由描画契約20回、選択・投影の隣接CTest 3件、画素ブラシ・PaintOp共有ライブラリー・既定PaintOpプラグイン構築、`verify-quick`、`git diff --check`が成功。
- 次の操作: Issue #59のPRをレビューする。続く製品作業は[Issue #58](https://github.com/serika12345/librepaint/issues/58)のブラシ設定試験8件を独立ブランチで再分類する。
- 未実施: 共通の描画試験とCMake所有境界を変更したため、macOS以外の構成・製品構築と実機操作は実施していない。
- 環境: 評価済みの`.direnv/flake-profile`、`build/tdd-macos`と共有コンパイラーキャッシュを継続利用する。
- 再開条件: ローカルFlakeの追加評価停止を継続する。未使用ソース増加の詳細は[Issue #66](https://github.com/serika12345/librepaint/issues/66)を参照する。Issue #59の実装はレビュー可能な状態にある。
