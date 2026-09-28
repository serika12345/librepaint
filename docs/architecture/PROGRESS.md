# LibrePaint作業スナップショット

- 更新日時: 2026-09-28 12:04 JST
- 状態: `in_progress`
- 現在の作業: [Issue #61](https://github.com/serika12345/librepaint/issues/61)の最初の未完了入力基準を、[Issue #75](https://github.com/serika12345/librepaint/issues/75)でマウス、タブレット／スタイラス、単点タッチのQt入力事象を受信順に記録し、別のQt受信先へ同期再生する契約として実装した。[Pull Request #76](https://github.com/serika12345/librepaint/pull/76)を提出済み。
- ブランチ: `issue-75-input-event-replay`
- 開始コミット: `98cd14833e6a9cc689560f0a5049304df35f3b9f`。開始時の作業ツリーは変更なし。
- 検証状態: 実装前に`KisInputEventSequence.h`が存在しない構築失敗を確認した。専用契約のクリーン構築範囲は5命令で、近接契約の`TestInputShortcutMatcher`は82命令、`KisInputManagerTest`は1,396命令だった。macOSで専用契約が20回連続、入力コンポーネントCTest 20件、`verify-quick`、`git diff --check`に成功した。最初にCTestを構築木へ直接実行した際はアプリ配置環境がなく既存の`KisToolProxyContractTest`が停止したが、規定のCTestプリセットでは同試験を含む全20件が成功した。追加のネイティブ全体検査は既存の未構築範囲5,749工程を要求したため、2,496工程まで失敗がないことを確認して停止した。
- 次の操作: Pull Request #76の差分と自動検査をレビューし、受入れ後に`develop`へ統合してIssue #75を閉じる。
- 未実施: Linux、Windows、Android、iOSでの実機入力採取と再生。親Issue #61のプラットフォーム対応表で共通Qt契約へ接続する。Android増分環境の指紋更新でNixOS上の固定プロファイルを一回評価した。評価後の死んだソースは1件、564,137,691バイトであり、事前測定は抽出コマンドの誤りで成立しなかったため増分は未判定。追加評価を停止し、更新済みプロファイルを継続利用する。
- 環境: 評価済みの`.direnv/flake-profile`、`build/tdd-macos`、共有コンパイラーキャッシュを使用する。構築と試験は共有Ninja木で直列に実行する。
- 再開条件: Pull Request #76、Issue #75の検証記録、`issue-75-input-event-replay`の作業ツリーを確認し、レビューまたは統合作業を開始する。
