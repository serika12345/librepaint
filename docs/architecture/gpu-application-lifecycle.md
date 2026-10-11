# GPU文書とアプリケーションの寿命

## 目的と構成

`libs/application/gpu/KisGpuApplicationState.*`は、OSのメモリー通知とQtの
アプリケーション状態を、[共有GPU文書所有者](gpu-document-device.md)へ渡す。
アプリケーション側から文書側へ依存し、画像側はOS通知とGUIの寿命から独立する。

`KisApplication`はGPU文書機能を有効にした構成で共有GPU所有者と通知所有者を保持する。
通知所有者を先に破棄し、文書所有者を後に破棄する。GPUの作成は最初の文書まで遅延する。
`gpuDocuments()`はGUIスレッドで借用する文書所有者を返し、機能無効時は空を返す。

## 通知と再開

| 入力 | 処理と結果 |
| --- | --- |
| `ApplicationSuspended` | 全文書の受付済み入力とCPU履歴を維持し、共有GPUを解放する。 |
| `ApplicationActive` | 休止した共有GPUを再作成し、同じ文書モデルの復元を進める。 |
| フォーカス喪失・非表示 | 文書とGPUを維持する。デスクトップの文書切替での再作成費用を避ける。 |
| メモリー通知50〜254 | 現在の下地と作業版を保ち、過去のGPU版を解放する。 |
| メモリー通知255 | CPUの回復値を保ち、全文書のGPU資源を解放する。 |
| メモリー通知0 | アプリケーションが活動中なら再開し、休止中なら次の活動通知を待つ。 |
| GPU喪失 | 全文書を休止し、活動中なら一度再作成を試みる。作成失敗時は次の活動通知・入力で再試行する。 |
| 終了 | 通知の受付と完了回収を止め、共有GPUを解放する。 |

強い圧迫後の新しい入力は、`KisApplication::resumeGpuDocumentsForInput()`で
再開を試み、復元完了後にモデルの新しい編集識別子で継続する。
OSから圧迫解消通知が届かない場合も、次の入力または活動通知が再試行の契機になる。
終了後の再開要求は拒否する。

文書が存在する間は16ms間隔で非同期文書処理とGPU喪失を回収する。
これは文書復元の進行用であり、描画側の入力処理・表示同期はそれぞれの所有者が担当する。
文書が空、GPU休止中、再作成失敗後と終了後は回収タイマーを停止する。
最後の文書の終了は共有GPUも解放する。

## OSとの接続

- Appleプラットフォームはメインキュー上の`DISPATCH_SOURCE_TYPE_MEMORYPRESSURE`を使用し、
  NORMALを0、WARNを50、CRITICALを255へ変換する。
- LinuxでQt DBusを利用する構成は、システムバスの
  `org.freedesktop.LowMemoryMonitor`、`/org/freedesktop/LowMemoryMonitor`から
  同名インターフェイスの`LowMemoryWarning(y)`を受け取る。
- Windowsは`LowMemoryResourceNotification`を`QWinEventNotifier`で受け取り、履歴を解放する。
  圧迫が継続する間の連続通知は一秒ごとに回収する。

ネイティブ通知の監視は終了または通知所有者の破棄時に解除する。
プラットフォームの通知が利用できるかどうかにかかわらず、Qtの休止・活動・終了通知と
GPU割当の拒否は同じ文書契約を使用する。通知によるGPU解放に画素読取りを追加しない。

## 検証と保守

`KisGpuApplicationStateTest`はQtアプリケーション通知の接続、履歴解放、
受付済み入力を保持した休止、活動通知・入力による再開、GPU喪失と終了を検査する。
所有者のGPU再作成と非同期復元には実GPUを使用する。
OS全体へのメモリー圧迫は起こさず、通知された値を同じ受付口へ渡して文書への効果を検査する。
Linuxの通知接続は、専用D-Busのアドレスを`DBUS_SYSTEM_BUS_ADDRESS`に設定し、
`KIS_GPU_TEST_ISOLATED_BUS=1`で同じテストを実行する。テストが専用バス上で
通知サービスを登録し、実際の`LowMemoryWarning(y)`による休止と再開を検査する。
構築・実GPU検査は[開発手順](DEVELOPMENT.md#gpu文書タイルの検証)に従う。
