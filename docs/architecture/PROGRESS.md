# LibrePaint作業スナップショット

- 状態: `in_progress`
- 現在の作業: [Issue #78](https://github.com/serika12345/librepaint/issues/78)のデスクトップ配布変更を3プラットフォームとも統合し、子Issueの実装レビューに基づく依存定義と成果物監査の修正・検証を完了した。対象OSの操作に関する受入れ確認を進める。
- リリース候補: [LibrePaint v1.0.3のドラフト](https://github.com/serika12345/librepaint/releases/tag/untagged-d14ccbcf00da6e1b6ceb)に、署名済みAndroid APK両ABI、Androidハッシュ一覧、Windows ZIP、Linux AppImage、未署名iOS IPA、macOS DMGの7個を添付した。GitHub上の添付一覧、サイズ、SHA-256、下書き状態と説明文の一致を確認した。アプリケーションソースは`4292db0e6fb0a1311d5f3e0caf33b03f451dc4c5`、デスクトップ配布処理は`62ee94aebac864524363159e3413f24c996edd00`に対応する。説明文に受入れ確認の残項目を記載した。ユーザー起票の[#40](https://github.com/serika12345/librepaint/issues/40)のiPad保存不具合、[#39](https://github.com/serika12345/librepaint/issues/39)のタッチ操作によるショートカット削除、[#2](https://github.com/serika12345/librepaint/issues/2)のiPhoneインストール対応をリリースノートに明記した。
- 配布構造: Windowsは構築と配布で同じJPEG XL依存を使用し、必要機能と`bin`内のDLLによる依存解決を検査する。Linuxは依存構築時にOpenColorIOの探索パスを設定し、配布時に基礎依存の消去済み参照を除去する。監査は各探索パス、動的リンカーと必要機能を確認する。macOSは元のMach-O参照先から導出するアプリ内依存を収録する。依存構築、本体のコンパイル、配布処理と成果物監査は、それぞれの入力と責務を持つ。
- 公開候補検査: `check-release-assets`はGitHub Releaseと添付前の候補ディレクトリーを同じ条件で検査する。修正済み候補一式に対するWindowsの665 PE、Linuxの3,728 ELFと170 Kritaプラグイン、macOSの962 arm64 Mach-Oと168 Kritaプラグイン、MLT 27個、frei0r 157個の監査が成功した。Android両ABIのハッシュ、構成、版番号、署名証明書と、iOS IPAの整合性・版番号も成功した。
- 配布結果: Linux AppImageは668,029,720バイトで、元のv1.0.3成果物から35.4%縮小した。macOS DMGは285,992,802バイトで、読取り専用マウント後の内部アプリ監査と厳格なアドホック署名検査が成功した。
- 共通検査: ローカル候補の欠落、未承認成果物、Windows ZIP破損を含む高速検査109件、完全なmacOSネイティブ検査883/883件、全システムNix評価が成功した。macOS DMGは更新後の成果物監査と厳格なアドホック署名検査を通過した。Linux本体とOpenColorIOの構築、OpenColorIOの上流10試験・Python色変換も成功した。
- Windowsの修正結果: 本体、配布ディレクトリー、322,967,107バイトのZIPを構築し、配布先とZIP展開後の665 PE・42実行時機能の監査が成功した。Windows実機のアプリ経由でJPEG XL 3画像の復号と、PNG→JPEG XL→PNGの可逆往復を実行し、基準画像との全画素一致を確認した。AVX2とAVX-512の経路を無効化する暫定設定は、当該経路の復号と可逆往復がWindows実機で成功することを削除条件とする。
- Linuxの修正結果: 668,029,720バイトのAppImageを構築し、440ストア項目、3,728 ELFと必要機能の監査が成功した。同梱環境でPNG→JPEG変換、仮想画面の可視ウィンドウと通常終了を確認した。表示検証はソフトウェア描画の範囲である。
- 次の開発工程: Issue #78の対象OSで、Windows実機の表示・音声、LinuxのWaylandと実GPUを含む残りの操作確認を進める。その結果を3プラットフォームの完了条件へ照合する。
