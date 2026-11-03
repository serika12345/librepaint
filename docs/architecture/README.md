# LibrePaintアーキテクチャガイド

変更内容から調査対象を絞り、主要な設計境界と実行経路を把握するための入口です。
作業の目的・前提・完了条件・検証結果は GitHub Issue、着手順と進行状態は GitHub Projects が管理します。

実際のリンク境界は各ディレクトリーの `CMakeLists.txt` を正本とし、図は責務と主要な実行経路を示します。

## パッケージ境界

`docs/architecture/package-boundaries.json` が責務と中核所有ターゲット、許可する直接リンク方向を保持します。
所有ターゲットは一つの責務へ一意に属し、許可方向は有向非巡回グラフを形成します。

| 責務ID | 中核所有ターゲット | 対象 |
| --- | --- | --- |
| `application-configuration` | `kritaapplication` | 設定値、スナップ方針、プラットフォームのファイル交換 |
| `application-orchestration` | `krita`、`kritaapplicationui` | 起動、OSライフサイクル、ウィンドウ、作業空間 |
| `canvas-presentation` | `kritabasicflakes`、`kritacanvas`、`kritaflake`、`kritaworkspacepresentation` | 座標変換、キャンバス、ベクター、作業空間表示 |
| `document-lifecycle` | `kritadocument`、`kritadocumentfiles`、`kritadocumentui` | 文書寿命、変更状態、保存、取り消し、文書表示 |
| `import-export` | `kritaimpex`、`kritaimpexui` | 形式選択、検証、文書入出力、結果通知 |
| `input-interpretation` | `kritainput`、`kritainputui` | ポインター、キーボード、タッチ、タブレット、ショートカット |
| `painting-rendering` | `kritacolor`、`kritaimage`、`kritalibbrush`、`kritapainting`、`kritapaintingmetadata`、`kritapaintingundo`、`kritapigment` | 色、ブラシ、画像、投影、ストローク、描画、メタデータ |
| `plugin-infrastructure` | `kritaplugin` | メタデータ探索、ファクトリーとサービス種別の登録 |
| `resource-management` | `kritaresources`、`kritaresourcestorage`、`kritaresourceui` | リソースの保存、検索、タグ、選択、表示 |
| `tool-invocation` | `kritatools`、`kritatoolsui` | ツール命令、描画設定表示、キャンバス状態への呼出し |

所有一意性・参照整合性・許可方向の非循環性は `scripts/architecture/check_package_boundaries.py` が、
各プラットフォーム構成時の実 CMake File API グラフは CMake 依存検査が検証します。

## 責務別の所有先

| 責務 | 所有ディレクトリー | 新しいAPIの名前空間 | 主ターゲット |
| --- | --- | --- | --- |
| プラグイン基盤 | `libs/koplugin` | `Krita::Plugin` | `kritaplugin` |
| リソース管理 | `libs/resources` | `Krita::Resources` | `kritaresources` |
| 画像・描画操作状態 | `libs/image` | `Krita::Image` | `kritaimage` |
| 描画 | `libs/painting` | `Krita::Painting` | `kritapainting` |
| 入出力 | `libs/impex` | `Krita::ImportExport` | `kritaimpex` |
| キャンバス表示 | `libs/canvas` | `Krita::Canvas` | `kritacanvas` |
| 文書寿命 | `libs/document` | `Krita::Document` | `kritadocument` |
| ツール呼出し | `libs/tools` | `Krita::Tools` | `kritatools` |
| 入力解釈 | `libs/input` | `Krita::Input` | `kritainput` |
| アプリケーション設定 | `libs/application` | `Krita::ApplicationConfiguration` | `kritaapplication` |
| アプリケーション調整 | `libs/application/ui` | `Krita::Application` | `kritaapplicationui` |

## GPU 常駐描画エンジン

通常の文書編集を GPU 主体の新エンジンへ置換する作業は
[#87](https://github.com/serika12345/librepaint/issues/87) と
[#89](https://github.com/serika12345/librepaint/issues/89) が所有する。製品要件と移行範囲は
[#107](https://github.com/serika12345/librepaint/issues/107) が定める。

- 編集中の画素は GPU 上の疎な領域を正本とし、文書構造と回復用チェックポイントは CPU で管理する。
- 応答に効く経路（主要ブラシ、レイヤー合成、表示、選択、変形プレビュー）は GPU 常駐にする。それ以外の操作は
  能力表で CPU 経由（対象タイルを読み出して CPU で計算し、書き戻す）で実行する。
- 確定画素は f32 で計算する。保存結果の画素は環境間・旧エンジンと 8bit 換算で1階調以内に収め、画素以外は完全に一致させる。
- 空タイルの合成色は、描画済みタイルと同じ GPU の合成関数で計算する。文書構造・既定色・合成設定が同じ版は
  計算結果を共有し、読出しと保存にも用いる。描画器を単独で呼ぶ場合は、命令の記録を始める前に
  `TileRenderer::prepareDefaultPixels()` で既定画素を準備する。
- 履歴は文書の版のリストで、版どうしは変更のないタイルを共有する。Undo/Redo は版の切替えで行い、
  履歴の上限は重複を除いたタイルのバイト数で決める。描画済みの版から新しい版や履歴のコピーを作る際は、
  タイルを引き継いで入力標本・形状・ブラシ設定を解放する。描画前に連続して確定した版は、最初の描画まで入力を保持する。
- 読込み・直接書込み・色変換の CPU 画素は、GPU へ転送する入力 `UploadTile` として受け取る。
  選択マスク・色領域・レイヤー統合は生成後の GPU 画素を保持し、生成元の文書や手順を履歴に残さない。
- GPU 喪失時は強制終了せず、処理を止めて通知し、最後の自動保存から復旧する。
- 描画・合成・表示は `libs/image/gpu/`、キャンバスは `libs/canvas/gpu/`、文書寿命は `libs/document/gpu/` が所有する。
- 通常の新規文書と KRA・PNG・JPEG 読込は既定で GPU エンジンを使用する。GPU 初期化に失敗した場合は診断を出し、CPU エンジンを使用する。
- 移行中は `LIBREPAINT_GPU_ENGINE=0` で CPU エンジンを選択できる。この設定は通常経路への統一後に撤去する。

採用する設計とバックログは Issue #89 の本文を参照します。
