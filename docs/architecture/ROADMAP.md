# LibrePaintロードマップ

## 目的と利用方法

GitHub Issuesを作業の目的、範囲、前提条件、状態、完了条件、検証結果の正本とする。
本書は段階の順序とIssueへの入口を所有する。再開に必要な手元の情報は
[作業スナップショット](PROGRESS.md)、操作手順は[開発マニュアル](DEVELOPMENT.md)、
設計と責務境界は[アーキテクチャガイド](README.md)を参照する。

全体の完了条件と段階間の対応は[#66](https://github.com/serika12345/librepaint/issues/66)で管理する。

## 段階と着手条件

| 段階 | 到達する状態 | 本実装の前提 | Issue |
| --- | --- | --- | --- |
| G0 | 再現可能な開発・検証と作業管理 | 起点 | [運用手順](DEVELOPMENT.md) |
| R1 | 責務、パッケージ、依存方向の確立 | G0 | [#41](https://github.com/serika12345/librepaint/issues/41) |
| R2 | 描画・入力・状態遷移・性能の基準契約 | R1 | [#61](https://github.com/serika12345/librepaint/issues/61) |
| R3 | 基準契約を維持した描画最適化 | R2 | [#62](https://github.com/serika12345/librepaint/issues/62) |
| R4 | 安定した描画境界からのVulkan導入 | R3 | [#63](https://github.com/serika12345/librepaint/issues/63) |
| R5 | 共通機能を利用するモバイルUI | R2、R4 | [#64](https://github.com/serika12345/librepaint/issues/64) |
| R6 | 共通言語基盤と責務単位の現代化 | R1〜R5 | [#65](https://github.com/serika12345/librepaint/issues/65) |

後続段階の調査と試作は先行できる。描画アルゴリズム、実行順序、同期方法の変更は
R2の互換性契約を前提とする。共通言語基準の独立した移行は
[#52](https://github.com/serika12345/librepaint/issues/52)のC++23対応条件を使用する。
製品のC++17設定は、その移行で検証を満たすまで維持する。

## 個別作業と既存Issueの対応

| 旧項目または責務 | Issue |
| --- | --- |
| R2-G19d-h クイックピンチ受入れ | [#56](https://github.com/serika12345/librepaint/issues/56) |
| R2-G19d-b OS固有契約 | [#57](https://github.com/serika12345/librepaint/issues/57) |
| R2-G19e ブラシ設定試験 | [#58](https://github.com/serika12345/librepaint/issues/58) |
| R2-G20 矩形選択描画 | [#59](https://github.com/serika12345/librepaint/issues/59) |
| R2-G19cから独立したダイアログ寿命 | [#60](https://github.com/serika12345/librepaint/issues/60) |
| iPadOSの機能・実機・配布条件 | [#67](https://github.com/serika12345/librepaint/issues/67) |
| Androidの署名済みAPKとGitHub Releases配布 | [#70](https://github.com/serika12345/librepaint/issues/70) |
| Windowsの依存構造 | [#68](https://github.com/serika12345/librepaint/issues/68) |
| 機能要望 | [#42](https://github.com/serika12345/librepaint/issues/42) |
| ドメイン処理と外部I/Oの境界 | [#48](https://github.com/serika12345/librepaint/issues/48) |
| 設計境界に必要な検査への集約 | [#69](https://github.com/serika12345/librepaint/issues/69) |
| 決定的計算の試験 | [#49](https://github.com/serika12345/librepaint/issues/49) |
| 上流機能の要求からの再実装 | [#51](https://github.com/serika12345/librepaint/issues/51) |
| 共通言語基準 | [#52](https://github.com/serika12345/librepaint/issues/52) |
| iPad実機の自動検査基盤 | [#55](https://github.com/serika12345/librepaint/issues/55) |

## 保守

作業開始時は既存Issueを検索し、同じ目的と完了条件を持つものへ集約する。大きい項目は
親Issueに全体の完了条件を置き、実装可能な単位の子Issueから前提と親を参照する。
個別の進捗、検証結果、保留理由、次の操作は対応Issueへ記録する。段階や対応先を
変更したときは本書を更新する。Issueの運用形式は開発マニュアルの
[Issueによる作業管理](DEVELOPMENT.md#issueによる作業管理)に従う。
