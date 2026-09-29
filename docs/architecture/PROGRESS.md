# LibrePaint作業スナップショット

- 更新日時: 2026-09-29 10:09 JST
- 状態: `in_progress`
- 現在の作業: [Issue #70](https://github.com/serika12345/librepaint/issues/70)の署名済みAndroid APK配布。公開版の識別子・版番号、ABI別の包装入力、署名済み成果物の検査、下書きReleaseでの署名手順を固定コミットにまとめ、両ABIの公開版・内部更新試験版APKとWindows・Linux・iOSの次版成果物を構築・検査した。
- ブランチ: `develop`
- 開始コミット: `98cd14833e6a9cc689560f0a5049304df35f3b9f`。開始時の作業ツリーは変更なし。
- 次の操作: 固定コミットを配布の入力として確認し、所有者が配布鍵と承認付き`android-release`環境を用意する。`v1.0.3`タグと非公開の下書きReleaseが用意された後、未署名APKを渡してCIで署名・検査する。完成APKのARM64実機・x86_64 Waydroid受入れと他3平台の成果物確認を終えてから公開する。下書きへの受け渡し後は一時的な包装出力の参照を整理する。Mac上の追加のローカルFlake評価は避け、共有テスト環境を使用する。
- 最新の検証: macOSでAPK検査の単体試験6件、`verify-quick`の23件と境界検査、ShellCheck、actionlint、Nix構文検査が成功した。NixOSで最終Nix定義の`nix flake check --no-build --all-systems`、両ABIのネイティブ中間出力と公開版・内部更新試験版の`nix build`、AndroidのAPK構成・ABI・ELF・資源・依存物監査が成功した。公開版は識別子`io.github.serika12345.librepaint`、`1.0.3`/`1000003`、内部版は同じ識別子の`1.0.2-internal`/`1000002`、最小API 28、16 KiB整列を確認した。Linuxの`nix build .#librepaint-linux-appimage`は成功し、Type 2 AppImageのELF・埋込実行ファイル・プラグイン・資源を確認した。Windowsの`nix build .#librepaint-windows-archive`は成功し、ZIP全5,122項目のCRC、x86_64 PE、Qt・G’MIC・資源の同梱を確認した。iOSの`nix build .#librepaint-ios-ipa`は成功し、IPA内部の版`1.0.3`とZIP構造を確認した。各成果物のSHA-256と正確なコマンドはIssue #70に記録する。`git diff --check`も成功した。
- 残る条件: 所有者が新しい配布鍵の原本と暗号化復旧用コピーを保管し、GitHubの`android-release`環境に承認者、秘密情報、公開証明書指紋を設定する。固定コミット・タグからの下書きRelease署名、完成APKの操作・更新・データ保持試験、他3平台の公開前受入れ、公開後の再取得照合を行う。ARM64実機のADB接続は現時点でなく、NixOSのWaydroidは停止中。Linux AppImageはSSH経由の起動試行が終了コード134となり、GUI操作は未確認。
- 環境と容量: Linuxでは旧`librepaint-issue-50/build/android`約204 GB、旧`librepaint-r1-g8/build`約51 GB、`~/worktrees`の17ツリー約36 GBを先に整理した。続いて8月の検証用3ツリーと別課題の3ツリー、旧包装結果への参照とNix出力5件、未参照のソース複製3件、旧Android Qt5依存出力、Krita AppImageの展開キャッシュ4件、別構成の小さな構築木と試験APK 8個を整理した。未コミットの変更は`~/.cache/librepaint/worktree-archives/issue70-cleanup-20260928`と同名の`-round2`へ保全・照合済み。現行の増分構築木、共有キャッシュ、開発環境のNix参照を保持した。今回の両ABIのネイティブ中間出力とAndroid・Linux・Windowsの包装結果も一時参照付きで保持している。NixOSの最終使用量は363,097,509,888バイト、空き989,248,028,672バイト。未参照の`*-source`は42件・1,335,522,905バイトだが、LibrePaint本体の入力は最初の1件・563,453,244バイトのままで、増加した41件・772,069,661バイトは外部依存ソース。今回生成したAppImage展開キャッシュ3.0 GBは検査後に削除した。管理者所有の失敗構築領域約24 GBは残る。Linuxでは同じ作業ツリー入力に対するFlake評価でLibrePaintソースの追加複製が起きないことを確認した。
- Linuxの作業ツリー: `git worktree prune`で古い登録6件も整理し、登録されているのは現行`/home/masato/Documents/librepaint`だけ。`~/worktrees`は空。旧作業ツリーの未コミット変更は上記の保全記録から復元できる。
- 再開条件: Issue #70の確定した公開条件と今回の検証記録を確認し、`develop`の固定コミット、Nix参照、既存プロファイルを保持して上記の次の操作を続ける。Mac上の追加Flake評価は保管容量と入力複製の状態を再確認してから行う。配布鍵とGitHub環境の設定は所有者による完了連絡を待つ。
