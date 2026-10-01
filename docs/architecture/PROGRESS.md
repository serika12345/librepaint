# LibrePaint作業スナップショット

- 状態: `in_progress`
- 現在の作業: [Issue #80](https://github.com/serika12345/librepaint/issues/80)で、Linux AppImageを製品機能から到達する実行時閉包へ限定し、成果物内部の依存と配布構成を検査可能にする。
- 検証の要約: Linux AppImageは668,078,872バイト、440ストア項目、44,642通常ファイル、展開後2,116,112,089バイトとなり、v1.0.3成果物から35.4%縮小した。3,728個のELF、170個のKritaプラグイン、Python／PyQt、G'MIC、主要画像形式、FFmpeg／MLT、Qt XCB／Wayland、アイコン、フォントと翻訳を検査し、Qt WebEngine、開発資料、構築用実行ファイル、未参照ストア項目を除外した。隔離したx86_64 Linux環境で、版番号、XCB起動、SVGからKRAへの保存、KRAからPNG／JPEGへの書き出し、PyQt／DBus、G'MIC、Krita Python、FFmpeg／FFprobe、MLT FFmpegの読み込みを確認した。全システムNix評価と高速検査は成功した。完全なネイティブ検査は883件中882件が成功し、変更対象外の`KisSafeDocumentLoaderTest`が単独再実行でも通知数の不一致で失敗する。
- 次の開発工程: Issue #80の変更をレビュー・統合し、[Issue #81](https://github.com/serika12345/librepaint/issues/81)でmacOSアプリバンドルの無効な参照と不要な実行時部品を除く。
