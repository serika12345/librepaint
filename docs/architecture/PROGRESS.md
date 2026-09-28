# LibrePaint作業スナップショット

- 更新日時: 2026-09-28 11:03 JST
- 状態: `complete`
- 現在の作業: [Issue #57](https://github.com/serika12345/librepaint/issues/57)のAndroid、Windows、LinuxのOS固有契約を完了し、[Pull Request #74](https://github.com/serika12345/librepaint/pull/74)で`develop`へ統合した。Androidのクラッシュ処理は実コールバックからunwindstackによるバックトレース生成までARM64実機で検査する。
- ブランチ: `develop`
- 開始コミット: `0e0ebecfa9e95861b41eff19c23359ee54706dc1`。開始時の作業ツリーは変更なし。
- 検証状態: x86_64 NixOSの標準構成と`-DHAVE_DBUS=ON`構成はともに`HAVE_DBUS=1`と30命令のcolord契約を選択し、色管理CTest 2件が各20回成功した。Windows 10.0.26200、Visual Studio Build Tools 17.14.41、MSVC 19.44.35229、Windows SDK 10.0.26100で`WinQuirksUnistdContractTest`が20回成功した。Pixel 10a、Android 17、API 37、arm64-v8aで`KisAndroidCrashHandlerContractTest`が5回、`KisCrashSignalHandlerSetupContractTest`が1回成功し、各APK監査と試験パッケージ削除も成功した。macOSの隣接契約と`verify-quick`、Linuxの`verify-quick`も成功した。
- 次の操作: [Issue #61](https://github.com/serika12345/librepaint/issues/61)配下から、前提条件を満たす次のIssueを選ぶ。
- 未実施: Linux、Windows、Androidの製品全体の構築・操作。Android増分環境の指紋更新でNixOS上の固定プロファイルを一回評価した。評価後の死んだソースは1件、564,137,691バイトであり、事前測定は抽出コマンドの誤りで成立しなかったため増分は未判定。追加評価を停止し、更新済みプロファイルを継続利用する。
- 環境: LinuxとAndroidは`nixos`、Windowsは`surface`を使用した。Android実機はMacのADBサーバーをSSH転送し、NixOSのARM64増分構築木から導入した。
- 再開条件: `develop`の最新状態からIssue #61と未完了の子Issueを読み、開始時の作業ツリーと前提条件を確認する。
