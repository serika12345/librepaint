# 文書パッケージ境界計画

## 目的

この計画は、R1-G6e後半で文書コードの依存方向、名前、配置を改善する実装順を定義する。
現在存在する文書状態、文書表示、文書ファイル保存を具体的な所有先へ集約し、
`libs/ui`から文書責務を段階的に減らす。

この段階はロジック構造の再設計を目的としない。パッケージ境界が成立した後、実装の重複、
差し替え要求、決定的試験の必要性、残る依存循環から根拠を得た場合に、値計算とI/Oの分離や
抽象接続面を独立した検査段階として計画する。

## リファクタリング順序

文書境界は次の順で改善する。

1. 依存経路を一方向にする。
2. パッケージとCMakeターゲットを、現在所有する具体的な関心に合わせて命名する。
3. 既存実装を文書状態、文書表示、文書ファイル保存へ分け、各所有先へ集約する。
4. 新しい所有境界から実際の必要性を確認した後、ロジック構造と抽象接続面を検討する。

正しい方向の具体依存で現在の実装を表現できる場合は、その依存を使用する。将来の差し替え、
将来のI/O隔離、試験用模擬実装だけを理由として中間層を追加しない。

## 起点と現在の問題

- `libs/document/undo/kis_document_undo_store.{h,cpp}`は文書所有の接続である一方、
  履歴処理とQt Widgets用操作を同じ公開面に持つ`kritapaintingundo`へ依存する。この依存により
  `kritadocument`の公開リンク閉包へQt GuiとQt Widgetsが入る。
- `libs/command/{kundo2model,kundo2view}.{h,cpp}`は履歴表示だが、文書表示の所有先にない。
- `libs/ui/KisDocument.cpp`は文書状態、保存用ファイル、バックアップ、自動保存、回復ファイル、
  ダイアログ、状態表示、Qt通知を同じ実装単位に持つ。
- `libs/ui/KoDocumentInfo.{h,cpp}`と`KoDocumentInfoDlg.{h,cpp}`は文書情報と編集画面を
  UI直下で所有する。

状態の小クラスだけを追加抽出しても、これらのパッケージ依存と配置は改善しない。
後続作業はフィールド数ではなく、依存辺、所有ファイル、CMakeターゲットを完了判定に使う。

## 目標所有

| 所有先 | CMakeターゲット | 現在確認できる関心 | 依存方向 |
| --- | --- | --- | --- |
| `libs/document/session` | `kritadocument` | 文書識別、変更状態、自動保存状態、回復状態 | Qt Coreだけを利用する |
| `libs/impex/metadata` | `kritaimpex` | 形式処理と文書表示が共有する直列化対象の文書情報 | 描画、プラグイン、リソースの公開面を利用する |
| `libs/document/files` | `kritadocumentfiles` | 文書ファイル、バックアップ、自動保存ファイル、回復ファイルの具体処理 | `kritadocument`、`kritaimpex`、描画、リソースの公開面を利用する |
| `libs/document/ui` | `kritadocumentui` | 履歴接続と表示、文書情報編集、保存・読込のダイアログ、状態表示、Qt通知 | `kritadocumentfiles`、`kritadocument`、`kritaimpex`を利用する |

`kritadocument`は`kritadocumentfiles`と`kritadocumentui`へ依存しない。
`kritadocumentfiles`は`kritadocumentui`と`kritaui`へ依存しない。
`kritadocumentui`が文書の具体機能を呼び出す最上位の文書パッケージとなる。

形式選択と形式変換は既存`kritaimpex`と`kritaimpexui`が所有する。文書ファイル保存から
必要な公開面を直接利用し、同じ処理を包む新しい形式変換接続面は作らない。

## YAGNIの適用

このマイルストーンは`usecases`、汎用`persistence`、`ports`、`adapters`という技術分類の
パッケージを作成しない。現在のコードから独立した所有物を示せず、空の層を先に固定するためである。

新しい抽象は、作業中の変更に次のいずれかの根拠がある場合だけ追加できる。

- 現在利用する製品実装が複数存在する。
- 値の受渡しでは成立しない決定的な試験境界が必要である。
- 要求された挙動が外部処理の差し替えを必要とする。
- 所有移動と直接依存では解消できない循環が残る。

該当する場合も、抽象名は保存、回復、時刻などの具体能力を表し、同じ変更で製品利用元、
製品実装、観測可能な契約、所有寿命を追加する。共通利用事例基底クラス、サービス探索器、
汎用リポジトリー、将来用の空ターゲットは追加しない。

## 実装検査段階

### R1-G6e-P0 計画と指示

状態は`completed`とする。`AGENTS.md`、この計画、R1 TODO、進捗スナップショット、
再配置計画が、リファクタリング順序、YAGNI、具体所有、検証条件を同じ内容で記録する。

### R1-G6e-P1 独立した文書UIの一括移設

状態は`completed`とする。

次の独立ファイル移設を一つのPRとして実装する。

- `libs/document/undo/kis_document_undo_store.{h,cpp}`から
  `libs/document/ui/undo/kis_document_undo_store.{h,cpp}`へ移す。
- `libs/command/{kundo2model,kundo2view}.{h,cpp}`から`libs/document/ui/undo`へ移す。
- `libs/ui/KisAutoSaveRecoveryDialog.{h,cpp}`から
  `libs/document/ui/recovery/KisAutoSaveRecoveryDialog.{h,cpp}`へ移す。
- `kritadocumentui`を、文書と取り消し履歴の接続、履歴表示、自動保存回復表示を所有する
  具体ターゲットとして作る。
- 履歴表示だけを所有していた旧`kritacommand`を除去し、旧配置、転送ヘッダー、別名を残さない。

既存の取消し、やり直し、マクロ、履歴通知、操作名、アクション有効状態、履歴表示に加え、
回復候補の初期選択と一括破棄を特性試験で固定する。完了時に`kritadocument`は
`kritapaintingundo`への公開依存を持たず、公開リンク閉包がQt Coreだけで成立する。

`KUndo2Stack`と`KUndo2Group`自体は既存の公開記号を持つため移動しない。文書境界の修正に
必要な接続と表示だけを移し、汎用取り消しライブラリーのAPI／ABI移行を同じPRへ含めない。

独立した製品ファイルを実装変更なしで所有ターゲットへ移せる変更はP1の一つのPRへまとめる。
`KoDocumentInfo`、`KoDocumentInfoDlg`、`KisDocument.cpp`内の処理は`KisDocument`の状態と
上位UI型へ直接依存しており、別ライブラリー化にはAPIと責務の再構築が必要である。このため
ファイル移動に見せかけた循環依存や互換層を追加せず、P2とP3の構造変更として扱う。

### R1-G6e-P2 文書表示の集約

状態は`completed`とする。

`libs/ui/KisDocument.cpp`の保存・読込ダイアログ、状態表示、Qt通知の接続を
`libs/document/ui/io/kis_document_io_presentation.{h,cpp}`へ移した。
`libs/ui/KoDocumentInfoDlg.{h,cpp}`と対応する2個のUIフォームは`libs/document/ui/info`へ、
`libs/ui/dialogs/KisRecoverNamedAutosaveDialog.{h,cpp,ui}`は`libs/document/ui/recovery`へ移した。
回復候補の画像生成はファイルを扱う`KisDocument`側に維持し、表示側は生成済みの`QIcon`値を
受け取る。

`libs/ui/KoDocumentInfo.{h,cpp}`は、形式処理が直接読み書きする直列化対象であることを
実依存から確認し、`libs/impex/metadata`の`kritaimpex`へ移した。これにより
`kritaimpexui`から上位の文書寿命への新しい逆方向依存を作らず、`kritadocument`はQt Coreだけの
公開リンク閉包を維持する。自動保存中か、文書が変更済みかという状態は親`QObject`から推測せず、
既存の呼出元が値として明示する。

表示が必要とする結果値には既存の入出力エラー型と文書状態を直接使用する。

表示コードを移すためだけの利用事例層やエラー変換階層は作らない。保存成功、警告を伴う成功、
失敗、取消し、バッチ処理時の非表示、Qt通知順序を既存契約として維持する。

### R1-G6e-P3 文書ファイル保存の集約

状態は`completed`とする。

`libs/ui/KisDocument.cpp`の文書ファイル、バックアップ、自動保存ファイル、回復ファイルに対する
具体処理を起点として、保存先検査を`libs/document/files/kis_document_save_target.{h,cpp}`へ、
バックアップ作成を`kis_document_backup_file.{h,cpp}`へ、自動保存名、回復候補の探索と読込、
使用可否判定、消去を`kis_document_autosave_files.{h,cpp}`へ集約した。
形式選択と形式変換は既存の入出力公開面を直接利用する。

既存処理を具体的な所有先へ移すことを主眼とし、保存処理のアルゴリズム、非同期実行方式、
結果型を再設計しない。ファイルパス、内容URI、書込可否、バックアップ世代、自動保存名、
回復ファイル消去の現行挙動を特性試験で固定する。

### R1-G6e-P4 境界評価

状態は`completed`とする。

文書クラスは`KisDocument`、外部ファイル層、操作管理、ノード・選択操作接続、Qtモデルと
表示状態の具体所有へ接続する。現在の宣言、実装、所有ターゲット、責務は
`ui-class-responsibilities.json`が記録する。

`KisDocument.cpp`にある130の一意なメソッド定義を8関心へ分類した。文書識別と状態は
`kritadocument`、具体的なファイル処理は`kritadocumentfiles`、文書表示は`kritadocumentui`、
形式処理は`kritaimpex`が所有する。画像、ノード、キャンバス表示、資源、アプリケーションとの
構成は対応する後続段階で扱う。

ノード表示モデルは`KisNodeManager`と操作接続へ、`KisFileLayer`は`KisPart`へ直接依存する。
先にファイルを移すと逆方向依存または移動専用の接続面が必要になるため、操作境界をR1-G6f、
アプリケーション構成をR1-G6hで先に解消する。現在は保存I/Oの差し替え、利用事例登録、
新しい純粋計算、汎用接続面を必要とする複数実装または試験阻害がなく、追加の抽象は導入しない。

## マイルストーン完了条件

- 文書パッケージ内の依存が`kritadocumentui`から`kritadocumentfiles`、
  `kritadocumentfiles`から`kritadocument`へ一方向に向く。
- `kritadocument`の公開リンク閉包がQt Coreだけで構成される。
- 文書表示と文書ファイル保存が`kritaui`ではなく、具体名を持つ所有ターゲットへ集約される。
- 形式選択と形式変換が既存の入出力所有に残り、重複する抽象接続面がない。
- このマイルストーンだけを根拠とする汎用利用事例、汎用永続化、接続面、アダプター、
  サービス探索器、共通基底クラス、空ターゲットが存在しない。
- R1-G6e開始時の25クラスすべてに、実依存に基づく最終所有先または後続ゲートが記録される。
- 5構成の中核所有ターゲットと全製品ターゲットが循環0件を維持する。

## 検証

各実装検査段階は、対象の特性試験で初期診断を確認してから最小の移動を行う。
対象CTest、`verify-quick`、macOSとLinuxの全ネイティブ試験を実行する。CMake境界を変更する
P1からP3ではmacOS、Linux、iOS、Android、Windowsの対象ターゲットを構築し、5構成の
CMake台帳、依存行列、循環、公開面を同一コミットから再生成する。
P4は製品ソースとCMake境界を変更せず、評価台帳の全件被覆、実装との一致、抽象化判断を
専用単体試験と`verify-quick`で検証する。

## 保守

各検査段階の完了時に、この文書、`TODO.md`、`PROGRESS.md`、責務台帳、公開面台帳、
再配置計画、CMake台帳を同じ変更で更新する。新しいパッケージまたは抽象を提案する場合は、
具体的な起点、現在の依存問題、製品利用元、製品実装、完了時に消える依存辺を記録する。
