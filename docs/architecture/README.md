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
[#89](https://github.com/serika12345/librepaint/issues/89) が所有する。

- 編集中の画素は GPU 上の疎な領域を正本とし、文書構造・操作記録・回復用チェックポイントは CPU で管理する。
- 描画操作は操作識別子・入力標本・形状・ブラシ設定・資源の識別子と版・乱数種・順序を保持する。
- 描画・合成・表示は `libs/image/gpu/`、キャンバスは `libs/canvas/gpu/`、文書寿命は `libs/document/gpu/` が所有する。
- 移行中の起動設定 `LIBREPAINT_GPU_ENGINE=1` は、通常経路への統一後に撤去する。

詳細な責務の置換元・置換先と実行段階は Issue #89 の本文を参照します。
