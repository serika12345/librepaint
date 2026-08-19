# 文書ドメイン分離計画

## 目的

この計画は、`libs/ui/KisDocument.{h,cpp}`に集まる文書の保存、回復、入出力、
文書情報更新を、表示接続、利用事例、ドメイン判断、副作用アダプターへ分ける
R1-G6e後半の実装順を定義する。

UIは画面状態、利用者通知、Qt Widgetsのイベント配線を所有し、文書操作を利用事例として
呼び出す。ドメイン層は文書状態と、入力値から状態遷移または実行指示を導く判断を所有する。
ファイル、設定、時刻、背景処理、形式変換は利用事例が所有する接続面を通じて実行する。

## 起点と構造的問題

最初の起点は次の実装である。

- `libs/document/undo/kis_document_undo_store.{h,cpp}`による文書と、履歴処理・
  Qt Widgets用アクション生成を同じ公開面に持つ`kritapaintingundo`との接続
- `libs/command/{kundo2model,kundo2view}.{h,cpp}`の履歴表示
- `libs/ui/KisDocument.cpp`の回復用自動保存、通常保存、自動保存、文書読込の調整
- `libs/ui/KisDocument.cpp`と`libs/ui/KoDocumentInfo.{h,cpp}`の文書情報、設定、時刻、
  ファイル属性の更新
- `libs/ui/KisDocument.cpp`のダイアログ、状態表示、Qt通知と文書処理の接続

`KisDocument`は文書状態を保持する一方で、ファイル属性と設定の取得、バックアップ作成、
形式変換、背景処理、状態遷移、ダイアログ表示、Qt通知を同じ制御経路で実行する。
この構造では、保存や回復の判断をUIとI/Oなしで検査できず、状態だけを個別クラスへ移すほど
利用事例の順序が`KisDocument`へ残る。後続実装は、状態単位ではなく観測可能な利用事例を
端から端まで分離する。

## 目標パッケージ

| 所有先 | CMakeターゲット | 責務 | 許可する主な依存 |
| --- | --- | --- | --- |
| `libs/document/session`、`libs/document/metadata` | `kritadocument` | 文書状態、値検証、状態遷移 | Qt Core |
| `libs/document/usecases` | `kritadocumentusecases` | 文書利用事例、利用事例固有の副作用接続面、実行指示と結果 | `kritadocument` |
| `libs/document/persistence` | `kritadocumentpersistence` | ファイル、形式変換、設定、時刻、背景処理の実アダプター | `kritadocumentusecases`、入出力、描画、リソースの公開面 |
| `libs/document/ui` | `kritadocumentui` | ダイアログ、状態表示、文書と取り消し履歴の接続、履歴表示、利用事例の呼出し | `kritadocumentusecases`、`kritadocument`、`kritapaintingundo`、Qt Widgets |
| `libs/application` | `kritaapplication` | 実アダプターの構成と寿命管理 | 文書の利用事例、アダプター、UI |

依存方向は、UIから利用事例、利用事例からドメインへ向ける。副作用アダプターは利用事例側が
定義する接続面を実装し、利用事例から具体的なI/O実装へのリンクを作らない。
`kritadocument`と`kritadocumentusecases`はQt Gui、Qt Widgets、`QFile`、`QTimer`、
`QSettings`へ依存しない。パス、MIME形式、時刻などの入力値にはQt Coreの値型を利用できる。

R1-G6eでは既存`KisDocument`の公開面をUI接続として維持し、利用事例とアダプターを
構築する範囲を一つの構成アダプターへ限定する。R1-G6hはこの構成をアプリケーション起動時の
所有へ移し、R1-G7が一時構成経路の不在を検査する。

## 設計規則

- 各実装PRは一つの利用事例または一つの依存純化を扱い、起点と移動先を明示する。
- 利用事例は要求値を受け、完了結果、状態遷移、または外部で実行する型付き指示を返す。
- 副作用接続面は具体的な利用事例が必要とする操作だけを持ち、汎用リポジトリー、
  サービス探索器、共通利用事例基底クラスを導入しない。
- 新しい副作用接続面は、同じ検査段階で一つ以上の製品アダプター、決定的な試験用実装、
  所有寿命、NULL許容性、完了方法、エラー契約を持つ。
- 非同期処理は操作識別子を持つ開始指示と完了入力に分け、UIオブジェクトを捕捉する
  コールバックをドメイン層へ渡さない。
- 利用者向け文字列とダイアログ選択はUIが所有する。利用事例は意味を持つ結果コードと
  表示に必要な値を返す。
- ファイルや文書状態は、外部処理の成功が確定してから一度だけ更新する。取消しと失敗は
  個別の結果として扱う。
- 旧ファイル、転送ヘッダー、旧名の別名は各移動単位で除去する。一時構成経路は
  `package-relocation-plan.json`に記録した最大範囲を超えない。

## 実装検査段階

### R1-G6e-D0 計画と開始条件

状態は`completed`とする。この文書、R1 TODO、責務再配置計画、進捗スナップショットが、
目標ターゲット、依存方向、実装順、完了条件、検証方法を同じ内容で記録する。

### R1-G6e-D1 文書ドメインからの取り消しUI依存除去

`libs/document/undo/kis_document_undo_store.{h,cpp}`を起点として、画像所有の取り消し格納先と
文書UI所有の履歴を結ぶ接続を`libs/document/ui/undo`へ移す。
`libs/command/{kundo2model,kundo2view}.{h,cpp}`も履歴表示として同じ所有へ移す。

完了時には`kritadocument`のリンク閉包からQt GuiとQt Widgetsがなく、既存の取消し、
やり直し、操作名、アクション有効状態、履歴表示が同じ契約を満たす。履歴状態と表示操作を
文書ドメインへ戻す依存、公開API退行、CMake循環を診断した場合は次段階へ進まない。

`KUndo2Stack`と`KUndo2Group`のアクション生成は既存の公開記号を持つため、この段階では
`kritapaintingundo`内に維持する。文書ドメインが同ターゲットを必要としない構造を先に成立させ、
汎用取り消しライブラリー自体のAPI／ABI移行は、利用元と互換範囲を扱う独立した検査段階で
判断する。

### R1-G6e-D2 回復用自動保存の利用事例

`KisDocument::requestRecoveryAutoSave()`、`startRecoveryAutoSave()`、
`finishRecoveryAutoSaveRequest()`、`slotCompleteRecoveryAutoSaving()`、
`slotContinuePendingRecoveryAutoSave()`を最初の縦方向分離とする。

`kritadocumentusecases`が既存`RecoveryAutoSaveState`を構成し、要求の受付、既存保存への合流、
開始指示、延期完了、結果確定を所有する。
ファイルの存在と大きさ、背景保存の開始は副作用入力または実行指示として扱い、Qt通知と
状態表示はUI接続が行う。要求ごとの一度限りの完了、同期完了、開始失敗、既存保存への合流を
UIと実ファイルなしで検査する。

### R1-G6e-D3 通常保存と書出しの利用事例

`KisDocument::exportDocumentImpl()`、`saveAs()`、`save()`、
`slotCompleteSavingDocument()`、`startExportInBackground()`を起点とする。

保存要求、書出し要求、上書き可否、バックアップ方針、背景保存開始、完了結果、保存成功後の
文書状態更新を利用事例とアダプターへ分ける。UIは警告と失敗の表示、状態表示を所有する。
通常保存と書出しの差、取消し、警告を伴う成功、背景保存競合、画像ロック失敗、Androidの
内容URI、macOSの保安範囲付きアクセスを既存契約として固定する。

### R1-G6e-D4 自動保存と回復ファイルI/O

`KisDocument::slotAutoSave()`、`slotAutoSaveImpl()`、`slotCompleteAutoSaving()`、
`autoSaveOnPause()`、`removeAutoSaveFiles()`を起点とする。

自動保存の実行要否、間隔変更、失敗後の再試行、複製利用、回復要求への完了引継ぎを
利用事例が判断する。タイマー、設定、ファイル作成と消去はアダプターが所有する。
通常保存、自動保存、回復用自動保存を同時に要求したときの順序と最終状態を決定的に検査する。

### R1-G6e-D5 文書読込と回復読込

`KisDocument::importDocument()`、`openPath()`、`openFile()`、`loadNativeFormat()`を起点とする。

パス解決、形式選択、読込実行、回復データ選択、文書状態の確定、利用者への診断を分ける。
形式変換は`kritaimpex`の所有を維持し、文書利用事例はその公開面を具体アダプターから利用する。
失敗した読込が既存文書を変更しないこと、回復済み状態、読込警告、全対象OSのパス表現を
契約として固定する。

### R1-G6e-D6 文書情報、設定、時刻の分離

`libs/ui/KoDocumentInfo.{h,cpp}`と`KisDocument::updateEditingTime()`を起点として、文書情報の
値と更新規則を`libs/document/metadata`へ、時刻、設定、ファイル属性を
`libs/document/persistence`へ、編集画面を`libs/document/ui`へ分ける。

編集時間、作成日時、更新日時、作成者情報、ファイル属性を固定入力で検査でき、ドメイン状態が
UI文書、現在時刻、設定保存場所を直接参照しないことを完了条件とする。

### R1-G6e-D7 文書UI接続と構成経路の確定

`libs/ui/KisDocument.{h,cpp}`に残るダイアログ、状態表示、Qt通知、利用事例呼出しを
`libs/document/ui`の`kritadocumentui`へ移す。文書状態の公開面は`kritadocument`、
利用事例の公開面は`kritadocumentusecases`が所有し、利用元を正規ヘッダーへ更新する。

R1-G6e開始時の文書状態25クラスについて最終所有先を記録する。ノード操作、選択操作、
表示モデルは文書寿命へまとめず、実責務に応じてR1-G6fまたはUI表示所有へ割り当てる。
実移動をR1-G6fへ送る項目は、起点、目標所有者、開始条件を責務台帳に残す。

`KisDocument`内で利用事例とアダプターを構成する一時経路は一つに限定し、R1-G6hの
アプリケーション構成が同じ所有寿命で置換できる契約を持つ。旧配置、未記録の接続層、
文書ターゲットからQt Widgetsまたは具体I/Oへの依存が残る場合はR1-G6eを完了にしない。

## マイルストーン完了条件

- UIが保存、書出し、自動保存、回復、読込を文書利用事例として呼び出す。
- 状態遷移と実行指示を、Qt Widgets、実ファイル、実時計、利用者設定なしで反復検査できる。
- ファイル、形式変換、設定、時刻、背景処理の各副作用が、利用事例所有の狭い接続面と
  製品アダプターを持つ。
- `kritadocument`と`kritadocumentusecases`のリンク閉包がQt GuiとQt Widgetsを含まない。
- `kritadocumentui`が表示とイベント配線を所有し、`kritadocumentpersistence`が具体I/Oを所有する。
- R1-G6e開始時の25クラスすべてに、実責務に基づく最終所有先が記録される。
- 一時構成経路が記録済みの一箇所だけとなり、R1-G6hでの置換条件とR1-G7での削除検査を持つ。
- 5構成の中核所有ターゲットと全製品ターゲットが循環0件を維持する。

## 検証

各検査段階は、変更対象の特性試験で初期診断を確認してから実装する。文書ドメイン、利用事例、
アダプター、UI接続の対象CTest、`verify-quick`、macOSとLinuxの全ネイティブ試験を実行する。
CMake境界を変更する段階ではmacOS、Linux、iOS、Android、Windowsの対象ターゲットを構築し、
5構成のCMake台帳、依存行列、循環、公開面を同一コミットから再生成する。

## 保守

検査段階の完了時に、この文書の状態、`TODO.md`、`PROGRESS.md`、責務台帳、公開面台帳、
再配置計画、CMake台帳を同じ変更で更新する。利用事例または副作用が増える場合は、既存の
所有者で表現できるかを先に確認し、新しい接続面が必要な場合は製品アダプターと決定的試験を
同じ検査段階へ含める。
