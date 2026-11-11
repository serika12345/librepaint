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

## Krita が自前のペンの処理から Qt の改変へ移った経緯

LibrePaint のリポジトリーに残る Krita のコミットの履歴で確かめた。

| 日付 | コミット | 内容（コミットの文から） |
| --- | --- | --- |
| 2019-03-11 | ec417d6c82 | Linux（X11）の自前のペンの処理を消し、Qt の処理を使う |
| 2019-03-15 | eb0928dff7 | Windows で Qt のペンの処理を使う設定を足す。「実験として毎日の版で有効にする」 |
| 2019-03-31 | 0f66ae41d6 | Qt の Windows Ink の実装への改変を足し、既定にする |
| 2019-04-02 | d5e5dd0ea5 | Windows の構築で Qt のペンの処理を既定にする |
| 2019-04-03 | 13181cd88e | Qt の中で Wintab と Windows Ink を切り替える改変。Qt 本家に提案（codereview 258067） |
| 2019-04-13 | 6806b44ec2 | Qt の相対モード（20px の差でカーソルの位置に切り替える処理）を外す。「20px はペンとマウスの位置の差としてごく普通で、通常の描画で問題を起こす」 |
| 2019-04-15 | 31c48ac7bf | Wintab の lcSys から対応範囲を読む（051139ed72 の元）。「Surface Pro 5 などは入力の範囲を内蔵の画面だけに対応させる」。QT_IGNORE_WINTAB_MAPPING で従来の仮想デスクトップに戻せる |
| 2019-05-29 | （Krita 4.2 の配布） | リリースノート：Windows の Wintab と Windows Ink のために書いたコードを Qt の既存のコードとまとめた。複数の画面の対応がよくなり、対応するタブレットが増え、多くの不具合が直った。Qt に改変を当てる必要があった |
| 2025-02-26 | 885f3348c6 | Qt 6 の切り替えの仕組みに移り、自前の Wintab（kis_tablet_support_win.cpp、978 行）と Windows Ink（kis_tablet_support_win8.cpp、1,092 行）の処理と付属のヘッダー、計 4,106 行を消す。「5 年以上使っておらず、動くかどうか誰も知らない」 |

- 移った理由として開発者が書いたのは、リリースノートの「Qt の既存のコードとまとめ、複数の画面の対応と対応する機種を広げ、不具合を減らした」だけである。Qt 5.12 が Windows Ink を持ったことが契機と読めるが、それを理由として書いた文は見つからなかった（推定）。
- Krita は自前の処理をやめた代わりに、Qt に改変を当て続けた（相対モードの無効化、lcSys の利用、ボタンの割り当て、切り替え）。本家に提案したものもあるが、Qt の dev の版に lcSys の利用はなく、相対モードも残っている。
- LibrePaint が Q4 で作り直したもの（WTPacketsPeek で覗いて位置を差し替える）は、この Qt の改変のうち lcSys の利用と相対モードの無効化を、Qt の外に移したものに当たる。
- 自前の処理に戻す場合の規模の目安は、Krita が 2025 年に消した 2,070 行（本体）である。5 年以上使われておらず、Qt 6 には合わせていない。

## 出典

- Qt qtbase dev の qwindowstabletsupport.cpp：https://raw.githubusercontent.com/qt/qtbase/dev/src/plugins/platforms/windows/qwindowstabletsupport.cpp
- Qt qtbase v6.11.3 の同じファイル：https://raw.githubusercontent.com/qt/qtbase/v6.11.3/src/plugins/platforms/windows/qwindowstabletsupport.cpp
- KDE の T10838「Qt patches required by Krita」：https://phabricator.kde.org/T10838
- Krita の fbba406b（2019-03-20、Qt の Wintab と Windows Ink を Krita の改変版に切り替えた）：https://invent.kde.org/graphics/krita/-/commit/fbba406b15d7f6b066fd40d5bd28e5d92964bb13
- WinPenKit の調査（第三者）：https://github.com/TheSevenPens/WinPenKit/issues/132、https://github.com/TheSevenPens/WinPenKit/issues/129、https://github.com/TheSevenPens/WinPenKit/blob/claude/project-thread-xpv4jf/Docs/WINTAB-MAPPING-PRIOR-ART.md
- Krita の Qt の使い方の調査（第三者）：https://github.com/TheSevenPens/devnotes/issues/23
- Blender の不具合の記録：https://developer.blender.org/T84179、https://projects.blender.org/blender/blender/issues/84832
- 取得できなかった資料：ワコムの開発者向けの Wintab の頁（HTTP 403）、Blender の GHOST_WintabWin32.cc（HTTP 404）、Qt の変更 259161 の頁（中身が空）。
