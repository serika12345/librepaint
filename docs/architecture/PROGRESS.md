# LibrePaintアーキテクチャ作業状況

## 現在の作業スナップショット

- 更新日時: 2026-09-19 21:39 JST
- 状態: `planned`
- 現在の検査段階: R2-G19c 利用者から観測できる振る舞いを守るテストへの整理
- 関連TODO: R2-G19a・R2-G19b完了、R2-G19c次作業、R2-G19d対象OS検証
- ブランチ: `develop`
- 目的: 既存の試験コードとCMake定義を起点として、観測可能な振る舞いを守るテスト整理を開始できる状態にする。
- 完了: 初期API固定作業を完了とし、専用の対応管理・検査と関連規則を整理した。
- 次の作業: TabletTestDialogの既存試験・実装・利用側から必要な保証を確認し、最初のテスト整理を行う。
- 検証: macOSで高速検査に成功。Nixの全対応システム評価は公式キャッシュ指定で成功。

## 次の実装単位

R2-G19cとして`libs/input/ui/tests/TabletTestDialogSchemaContractTest.cpp`と
`libs/input/ui/wintab/drawpile_tablettester/`の実装・利用側を確認する。
対象試験の変更なし計画と`libs/input/ui/tests/CMakeLists.txt`の直接依存を調べ、
利用者に必要な保証、明示的な互換性要件、宣言形状だけの検査を区別する。
必要な意味論の既存検証を確認した上で、不要な検証と専用対象を整理する。

## 検証範囲と継続利用する生成物

製品C++と既存CTestの構成は維持し、今回の検証範囲は運用規則、文書、検査入口、Nix開発ツールである。
主増分構築木`build/tdd-macos`と共有コンパイラーキャッシュを継続利用する。
Androidのcrash通知統合、WindowsのMSVC互換実行環境、Linuxの色管理構成はR2-G19dで扱う。

## 再出発時点の検証結果

- `nix develop .#test --command ./scripts/verify-quick`: 成功。自己試験39件、10責務の境界方針、
  外部利用ヘッダー533件、プラグイン登録172件、テキスト、シェル、文書、リンク、生成図を確認した。
- `nix flake check --no-build --all-systems --option substituters https://cache.nixos.org --option connect-timeout 5 --option download-attempts 1`:
  成功。既定のローカルキャッシュは接続タイムアウトのため、この評価では公式キャッシュを指定した。
- `git diff --check`: 成功。
- 旧API不足報告1件（1,366 bytes）と削除済み検査のPython生成物を除去した。
- 全ネイティブ試験と対象OSでの実行検証は、製品コード・CTest構成を変更しない今回の範囲外である。
