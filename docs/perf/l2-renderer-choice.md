# L2（Windows の遅れを直す）の描画方式の選択

Issue #108 の L2（Windows の遅れを直す）で、Windows の描画方式を ANGLE（OpenGL ES を
Direct3D に変換する経路）から desktop の OpenGL に変えるかを決めるための資料である。
対象の指標は、Surface Pro 7 のホバー輪郭とインクの入力から表示まで（中央値と P95）である。
実装はこの資料では行わない。

## 1. 現在の LibrePaint の描画方式の選択

### 1.1 設定と既定

Windows の既定は ANGLE である。`krita/main.cc` の `main()` は `kritadisplayrc` の
`OpenGLRenderer` を読み、Windows では既定値を `"angle"`、それ以外の OS では `"auto"` にしている。
`KisOpenGL::convertConfigToOpenGLRenderer()` が `"desktop"`、`"angle"`、`"software"`、
`"none"`、`"auto"` を `KisOpenGL::OpenGLRenderer` に変換する。

L1（Windows のペン入力から表示までの約 50ms の遅れの原因を仮説ごとに数値で特定する）の
実験用に `KisL1Switches::openGLRenderer()` が環境変数 `LIBREPAINT_L1_OPENGL_RENDERER` を読み、
`"desktop"`、`"angle"`、`"auto"` で上の設定を上書きする。削除の条件は
`libs/global/KisL1Switches.h` に書いてある。

GUI の設定は「Display」ページの Renderer にあり、`KisOpenGL::getUserPreferredOpenGLRendererConfig()`
と `KisOpenGL::setUserPreferredOpenGLRendererConfig()` が `kritadisplayrc` の
`OpenGLRenderer` を読み書きする。現在の選択肢は Auto、OpenGL、OpenGL ES、ANGLE Direct3D
（Windows だけ）である。

### 1.2 起動時の判定

起動時の判定は `KisOpenGL::selectSurfaceConfig()` が行う。

1. `KisOpenGL::RendererAuto` の構成で `KisOpenGLModeProber::probeFormat()` を呼び、
   `QWindow` と `QOpenGLContext` を作って実際にコンテキストができるかを確かめる。
2. 失敗したら desktop の OpenGL を互換プロファイル付きで試し、次に互換プロファイルなしで試す。
3. 次に OpenGL ES を試す。
4. Windows では最後に WARP（Direct3D 11 のソフトウェアラスタライザー）を試す。
5. どれも失敗したら空の `KisOpenGL::RendererConfig` を返し、OpenGL を使わない経路になる。

この順で得た結果から `getRendererFromProbeResult()` が renderer を決める。OpenGL ES の
renderer 文字列が `"basic render driver"` または `"software"` を含む場合は
`RendererSoftware`、それ以外の OpenGL ES は `RendererOpenGLES`、OpenGL は
`RendererDesktopGL` である。

続いて `selectSurfaceConfig()` は DesktopGL、OpenGLES、Windows では Software の
3 つを `renderersToTest` に入れ、それぞれの構成を `probeFormat()` で試す。Windows では
`preferredAutoRenderer` を `RendererOpenGLES` に固定している。コードのコメントは
「On Windows we always prefer Angle, not what Qt suggests us」である。このため
`OpenGLRenderer=auto` でも ANGLE が選ばれる。

最終的な順序は `FormatPositionLess::operator()` が決める。順序は、ソフトウェアを最後に置くこと、
色空間の一致、HDR の好み、利用者の指定（`auto` 以外のとき）、ブラックリスト、HDR 用の
指定、Qt の好み（Windows では ANGLE）、色深度である。利用者の指定が `"angle"` のときは
利用者の指定がブラックリストより先に効くため、既定の Windows は ANGLE になる。

`isOpenGLRendererBlacklisted()` は Windows だけで動き、Intel の renderer 文字列に対して
ドライバーのビルド番号を見る。4637 から 4728 までと 4358、および解釈できない形式を
desktop の OpenGL のブラックリストに入れる。ANGLE 側は
`compareOp.setOpenGLESBlacklisted(false)` でブラックリストにしていない。

`KisOpenGL::setDefaultSurfaceConfig()` は、選んだ構成が OpenGL ES なら
`Qt::AA_UseOpenGLES` を立て、Windows では `QT_ANGLE_PLATFORM` を Direct3D 11 に設定する。
OpenGL なら `Qt::AA_UseDesktopOpenGL` を立てる。`KisOpenGL::initialize()` は Windows で
renderer 文字列に `"ANGLE"` を含むとき、`KRITA_UNLOCK_TEXTURE_BUFFERS` が未設定なら
テクスチャーバッファーを無効にする。ANGLE がストリームと動的バッファーを区別せず、
RAM、ステージング、ネイティブ、テクスチャーと余分なコピーをするためである。

### 1.3 失敗したときの戻し

起動時のコンテキスト作成に失敗した場合は、`selectSurfaceConfig()` の試行順が ANGLE や
WARP に戻す。desktop を明示していても、desktop の構成が失敗すれば、`renderersToTest` に
残った OpenGLES の構成が選ばれる。OpenGL がまったく使えない場合は
`KisOpenGL::hasOpenGL()` が偽になり、`KisCanvas2::createCanvas()` が
`KisQPainterCanvas` に切り替える。OpenGL キャンバスの初期化が `OPENGL_FAILED` を返した
場合も同じく QPainter に戻る。

Windows では、起動後の最初のフレームの検査（3.5）が失敗した renderer を
`kritadisplayrc` に記録し、次の起動で `selectSurfaceConfig()` がその renderer を飛ばす。
`OpenGLRenderer` に renderer の名前を明示したときはこの記録より優先するため、利用者は設定から戻せる。`"auto"` と未設定は記録に従う。
検査をすり抜けて起動後に描画が壊れる場合の自動の戻しはない。利用者が
`kritadisplayrc` の `OpenGLRenderer` を `"angle"`、`"software"`、`"none"` に変えるか、
GUI の設定を変えて再起動する。起動そのものができない場合は設定ファイルを直接編集する。

## 2. 出典で確かめたこと

### 2.1 Krita の Windows の既定と ANGLE を選ぶ理由

Krita の公式マニュアルは、Windows で Direct3D を使う ANGLE を選べること、ANGLE は
OpenGL の呼び出しを Direct3D に変換すること、native の OpenGL より遅い場合がある一方で
Windows の一般的なグラフィックスドライバーとの互換性が高いことを書いている
（https://docs.krita.org/en/reference_manual/preferences/display_settings.html）。

Krita の FAQ は、NVIDIA と最近の Intel の GPU が最も良い結果になり、AMD/ATI の GPU は
問題を起こしやすいと書いている。Windows では Krita 3.3 以降、Intel の GPU で ANGLE が
自動的に有効になる。NVIDIA のドライバーの不具合でウィンドウが半透明になるときは、
ANGLE に切り替えることで回避できる
（https://docs.krita.org/en/KritaFAQ.html）。

Krita のソース `libs/ui/opengl/kis_opengl.cpp` は、Windows で常に ANGLE を優先し、
`isOpenGLRendererBlacklisted()` で Intel の特定のドライバービルドだけ desktop の OpenGL を
避ける。この形は LibrePaint の現在のコードと同じである
（https://invent.kde.org/graphics/krita/-/blob/master/libs/ui/opengl/kis_opengl.cpp）。

### 2.2 ドライバー別の記録とブラックリスト

KDE の Phabricator T7411 は、Qt が持つブラックリストに追加する OpenGL のドライバーを
まとめている。Intel HD Graphics のドライバービルド 4636 と 4729 の間（両端を除く）で
ウィンドウの内容がずれて画面の外へ広がる問題、ビルド 4358 でキャンバスが更新されない問題を記録し、
ANGLE を Intel の既定にする変更を同じ場所で扱っている。ANGLE がノート PC を
オーバーヒートするほど負荷をかけた例も、この課題のコメントに残っている
（https://phabricator.kde.org/T7411）。

KDE bug 360601 は、Intel のドライバーによる Krita の OpenGL キャンバスの問題をまとめた
古い記録である（https://bugs.kde.org/show_bug.cgi?id=360601）。

AMD では、Windows 11 の Radeon RX 7600 と Radeon 780M で、ANGLE のキャンバスが
目に見えて滑らかでなく、desktop の OpenGL に変えると滑らかになったという報告がある。
この報告は未確認のまま残っている
（https://bugs.kde.org/show_bug.cgi?id=480440）。AMD のドライバーで OpenGL の関数を
コンテキストなしで呼ぶとクラッシュするという報告もある
（https://bugs.kde.org/show_bug.cgi?id=403086）。

NVIDIA では、Windows 8.1 にしたときに OpenGL のキャンバスが灰色のまま何も表示されない
問題が記録され、後に修正された（https://bugs.kde.org/show_bug.cgi?id=326417）。
現在の Krita の FAQ は、NVIDIA のドライバーの不具合でウィンドウが半透明になるときの
回避に ANGLE を使うと書いている。

### 2.3 Qt の QT_OPENGL と ANGLE の既知の問題

Qt 6 の Windows の文書は、`QT_OPENGL` と `Qt::AA_UseDesktopOpenGL`、
`Qt::AA_UseSoftwareOpenGL` の関係を説明している。Qt 6 では `Qt::AA_UseOpenGLES` は
効果がない。`QT_OPENGL_BUGLIST` の JSON は、vendor_id、device_id、driver_version などで
GPU とドライバーを照合し、`disable_desktopgl` で desktop の OpenGL を止めて
ANGLE やソフトウェアの経路に切り替える。組み込みの一覧は、不安定な desktop の
OpenGL を持つ古い GPU を `disable_desktopgl` で除外している
（https://doc.qt.io/qt-6/windows-graphics.html）。

Qt 6 は ANGLE を同梱しない。`Qt::AA_UseOpenGLES` と `QT_OPENGL=angle` は効果がなく、
動的な OpenGL の構成では OpenGL の初期化に失敗しても ANGLE への自動の退避はない。
`QOpenGLWidget` を使うアプリは native の OpenGL だけを使う。ソフトウェアの OpenGL
（Mesa llvmpipe の `opengl32sw.dll`）は使える
（https://doc.qt.io/qt-6/opengl-changes-qt6.html）。

Qt 5 の時代の文書は、ANGLE を選ぶ理由として、Windows の版と GPU が多様な利用者、
最近の GPU ドライバーを利用者にインストールさせたくないこと、Windows の OpenGL の
品質がドライバーによって低いことを挙げている。同時に、OpenGL ES にない OpenGL の
機能を使うアプリには desktop の OpenGL を勧めている
（https://wiki.qt.io/Qt_5_on_Windows_ANGLE_and_OpenGL）。

LibrePaint の現在のコードにも ANGLE の既知の問題がある。ANGLE ではテクスチャーバッファーを
無効にし、大きなブラシで余分なコピーが起きないようにしている。Qt 6 の正式版には ANGLE が
ないため、Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）では
この経路自体がなくなる。

### 2.4 他の Windows のペイントソフトの描画方式

Clip Studio Paint は Windows の動作要件に OpenGL 3.3 対応の GPU を挙げている。ANGLE や
Direct3D への変換を要件にしていない
（https://www.clipstudio.net/en/dl/system/）。

Paint.NET 5.1 は Direct2D と DXGI Flip Model、Async Present を使い、入力の遅れを
大きく減らしたと公式ブログに書いている。GPU の効果と調整は Direct2D の効果グラフで
実行する（https://blog.paint.net/2024/11/12/paint-net-5-1-is-now-available/）。

GIMP 3 は表示を GTK3 で行い、GPU を使う設定は Playground の OpenCL だけである。
OpenCL は特定の処理を GPU で実行する実験的な機能で、公式マニュアルはクラッシュが多いと
警告している。画面の表示経路を OpenGL で描く設定はない
（https://www.gimp.org/news/2025/03/16/gimp-3-0-released/、
https://docs.gimp.org/3.0/en/gimp-prefs-playground.html）。

MyPaint はタイルに保持した画素を Cairo の pixbuf に合成して表示する。画面の更新は
Cairo の経路で、OpenGL の描画経路を既定にしていない
（https://www.mypaint.app/en/docs/backend/canvas/）。

### 2.5 一般的水準として読めること

Windows のペイントソフトは、OpenGL を直接使うか、Direct2D や Direct3D を使うか、
CPU と Cairo を使うかに分かれる。ANGLE を使って OpenGL を Direct3D に変換するのは、
Krita のように古い OpenGL ドライバーを広く支える必要があるアプリの選択である。
Qt 6 は ANGLE をやめ、Windows では Direct3D を既定にした。desktop の OpenGL を
既定にする判断は、この一般的水準から外れない。ドライバーごとの失敗を起動時に検出し、
ANGLE またはソフトウェアに戻す仕組みを同じ変更に含めることが条件になる。

## 3. 案

### 3.1 上策: desktop を既定にし、起動時の検査と ANGLE への自動の戻しを持つ

Windows の `OpenGLRenderer` の既定を `"desktop"` にし、`selectSurfaceConfig()` の
`preferredAutoRenderer` も `RendererDesktopGL` にする。`probeFormat()` が desktop の
コンテキスト作成、バージョン、色空間、色深度を確かめ、失敗したら既存の
`renderersToTest` の順で ANGLE に戻す。

起動後の失敗に備えて、desktop で最初のフレームを描く検査を足す。シェーダーのコンパイル、
小さなオフスクリーンの描画、数画素の読み戻しを `initializeContext()` の後に行い、
失敗したら `kritadisplayrc` に「desktop は失敗した」という印を残して ANGLE で再起動する。
印は desktop が次の起動で成功したときに消す。`KisCanvas2::createCanvas()` の
QPainter への退避は最後の手段として残す。

遅れの効果は、L1 の同じ回の比較でホバー 35.4 → 13.6ms、インク 39.1 → 17.9ms である
（−21.8 / −21.2ms）。P95 はホバー 37.9 → 21.4ms、インク 41.7 → 25.6ms である。
これは H5（ホバーの間引きの解除）を含み、H1（先積みのフレーム数）=1 を含まない。
desktop の経路には D3D11 のデバイスがないため H1 は効かない。

壊れたときの見え方は、起動時の検査で捕まえれば一瞬だけ desktop を試して ANGLE の
ウィンドウが出る。検査をすり抜けた場合は、Intel の記録にあるウィンドウのずれや
キャンバスの更新停止、NVIDIA の記録にある半透明のウィンドウ、AMD の記録にある
キャンバスの停止やクラッシュが起こる。利用者は設定を `"angle"` に戻すか、
印によって自動で ANGLE に戻った状態で起動する。

コード量の見込みは 100〜200 行である。内訳は、既定と `preferredAutoRenderer` の変更が
約 5 行、最初のフレームの検査が 40〜80 行、失敗の印の読み書きと再起動が 30〜60 行、
契約テストが 30〜60 行である。既存の `selectSurfaceConfig()` の試行順と
`KisCanvas2::createCanvas()` の退避を使う。

Q4（フォークをやめ、全プラットフォームで正式版の同じ Qt にする）への依存は中程度である。
Q4 で ANGLE を外す場合、戻す先は Qt のソフトウェア OpenGL（`opengl32sw.dll` の
Mesa llvmpipe）を明示して選ぶか、QPainter になる。WARP は ANGLE の Direct3D 11 の
ソフトウェア経路なので、ANGLE を外すと使えない。Q4 でフォークを残す場合、戻す先は
ANGLE になる。Q4 の判断を先に決めると、戻す先のコードを 1 回で書ける。この案は
どちらの判断でも成立し、Q4 後の最終形に近い。

### 3.2 中策: GPU のベンダーとドライバーの版で選ぶ

`isOpenGLRendererBlacklisted()` を広げ、Intel、AMD、NVIDIA の既知の悪いドライバーでは
ANGLE を選び、それ以外では desktop を選ぶ。判定には `probeFormat()` の renderer 文字列と
ドライバーの版を使う。Qt の `QT_OPENGL_BUGLIST` と同じ形式の一覧を LibrePaint 側に持ち、
vendor_id、device_id、driver_version で照合する方法も取れる。

遅れの効果は、一覧にない GPU では上策と同じ −21.8 / −21.2ms である。一覧に入った
Intel、AMD、NVIDIA の GPU は ANGLE のままなので、ホバー 35.4ms、インク 39.1ms に
とどまる。AMD の未確認の報告のように、一覧にない新しいドライバーが ANGLE より
desktop で遅い、または desktop で壊れる場合、利用者はこの案の外側に残る。

壊れたときの見え方は上策と同じだが、既知のドライバーでは最初から ANGLE なので
その症状を避けられる。一覧の更新が遅れると、新しい GPU やドライバーで desktop の
症状が出る。ドライバーの版は Windows Update やベンダーの更新で変わるため、一覧の
保守が続く。

コード量の見込みは 150〜250 行である。内訳は、一覧の照合が 50〜100 行、判定と
`selectSurfaceConfig()` への接続が 30〜60 行、一覧のデータが 20〜50 行、契約テストが
50〜80 行である。一覧のデータはドライバーの更新のたびに足す。

Q4 への依存は大きい。Q4 で ANGLE を外すと、この案の分岐は意味を失う。Q4 を保留して
フォークを残す場合にだけ、この案は成立する。Q4 の判断が「フォークを残す」に決まるまで、
この案の実装は始めない。

### 3.3 下策: 設定で選べるだけにする

Windows の既定は ANGLE のままにし、GUI の Renderer と `kritadisplayrc` の
`OpenGLRenderer` で desktop を選べる状態を維持する。現在のコードは既にこの形である。
説明を Display ページに足す場合も、既定の動作は変えない。

遅れの効果は、既定の利用者では 0ms である。ホバー 35.4ms、インク 39.1ms のままである。
desktop を選んだ利用者だけがホバー 13.6ms、インク 17.9ms になる。

壊れたときの見え方は、既定の利用者には今の ANGLE の互換性が残る。desktop を選んだ
利用者には上策と同じ症状が出る。起動できない場合は `kritadisplayrc` を編集するか、
安全な設定で起動する手段が要る。

コード量の見込みは 0〜30 行である。設定と読み書きは既にあり、説明や警告を足す場合だけ
行数が増える。

Q4 への依存は大きい。Q4 で ANGLE を外すと、選べる ANGLE はなくなり、この案は
「desktop だけを使う」状態になる。Q4 を保留してフォークを残す場合、この案は現在の
動作を保つ。L2 の主指標を既定で改善しないため、CHARTER の優先順位の第 1 位を満たさない。

### 3.4 比較

| 項目 | 上策: desktop 既定と自動の戻し | 中策: ベンダーとドライバーで選ぶ | 下策: 設定で選ぶだけ |
| --- | --- | --- | --- |
| 既定のホバー / インク | 13.6 / 17.9ms | 一覧外は 13.6 / 17.9ms、一覧内は 35.4 / 39.1ms | 35.4 / 39.1ms |
| 既定の改善量 | −21.8 / −21.2ms | GPU によって 0 または −21.8 / −21.2ms | 0ms |
| 壊れたときの自動の戻し | 起動時と最初のフレームで ANGLE へ戻す | 既知の GPU は最初から ANGLE | なし |
| コード量の見込み | 100〜200 行 | 150〜250 行と一覧の保守 | 0〜30 行 |
| Q4 で ANGLE を外す場合 | 戻す先を Qt のソフトウェア OpenGL と QPainter に変えて成立する | 分岐が不要になり、実装しない | ANGLE を選べなくなる |
| Q4 でフォークを残す場合 | ANGLE へ戻す形で成立する | 成立する | 現在の動作を保つ |

### 3.5 上策の条件 2 と 7 の実装（2026-11-11）

Windows は、最初の OpenGL コンテキストの初期化の後に
`KisOpenGLStartupCheck::verifyFirstFrameOnCurrentContext()` を 1 回呼ぶ。現在の
コンテキストで小さな頂点・断片シェーダーをコンパイルしてリンクし、8×8 のオフスクリーンの
フレームバッファーに三角形を描き、中央の 1 画素を読み戻して期待する色（64,128,191,255、
許容 3 階調）と比べる。失敗した段（コンテキスト、シェーダー、リンク、フレームバッファー、
描画、読み戻し、画素）を理由として記録する。

失敗した renderer の名前（`OpenGLRenderer` と同じ `"desktop"`、`"angle"`、`"software"`）を
`kritadisplayrc` の `OpenGLFirstFrameFailedRenderers` に書き、次の起動の
`selectSurfaceConfig()` はその renderer を候補から外して次を選ぶ。すべての候補に印が
あるときは印を無視して全部を試す。ドライバーの更新で直る道を残し、QPainter は最後の手段に
とどめるためである。その renderer の検査が成功した起動では、印を消す。

`OpenGLRenderer` が `"desktop"`、`"angle"`、`"software"`、`"none"` のいずれかを明示したときと、
L1 の描画方式の切り替えがこれらの名前を指定したときだけ、印を無視する。これは条件 7 の
戻しを、印が残っていても効かせるためである。`"auto"` と未設定は明示とみなさず、印に従う。
設定の保存や以前の起動で `"auto"` が書かれた環境（Surface もこの状態）でも、自動の戻しが
働くようにするためである。`"angle"`、`"software"`、`"none"` のどれも、起動できない状態からの
復帰に使える。

Windows の既定は `"angle"` のままで、desktop を既定にする変更は Q4 の判断の後に分けて
行う。失敗を起こす隠れた変数 `LIBREPAINT_TEST_FAIL_OPENGL_FIRST_FRAME` は契約テスト
（`KisOpenGLStartupCheckContractTest`）だけのための入口で、Intel・AMD・NVIDIA の実機で
検査を確かめた後に消す。削除の条件は `KisOpenGLStartupCheck.h` に書いてある。

選択と検査の結果は、標準エラーに `OpenGL renderer selection: chosen=… candidates=… skipped=…`
と `OpenGL first frame check: renderer=… result=…` の1行ずつを出し、`LIBREPAINT_LATENCY_LOG`
の `probe.csv` にも `l1_renderer_selection`、`l1_renderer_skipped`、`l1_first_frame_check`
として残す。`selectSurfaceConfig()` は `KisUsageLogger` の初期化より前に走るため、sysinfo の
ログだけでは計測の起動に残らず、警告の UI も計測では表示されないためである。

## 4. L2 で desktop を既定にする条件

L2 で desktop を既定にする判断は、次の条件をすべて満たすときに行う。

1. 起動時の `probeFormat()` が desktop のコンテキストを作れない場合に、ANGLE（フォークを
   残す場合）または Qt のソフトウェア OpenGL と QPainter（Q4 で ANGLE を外す場合）へ
   自動で戻る。
2. 起動後の最初のフレームで、シェーダーのコンパイル、小さな描画、読み戻しを確かめ、
   失敗したら次の起動で ANGLE に戻る印を残す。
3. Intel、AMD、NVIDIA の実機で、起動、ホバー、インク、押下、ウィンドウの移動と拡大、
   ダイアログの表示を確認する。Surface Pro 7 の Intel だけでなく、AMD と NVIDIA の
   Windows 機を少なくとも 1 台ずつ含める。
4. 表示の正しさを 100% と 400% で確認する。現在の desktop の確認は色とグレーの差 0 階調、
   1px の線、遷移 1px で合格している。HDR と 10bit を残す場合は、desktop の経路で
   その表示を確認する。HDR を残さない場合は、その判断を DECISIONS.md に書く。
5. タブレットの入力が ANGLE のときと同じであることを確認する。Wintab と Windows Ink の
   押下、ホバー、Enter と Leave、タッチを、同じ合成入力と実機のペンで比べる。
6. CPU と電力が ANGLE の H1（先積みのフレーム数）=1 と H5（ホバーの間引き）の組より
   10% を超えて悪化しないことを確認する。CHARTER は取りこぼし率、CPU、電力を
   悪化させないことを求めている。
7. 利用者が `kritadisplayrc` の `OpenGLRenderer` を `"angle"`、`"software"`、`"none"` に
   変えて戻せることを、起動できない状態でも確かめる。
8. 同じセッションで desktop と ANGLE を交互に各 5 回測り、回ごとの中央値のばらつきが
   5% 未満であることを確かめる。目標は Surface のホバー中央値 25ms 前後、P95 35ms 以下で、
   インクを悪化させないことである。

上策は 1、2、7 を同じ変更に含められるため、この条件を満たしやすい。中策は 1 を
既知のドライバーの範囲でしか満たさず、一覧の外の失敗を残す。下策は 1 と 2 を持たず、
既定の改善もしない。

条件を満たすまで desktop を既定にしない。条件を満たした時点で上策を採り、Q4 の判断を
先に決めて戻す先を 1 つに定める。Q4 でフォークを残す判断が出て、かつ実機の確認に
時間がかかる場合は、中策を一時的な代替にする。下策は現在の動作の記録として残し、
L2 の既定にはしない。

Q4 の判断は、条件 1 の戻す先を決める。Q4 でフォークをやめるときは、ANGLE への戻しを
Qt のソフトウェア OpenGL と QPainter に置き換える。Q4 でフォークを残すときは、ANGLE への
戻しをそのまま使う。Q4 の判断を先に決めると、同じ戻しの仕組みを 2 回書かずに済む。

## 5. 出典

- Krita Manual「Display Settings」: https://docs.krita.org/en/reference_manual/preferences/display_settings.html
- Krita FAQ「What Graphics Cards does Krita support?」「Windows: Krita's window is semi-transparent」: https://docs.krita.org/en/KritaFAQ.html
- KDE Phabricator T7411「Additional OpenGL graphics drivers to be blacklisted for Krita」: https://phabricator.kde.org/T7411
- KDE bug 360601「[INTEL] Various issues caused by buggy Intel drivers.」: https://bugs.kde.org/show_bug.cgi?id=360601
- KDE bug 351149「[AMD] canvas freezes on brush tool use」: https://bugs.kde.org/show_bug.cgi?id=351149
- KDE bug 403086「OpenGL Function call without setting up proper context results in crash on (some?) AMD drivers」: https://bugs.kde.org/show_bug.cgi?id=403086
- KDE bug 480440「[Windows] Krita should use OpenGL as the default renderer for AMD GPUs」: https://bugs.kde.org/show_bug.cgi?id=480440
- KDE bug 326417「OpenGL canvas doesn't work with the latest NVidia drivers.」: https://bugs.kde.org/show_bug.cgi?id=326417
- Qt 6「Qt for Windows - Graphics Acceleration」: https://doc.qt.io/qt-6/windows-graphics.html
- Qt 6「Changes to Qt OpenGL」: https://doc.qt.io/qt-6/opengl-changes-qt6.html
- Qt Wiki「Qt 5 on Windows ANGLE and OpenGL」: https://wiki.qt.io/Qt_5_on_Windows_ANGLE_and_OpenGL
- Krita の `libs/ui/opengl/kis_opengl.cpp`: https://invent.kde.org/graphics/krita/-/blob/master/libs/ui/opengl/kis_opengl.cpp
- Clip Studio Paint「System Requirements」: https://www.clipstudio.net/en/dl/system/
- Paint.NET Blog「Paint.NET 5.1 is now available!」: https://blog.paint.net/2024/11/12/paint-net-5-1-is-now-available/
- GIMP「GIMP 3.0 Released」: https://www.gimp.org/news/2025/03/16/gimp-3-0-released/
- GIMP Documentation「6.6. Experimental Playground」: https://docs.gimp.org/3.0/en/gimp-prefs-playground.html
- MyPaint Documentation「Canvas」: https://www.mypaint.app/en/docs/backend/canvas/
