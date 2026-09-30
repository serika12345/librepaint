# LibrePaint作業スナップショット

- 状態: `planned`
- 現在の作業: [Issue #70](https://github.com/serika12345/librepaint/issues/70)を完了し、macOSを含む[LibrePaint v1.0.3](https://github.com/serika12345/librepaint/releases/tag/v1.0.3)を公開した。
- 検証の要約: 両ABIのAndroid完成APKで署名・構成監査が成功した。ARM64のAndroid 17では導入と初期画面への起動を確認し、x86_64のAndroid 13では描画、保存・再読込、書き出し、画面回転、休止・復帰、終了、更新後の作品保持、Kritaとの共存を確認した。Linux AppImageは展開実行で版番号を確認した。macOSのApple Silicon用DMGは整合性とアプリ署名を検査し、起動、文書作成、描画、KRA保存・再読込を確認した。公開後にAndroid、Windows、Linux、iOS、macOSの7件の添付を再取得し、形式、ハッシュ、Android署名証明書を照合した。
- 次の開発工程: R2の[Issue #61](https://github.com/serika12345/librepaint/issues/61)から着手可能な基準契約を選定する。
