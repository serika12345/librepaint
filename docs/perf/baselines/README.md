# 主指標の基準の記録

`scripts/perf/run.py` の `--baseline` に渡す基準を、機器ごとの JSON で置く。比較の手順と判定の規則は
[`DEVELOPMENT.md`](../../architecture/DEVELOPMENT.md) の「性能計測」に書く。JSON は
`--baseline-out` が出す許可リスト方式の形式で、中央値、P95、回ごとの中央値、窓のばらつき、
安定の記録、計測時の条件（キャンバス、フレーム、拡大率、更新率）、機器の名前だけを持つ。

## 記録の一覧

| ファイル | 機器と対象 | 測った LibrePaint | 状態 |
| --- | --- | --- | --- |
| `qt612-base-win.json` | Surface Pro 7（Windows、60Hz）。LibrePaint と参照アプリ PenProbe | 最終版の Windows 版（Qt 6.12.0） | LibrePaint は未安定（ホバー 6.2%、インク 32.3%）。PenProbe は安定（ホバー 2.5%、インク 2.0%） |
| `qt612-base-linux.json` | GNOME、1920×1080、60Hz。X11 と Wayland それぞれの LibrePaint と参照アプリ | develop 2ae2650d56 の内容の Linux 版（Qt 6.12.0） | 4 対象とも未安定（ホバー 6.6〜16.0%、インク 11.8〜17.9%） |

どちらも 2026-11-16 に、ウォームアップ 1 回と集計 10 回（`max_runs`）で測った。`commit` は計測に使った
道具のコミットで、測った LibrePaint のコミットではない。macOS と iPad の基準は未測定である。

## 読み方

- 中央値と P95 は ms で、機器の `capture_offset_ms` を引いた値である。
- `spread` は、窓（直近 `runs` 回）の回ごとの中央値の（最大−最小）÷中央値である。`settle.unsettled` に
  指標が並ぶ対象は、`max_runs` まで足しても `spread_limit`（5%）未満にならなかった。
- 未安定の基準と比べると、判定は `check` になり、メモに `baseline unsettled` が付く。比べた差が基準の
  `spread` の外にあるかは、表の `run_medians` で人が確かめる。
- 計測時の条件が基準と 2% を超えて違う比較（キャンバスの大きさなど）は、判定を拒否する。
- Windows の LibrePaint のホバーは回ごとに 2 群に分かれ、インクにも別の群がある。原因は未特定で、
  Issue #125 で扱う。

## 更新

新しい基準は、同じシナリオ（`scripts/perf/scenarios/pen-light.json`）と同じ機器で測り直し、
`--baseline-out docs/perf/baselines/<名前>.json` で出す。名前の先頭に Qt の版を付ける。測った
LibrePaint の版は `DECISIONS.md` の該当の行に書く。
