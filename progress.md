
## 2026-11-11 20:10:10
- judge-fix3-spec: run-surface.ps1 でペン手順の前に主窓を前面にして D キーを送り、facts.pen.colors_reset を記録する。edge/single/dot の描画直前に *-before を取り、facts.pen.captures に加える。check.py は *_before があるとき画素差（R/G/B のどれかが 48 超）を black/white にして同じ dark_runs/dark_components/count_dark で数える。
- 検証: ./scripts/run-shared-test-env python3 -m unittest scripts.tests.test_q4_e2e_check（33 tests OK）。run-surface.ps1 は ASCII のみ。
- 残る危険: 実機での before 撮影と D キーの効果は Surface での取り直しで確認する。差の写しがない場合は従来の判定のまま。
- 次: oc.sh event で計測を依頼する。
