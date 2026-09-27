# iOS／iPadOS資料

LibrePaintのiOS／iPadOS向け機能、設計、検証記録をまとめています。
環境構築、ビルド、実機配備、キャッシュ保守は
[日本語開発マニュアルのiOS編](../development/ios.md)を参照してください。

| 資料 | 内容 |
| --- | --- |
| [機能範囲](feature-scope.md) | iPad向け採用機能と対象範囲 |
| [Androidとの共通化](android-reuse-audit.md) | 共通モバイル処理とプラットフォーム境界 |
| [依存構築の設計](dependency-design.md) | 依存物のリンク契約とアプリ・IPAの責務 |
| [NixとXcodeの分担](adr/0001-nix-xcode-boundary.md) | ホストとSDKの境界 |
| [依存物の派生物設計](adr/0002-nix-target-derivations.md) | 再現性とキャッシュの設計 |
| [配備とインポート](altstore-deployment.md) | AltStore、IPA権限、LiveContainer |
| [M1検証](validation-m1.md) | ホストと最小アプリ |
| [M2検証](validation-m2.md) | 基礎依存物 |
| [M3検証](validation-m3.md) | QtとFrameworks |
| [M4検証](validation-m4-static-plugins.md) | 静的プラグイン |
| [リスク](risk-register.md) | プラットフォーム固有の検証課題 |
| [非コード資産](non-code-assets.md) | 資産の採用と権利情報 |
