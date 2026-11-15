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

## Qt を使う描画ソフトの扱い（ソースで確認）

2026-11-11 に各ソフトのリポジトリーのソースと変更履歴を直接読んだ。

| ソフト（Qt の版） | Windows のペンの入力 | 対応範囲 | 根拠 |
| --- | --- | --- | --- |
| Krita（Qt 6） | Qt の処理に、自前の Qt のフォーク（for-krita/6.11.3）で改変を当てる。lcSys の利用の改変（051139ed72）を今も持つ | lcSys | LibrePaint の docs/perf/qt-fork-patches.md（フォークの改変の一覧）、Krita の 885f3348c6（2025 年に自前の処理を削除） |
| Drawpile（Qt 6、開発が続いている） | Qt のペンの処理を使わず、Krita が昔使っていた自前の処理（Wintab の kis_tablet_support_win.cpp 約 990 行、Windows Ink の kis_tablet_support_win8.cpp）を取り込み、ネイティブのイベントフィルターで動かす。Windows の既定は自前の Windows Ink | 自前の Wintab は lcSys（wintabDesktopRect） | drawpile/Drawpile の src/desktop/bundled/kis_tablet/、40c09b270（2023-04-11「Qt を改変せずに KisTablet を動かす」）、ChangeLog「Qt のものはかなり壊れているので、KisTablet の Windows Ink を既定にする」、src/desktop/tabletinput.cpp のコメント「Qt 6 で Wintab のモードを有効にすると、メニューがペンの押下に時々しか反応しない。Qt 6 を Windows Ink のモードにしたまま KisTablet の Wintab を使えば問題ない」 |
| OpenToonz（Qt 5.15.2） | Windows だけ、Wintab と Windows Ink を切り替える改変を当てた Qt を配る | 正式版 Qt と同じ（仮想デスクトップとカーソルへの切り替え） | opentoonz/opentoonz の stuff/doc/LICENSE/notice_about_modified_qt.txt |
| Pencil2D、Tahoma2D | 正式版 Qt のまま。Wintab に関するコードは見つからなかった | 正式版 Qt と同じ | pencil2d/pencil、tahoma2d/tahoma2d のコードの検索 |

Qt を使わない Blender の扱い（intern/ghost/intern/GHOST_Wintab.cc を読んだ）：Wintab に生の座標を渡させ（「一部のドライバーは高 DPI や複数の画面を正しく扱わないので、拡大は手で行う」）、lcSys に整数の画素で当てはめる。ボタンの押下で、Windows のマウスの押下の知らせの位置と 1px 以内で一致したときだけ Wintab の位置を信じ、フォーカスを失うと信じるのをやめる。WM_DISPLAYCHANGE で範囲を読み直す。

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

## アプリ側で位置を差し替えることの副作用

Qt の外（LibrePaint のネイティブのイベントフィルターと KisInputManager）で位置を差し替える作りの副作用を、観点ごとに挙げる。「対策」は 2026-11-11 の利用者の決定（上策）で足すものを含む。

| 観点 | 起きうること | 対策と確かめ方 |
| --- | --- | --- |
| 性能（Windows Ink の経路） | ネイティブのイベントフィルターはすべての Windows のメッセージで呼ばれる | WT_PACKET 以外はメッセージ番号の比較 1 回で戻る。Windows Ink の経路の遅れと CPU を、差し替えのない版と交互に測る（自動） |
| 性能（Wintab の経路） | パケットごとに WTPacketsPeek（ドライバーの呼び出し）と表への書き込み、ペンのイベントごとに表の検索と QTabletEvent の作り直しが加わる | 実物のワコムのペンで、差し替えの有無の CPU を threads.csv で比べる。合成入力では Wintab の経路を通せない |
| 時刻の対応づけ | Wintab のパケットの時刻（pkTime）はミリ秒の単位で、266Hz のペンでは同じ時刻のパケットが続きうる。時刻だけで引くと、別のパケットの位置を当てる | 同じ時刻のパケットは、届いた順に 1 つずつ消費する（Qt もパケットの順にイベントを出す） |
| ドライバーが誤った範囲を報告する | Huion のドライバーは拡大率の違う複数の画面で lcSys を誤る。差し替えると位置がずれる | 上策の確かめ：近接の最初の押下で、lcSys で計算した位置とカーソルの位置の差が 2px を超えたら、その近接の間は差し替えずに Qt の位置を使う。印（wintab_trust）を残す |
| 差し替えが及ぶ範囲 | 差し替えるのはキャンバス（KisInputManager が受けるイベント）だけで、ドックやメニューなど他の部品には Qt の位置が届く | 他の部品は整数の位置で足りる（ボタンやスライダーの操作）。キャンバスと他の部品で位置の細かさが違うことを文書に残す |
| イベントの届け先 | Qt は自分の計算した位置で届け先の窓を決める。範囲が食い違うと、Qt はカーソルの位置（正しい画面）に切り替えるので、届け先は正しい | 届け先は変えない。範囲を絞った設定で、キャンバスに届くことを実物のペンで確かめる |
| 近接の出入りのイベント | Qt が作る近接の出入りのイベントは Qt の位置のまま | ホバーの輪郭はキャンバスの移動のイベントで描くので影響しない。実物のペンで確かめる |
| Qt の更新への依存 | WT_PACKET がネイティブのイベントフィルターに届くこと、イベントの時刻が pkTime であること、パケットの並び（PACKETDATA）が、Qt の内部の作りに依存する。Qt の更新で黙って壊れうる | パケットの並び（lcPktData）が一致しないときは何もしない（実装済み）。差し替えた割合を印に残し、Qt を更新したら実物のペンで確かめる手順を DEVELOPMENT.md に書く |
| 挙動の維持（旧版との比較） | 1.0.3（フォークの Qt）と同じく、ドライバーの範囲に小数の画素で当てはめ、設定ページの 3 つの選択が効く | 範囲を画面の左半分にした設定で、線が左半分に収まることを実物のペンで確かめる。1.0.3 と同じ操作で同じ結果になることを比べる |

## 試作：Windows のプラットフォームプラグインだけを改変して作る（2026-11-11）

正式版 Qt の全体を作り直さずに、Windows のプラットフォームプラグイン（qwindows.dll、ソースは qtbase の src/plugins/platforms/windows/）だけを、改変したソースから作れるかを試した。作業はリポジトリーの外（Linux 開発機の /tmp/qwp）で、手で行った。

- 方法：正式版 Qt 6.11.3 のソースの tar（Nix のストアにあるもの）から plugins/platforms/windows/ と 3rdparty/wintab/ を取り出す。Qt の内部用の CMake（Qt6BuildInternals）で小さな計画を作り、find_package に Core、CorePrivate、Gui、GuiPrivate、OpenGL、OpenGLPrivate を並べて add_subdirectory でプラグインを組む。相手の Qt は、LibrePaint の Windows の差分構築が使っている正式版 Qt の出力（qtbase-x86_64-w64-mingw32-6.11.3）で、コンパイラーも同じもの（gcc 15.3.0 の MinGW）。mcfgthread のヘッダーとライブラリー、Vulkan のヘッダー、Wintab のヘッダーの場所を Nix のコンパイラーの包みの環境変数で渡した。Qt の内部用の CMake の名前が、入っているプラグインと重ならないように、対象の名前を LibrePaintQWindowsPlugin に替えた。
- 結果：プラグイン 1 つ（56 の段）が何もない状態から 134 秒（8 並列）で作れた。正式版の Qt6Core.dll、Qt6Gui.dll はそのまま使う。
- 正式版の qwindows.dll を自作のもの（改変なし）に差し替えた LibrePaint は、Surface で起動し、合成入力の計測でホバー 11.0ms、インク 22.5ms（同じ時間帯の段 10 は 10.0〜15.0ms、20.8〜22.3ms）だった。
- Krita のフォークの改変 051139ed72（Wintab の対応範囲を lcSys から読む、QT_WINTAB_DESKTOP_RECT と QT_IGNORE_WINTAB_MAPPING）は、正式版 6.11.3 のソースにずれなく当たり、作り直しは 16 秒（変わったファイルの 3 つのコンパイルとリンク）。続けて 7bb46828ae（相対モード、つまりカーソルの位置への切り替えの無効化）も当たり（84 行のずれ）、作り直しは 12 秒。両方を当てたプラグインでも Surface で起動した。Wintab の経路での効き目は、実物のワコムのペンで確かめていない。
- 未確認：Drawpile が記録している「Qt 6 の Wintab のモードでメニューがペンの押下に時々しか反応しない」問題が、このプラグインの改変で直るかどうか。Krita のフォークのペンの改変のうち、プラグインの外（qguiapplication.cpp、qwidgetwindow.cpp）にあるものは、この方法では当てられない（LibrePaint 側で作り直し済み）。

## 出典

- Qt qtbase dev の qwindowstabletsupport.cpp：https://raw.githubusercontent.com/qt/qtbase/dev/src/plugins/platforms/windows/qwindowstabletsupport.cpp
- Qt qtbase v6.11.3 の同じファイル：https://raw.githubusercontent.com/qt/qtbase/v6.11.3/src/plugins/platforms/windows/qwindowstabletsupport.cpp
- KDE の T10838「Qt patches required by Krita」：https://phabricator.kde.org/T10838
- Krita の fbba406b（2019-03-20、Qt の Wintab と Windows Ink を Krita の改変版に切り替えた）：https://invent.kde.org/graphics/krita/-/commit/fbba406b15d7f6b066fd40d5bd28e5d92964bb13
- WinPenKit の調査（第三者）：https://github.com/TheSevenPens/WinPenKit/issues/132、https://github.com/TheSevenPens/WinPenKit/issues/129、https://github.com/TheSevenPens/WinPenKit/blob/claude/project-thread-xpv4jf/Docs/WINTAB-MAPPING-PRIOR-ART.md
- Krita の Qt の使い方の調査（第三者）：https://github.com/TheSevenPens/devnotes/issues/23
- Blender の不具合の記録：https://developer.blender.org/T84179、https://projects.blender.org/blender/blender/issues/84832
- 取得できなかった資料：ワコムの開発者向けの Wintab の頁（HTTP 403）、Blender の GHOST_WintabWin32.cc（HTTP 404）、Qt の変更 259161 の頁（中身が空）。
