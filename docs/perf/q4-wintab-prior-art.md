# Windows の Wintab の対応範囲：先行研究

この文書は、Q4 の段 4（フォークの改変 051139ed72、Wintab の対応範囲）を作り直すかどうか、どう作るかを決めるための調査の記録である。読者は監査役と利用者である。2026-11-11 に調べた。

## 問題

Wintab はペンタブレットのドライバー（主にワコム）が座標を渡す仕組みである。ドライバーには、タブレットの面を画面のどこに対応させるか（全部の画面、1 つの画面、画面の一部）の設定があり、その範囲は Wintab のシステムの文脈の lcSysOrg と lcSysExt で読める。

正式版 Qt 6.11.3 の `QWindowsTabletSupport::translateTabletPacketEvent()` は、タブレットの生の座標を常に仮想デスクトップ全体に当てはめる。lcSysOrg と lcSysExt は使わない（dev の版も同じ）。当てはめた位置とカーソルの差が、近接の始めに 20px を超えると、その近接の間はカーソルの位置（整数の画素）を使う。このため、ドライバーで範囲を絞った利用者では位置が整数に丸まり、拡大した描画で線が階段状になる。LibrePaint の設定ページ（KisDlgCustomTabletResolution）の「対応範囲」も効かない。

## 他のソフトの扱い

| ソフト | 当てはめ先 | 範囲が食い違うとき | 確かさ |
| --- | --- | --- | --- |
| 正式版 Qt 5.15 / 6.x（OpenToonz、Tahoma2D も同じ） | 仮想デスクトップ | 近接ごとにカーソルの位置へ切り替える | Qt の dev のソースで確認 |
| Krita 4.2 以降（Krita の改変を当てた Qt） | lcSys（既定）。設定で仮想デスクトップか任意の矩形 | 相対モードの切り替えを外す改変を当てていたとされる | 第三者の調査の記録による。Krita の Qt 6 の版で残っているかは不明 |
| Blender | lcSys | 押下のときにカーソルと 1px 以内で一致しなければ、その間はカーソルの位置と Wintab の筆圧を使う | 第三者の調査の記録による（Blender のソースは取得できなかった） |
| Drawpile | lcSys | 不明 | 第三者の調査の記録による |
| GTK（GIMP 3、Inkscape、MyPaint） | 既定は Windows Ink。Wintab では仮想デスクトップ全体 | 不明 | 第三者の調査の記録による |
| CLIP STUDIO PAINT | 不明 | 利用者に画面の拡大率をそろえるか、マウスモードを使うよう案内している | 第三者の調査の記録による |

Qt 本家には、1 つの画面への対応の不具合の記録がある（QTBUG-39579 は Closed Incomplete、QTBUG-69595 は Closed Out of scope、第三者の調査の記録による）。Krita が本家に出した変更（codereview.qt-project.org の 259161）は、Qt の dev のソースに lcSys の利用がないことから、取り込まれていないと見られる（変更の頁は取得できなかった）。

## ドライバーによる違い

第三者の調査の作者の実測では、ワコムのドライバーの lcSysExt は常に物理の画素の仮想デスクトップと一致した。Huion のドライバー（V20）は、拡大率の違う複数の画面で、1 つの画面に対応させたときに lcSysExt を誤って報告した。このため、lcSys を無条件に信じる作りは、その条件の Huion の利用者で位置がずれる。Windows Ink（WM_POINTER）は、試された配置のほぼすべてで正しかった。

## LibrePaint の現状

- Windows のタブレットの経路は、設定がないとき Wintab になる（krita/main.cc の forceWinTab）。Windows Ink は設定で選ぶ。
- 段 4 の実装（4237a0c188、43860eb8ab、db5d1ed9fb）は、Qt がパケットを取り出す前に WTPacketsPeek で読み、lcSys（または仮想デスクトップ、任意の矩形）に当てはめ、時刻で対応づけて QTabletEvent の位置を差し替える。Krita 4.2 以降と同じ当てはめ方で、Blender のような押下時の一致の確かめはない。実機では未確認。

## 出典

- Qt qtbase dev の qwindowstabletsupport.cpp：https://raw.githubusercontent.com/qt/qtbase/dev/src/plugins/platforms/windows/qwindowstabletsupport.cpp
- Qt qtbase v6.11.3 の同じファイル：https://raw.githubusercontent.com/qt/qtbase/v6.11.3/src/plugins/platforms/windows/qwindowstabletsupport.cpp
- KDE の T10838「Qt patches required by Krita」：https://phabricator.kde.org/T10838
- Krita の fbba406b（2019-03-20、Qt の Wintab と Windows Ink を Krita の改変版に切り替えた）：https://invent.kde.org/graphics/krita/-/commit/fbba406b15d7f6b066fd40d5bd28e5d92964bb13
- WinPenKit の調査（第三者）：https://github.com/TheSevenPens/WinPenKit/issues/132、https://github.com/TheSevenPens/WinPenKit/issues/129、https://github.com/TheSevenPens/WinPenKit/blob/claude/project-thread-xpv4jf/Docs/WINTAB-MAPPING-PRIOR-ART.md
- Krita の Qt の使い方の調査（第三者）：https://github.com/TheSevenPens/devnotes/issues/23
- Blender の不具合の記録：https://developer.blender.org/T84179、https://projects.blender.org/blender/blender/issues/84832
- 取得できなかった資料：ワコムの開発者向けの Wintab の頁（HTTP 403）、Blender の GHOST_WintabWin32.cc（HTTP 404）、Qt の変更 259161 の頁（中身が空）。
