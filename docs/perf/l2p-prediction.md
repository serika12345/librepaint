# L2P（予測の線）の調査と設計

Issue #108 の L2P（予測の線）の設計資料。L2P は、L2（Windows の遅れを直す）の後に置き、L3（Linux・Mac・iPad の残りの差の内訳）の前に置く段階である。ペン先と表示された線の端の間を、入力の予測で短く描き足す。予測は表示だけの層に描き、画素の正本（CPU の `KisPaintDevice`）には書かない。CHARTER に予測の線の条文が入るまで、この文書は設計と計測の方法だけを定め、実装は行わない。

## 目的と範囲

対象の指標は、インクの入力から表示まで（中央値と P95）と、ペン先と表示された線の端の距離（px と ms 換算）である。予測の線は、遅れそのものを縮めない。遅れの間にペンが動いた距離を、表示の上で先に埋める。したがって、予測が当たっている間は線の端がペン先に近づき、外れたときは一時的にペン先より先へ出る。

この文書は次の 4 つを扱う。

1. 他の描画ソフトと OS の予測の方式を、確認できる出典つきで比べる。
2. LibrePaint で予測の線を描ける場所を、ファイルと関数名で示す。
3. 予測の線の距離と外れを、既存の取り込みで測る指標を定める。
4. 予測の長さ、置き換え、筆圧と傾き、外れたときの見え方の設計案を定める。

## 他のソフトと OS の方式

### 方式の表

予測の長さは、文書に ms の記載があるものだけを ms で書く。記載がないものは「不明」と書く。予測しないソフトは 0ms と書く。

| 方式 | 予測の長さ | 確定の線との置き換え | 筆圧・傾き | 外れたときの見え方 | 出典 |
| --- | --- | --- | --- | --- | --- |
| Windows Ink の `InkPresenter`（DirectInk） | 既定 15ms。`InkModelerAttributes.PredictionTime` で 8ms などに変更できる | 低遅延の背景スレッドで wet の線を描き、ストロークが完了すると UI スレッドで dry の線にして wet を置き換える。予測点だけをいつ捨てるかは文書に記載がない（不明） | 筆圧、ペン先の形状、回転は描画属性で扱う。予測部分の筆圧と傾きの作り方は不明。`UseVelocityBasedPressure` は速度と筆圧を組み合わせる | 予測が外れると補正の artifact が出る。予測時間を短くすると artifact を減らせる | `InkPresenter`（https://learn.microsoft.com/en-us/uwp/api/windows.ui.input.inking.inkpresenter）、`InkModelerAttributes`（https://learn.microsoft.com/en-us/uwp/api/windows.ui.input.inking.inkmodelerattributes）、ペン入力の説明（https://learn.microsoft.com/en-us/windows/uwp/ui-input/pen-and-stylus-interactions） |
| Windows の `DelegatedInkTrailVisual` | アプリが予測点を渡す。OS が決める長さはない（不明） | 実際の点の後ろに予測点を追加する。次の `AddTrailPoints` か `AddTrailPointsWithPrediction` が、追加する前に予測点を全部消す。実際の位置が届くと予測点を上書きする | `InkTrailPoint` は位置と半径だけを扱う。筆圧と傾きは対象外 | 次の呼び出しで予測点が消える。外れた予測がどう見えるかはアプリが描く線による | `AddTrailPointsWithPrediction`（https://learn.microsoft.com/en-us/uwp/api/windows.ui.composition.delegatedinktrailvisual.addtrailpointswithprediction） |
| Android の `MotionEventPredictor`（androidx.input） | 最後のイベントからの経過時間に、画面の最速フレーム時間を足した値。上限は 32ms。Android U 以降はシステムの予測 API、それ以前は Kalman filter のライブラリを使う | 予測した `MotionEvent` を返す。新しい予測点を追加するときに古い予測点を消す。予測点を最終の描画に使わない | 予測イベントは x、y、pressure、orientation、tilt を含む。筆圧は Kalman filter で外挿し、orientation と tilt は最後の値を使う | 予測は一時的な点として消える。公式の文書は見え方の形を定めていない（不明） | `MotionEventPredictor`（https://developer.android.com/reference/androidx/input/motionprediction/MotionEventPredictor）、`PredictionEstimator`（https://android.googlesource.com/platform/frameworks/support/+/androidx-main/input/input-motionprediction/src/main/java/androidx/input/motionprediction/common/PredictionEstimator.java）、`SinglePointerPredictor`（https://android.googlesource.com/platform/frameworks/support/+/androidx-main/input/input-motionprediction/src/main/java/androidx/input/motionprediction/kalman/SinglePointerPredictor.java）、高度なスタイラス機能（https://developer.android.com/develop/ui/views/touch-and-input/stylus-input/advanced-stylus-features） |
| iPadOS の `UIEvent.predictedTouches(for:)` と PencilKit | 文書は「次に起こるとシステムが予測するタッチ」とだけ説明する。ms の記載はない（不明）。PencilKit の内部の予測の長さも不明 | 予測タッチを一時的に描画へ追加し、新しいイベントが届いたら捨てる。PencilKit の内部の置き換えは不明 | 予測タッチの筆圧と傾きの作り方は不明 | 次のイベントで捨てる。外れた予測の見え方は文書に記載がない（不明） | `UIEvent.predictedTouches(for:)`（https://developer.apple.com/documentation/uikit/uievent/predictedtouches(for:)）、`PKCanvasView`（https://developer.apple.com/documentation/pencilkit/pkcanvasview） |
| Chrome の `PointerEvent.getPredictedEvents()` | 最大 3 点。点の間隔は実測したイベント間隔（既定 8ms、最小 2.5ms）で、最大 25ms 先まで。予測器は Kalman が既定で、1 次線形、2 次線形、最小二乗も選べる。リサンプリングは最大 20ms | 仕様の例は、前回の予測点を消し、coalesced の実点を描いてから新しい予測点を描く。予測は次の pointer event が届くまで有効である | 予測イベントは実イベントの複製で、位置と時刻だけを更新する。筆圧、傾き、twist は最後の実値のまま | 次の実イベントで予測点が消える。外れた分は一時的に前へ出てから戻る | Pointer Events の Predicted events（https://w3c.github.io/pointerevents/#predicted-events）、Chrome の機能ページ（https://chromestatus.com/feature/5765569655603200）、`input_predictor.h`（https://chromium.googlesource.com/chromium/src/+/main/ui/base/prediction/input_predictor.h）、`input_event_prediction.h`（https://chromium.googlesource.com/chromium/src/+/main/third_party/blink/renderer/platform/widget/input/input_event_prediction.h） |
| Krita | 予測しない（0ms）。`Smoothing` と `Stabilizer` は入力を平均し、線がカーソルの後ろに残る | 予測の線がないため置き換えはない。確定の線だけを描く | ブラシエンジンが筆圧と傾きを扱う。`Stabilize sensors` は筆圧、速度、傾きを平滑化する | 予測の外れはない。遅れは線がカーソルの後ろに残る形で見える | Freehand Brush Tool の Smoothing（https://docs.krita.org/en/reference_manual/tools/freehand_brush.html） |
| Clip Studio Paint | 公式マニュアルに予測の記述がない（0ms）。`Stabilization`、`Post correction`、`stroke preview` を持つ | 予測の線がないため置き換えはない。補正後の線を描く | ブラシ設定と `Stabilization` が筆圧と傾きを扱う。予測部分はない | 予測の外れはない。補正の強さによって線が遅れる | Correction の設定（https://help.clip-studio.com/en-us/manual_en/810_subtools/C.htm）、Preferences の表示設定（https://help.clip-studio.com/en-us/manual_en/720_preferences/Preferences.htm） |
| Procreate | 公式ハンドブックに予測の記述がない（0ms）。`Stabilization` は移動平均、`Motion Filtering` と `StreamLine` は線の揺れを抑える | 予測の線がないため置き換えはない。平滑化した線を描く | `Pressure and Smoothing` と `StreamLine` の `Pressure` が筆圧を扱う。予測部分はない | 予測の外れはない。平滑化の強さによって線が遅れる | Brush Studio Settings の Stabilization、StreamLine、Motion Filtering（https://help.procreate.com/procreate/handbook/brushes/brush-studio-settings）、Apple Pencil, Pressure and Smoothing（https://help.procreate.com/articles/tIdR13-apple-pencil-pressure-and-smoothing） |

### 表から読み取れること

Windows Ink と Chrome は、予測の長さを「表示までの残りの時間」に合わせて 15〜25ms 程度に置く。Android は最後のイベントからの経過時間を足すため、間隔が空いたときだけ長くなり、上限 32ms で切る。iPadOS は ms を公開しないが、次に届くタッチを一時的に足す形は同じである。

置き換えの形は 3 つに分かれる。

1. 予測点を実際の点で上書きする（Windows の wet ink、DelegatedInkTrail、Chrome、Android、iPadOS）。
2. 予測点を次の入力のたびに全部消してから描き直す（Chrome、DelegatedInkTrail、Android の「新しい予測点を追加するときに古い予測点を消す」）。
3. 予測を使わず、平滑化だけで遅れを受け入れる（Krita、Clip Studio Paint、Procreate）。

筆圧と傾きは、位置の予測ほど共通した扱いがない。Android は筆圧を Kalman filter で外挿し、傾きと orientation は最後の値を使う。Chrome は位置と時刻だけを更新し、筆圧と傾きは最後の実値のままにする。Windows Ink は予測部分の筆圧と傾きの作り方を公開していない。LibrePaint は Chrome と Android に合わせ、位置だけを予測し、筆圧と傾きは最後の実値を使うのが安全である。

## LibrePaint で予測の線を描く場所

### 入力と予測の入口

入力の位置、筆圧、傾き、時刻は、ツールのポインターイベントにそろっている。

- `libs/tools/kis_tool_paint_interaction.cpp` の `KisToolPaintInteraction::mouseMoveEvent`、`mousePressEvent`、`mouseReleaseEvent` が、ホバーと描画のポインターイベントを受ける。`KisToolPaintInteraction::getOutlinePath` が、その位置からブラシの輪郭を作る。
- `libs/ui/tool/kis_tool_freehand.cc` の `KisToolFreehand::beginPrimaryAction`、`continuePrimaryAction`、`endPrimaryAction` が、フリーハンドのストロークの開始、継続、終了を受ける。`continuePrimaryAction` は `requestUpdateOutline` を呼んでから `doStroke` を呼ぶ。
- `libs/ui/tool/kis_tool_paint.cc` の `KisToolPaint::requestUpdateOutline` が、`m_outlineDocPoint` と `m_currentOutline` を更新し、`KisCanvas2::updateCanvasToolOutlineDoc` で表示の更新を要求する。
- `libs/flake/KoPointerEvent.h` の `KoPointerEvent::pressure`、`xTilt`、`yTilt`、`rotation`、`tangentialPressure`、`timestamp` が、予測器へ渡す値になる。

予測器はこの層で動かす。予測器は最後の数点の位置と時刻だけを使い、`KisPaintInformation` や `KisPaintDevice` には触れない。

### ホバー輪郭の描画

ホバーの輪郭は、予測の線とは別の層で描かれている。予測の線を輪郭の描画に混ぜると、輪郭の更新の経路と予測の更新の経路が 1 つになり、どちらの遅れか分からなくなる。そのため、この経路は予測の線には使わない。場所の確認だけをここに残す。

- `libs/ui/tool/kis_tool_paint.cc` の `KisToolPaint::paint` が、`tryFixBrushOutline` を通した `KisOptimizedBrushOutline` を `paintToolOutline` へ渡す。
- `libs/ui/canvas/kis_canvas_tool_support.cpp` の `KisCanvas2::drawToolOutline` が、OpenGL キャンバスでは `KisOpenGLCanvas2::paintToolOutline` を呼び、それ以外では `QPainter` の XOR で輪郭を描く。
- `libs/ui/opengl/kis_opengl_canvas2.cpp` の `KisOpenGLCanvas2::paintToolOutline` が、`KisOpenGLCanvasRenderer::paintToolOutline` へ渡す。
- `libs/ui/opengl/KisOpenGLCanvasRenderer.cpp` の `KisOpenGLCanvasRenderer::paintToolOutline` が、`solidColorShader`、`outlineVAO`、`lineVertexBuffer` を使って、輪郭を GPU で描く。`glScissor` で更新の矩形に切る。

この経路は、線の太さと色だけを持つ単純な線を GPU で描くのに向いている。予測の線をブラシの模様まで含めて描くには足りない。

### キャンバスの装飾の層

キャンバスの装飾は、画素の正本とは別の層で、ツールの輪郭より先に描かれる。予測の線はここに置く。

- `libs/ui/canvas/kis_canvas_decoration.h` の `KisCanvasDecoration` が、`paint` と `drawDecoration` を持つ。`drawDecoration` はウィジェット座標の `QPainter` を受け取る。
- `libs/ui/canvas/kis_canvas_widget_base.cpp` の `KisCanvasWidgetBase::drawDecorations` が、シェイプ、登録された装飾、ツールの順に描く。装飾は `m_d->decorations` を優先度の順に回し、`deco->paint` を呼ぶ。
- `libs/ui/canvas/kis_canvas2.cpp` の `KisCanvas2::addDecoration` が装飾を加え、`KisCanvas2::updateCanvasDecorations` と `KisCanvas2::updateCanvasDecorations(const QRectF &)` が、装飾の更新を要求する。
- `libs/ui/canvas/KisDecorationsManager.cpp` の `KisDecorationsManager` が、助手、参照画像、グリッドなどの装飾の表示を管理する。予測の装飾は、この管理に加えず、フリーハンドツールの有効な間だけ `KisCanvas2` が持つ。
- `libs/ui/opengl/kis_opengl_canvas2.cpp` の `KisOpenGLCanvas2::paintGL` が、`renderer->paintCanvasOnly` の後に `drawDecorations` を呼ぶ。`libs/ui/canvas/kis_qpainter_canvas.cpp` の `KisQPainterCanvas::paintEvent` も `drawDecorations` を呼ぶ。OpenGL と QPainter のどちらのキャンバスでも、同じ装飾の経路を通る。

装飾は画素の正本を読まない。予測の折れ線をウィジェット座標に変換して `QPainter` で描くだけなら、`KisPaintDevice`、`KisPainter`、`KisStrokeStrategy` のどれも通らない。これが「表示だけの層」の中心になる。

### ストロークの確定の流れ

確定の線は、予測の線とは別の経路を通る。予測の点をこの経路へ入れると、画素の正本に予測が残る。そのため、予測の点はこの経路へ入れない。

- `libs/ui/tool/kis_tool_freehand_helper.cpp` の `KisToolFreehandHelper::initPaint`、`paintEvent`、`endPaint` が、入力の点をストロークのジョブにする。
- `libs/painting/strokes/freehand_stroke.cpp` の `FreehandStrokeStrategy::doStrokeCallback` が、`KisMaskedFreehandStrokePainter::paintLine` を呼ぶ。
- `libs/painting/strokes/KisMaskedFreehandStrokePainter.cpp` の `KisMaskedFreehandStrokePainter::paintLine` が、`KisPainter::paintLine` を呼ぶ。
- `libs/image/kis_painter.cc` の `KisPainter::paintLine` が、ブラシの dabs を `KisPaintDevice` に描く。ここが画素の正本への書き込みである。

予測の線は、`KisToolFreehandHelper::paintEvent` の前で止め、`FreehandStrokeStrategy` へ渡さない。確定の線が予測の点に追いついたときは、予測の装飾を消すだけで、確定の線はそのまま残る。

## 設計案

### 予測器

予測器は、フリーハンドの入力の最後の 5〜8 点を、位置と時刻の組で持つ。最初の実装は 1 次線形にする。最後の 2 点の速度を求め、予測の時刻までの位置を `p = p_last + v * (t_pred - t_last)` で出す。2 次線形と Kalman は、速さが変わった直後の誤差が 1 次線形で足りないと計測が示したときだけ加える。Chrome と Android が Kalman を既定にしていることは、 Kalman が必ず必要という意味ではない。LibrePaint の入力はタブレットのサンプルが 2〜4ms 間隔で届くため、1 次線形でも安定した速度を得やすい。

予測器をリセットする条件を次にそろえる。

- 押下の開始と終了。
- 最後の点から 20ms 以上空いたとき。Chrome の `kMaxTimeDelta` と Android の `EVENT_TIME_IGNORED_THRESHOLD_MS` が 20ms である。
- ツール、プリセット、ブラシの大きさ、色、キャンバスのズームや回転が変わったとき。
- ポインターがキャンバスの外へ出たとき。
- 予測の長さが 8ms 未満になったとき。

### 予測の長さ

予測の長さは、予測の点が表示される時刻に、実際のペン先がそこへ着くように選ぶ。初期値は表示の 1 周期（60Hz で 16.7ms）とし、8ms から 25ms の範囲に切る。25ms の上限は Chrome の `kMaxPredictionTime` に合わせる。Android の 32ms は、最後のイベントからの経過時間を足すための上限であり、LibrePaint のように入力が 2〜4ms 間隔で届く環境では長すぎる。

予測の長さは、L2 の後に残る遅れで決める。2026-11-10 の計測では、インクの遅れは 1.0.3 が 55〜61ms、H1（先積みのフレーム数）=1 と H5（ホバーの間引き）の組が 39ms、desktop の H5 が 17.6ms だった。16.7ms の予測は、desktop の 17.6ms のほとんどを埋める。39ms の経路では 22ms が残り、25ms の上限でも 14ms が残る。1.0.3 の 60ms では 43ms が残る。L2P を L2 の後に置く理由はここにある。

速さが変わった直後は、一定の速さのときより遅れが増える。2026-11-10 の緩急の計測では、desktop の一定の区間が 13.0〜13.6ms、速さが変わった直後が 27.6〜28.1ms だった。16.7ms の予測は一定の区間をほぼ埋めるが、変わった直後の 11〜12ms は残る。予測器を Kalman にしても、この過渡の誤差は消えない。速さの変化を予測で隠さず、計測の `speed_change` の区間で別に報告する。

### 置き換えと外れたときの見え方

予測の線は、最後の実点から予測の点までを結ぶ 1 本の折れ線として描く。次の実点が届いたら、前の予測の線を消し、新しい実点から新しい予測の線を描く。押下の終了、ポインターの離脱、20ms 以上の空白では、予測の線をすぐに消す。確定の線は消さない。

外れたときの見え方は、次の 3 つに分けて扱う。

1. 予測が短いときは、線の端がペン先より後ろに残る。確定の線が追いつくにつれて縮む。
2. 予測が長いときは、線の端がペン先より先へ出る。次の実点で消えるため、1 表示周期だけ戻るように見える。
3. 速さが急に落ちたときや止まったときは、前の速度の予測が残ると、ペン先より先へ大きく出る。停止の直前の速度が 5.4px/ms なら、16.7ms の予測は 90px 先へ出る。このため、速さが 0.2px/ms 未満になったとき、筆圧が 0.05 未満になったとき、最後の点から 20ms 以上空いたときは予測を出さない。予測の長さに、速さと揺れから作る信頼度（0〜1）を掛ける。Chrome と Android の confidence factor と同じ考えである。

予測の線の色と太さは、確定の線と同じにする。最初の実装は、現在のブラシの色と大きさで、丸い端と丸い結合の折れ線を描く。模様のあるブラシでは、確定の線と見た目が変わる。模様の差が許容できないと計測で分かったときだけ、予測の区間だけの小さなオフスクリーンの `KisPaintDevice` に同じ paint op で描き、装飾として重ねる。その場合も正本の `KisPaintDevice` には書かない。

### 筆圧と傾き

予測の点の筆圧と傾きは、最後の実値を使う。位置だけを予測し、筆圧、傾き、回転、tangential pressure は最後の値で固定する。Chrome が予測イベントの位置と時刻だけを更新し、Android が傾きと orientation を最後の値にする扱いに合わせる。筆圧を外挿すると、止まる直前に筆圧が 0 へ向かう偽の細りが出やすい。筆圧の外挿は、計測で線の太さの差が問題になったときだけ加える。

### 描画の層と更新

新しい装飾を `libs/ui/canvas` に置く。名前は `KisPredictionDecoration` とする。予測の状態は `libs/tools` の `KisStrokePrediction` が持ち、装飾はその写しを描く。どちらも GUI スレッドで動く。

1. `KisToolPaintInteraction` のポインターイベントで、`KisStrokePrediction::update` を呼ぶ。
2. `KisStrokePrediction::predict` が、最後の実点と予測の点を文書座標で返す。
3. `KisToolPaintInteraction` が、古い予測の矩形と新しい予測の矩形を合わせ、ブラシの半径の分だけ広げて `KisCanvas2::updateCanvasDecorations` に渡す。
4. `KisCanvasWidgetBase::drawDecorations` が `KisPredictionDecoration::drawDecoration` を呼び、`KisCoordinatesConverter::documentToWidgetTransform` でウィジェット座標に変換して描く。
5. 次の実点、押下の終了、離脱で `KisStrokePrediction::clear` を呼び、古い矩形を更新する。

OpenGL キャンバスでは、装飾の `QPainter` は `KisOpenGLCanvas2::paintGL` の中の `drawDecorations` で動く。予測の線が 1 本だけなら、この経路の費用は小さい。計測で `paintgl_begin` から `paintgl_end` の時間が増えた場合だけ、`KisOpenGLCanvasRenderer::paintToolOutline` と同じ `solidColorShader` と VBO を使う GPU の経路へ移す。その場合も画素の正本は触らない。

### 無効化と削除の条件

予測の線は、CHARTER に条文が入るまで実装しない。実装するときは、隠し設定 `useStrokePrediction`（既定 `false`）と `strokePredictionHorizonMs`（既定 16.7、8〜25 に切る）を kritarc に置く。`useStrokePrediction=false` のときは、予測器も装飾も作らない。予測の線が主指標を悪化させた場合、または表示の正しさの確認で確定の線と区別できない差が出た場合は、既定を `false` のままにする。

## 計測の方法

### 既存の取り込み

計測は `scripts/perf/run.py` の既存の取り込みを使う。画面の取り込みは `capture.csv` にフレームの時刻と、バンドごとの `xmin`、`xmax`、`dense_xmax`、`arrival_ns` を書く。`scripts/perf/pen-latency/capture_analyze.py` の `frame_samples` が、`inject.csv` の入力の時刻と `capture.csv` の線の位置を対応づける。`analyze_run` が `ink.input_to_capture`、`ink.pen_tip_to_edge_px`、`ink.speed_groups`、`ink.speed_change` を出す。`scripts/perf/pen-latency/trajectory.py` の `speed_profiles` が、1 本の線の中で速さを変える軌跡を作る。

予測の線は確定の線と同じ色で描くため、取り込みの `dense_xmax` は予測の線の端を写す。停止のあいだは予測の線を消すので、`dwell_offsets` の校正（線の端とポインターの位置の差）は予測なしの回と同じに保てる。この性質を、予測の線の計測の前提にする。

### 指標

予測の線の効果は、既存の `ink.pen_tip_to_edge_px` と、新しく足す符号付きの距離で測る。符号付きの距離は、フレームの時刻のペン先の位置と、そのフレームに写った線の端の位置の差である。線の端は、校正のオフセットを引いてポインターの座標に直す。

- `l2p.signed_gap_px` = `direction * ((dense_xmax - offset) - pen_x(frame_time))`。
  - 正：線の端がペン先より後ろにある（遅れが残る）。
  - 負：線の端がペン先より先にある（予測が外れて出る）。
- `l2p.signed_gap_ms` = `l2p.signed_gap_px / speed_px_ms`。速さは、フレームの時刻を挟む 2 つの注入サンプルから求める。速さが 0.2px/ms 未満のフレームは、ms に直さずに px だけで報告する。
- `l2p.residual_px` = `max(0, l2p.signed_gap_px)`。予測が足りない量。
- `l2p.overshoot_px` = `max(0, -l2p.signed_gap_px)`。予測が先へ出た量。
- `l2p.hit_rate` = `|l2p.signed_gap_px|` がブラシの半径以下のフレームの割合。
- `l2p.settle_ms` = 線の終わりから、`prediction_clear` の印、または `l2p.overshoot_px` が 0 になった最初のフレームまでの時間。
- `l2p.speed_change_overshoot_px` と `l2p.speed_change_residual_px` = 既存の `ink.speed_change` の 80ms の窓での最大値。一定の区間は 250ms 以降の窓で別に出す。

`capture_analyze.py` では、`frame_samples` がフレームごとのペン先の位置と速さも返すようにし、`analyze_run` が上の指標を `stats` に足す。`ink_speed_groups` と `speed_change_entries` は、`l2p.*` の値を速さの群と変化の窓へ足す。`Pass.locate` は、予測で線の端が入力の範囲の外へ出たときに `None` を返すため、符号付きの距離は `locate` を通さずに計算する。既存の `ink.input_to_capture` は確定の線の比較に残し、予測の回では符号付きの距離を主にする。

予測器の内部の値を確かめるために、`probe.csv` へ次の印を足す。印は `KisLatencyProbe::mark` を使う。

- `prediction_update`：実点を予測器へ入れた時刻と位置。
- `prediction_point`：予測の点の位置、予測の長さ、信頼度。
- `prediction_clear`：予測の線を消した時刻と理由（実点、終了、離脱、空白、停止）。
- `prediction_paint`：装飾が予測の線を描いた時刻と、描いた矩形。`prediction_paint` と `paintgl_end` の差が、予測の線の描画の費用になる。

### variable-speed の見込み

既存の `scripts/perf/scenarios/pen-light-speed-varied.json` は、前半が 0.3 から 3.0、後半が 3.0 から 0.2 へ変わる 6 本の線を持つ。速さは Surface で約 0.9、2.7、5.4px/ms に対応する。予測の線の見込みは、次の式で出す。

`残る距離 = (遅れ - 予測の長さ) * 速さ`

2026-11-10 の desktop の H5 の値（遅れ 17.6ms、速さ 0.9、2.7、5.4px/ms）に、16.7ms の予測を当てる。

| 速さ | 予測なしの距離 | 16.7ms の予測の後の残り | 予測が先へ出る最大（停止の直前） |
| --- | --- | --- | --- |
| 0.9px/ms | 16px | 約 1px | 約 15px |
| 2.7px/ms | 47px | 約 2px | 約 45px |
| 5.4px/ms | 90px | 約 5px | 約 90px |

H1（先積みのフレーム数）=1 と H5（ホバーの間引き）の組（遅れ 39ms）では、16.7ms の予測の後の残りは 0.9、2.7、5.4px/ms で約 20、60、120px になる。desktop の 5.4px/ms で 5px 以下を狙うには、予測の長さを 16.7ms より長くするか、L2 の遅れをさらに縮める必要がある。1.0.3 の 60ms では、16.7ms の予測の後の残りが約 39〜235px で、予測だけでは足りない。

速さが変わった直後は、desktop の遅れが 27.6〜28.1ms に増える。16.7ms の予測の後の残りは約 11〜12ms で、5.4px/ms では約 60〜65px になる。この区間は `l2p.speed_change_residual_px` で別に報告し、一定の区間の値と混ぜない。

### 手順と合格の目安

1. 予測なしと予測ありの 2 対象を、同じ回に交互に各 5 回測る。予測の有無だけを変え、版、窓、kritarc、軌跡、表示をそろえる。
2. 軌跡は `pen-light-speed-0p5.json`、`pen-light-speed-1p5.json`、`pen-light-speed-3p0.json`、`pen-light-speed-varied.json` の 4 つを使う。速さの違う線を同じ回で測るため、`speed-varied` を主にし、一定の速さは確認に使う。
3. 各回で `ink.input_to_capture`、`ink.pen_tip_to_edge_px`、`l2p.signed_gap_px`、`l2p.signed_gap_ms`、`l2p.residual_px`、`l2p.overshoot_px`、`l2p.hit_rate`、`l2p.settle_ms`、`l2p.speed_change_*` を出す。中央値と P95 を、回ごとの中央値の中央値でまとめる。
4. 予測の効果は、同じ回の予測なしとの差で見る。主指標は `l2p.residual_px` の中央値と P95、副指標は `ink.input_to_capture` の中央値と P95 とする。
5. 合格の目安は、一定の速さで `l2p.residual_px` の中央値が予測なしの 20% 以下になり、`l2p.overshoot_px` の P95 がブラシの直径以下で、`l2p.settle_ms` の P95 が 1 表示周期（16.7ms）以下になることである。速さが変わった直後と停止の直前は、この目安から分けて報告する。
6. 予測の線が確定の線と見分けられないことは、`l2p.hit_rate` と表示の取り込みの目視で確かめる。予測の線が確定の線より太い、細い、または色が違う場合は、模様のあるブラシの扱いを見直す。

## 残りの危険

- iPadOS と PencilKit の予測の長さは、公開された文書に ms の記載がない。実機で測るまでは、16.7ms の初期値を当てにしない。
- Windows Ink の予測部分の筆圧と傾きの扱いは公開されていない。LibrePaint が最後の値を使う設計は、Windows Ink の見た目と一致しない可能性がある。
- 模様のあるブラシでは、色と太さだけの折れ線が確定の線と一致しない。オフスクリーンの `KisPaintDevice` に同じ paint op で描く案は、画素の正本を汚さないが、1 回の描画の費用が増える。
- 予測の線は、確定の線がまだ届いていない位置を先に描く。確定の線が予測の線に追いつくまでの間、同じ画素を 2 回描くことになる。不透明な線では見えないが、半透明のブラシでは濃くなる可能性がある。半透明のブラシは、予測の線を確定の線と別の合成にし、確定の線が届いた時点で予測の線を消す。
- 予測の長さを 25ms まで伸ばすと、停止の直前の外れが大きくなる。停止の判定と信頼度の下限を、計測の `l2p.overshoot_px` と `l2p.settle_ms` で確かめる。

## 出典

- Microsoft, `InkPresenter` Class: https://learn.microsoft.com/en-us/uwp/api/windows.ui.input.inking.inkpresenter
- Microsoft, `InkModelerAttributes` Class: https://learn.microsoft.com/en-us/uwp/api/windows.ui.input.inking.inkmodelerattributes
- Microsoft, Pen interactions and Windows Ink in Windows apps: https://learn.microsoft.com/en-us/windows/uwp/ui-input/pen-and-stylus-interactions
- Microsoft, `DelegatedInkTrailVisual.AddTrailPointsWithPrediction`: https://learn.microsoft.com/en-us/uwp/api/windows.ui.composition.delegatedinktrailvisual.addtrailpointswithprediction
- Android Developers, `MotionEventPredictor`: https://developer.android.com/reference/androidx/input/motionprediction/MotionEventPredictor
- Android Developers, Advanced stylus features（Motion prediction）: https://developer.android.com/develop/ui/views/touch-and-input/stylus-input/advanced-stylus-features
- AndroidX, `PredictionEstimator.java`: https://android.googlesource.com/platform/frameworks/support/+/androidx-main/input/input-motionprediction/src/main/java/androidx/input/motionprediction/common/PredictionEstimator.java
- AndroidX, `SinglePointerPredictor.java`: https://android.googlesource.com/platform/frameworks/support/+/androidx-main/input/input-motionprediction/src/main/java/androidx/input/motionprediction/kalman/SinglePointerPredictor.java
- Apple, `UIEvent.predictedTouches(for:)`: https://developer.apple.com/documentation/uikit/uievent/predictedtouches(for:)
- Apple, `PKCanvasView`: https://developer.apple.com/documentation/pencilkit/pkcanvasview
- W3C, Pointer Events, Predicted events: https://w3c.github.io/pointerevents/#predicted-events
- Chrome Platform Status, `PointerEvent.getPredictedEvents()`: https://chromestatus.com/feature/5765569655603200
- Chromium, `ui/base/prediction/input_predictor.h`: https://chromium.googlesource.com/chromium/src/+/main/ui/base/prediction/input_predictor.h
- Chromium, `third_party/blink/renderer/platform/widget/input/input_event_prediction.h`: https://chromium.googlesource.com/chromium/src/+/main/third_party/blink/renderer/platform/widget/input/input_event_prediction.h
- Chromium, `ui/base/prediction/kalman_predictor.cc`: https://chromium.googlesource.com/chromium/src/+/main/ui/base/prediction/kalman_predictor.cc
- Krita, Freehand Brush Tool（Smoothing）: https://docs.krita.org/en/reference_manual/tools/freehand_brush.html
- Clip Studio Paint, Correction: https://help.clip-studio.com/en-us/manual_en/810_subtools/C.htm
- Clip Studio Paint, Preferences: https://help.clip-studio.com/en-us/manual_en/720_preferences/Preferences.htm
- Procreate, Brush Studio Settings（Stabilization、StreamLine、Motion Filtering）: https://help.procreate.com/procreate/handbook/brushes/brush-studio-settings
- Procreate, Apple Pencil, Pressure and Smoothing: https://help.procreate.com/articles/tIdR13-apple-pencil-pressure-and-smoothing
